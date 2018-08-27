// WhoYouAre.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <io.h>
#include <vector>
#include <dlib/opencv.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing.h>
//#include <dlib/image_io.h>
//#include <dlib/opencv/cv_image.h>
#include "common_lib.h"
#include "MachineVisionLib.h"
#include "FaceRecognition.h"
#include "MVLVideo.h"
#include "WhoYouAre.h"

#define NEED_DEBUG_CONSOLE	1	//* 是否需要控制台窗口输出

#if !NEED_DEBUG_CONSOLE
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
#endif

//* 添加人脸
static BOOL __AddFace(const CHAR *pszFaceImgFile, const CHAR *pezPersonName)
{
	FaceDatabase face_db;

	//* 看看是否已经添加到数据库中
	if (face_db.IsPersonAdded(pezPersonName))
	{
		cout << pezPersonName << " has been added to the face database." << endl;

		return TRUE;
	}


	//* 加载特征提取网络
	if (!face_db.LoadCaffeVGGNet("C:\\windows\\system32\\models\\vgg_face_caffe\\VGG_FACE_extract_deploy.prototxt",
		"C:\\windows\\system32\\models\\vgg_face_caffe\\VGG_FACE.caffemodel"))
	{
		cout << "Load Failed, the process will be exited!" << endl;

		return FALSE;
	}

	if (face_db.AddFace(pszFaceImgFile, pezPersonName))
	{
		cout << pezPersonName << " was successfully added to the face database." << endl;

		return TRUE;
	}		

	return FALSE;
}

//* 通过图片预测人脸
static void __PicturePredict(const CHAR *pszFaceImgFile)
{
	FaceDatabase face_db;

	if (!face_db.LoadFaceData())
	{
		cout << "Load face data failed, the process will be exited!" << endl;
		return;
	}

	if (!face_db.LoadCaffeVGGNet("C:\\windows\\system32\\models\\vgg_face_caffe\\VGG_FACE_extract_deploy.prototxt",
		"C:\\windows\\system32\\models\\vgg_face_caffe\\VGG_FACE.caffemodel"))
	{
		cout << "Load Failed, the process will be exited!" << endl;
		return;
	}

	cout << "Start find ..." << endl;
	string strPersonName;
	DOUBLE dblSimilarity = face_db.Predict(pszFaceImgFile, strPersonName);
	if (dblSimilarity > 0.55)
	{
		cout << "Found a matched face: 『" << strPersonName << "』, the similarity is " << dblSimilarity << endl;
	}
	else
		cout << "Not found matched face." << endl;

	cout << "Stop find." << endl;
}

void __PredictThroughVideoData(cv::Mat &mVideoData, DWORD64 dw64InputParam)
{
	FaceDatabase *pface_db = (FaceDatabase*)dw64InputParam;
	
	pface_db->pvideo_predict->Predict(mVideoData);

	imshow("Camera video", mVideoData);
}

//* 通过摄像头或视频预测
template <typename DType>
static void __VideoPredict(DType dtVideoSrc, BOOL blIsNeedToReplay)
{
	FaceDatabase face_db;
	face_db.pvideo_predict = new FaceDatabase::VideoPredict(&face_db);

	if (!face_db.LoadFaceData())
	{
		cout << "Load face data failed, the process will be exited!" << endl;
		return;
	}

	if (!face_db.LoadCaffeVGGNet("C:\\windows\\system32\\models\\vgg_face_caffe\\VGG_FACE_extract_deploy.prototxt",
		"C:\\windows\\system32\\models\\vgg_face_caffe\\VGG_FACE.caffemodel"))
	{
		cout << "Load Failed, the process will be exited!" << endl;
		return;
	}

	cv2shell::CV2ShowVideo(dtVideoSrc, __PredictThroughVideoData, (DWORD64)&face_db, blIsNeedToReplay);
}

//* 将从网络摄像机检测到的人脸保存起来
static BOOL __MVLVideoSaveFace(Mat& mFace, UINT unFaceID)
{
	ostringstream oss;
	oss << "FaceID: " << unFaceID;
	string strWinName(oss.str());

	imshow(strWinName, mFace);

	CHAR bKey = waitKey(1000);

	//* 等于回车，则保存之
	if (bKey == 13)
	{
		oss.str("");
		oss << "Face-" << unFaceID << ".jpg";
		cout << oss.str() << endl;
		imwrite(oss.str(), mFace);
		destroyWindow(strWinName);
		return TRUE;
	}

	return FALSE;
}

#if VLCPLAYER_DISPLAY_PREDICT_RESULT
//* 网络摄像机实时预测人脸
void __MVLVideoPredict(Mat& mFace, UINT unFaceID, FaceDatabase *pobjFaceDB, unordered_map<UINT, ST_FACE> *pmapFaces, THMUTEX *pthMutexMapFaces)
{
	string strPersonName;
	DOUBLE dblSimilarity = pobjFaceDB->Predict(mFace, strPersonName);
	if (dblSimilarity > 0.8)
	{		
		(*pmapFaces)[unFaceID].strPersonName = strPersonName;
		(*pmapFaces)[unFaceID].flPredictConfidence = (FLOAT)dblSimilarity;		
	}
}
#endif

static void __MVLVideoDispPreprocessor(Mat& mVideoFrame, void *pvParam, UINT unCurFrameIdx)
{	
	static UINT unFaceID = 0;
	BOOL blIsNotFound;	

#if FACE_DETECT_USE_DNNNET
	//* 按照预训练模型的Size，输入图像必须是300 x 300，大部分图像基本是16：9或4：3，很少等比例的，为了避免图像变形，我们只好截取图像中间最大面积的正方形区域识别人脸
	Mat mROI;
	if (mVideoFrame.cols > mVideoFrame.rows)
		mROI = mVideoFrame(Rect((mVideoFrame.cols - mVideoFrame.rows) / 2, 0, mVideoFrame.rows, mVideoFrame.rows));
	else if (mVideoFrame.cols < mVideoFrame.rows)
		mROI = mVideoFrame(Rect(0, (mVideoFrame.rows - mVideoFrame.cols) / 2, mVideoFrame.cols, mVideoFrame.cols));
	else
		mROI = mVideoFrame;	

	PST_VLCPLAYER_FCBDISPPREPROC_PARAM pstParam = (PST_VLCPLAYER_FCBDISPPREPROC_PARAM)pvParam;
	Mat &matFaces = cv2shell::FaceDetect(*pstParam->pobjDNNNet, mROI);
	if (matFaces.empty())
		return;	
#endif

#if VLCPLAYER_DISPLAY_PREDICT_RESULT
	//* 在视频上显示预测结果，这里是先显示再更新脸库，以减少不必要的处理（比如新添加的脸不必要显示，因为还未预测）
	EnterThreadMutex(pstParam->pthMutexMapFaces);
	{
		unordered_map<UINT, ST_FACE>::iterator iterFace;
		for (iterFace = pstParam->pmapFaces->begin(); iterFace != pstParam->pmapFaces->end(); iterFace++)
		{
			//* 有结果了，显示人名
			if (iterFace->second.flPredictConfidence > 0.0f)
			{
				//* 画出矩形
				Rect rectObj(iterFace->second.nLeftTopX, iterFace->second.nLeftTopY, (iterFace->second.nRightBottomX - iterFace->second.nLeftTopX), (iterFace->second.nRightBottomY - iterFace->second.nLeftTopY));
				rectangle(mROI, rectObj, Scalar(0, 255, 0));

				INT nBaseLine = 0;
				String strPersonLabel;
				string strConfidenceLabel;
				Rect rect;

				//* 标出人名和可信度
				strPersonLabel = "Name: " + iterFace->second.strPersonName;
				strConfidenceLabel = "Confidence: " + static_cast<ostringstream*>(&(ostringstream() << iterFace->second.flPredictConfidence))->str();

				Size personLabelSize = getTextSize(strPersonLabel, FONT_HERSHEY_SIMPLEX, 0.5, 1, &nBaseLine);
				Size confidenceLabelSize = getTextSize(strConfidenceLabel, FONT_HERSHEY_SIMPLEX, 0.5, 1, &nBaseLine);
				rect = Rect(Point(iterFace->second.nLeftTopX, iterFace->second.nLeftTopY - (personLabelSize.height + confidenceLabelSize.height + 3)),
							Size(personLabelSize.width > confidenceLabelSize.width ? personLabelSize.width : confidenceLabelSize.width,
							personLabelSize.height + confidenceLabelSize.height + nBaseLine + 3));
				rectangle(mROI, rect, Scalar(255, 255, 255), CV_FILLED);
				putText(mROI, strPersonLabel, Point(iterFace->second.nLeftTopX, iterFace->second.nLeftTopY - confidenceLabelSize.height - 3), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(107, 194, 53));
				putText(mROI, strConfidenceLabel, Point(iterFace->second.nLeftTopX, iterFace->second.nLeftTopY), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(107, 194, 53));
			}
		}
	}
	ExitThreadMutex(pstParam->pthMutexMapFaces);
#else	
	#if FACE_DETECT_USE_DNNNET
		cv2shell::MarkFaceWithRectangle(mROI, matFaces, 0.8);
	#endif
#endif

#if VLCPLAYER_DISPLAY_PREDICT_RESULT	
	//* 取出人脸并保存之
	for (INT i = 0; i < matFaces.rows; i++)
	{ 
		FLOAT flConfidenceVal = matFaces.at<FLOAT>(i, 2);
		if (flConfidenceVal < FACE_DETECT_MIN_CONFIDENCE_THRESHOLD)
			continue;

		blIsNotFound = TRUE;

		INT nCurLTX = static_cast<INT>(matFaces.at<FLOAT>(i, 3) * mROI.cols) - 40;
		INT nCurLTY = static_cast<INT>(matFaces.at<FLOAT>(i, 4) * mROI.rows) - 30;
		INT nCurRBX = static_cast<INT>(matFaces.at<FLOAT>(i, 5) * mROI.cols) + 40;
		INT nCurRBY = static_cast<INT>(matFaces.at<FLOAT>(i, 6) * mROI.rows) + 30;

		//* 确保坐标没有超出ROI范围
		nCurLTX = nCurLTX < 0 ? 0 : nCurLTX;
		nCurLTY = nCurLTY < 0 ? 0 : nCurLTY;
		nCurRBX = nCurRBX > mROI.cols ? mROI.cols : nCurRBX;
		nCurRBY = nCurRBY > mROI.rows ? mROI.rows : nCurRBY;

		//* 遍历脸部数据库，看前面是否已将其添加到库中，没有的则添加，有的则更新坐标及脸部图像数据
		EnterThreadMutex(pstParam->pthMutexMapFaces);
		{
			unordered_map<UINT, ST_FACE>::iterator iterFace;
			for (iterFace = pstParam->pmapFaces->begin(); iterFace != pstParam->pmapFaces->end(); iterFace++)
			{
				INT nOldFaceLTX = iterFace->second.nLeftTopX;
				INT nOldFaceLTY = iterFace->second.nLeftTopY;
				INT nOldFaceRBX = iterFace->second.nRightBottomX;
				INT nOldFaceRBY = iterFace->second.nRightBottomY;

				//* 全部符合则初步认为是同一张脸，更新数据即可，不需要添加新脸
				if (abs(nCurLTX - nOldFaceLTX) < MIN_PIXEL_DISTANCE_FOR_NEW_FACE &&
					abs(nCurLTY - nOldFaceLTY) < MIN_PIXEL_DISTANCE_FOR_NEW_FACE &&
					abs(nCurRBX - nOldFaceRBX) < MIN_PIXEL_DISTANCE_FOR_NEW_FACE &&
					abs(nCurRBY - nOldFaceRBY) < MIN_PIXEL_DISTANCE_FOR_NEW_FACE &&
					(unCurFrameIdx - iterFace->second.unFrameIdx) < FACE_DISAPPEAR_FRAME_NUM)
				{
					//* 保存坐标数据，可在前两段代码之前也可在之后
					iterFace->second.nLeftTopX = nCurLTX;
					iterFace->second.nLeftTopY = nCurLTY;
					iterFace->second.nRightBottomX = nCurRBX;
					iterFace->second.nRightBottomY = nCurRBY;

					//* 将脸部图像存入内存
					Mat mFace = mROI(Rect(nCurLTX, nCurLTY, nCurRBX - nCurLTX, nCurRBY - nCurLTY));
					mFace.copyTo(iterFace->second.mFace);		

					//* 注意这个一定是在脸部数据存入后更新，这样可确保脸部数据已经被存入内存，其它线程不会得到过期的脸部数据
					iterFace->second.unFrameIdx = unCurFrameIdx;

					blIsNotFound = FALSE;
					break;
				}
			}

			//* 没有找到则添加之
			if (blIsNotFound)
			{
				ST_FACE stFace;

				//* 脸部坐标数据
				stFace.nLeftTopX = nCurLTX;
				stFace.nLeftTopY = nCurLTY;
				stFace.nRightBottomX = nCurRBX;
				stFace.nRightBottomY = nCurRBY;
				stFace.flPredictConfidence = 0.0f;
				stFace.unFrameIdxForPrediction = 0;				

				//* 将脸部图像存入内存
				Mat mFace = mROI(Rect(nCurLTX, nCurLTY, nCurRBX - nCurLTX, nCurRBY - nCurLTY));
				mFace.copyTo(stFace.mFace);
				stFace.unFrameIdx = unCurFrameIdx;

				stFace.unFaceID = unFaceID++;

				//* 添加到脸库
				(*pstParam->pmapFaces)[stFace.unFaceID] = stFace;
			}
		}
		ExitThreadMutex(pstParam->pthMutexMapFaces);		
	}
#endif
}

using namespace dlib;

//* 通过网络摄像头识别
static void __MVLVideoFaceHandler(const CHAR *pszURL, BOOL blIsCatchFace, BOOL blIsRTSPStream)
{
#if VLCPLAYER_DISPLAY_PREDICT_RESULT
	unordered_map<UINT, ST_FACE> mapFaces;	//* 网络摄像机检测到的多张人脸信息存储该map中
	unordered_map<UINT, ST_FACE>::iterator iterFace;
	THMUTEX thMutexMapFaces;
#endif

	FaceDatabase objFaceDB;
	if (!blIsCatchFace)	//* 预测，则加载预测网络
	{
		objFaceDB.pvideo_predict = new FaceDatabase::VideoPredict(&objFaceDB);

		if (!objFaceDB.LoadFaceData())
		{
			cout << "Load face data failed, the process will be exited!" << endl;
			return;
		}

		if (!objFaceDB.LoadCaffeVGGNet("C:\\windows\\system32\\models\\vgg_face_caffe\\VGG_FACE_extract_deploy.prototxt",
			"C:\\windows\\system32\\models\\vgg_face_caffe\\VGG_FACE.caffemodel"))
		{
			cout << "Load Failed, the process will be exited!" << endl;
			return;
		}
	}

	VLCVideoPlayer objVideoPlayer;

#if VLCPLAYER_DISPLAY_EN
	cvNamedWindow(pszURL, CV_WINDOW_AUTOSIZE);
#endif	

	if (blIsRTSPStream)
	{
		objVideoPlayer.OpenVideoFromeRtsp(pszURL,
#if VLCPLAYER_DISPLAY_EN
			__MVLVideoDispPreprocessor,
#else
			NULL,
#endif
			pszURL);
	}
	else
	{
		objVideoPlayer.OpenVideoFromFile(pszURL,
#if VLCPLAYER_DISPLAY_EN
			__MVLVideoDispPreprocessor,
#else
			NULL, 
#endif
			pszURL);
	}

	if (!objVideoPlayer.start())
	{
		cout << "start rtsp stream failed!" << endl;
		return;
	}

#if FACE_DETECT_USE_DNNNET
	//* 初始化DNN人脸检测网络
	Net dnnNet = cv2shell::InitFaceDetectDNNet();
#endif

	//* 设置播放器显示预处理函数的入口参数
	ST_VLCPLAYER_FCBDISPPREPROC_PARAM stFCBDispPreprocParam;
#if FACE_DETECT_USE_DNNNET
	stFCBDispPreprocParam.pobjDNNNet = &dnnNet;
#endif
#if VLCPLAYER_DISPLAY_PREDICT_RESULT
	stFCBDispPreprocParam.pmapFaces = &mapFaces;
	InitThreadMutex(&thMutexMapFaces);	//* 对mapFaces的访问必须进行保护，因为这是两个线程并行运行
	stFCBDispPreprocParam.pthMutexMapFaces = &thMutexMapFaces;
#endif

#if VLCPLAYER_DISPLAY_EN
	objVideoPlayer.SetDispPreprocessorInputParam(&stFCBDispPreprocParam);
#endif
	
	frontal_face_detector detector = get_frontal_face_detector();
	shape_predictor pose_model;
	deserialize("C:\\OpenCV3.4\\dlib-19.10\\models\\shape_predictor_68_face_landmarks.dat") >> pose_model;

	CascadeClassifier a;
	a.load("C:\\OpenCV3.4\\opencv\\build\\etc\\haarcascades\\haarcascade_frontalface_alt2.xml");

	CHAR bKey;
	BOOL blIsPaused = FALSE;
	while (TRUE)
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
				cout << "与网络摄像头的连接断开，无法继续获取实时视频流！" << endl;

			break;
		}	

#if !VLCPLAYER_DISPLAY_PREDICT_RESULT
		Mat mSrcFrame = objVideoPlayer.GetNextFrame();
		if (mSrcFrame.empty())
			continue;

		Mat mFrame = mSrcFrame.clone();
		Mat mGray;
		cvtColor(mFrame, mGray, CV_BGR2GRAY);		
		std::vector<cv::Rect> faces;

		a.detectMultiScale(mGray, faces, 1.1, 3, 0, Size(50, 50), Size(500, 500));


		//cv_image<bgr_pixel> cimg(mFrame);
		// Detect faces 
		//std::vector<dlib::rectangle> faces = detector(cimg);

		std::vector<full_object_detection> shapes;

		cout << faces.size() << endl;
		
		for (unsigned long i = 0; i < faces.size(); ++i)
		{
			dlib::rectangle rect(faces[i].tl().x, faces[i].tl().y, faces[i].br().x, faces[i].br().y);
			shapes.push_back(pose_model(dlib::cv_image<uchar>(mGray), rect));
		}

		if (!shapes.empty()) {
			for (int i = 0; i < 68; i++) {
				circle(mFrame, cvPoint(shapes[0].part(i).x(), shapes[0].part(i).y()), 3, cv::Scalar(0, 0, 255), -1);
				//	shapes[0].part(i).x();//68个
			}
		}


		//objFaceDB.pvideo_predict->Predict(mFrame);

		imshow("Predict Result", mFrame);
#else
		//* 识别人脸
		for (iterFace = mapFaces.begin(); iterFace != mapFaces.end(); iterFace++)
		{
			//* 当前帧还未处理，处理之
			if (iterFace->second.unFrameIdxForPrediction != iterFace->second.unFrameIdx)
			{
				Mat mFace;
				UINT unCurFrameIdx;
				EnterThreadMutex(&thMutexMapFaces);
				{
					iterFace->second.mFace.copyTo(mFace);
					unCurFrameIdx = iterFace->second.unFrameIdx;	//* 必须在这里赋值，否则有可能显示线程在上面的if(……unFrameIdxForPrediction != ……unFrameIdx)之后更新该值
				}
				ExitThreadMutex(&thMutexMapFaces);

				if (blIsCatchFace)
				{
					//* 如果为真，则表明已经存储了一张人脸，进程直接退出即可
					if (__MVLVideoSaveFace(mFace, iterFace->second.unFaceID))
						goto __lblStop;											
				}
				else
					__MVLVideoPredict(mFace, iterFace->second.unFaceID, &objFaceDB, &mapFaces, &thMutexMapFaces);
					
				mapFaces[iterFace->second.unFaceID].unFrameIdxForPrediction = unCurFrameIdx;
			}
		}

		//* 遍历，删除已经消失的人脸数据
		EnterThreadMutex(&thMutexMapFaces);
		{
			UINT unCurFrameIdx = objVideoPlayer.GetCurFrameIndex();			
			for (iterFace = mapFaces.begin(); iterFace != mapFaces.end();)
			{
				//* 超时则删除之
				if ((unCurFrameIdx - iterFace->second.unFrameIdx) > FACE_DISAPPEAR_FRAME_NUM)
				{
					mapFaces.erase(iterFace++);	//* 注意，由于要删除元素，因此只能在这里++，在for中++的话会导致当前元素已删除，无法继续++
												//* 在这里是先保存iterFace当前指向，然后++，然后再将保存的当前指向传递给erase()函数
				}
				else
					iterFace++;	//* 下一个
			}
		}
		ExitThreadMutex(&thMutexMapFaces);
#endif
	}


__lblStop:
	//* 其实不调用这个函数也可以，VLCVideoPlayer类的析构函数会主动调用的
	objVideoPlayer.stop();	

#if VLCPLAYER_DISPLAY_PREDICT_RESULT
	UninitThreadMutex(&thMutexMapFaces);
#endif

	destroyAllWindows();
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 3 && argc != 4 && argc != 2)
	{
		cout << "Usage: " << endl << argv[0] << " add [Img Path] [Person Name]" << endl;
		cout << argv[0] << " predict [Img Path]" << endl;
		cout << argv[0] << " video [camera number, If not specified, the default value is 0]" << endl;
		cout << argv[0] << " video [video file path]" << endl;
		cout << argv[0] << " mvlvideo_rtsp_predict [rtsp url] - 使用MVLVideo模块读取RTSP流来识别人脸" << endl;
		cout << argv[0] << " mvlvideo_predict [video file path] - 使用MVLVideo模块读取视频文件识别人脸" << endl;
		cout << argv[0] << " mvlvideo_catchface [rtsp url] - 使用MVLVideo模块读取RTSP流检测并保存人脸" << endl;

		return -1;
	}

	String strOptType(argv[1]);
	if (String("add") == strOptType.toLowerCase())
	{
		return __AddFace(argv[2], argv[3]);
	}
	else if (string("predict") == strOptType.toLowerCase())
	{
		__PicturePredict(argv[2]);
	}
	else if (string("camera") == strOptType.toLowerCase())
	{
		INT nCameraID = 0;
		if (argc == 3)
			nCameraID = atoi(argv[2]);

		__VideoPredict(nCameraID, TRUE);
	}
	else if (string("video") == strOptType.toLowerCase())
	{
		if(argc != 3)
			goto __lblUsage;

		CHAR szVideoFile[MAX_PATH];
		sprintf_s(szVideoFile, "%s", argv[2]);

		__VideoPredict((const CHAR*)szVideoFile, FALSE);
	}
	else if (string("mvlvideo_rtsp_predict") == strOptType.toLowerCase())
	{
		if (argc != 3)
			goto __lblUsage;

		CHAR szURL[MAX_PATH];
		sprintf_s(szURL, "%s", argv[2]);

		__MVLVideoFaceHandler((const CHAR*)szURL, FALSE, TRUE);
	}
	else if (string("mvlvideo_catchface") == strOptType.toLowerCase())
	{
		if (argc != 3)
			goto __lblUsage;

		CHAR szURL[MAX_PATH];
		sprintf_s(szURL, "%s", argv[2]);

		__MVLVideoFaceHandler((const CHAR*)szURL, TRUE, TRUE);
	}
	else if (string("mvlvideo_predict") == strOptType.toLowerCase())
	{
		if (argc != 3)
			goto __lblUsage;

		CHAR szURL[MAX_PATH];
		sprintf_s(szURL, "%s", argv[2]);

		__MVLVideoFaceHandler((const CHAR*)szURL, FALSE, FALSE);
	}
	else
		goto __lblUsage;

	return 0;

__lblUsage:	
	cout << "Usage: " << endl << argv[0] << " add [Img Path] [Person Name]" << endl;
	cout << argv[0] << " predict [Img Path]" << endl;
	cout << argv[0] << " video [camera number, If not specified, the default value is 0]" << endl;
	cout << argv[0] << " video [video file path]" << endl;
	cout << argv[0] << " mvlvideo_rtsp_predict [rtsp url] - 使用MVLVideo模块读取RTSP流来识别人脸" << endl;
	cout << argv[0] << " mvlvideo_predict [video file path] - 使用MVLVideo模块读取视频文件识别人脸" << endl;
	cout << argv[0] << " mvlvideo_catchface [rtsp url] - 使用MVLVideo模块读取RTSP流检测并保存人脸" << endl;

    return 0;
}

