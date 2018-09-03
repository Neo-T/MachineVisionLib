// WhoYouAreByVideo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <io.h>
#include <vector>
#include "common_lib.h"
#include "MachineVisionLib.h"
#include "FaceRecognition.h"
#include "MVLVideo.h"
#include "WhoYouAreByVideo.h"

static HANDLE hSHMFaceDB;
static CHAR *pbSHMFaceDB;
static HANDLE hMutexFaceDB;
static PST_SHM_FACE_DB_HDR pstFaceDBHdr;
static UCHAR *pubFDBFrameROIData;
static PST_FACE pstFaceDB;

static BOOL blIsRunning;
BOOL WINAPI ConsoleCtrlHandler(DWORD dwEvent)
{
	blIsRunning = FALSE;
	printf("The process receives the CTL+C signal and will exit.\r\n");
	return TRUE;
}

//* 添加一张“脸”到链表
static void __PutNodeToFaceLink(USHORT *pusNewLink, USHORT usFaceNode, BOOL blIsExistFace)
{
	PST_FACE pstFace = pstFaceDB;

	//* 如果FaceLink存在这张脸，则先摘除之
	if (blIsExistFace)
	{
		if (pstFace[usFaceNode].usPrevNode != INVALID_FACE_NODE_INDEX)
			pstFace[pstFace[usFaceNode].usPrevNode].usNextNode = pstFace[usFaceNode].usNextNode;
		//* 显然是链表头部
		else 
			pstFaceDBHdr->usFaceLink = pstFace[usFaceNode].usNextNode;

		if (pstFace[usFaceNode].usNextNode != INVALID_FACE_NODE_INDEX)
			pstFace[pstFace[usFaceNode].usNextNode].usPrevNode = pstFace[usFaceNode].usPrevNode;
	}

	pstFace[usFaceNode].usPrevNode = INVALID_FACE_NODE_INDEX;
	if ((*pusNewLink) == INVALID_FACE_NODE_INDEX)
	{		
		pstFace[usFaceNode].usNextNode = INVALID_FACE_NODE_INDEX;		

		*pusNewLink = usFaceNode;

		return;
	}

	pstFace[*pusNewLink].usPrevNode = usFaceNode;
	pstFace[usFaceNode].usNextNode = *pusNewLink;	
	*pusNewLink = usFaceNode;
}

//* 获取一个空闲节点
static USHORT __GetNodeFromFreeLink(void)
{
	PST_FACE pstFace = pstFaceDB;

	//* 还有空闲节点
	if (pstFaceDBHdr->usFreeLink != INVALID_FACE_NODE_INDEX)
	{
		USHORT usFreeNode = pstFaceDBHdr->usFreeLink;
		if (pstFace[usFreeNode].usNextNode != INVALID_FACE_NODE_INDEX)
			pstFace[pstFace[usFreeNode].usNextNode].usPrevNode = INVALID_FACE_NODE_INDEX;

		
		pstFaceDBHdr->usFreeLink = pstFace[usFreeNode].usNextNode;
		pstFace[usFreeNode].usNextNode = INVALID_FACE_NODE_INDEX;

		return usFreeNode;
	}

	return INVALID_FACE_NODE_INDEX;
}

//* 释放占用的节点
static void __FreeFaceNode(USHORT usFaceNode)
{
	PST_FACE pstFace = pstFaceDB;

	//* 先从FaceLink中摘除之
	//* ==========================================================================
	if (pstFace[usFaceNode].usPrevNode != INVALID_FACE_NODE_INDEX)
		pstFace[pstFace[usFaceNode].usPrevNode].usNextNode = pstFace[usFaceNode].usNextNode;
	//* 显然是链表头部
	else
		pstFaceDBHdr->usFaceLink = pstFace[usFaceNode].usNextNode;

	if (pstFace[usFaceNode].usNextNode != INVALID_FACE_NODE_INDEX)
		pstFace[pstFace[usFaceNode].usNextNode].usPrevNode = pstFace[usFaceNode].usPrevNode;
	//* ==========================================================================

	pstFace[usFaceNode].usPrevNode = INVALID_FACE_NODE_INDEX;
	if (pstFaceDBHdr->usFreeLink == INVALID_FACE_NODE_INDEX)
	{
		pstFace[usFaceNode].usNextNode = INVALID_FACE_NODE_INDEX;

		pstFaceDBHdr->usFreeLink = usFaceNode;

		return;
	}

	pstFace[pstFaceDBHdr->usFreeLink].usPrevNode = usFaceNode;
	pstFace[usFaceNode].usNextNode = pstFaceDBHdr->usFreeLink;
	pstFaceDBHdr->usFreeLink = usFaceNode;
}

//* 释放FaceLink所有节点，注意这个函数没有加锁，需要上层掉用函数加锁才可
static void __FreeFaceLink(void)
{
	PST_FACE pstFace = pstFaceDB;
	USHORT usFaceNode;
	while ((usFaceNode = pstFaceDBHdr->usFaceLink) != INVALID_FACE_NODE_INDEX)
	{
		__FreeFaceNode(usFaceNode);
	}
}

//* VLC播放器之回调函数：人脸
static void __FCBVLCPlayerFaceHandler(Mat& mVideoFrame, void *pvParam, UINT unCurFrameIdx)
{
	PST_PLAYER_FCBDISPPREPROC_PARAM pstParam = (PST_PLAYER_FCBDISPPREPROC_PARAM)pvParam;
	static UINT unFaceID = 0;
	
	Mat mROI = mVideoFrame(Rect(pstFaceDBHdr->stROIRect.x, pstFaceDBHdr->stROIRect.y, pstFaceDBHdr->stROIRect.unWidth, pstFaceDBHdr->stROIRect.unHeight));
	Mat &matFaces = cv2shell::FaceDetect(*pstParam->pobjDNNNet, mROI);
	if (matFaces.empty())
		return;

	Mat mFaceGray;
	cvtColor(mROI, mFaceGray, CV_BGR2GRAY);
	
	//* 取出每张脸并将其存入"脸库"
	IPCEnterCriticalSection(hMutexFaceDB);
	{
		BOOL blIsNotFound;
		USHORT usNewFaceLink = INVALID_FACE_NODE_INDEX;
		for (INT i = 0; i < matFaces.rows; i++)
		{
			FLOAT flConfidenceVal = matFaces.at<FLOAT>(i, 2);
			if (flConfidenceVal < FACE_DETECT_MIN_CONFIDENCE_THRESHOLD)
				continue;

			INT nCurLTX = static_cast<INT>(matFaces.at<FLOAT>(i, 3) * mROI.cols);
			INT nCurLTY = static_cast<INT>(matFaces.at<FLOAT>(i, 4) * mROI.rows);
			INT nCurRBX = static_cast<INT>(matFaces.at<FLOAT>(i, 5) * mROI.cols);
			INT nCurRBY = static_cast<INT>(matFaces.at<FLOAT>(i, 6) * mROI.rows);

			blIsNotFound = TRUE;
			PST_FACE pstFace = pstFaceDB;
			USHORT usCurNode = pstFaceDBHdr->usFaceLink;
			while (usCurNode != INVALID_FACE_NODE_INDEX)
			{
				INT nPrevFaceLTX = pstFace[usCurNode].nLeftTopX;
				INT nPrevFaceLTY = pstFace[usCurNode].nLeftTopY;
				INT nPrevFaceRBX = pstFace[usCurNode].nRightBottomX;
				INT nPrevFaceRBY = pstFace[usCurNode].nRightBottomY;

				//* 全部符合则初步认为是同一张脸，更新数据即可，不需要添加新脸
				if (abs(nCurLTX - nPrevFaceLTX) < MIN_PIXEL_DISTANCE_FOR_NEW_FACE &&
					abs(nCurLTY - nPrevFaceLTY) < MIN_PIXEL_DISTANCE_FOR_NEW_FACE &&
					abs(nCurRBX - nPrevFaceRBX) < MIN_PIXEL_DISTANCE_FOR_NEW_FACE &&
					abs(nCurRBY - nPrevFaceRBY) < MIN_PIXEL_DISTANCE_FOR_NEW_FACE)
				{
					//* 保存坐标数据
					pstFace[usCurNode].nLeftTopX = nCurLTX;
					pstFace[usCurNode].nLeftTopY = nCurLTY;
					pstFace[usCurNode].nRightBottomX = nCurRBX;
					pstFace[usCurNode].nRightBottomY = nCurRBY;

					//* 注意这个一定是最后更新
					pstFaceDB[usCurNode].unFrameIdx = unCurFrameIdx;

					//* 放入链表
					__PutNodeToFaceLink(&usNewFaceLink, usCurNode, TRUE);

					blIsNotFound = FALSE;
					break;
				}

				//* 查找下一个
				usCurNode = pstFace[usCurNode].usNextNode;
			}

			//* 没有找到则添加之
			if (blIsNotFound)
			{
				USHORT usFreeNode = __GetNodeFromFreeLink();
				if (usFreeNode == INVALID_FACE_NODE_INDEX)	//* 没有空闲节点了，剩下的就不处理了
				{
					cout << "Warning: Insufficient face database space!" << endl;					
					break;
				}

				//* 保存坐标数据
				pstFaceDB[usFreeNode].nLeftTopX = nCurLTX;
				pstFaceDB[usFreeNode].nLeftTopY = nCurLTY;
				pstFaceDB[usFreeNode].nRightBottomX = nCurRBX;
				pstFaceDB[usFreeNode].nRightBottomY = nCurRBY;

				pstFaceDB[usFreeNode].flPredictConfidence = 0.0f;

				pstFaceDB[usFreeNode].unFrameIdx = unCurFrameIdx;
				pstFaceDB[usFreeNode].unFaceID = unFaceID++;

				//* 放入链表
				__PutNodeToFaceLink(&usNewFaceLink, usFreeNode, FALSE);
			}
		}

		//* 先归还已经不再使用的节点
		__FreeFaceLink();

		//* 更新链表
		pstFaceDBHdr->usFaceLink = usNewFaceLink;

		//* 复制帧数据		
		memcpy(pubFDBFrameROIData, mFaceGray.data, pstFaceDBHdr->unFrameROIDataBytes);
		//Mat mROIGray = Mat(pstFaceDBHdr->stROIRect.unHeight, pstFaceDBHdr->stROIRect.unWidth, CV_8UC1, pubFDBFrameROIData);
		//imshow("Gray-Main", mROIGray);		
	}
	IPCExitCriticalSection(hMutexFaceDB);

	//* 显示识别结果
	IPCEnterCriticalSection(hMutexFaceDB);
	{
		PST_FACE pstFace = pstFaceDB;
		USHORT usFaceNode = pstFaceDBHdr->usFaceLink;
		while (usFaceNode != INVALID_FACE_NODE_INDEX)
		{
			//* 有结果了，显示人名
			if (pstFace[usFaceNode].flPredictConfidence > 0.0f)
			{
				//* 画出矩形
				Rect objRect(pstFace[usFaceNode].nLeftTopX, pstFace[usFaceNode].nLeftTopY, (pstFace[usFaceNode].nRightBottomX - pstFace[usFaceNode].nLeftTopX), (pstFace[usFaceNode].nRightBottomY - pstFace[usFaceNode].nLeftTopY));
				rectangle(mROI, objRect, Scalar(0, 255, 0));

				INT nBaseLine = 0;
				String strPersonLabel;
				string strConfidenceLabel;
				Rect rect;

				//* 标出人名和可信度
				strPersonLabel = "Name: " + pstFace[usFaceNode].strPersonName;
				strConfidenceLabel = "Confidence: " + static_cast<ostringstream*>(&(ostringstream() << pstFace[usFaceNode].flPredictConfidence))->str();

				Size personLabelSize = getTextSize(strPersonLabel, FONT_HERSHEY_SIMPLEX, 0.5, 1, &nBaseLine);
				Size confidenceLabelSize = getTextSize(strConfidenceLabel, FONT_HERSHEY_SIMPLEX, 0.5, 1, &nBaseLine);
				rect = Rect(Point(pstFace[usFaceNode].nLeftTopX, pstFace[usFaceNode].nLeftTopY - (personLabelSize.height + confidenceLabelSize.height + 3)),
					Size(personLabelSize.width > confidenceLabelSize.width ? personLabelSize.width : confidenceLabelSize.width,
						personLabelSize.height + confidenceLabelSize.height + nBaseLine + 3));
				rectangle(mROI, rect, Scalar(255, 255, 255), CV_FILLED);
				putText(mROI, strPersonLabel, Point(pstFace[usFaceNode].nLeftTopX, pstFace[usFaceNode].nLeftTopY - confidenceLabelSize.height - 3), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(107, 194, 53));
				putText(mROI, strConfidenceLabel, Point(pstFace[usFaceNode].nLeftTopX, pstFace[usFaceNode].nLeftTopY), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(107, 194, 53));
			}

			//* 下一个
			usFaceNode = pstFace[usFaceNode].usNextNode;
		}
	}
	IPCExitCriticalSection(hMutexFaceDB);
}

//* 主进程的入口处理函数
static void __MainProcHandler(const CHAR *pszVideoPath, DWORD& dwSubprocID)
{
	BOOL blIsRTSPStream;

	//* 初始化DNN人脸检测网络
	Net objDNNNet = cv2shell::InitFaceDetectDNNet();

	//* 声明一个VLC播放器，并建立播放窗口
	VLCVideoPlayer objVideoPlayer;
	cvNamedWindow(pszVideoPath, CV_WINDOW_AUTOSIZE);
	//cvNamedWindow("Gray-Main", CV_WINDOW_AUTOSIZE);

	if ((CHAR)toupper((INT)pszVideoPath[0]) == 'R'
		&& (CHAR)toupper((INT)pszVideoPath[1]) == 'T'
		&& (CHAR)toupper((INT)pszVideoPath[2]) == 'S'
		&& (CHAR)toupper((INT)pszVideoPath[3]) == 'P'
		&& (CHAR)toupper((INT)pszVideoPath[4]) == ':')
	{
		blIsRTSPStream = TRUE;
		objVideoPlayer.OpenVideoFromeRtsp(pszVideoPath, __FCBVLCPlayerFaceHandler, pszVideoPath, 1000);
	}
	else
	{
		blIsRTSPStream = FALSE;
		objVideoPlayer.OpenVideoFromFile(pszVideoPath, __FCBVLCPlayerFaceHandler, pszVideoPath);
	}		

	//* 设置播放器显示预处理函数的入口参数
	ST_PLAYER_FCBDISPPREPROC_PARAM stFCBDispPreprocParam;
	stFCBDispPreprocParam.pobjDNNNet = &objDNNNet;	
	objVideoPlayer.SetDispPreprocessorInputParam(&stFCBDispPreprocParam);

	//* 开始播放
	if (!objVideoPlayer.start())
	{
		cout << "start rtsp stream failed!" << endl;
		return;
	}

	CHAR bKey;
	BOOL blIsPaused = FALSE;
	while (blIsRunning)
	{
		bKey = waitKey(10);
		if (27 == bKey)
			break;

		//* 空格暂停，这个暂停是真暂停，播放器会缓存一段时间的实时视频流，换言之，其恢复播放后视频与当前时刻存在一定的时间差
		if (' ' == bKey)
		{
			blIsPaused = !blIsPaused;
			objVideoPlayer.pause(blIsPaused);
		}

		if (objVideoPlayer.IsPlayEnd())
		{
			if (blIsRTSPStream)
				cout << "The connection with the RTSP stream is disconnected, unable to get the next frame, the process will exit." << endl;

			break;
		}
	}

	StopProcess(dwSubprocID);

	//* 其实不调用这个函数也可以，VLCVideoPlayer类的析构函数会主动调用的
	objVideoPlayer.stop();

	destroyAllWindows();
}

//* 判断当前FaceID是否还存在，如果不存在就没必要预测或者更新数据了，返回值为该ID在脸库中的保存位置
static USHORT __IsTheFaceIDExist(UINT unFaceID)
{
	PST_FACE pstFace = pstFaceDB;
	USHORT usFaceNode = pstFaceDBHdr->usFaceLink;
	while (usFaceNode != INVALID_FACE_NODE_INDEX)
	{
		if (unFaceID == pstFace[usFaceNode].unFaceID)
			return usFaceNode;

		//* 下一个
		usFaceNode = pstFace[usFaceNode].usNextNode;
	}

	return INVALID_FACE_NODE_INDEX;
}

//* 同上，前缀S代表这个是带进程锁的函数，不需要上层调用者对“脸库”加锁
static USHORT __S_IsTheFaceIDExist(UINT unFaceID)
{
	USHORT usFaceNode;

	IPCEnterCriticalSection(hMutexFaceDB);
	{
		usFaceNode = __IsTheFaceIDExist(unFaceID);
	}
	IPCExitCriticalSection(hMutexFaceDB);

	return usFaceNode;
}

//* 子进程的入口处理函数
static void __SubProcHandler(void)
{
	FaceDatabase objFaceDB;

	//* 初始化人脸识别模块
	//* =======================================================================================
	if (!objFaceDB.LoadFaceData())
	{
		cout << "Load face data failed, the process will be exited!" << endl;
		return;
	}

	if (!objFaceDB.LoadDLIB68FaceLandmarksModel("C:\\OpenCV3.4\\dlib-19.10\\models\\shape_predictor_68_face_landmarks.dat"))
		return;

	if (!objFaceDB.LoadCaffeVGGNet("C:\\windows\\system32\\models\\vgg_face_caffe\\VGG_FACE_extract_deploy.prototxt",
		"C:\\windows\\system32\\models\\vgg_face_caffe\\VGG_FACE.caffemodel"))
	{
		cout << "Load Failed, the process will be exited!" << endl;
		return;
	}
	objFaceDB.o_pobjVideoPredict = new FaceDatabase::VideoPredict(&objFaceDB);
	//* =======================================================================================


	ST_FACE staFace[SHM_FACE_DB_SIZE_MAX];
	UCHAR *pubaROIGrayData = new UCHAR[pstFaceDBHdr->unFrameROIDataBytes];
	Mat mROIGray = Mat(pstFaceDBHdr->stROIRect.unHeight, pstFaceDBHdr->stROIRect.unWidth, CV_8UC1, pubaROIGrayData);
	while (blIsRunning)
	{
		UINT unFaceNum = 0;		

		//* 取出人脸数据
		IPCEnterCriticalSection(hMutexFaceDB);
		{
			PST_FACE pstFace = pstFaceDB;
			USHORT usFaceNode = pstFaceDBHdr->usFaceLink;
			while (usFaceNode != INVALID_FACE_NODE_INDEX)
			{
				staFace[unFaceNum++] = pstFace[usFaceNode];

				//* 下一个
				usFaceNode = pstFace[usFaceNode].usNextNode;
			}							

			
			memcpy(pubaROIGrayData, pubFDBFrameROIData, pstFaceDBHdr->unFrameROIDataBytes);											
		}		
		IPCExitCriticalSection(hMutexFaceDB);
		

		//* 预测		
		for (UINT i = 0; i < unFaceNum; i++)
		{
			staFace[i].blIsPredicted = FALSE;

			//* 帧号相同表明已经预测过，尚未更新帧数据，或者人脸已经消失，则不再预测，毕竟预测太耗时
			if (staFace[i].unFrameIdx == staFace[i].unFrameIdxForPrediction || 
					INVALID_FACE_NODE_INDEX == __S_IsTheFaceIDExist(staFace[i].unFaceID))
			{
				continue;
			}

			Face objFace(staFace[i].nLeftTopX, staFace[i].nLeftTopY, staFace[i].nRightBottomX, staFace[i].nRightBottomY);			
			DOUBLE dblConfidenceVal = objFaceDB.o_pobjVideoPredict->Predict(mROIGray, objFace, objFaceDB.o_objShapePredictor, staFace[i].strPersonName);
			if (dblConfidenceVal > 0.8f)
			{
				staFace[i].flPredictConfidence = dblConfidenceVal;
				staFace[i].blIsPredicted = TRUE;
				staFace[i].unFrameIdxForPrediction = staFace[i].unFrameIdx;				
			}						
		}

		//* 将预测结果放入“脸库”
		IPCEnterCriticalSection(hMutexFaceDB);
		{
			for (UINT i = 0; i < unFaceNum; i++)
			{
				USHORT usFaceNode;

				//* 未作出预测或者未找到匹配记录则不更新
				if (!staFace[i].blIsPredicted)
					continue;

				usFaceNode = __IsTheFaceIDExist(staFace[i].unFaceID);
				if (usFaceNode != INVALID_FACE_NODE_INDEX)
				{										
					pstFaceDB[usFaceNode].strPersonName = staFace[i].strPersonName;
					pstFaceDB[usFaceNode].flPredictConfidence = staFace[i].flPredictConfidence;					
				}
			}
		}
		IPCExitCriticalSection(hMutexFaceDB);		
	}

	delete[] pubaROIGrayData;
}

//* 主进程初始化
static BOOL __MainProcInit(DWORD& dwSubprocID)
{
	Rect objROIRect;
	UINT unFrameROIDataBytes;

	UINT unFrameWidth = DEFAULT_VIDEO_FRAME_WIDTH;
	UINT unFrameHeight = (unFrameWidth / 16) * 9;
	if (unFrameWidth > unFrameHeight)
		objROIRect = Rect((unFrameWidth - unFrameHeight) / 2, 0, unFrameHeight, unFrameHeight);
	else if (unFrameWidth < unFrameHeight)
		objROIRect = Rect(0, (unFrameHeight - unFrameWidth) / 2, unFrameWidth, unFrameWidth);
	else
		objROIRect = Rect(0, 0, unFrameWidth, unFrameHeight);

	unFrameROIDataBytes = objROIRect.width * objROIRect.height * sizeof(UCHAR);	

	//* 脸库的组织结构为：ST_SHM_FACE_DB_HDR + mROIGray.data + FaceDB
	if (NULL == (pbSHMFaceDB = IPCCreateSHM(SHM_FACE_DB_NAME, sizeof(ST_SHM_FACE_DB_HDR) + unFrameROIDataBytes + SHM_FACE_DB_SIZE_MAX * sizeof(ST_FACE), &hSHMFaceDB)))
		return FALSE;

	//* 建立脸库锁
	hMutexFaceDB = IPCCreateCriticalSection(IPC_MUTEX_FACEDB_NAME);
	if (hMutexFaceDB == INVALID_HANDLE_VALUE)
	{
		IPCCloseSHM(pbSHMFaceDB, hSHMFaceDB);
		return FALSE;
	}

	//* “脸库”相关地址
	pstFaceDBHdr = (PST_SHM_FACE_DB_HDR)pbSHMFaceDB;											//* 脸库头部数据结构
	pubFDBFrameROIData = (UCHAR*)(pbSHMFaceDB + sizeof(ST_SHM_FACE_DB_HDR));					//* 视频帧ROI区域数据在"脸库"中的首地址
	pstFaceDB = (PST_FACE)(pbSHMFaceDB + sizeof(ST_SHM_FACE_DB_HDR) + unFrameROIDataBytes);		//* 每张脸的坐标及相关数据

	//* 建立链表	
	//* =========================================================================
	PST_FACE pstFace = pstFaceDB;
	pstFace->usPrevNode = INVALID_FACE_NODE_INDEX;
	for (INT i = 1; i < SHM_FACE_DB_SIZE_MAX; i++)
	{
		pstFace[i].usPrevNode = i - 1;
		pstFace[i].usNextNode = INVALID_FACE_NODE_INDEX;
		pstFace[i - 1].usNextNode = i;
	}
	//* 空闲链表指向第一个节点即可，Face链表尚未有任何数据
	pstFaceDBHdr->usFreeLink = 0;
	pstFaceDBHdr->usFaceLink = INVALID_FACE_NODE_INDEX;
	//* =========================================================================	

	//* 保存ROI区域相关坐标位置及容量
	pstFaceDBHdr->stROIRect.x = objROIRect.x;
	pstFaceDBHdr->stROIRect.y = objROIRect.y;
	pstFaceDBHdr->stROIRect.unWidth = objROIRect.width;
	pstFaceDBHdr->stROIRect.unHeight = objROIRect.height;
	pstFaceDBHdr->unFrameROIDataBytes = unFrameROIDataBytes;

	//* 启动子进程
	dwSubprocID = StartProcess("WhoYouAreByVideo.exe", NULL);
	if (INVALID_PROC_ID == dwSubprocID)
	{
		IPCDelCriticalSection(hMutexFaceDB);
		IPCCloseSHM(pbSHMFaceDB, hSHMFaceDB);

		cout << "Sub process startup failure for face recognition, the process exit!" << endl;
		return FALSE;
	}

	return TRUE;
}

static BOOL __SubProcInit(void)
{
	//* 打开脸库
	if (NULL == (pbSHMFaceDB = IPCOpenSHM(SHM_FACE_DB_NAME, &hSHMFaceDB)))
		return FALSE;

	//* 打开脸库锁
	hMutexFaceDB = IPCOpenCriticalSection(IPC_MUTEX_FACEDB_NAME);
	if (hMutexFaceDB == INVALID_HANDLE_VALUE)
	{
		IPCCloseSHM(pbSHMFaceDB, hSHMFaceDB);
		return FALSE;
	}

	//* 脸库访问地址	
	pstFaceDBHdr = (PST_SHM_FACE_DB_HDR)pbSHMFaceDB;
	pubFDBFrameROIData = (UCHAR*)(pbSHMFaceDB + sizeof(ST_SHM_FACE_DB_HDR));
	pstFaceDB = (PST_FACE)(pbSHMFaceDB + sizeof(ST_SHM_FACE_DB_HDR) + pstFaceDBHdr->unFrameROIDataBytes);		

	return TRUE;
}

//* 去初始化，释放申请占用的各种资源
static void __Uninit(void)
{
	//* 释放脸库锁并关闭“脸库”
	IPCDelCriticalSection(hMutexFaceDB);
	IPCCloseSHM(pbSHMFaceDB, hSHMFaceDB);
}

int _tmain(int argc, _TCHAR* argv[])
{
	DWORD dwSubprocID;

	if (argc != 1 && argc != 2)
	{
		cout << "Usage: " << argv[0] << " [rtsp address]" << endl;
		cout << "Usage: " << argv[0] << " [video file path]" << endl;

		return 0;
	}

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleCtrlHandler, TRUE);

	blIsRunning = TRUE;

	//* 建立“脸库”
	if (argc == 2)
	{		
		if (!__MainProcInit(dwSubprocID))
		{
			cout << "Main process initialization failed" << endl;
			return 0;
		}		

		__MainProcHandler(argv[1], dwSubprocID);
	}
	else
	{			
		if (!__SubProcInit())
		{
			cout << "Subprocess initialization failed" << endl;
			return 0;
		}

		__SubProcHandler();		
	}

	__Uninit();

    return 0;
}

