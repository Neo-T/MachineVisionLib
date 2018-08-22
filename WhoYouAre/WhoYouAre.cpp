// WhoYouAre.cpp : 定义控制台应用程序的入口点。
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

static void __NetcamDispPreprocessor(Mat& mVideoFrame, void *pvParam)
{	
	//* 按照预训练模型的Size，输入图像必须是300 x 300，大部分图像基本是16：9或4：3，很少等比例的，为了避免图像变形，我们只好截取图像中间最大面积的正方形区域识别人脸
	Mat mROI;
	if (mVideoFrame.cols > mVideoFrame.rows)
		mROI = mVideoFrame(Rect((mVideoFrame.cols - mVideoFrame.rows) / 2, 0, mVideoFrame.rows, mVideoFrame.rows));
	else if (mVideoFrame.cols < mVideoFrame.rows)
		mROI = mVideoFrame(Rect(0, (mVideoFrame.rows - mVideoFrame.cols) / 2, mVideoFrame.cols, mVideoFrame.cols));
	else
		mROI = mVideoFrame;

	Net *pobjDNNNet = (Net*)pvParam;
	Mat &matFaces = cv2shell::FaceDetect(*pobjDNNNet, mROI);
	if (matFaces.empty())
		return;

	cv2shell::MarkFaceWithRectangle(mROI, matFaces, 0.8);
}

//* 通过网络摄像头识别
static void __NetcamPredict(const CHAR *pszURL, BOOL blIsNeedToReplay)
{
	VLCVideoPlayer objVideoPlayer;

	cvNamedWindow(pszURL, CV_WINDOW_AUTOSIZE);
	objVideoPlayer.OpenVideoFromeRtsp(pszURL, __NetcamDispPreprocessor, pszURL);

	if (!objVideoPlayer.start())
	{
		cout << "start rtsp stream failed!" << endl;
		return;
	}

	//* 初始化DNN人脸检测网络，并将其传递给VLC播放器，以便显示预处理函数使用
	Net dnnNet = cv2shell::InitFaceDetectDNNet();
	objVideoPlayer.SetDispPreprocessorInputParam(&dnnNet);

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
			cout << "与网络摄像头的连接断开，无法继续获取实时视频流！" << endl;
			break;
		}	

		//* 检测人脸
		Mat mFrame = objVideoPlayer.GetNextFrame();
		if (mFrame.empty())				
			continue;		
	}

	//* 其实不调用这个函数也可以，VLCVideoPlayer类的析构函数会主动调用的
	objVideoPlayer.stop();
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 3 && argc != 4 && argc != 2)
	{
		cout << "Usage: " << endl << argv[0] << " add [Img Path] [Person Name]" << endl;
		cout << argv[0] << " predict [Img Path]" << endl;
		cout << argv[0] << " video [camera number, If not specified, the default value is 0]" << endl;
		cout << argv[0] << " video [video file path]" << endl;
		cout << argv[0] << " netcam [rtsp url]" << endl;

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
	else if (string("netcam") == strOptType.toLowerCase())
	{
		if (argc != 3)
			goto __lblUsage;

		CHAR szURL[MAX_PATH];
		sprintf_s(szURL, "%s", argv[2]);

		__NetcamPredict((const CHAR*)szURL, FALSE);
	}
	else
		goto __lblUsage;

	return 0;

__lblUsage:	
	cout << "Usage: " << endl << argv[0] << " add [Img Path] [Person Name]" << endl;
	cout << argv[0] << " predict [Img Path]" << endl;
	cout << argv[0] << " video [camera number, If not specified, the default value is 0]" << endl;
	cout << argv[0] << " video [video file path]" << endl;
	cout << argv[0] << " netcam [rtsp url]" << endl;

    return 0;
}

