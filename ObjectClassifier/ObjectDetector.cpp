// ObjectClassifier.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <io.h>
#include <algorithm>
#include <vector>
#include "common_lib.h"
#include "MachineVisionLib.h"
#include "MVLVideo.h"

using namespace common_lib;
using namespace cv2shell;
BOOL blIsSetAlarmArea;		//* 是否设置人员入侵报警区域
BOOL blIsIntrusionAlarm;	//* 是否开启人员入侵报警
INT nAlarmAreaIdx;

//* Usage，使用说明
static const CHAR *pszCmdLineArgs = {
	"{help            h |     | print help message}"
	"{@file             |     | input data source, image file path or rtsp stream address}"
	"{model           m | VGG | which model to use, value is VGG、Yolo2、Yolo2-Tiny、Yolo2-VOC、Yolo2-Tiny-VOC、MobileNetSSD}"
	"{picture         p |     | image file detector, non video detector}"
	"{set_alarm_area  s |     | set up an alarm area for intrusion}"
	"{intrusion_alarm a |     | intrusion alarm}"
};

//* 显示检测到物体（图片）
static void __ShowDetectedObjectsOfPicture(Mat mShowImg, vector<RecogCategory>& vObjects, iOCV2DNNObjectDetector *pobjDetector)
{
	//* 识别耗费的时间	
	cout << "detection time: " << GetTimeSpentInNetDetection(pobjDetector->GetNet()) << " ms" << endl;

	pobjDetector->MarkObject(mShowImg, vObjects);

	Mat mDstImg = mShowImg;

	INT nPCScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	INT nPCScreenHeight = GetSystemMetrics(SM_CYSCREEN);
	if (mShowImg.cols > nPCScreenWidth || mShowImg.rows > nPCScreenHeight)
	{
		resize(mShowImg, mDstImg, Size((mShowImg.cols * 2) / 3, (mShowImg.rows * 2) / 3), 0, 0, INTER_AREA);
	}

	imshow("Object Detector", mDstImg);
	waitKey(0);
}

//* 显示检测到物体（视频）
static void __MarkDetectedObjectsOfVideo(Mat mShowFrame, vector<RecogCategory>& vObjects, iOCV2DNNObjectDetector *pobjDetector)
{
	DOUBLE dblTimeSpent = GetTimeSpentInNetDetection(pobjDetector->GetNet());
	ostringstream oss;
	oss << "The time spent: " << dblTimeSpent << " ms";
	putText(mShowFrame, oss.str(), Point(mShowFrame.cols - 250, 20), 0, 0.5, Scalar(0, 0, 255), 2);

	pobjDetector->MarkObject(mShowFrame, vObjects);	
}

//* 播放视频
static BOOL __PlayVideo(VLCVideoPlayer& objVideoPlayer, const string& strFile, BOOL blIsVideoFile)
{
	//* 声明一个VLC播放器，并建立播放窗口	
	cvNamedWindow(strFile.c_str(), CV_WINDOW_AUTOSIZE);

	if (blIsVideoFile)
		objVideoPlayer.OpenVideoFromFile(strFile.c_str(), NULL, strFile.c_str());
	else
		objVideoPlayer.OpenVideoFromeRtsp(strFile.c_str(), NULL, strFile.c_str(), 500);

	//* 开始播放
	if (!objVideoPlayer.start())
	{
		cout << "Video playback failed!" << endl;
		return FALSE;
	}

	return TRUE;
}

//* OCV2 DNN接口之图片检测器
static void __OCV2DNNPictureDetector(const string& strFile, iOCV2DNNObjectDetector *pobjDetector)
{
	Mat mSrcImg = imread(strFile);

	vector<RecogCategory> vObjects;
	pobjDetector->detect(mSrcImg, vObjects);

	__ShowDetectedObjectsOfPicture(mSrcImg, vObjects, pobjDetector);
}

//* 用于设置入侵检测区域的鼠标事件回调函数
static void EHSetAlarmArea_OnMouse(INT nEvent, INT x, INT y, INT nFlags, void *pvRect)
{
	//* 注意设置入侵检测区域时一定要顺时针设置4个点
	if (nEvent == EVENT_LBUTTONUP)
	{		
		((vector<Point> *)pvRect)->push_back(Point(x, y));						
	}
}

//* 保存设置的入侵检测区域
static void __SaveAlarmArea(vector<vector<Point>>& vvptRects)
{
	FileStorage objFS("alarm_area.xml", FileStorage::WRITE);
	ostringstream oss;
	INT nRectNum = vvptRects.size();
	//cout << "__SaveAlarmArea: " << nRectNum << endl;
	objFS << "RECT_NUM" << nRectNum;
	for (INT i = 0; i < nRectNum; i++)
	{
		oss.str("");
		oss << "RECT_" << i;
		objFS << oss.str() << vvptRects[i];
		//cout << oss.str() << endl;
	}

	objFS.release();
}

//* 显示设置的入侵检测报警区域
static void __ShowAlarmArea(Mat& mShowFrame, vector<vector<Point>>& vvptRects, vector<Point>& vptRect)
{
	if (blIsSetAlarmArea)
	{
		for (INT i = 0; i < vptRect.size(); i++)
		{
			circle(mShowFrame, vptRect[i], 2, Scalar(0, 255, 0), 2);			
			if (i)
				line(mShowFrame, vptRect[i - 1], vptRect[i], Scalar(0, 0, 255), 2);
		}

		//* 保存入侵检测区域
		if (vptRect.size() > 3)
		{			
			vvptRects.push_back(vptRect);

			vptRect.clear();
		}
	}

	//* 画出入侵检测区域
	for (INT i = 0; i < vvptRects.size(); i++)
	{
		for (INT k = 0; k < vvptRects[i].size(); k++)
		{
			circle(mShowFrame, vvptRects[i][k], 2, Scalar(0, 255, 0), 2);
			if (k)
			{
				if (nAlarmAreaIdx == i)
					line(mShowFrame, vvptRects[i][k - 1], vvptRects[i][k], Scalar(0, 0, 255), 2);
				else
					line(mShowFrame, vvptRects[i][k - 1], vvptRects[i][k], Scalar(0, 255, 0), 2);
			}				
		}

		if(nAlarmAreaIdx == i)
			line(mShowFrame, vvptRects[i][3], vvptRects[i][0], Scalar(0, 0, 255), 2);
		else
			line(mShowFrame, vvptRects[i][3], vvptRects[i][0], Scalar(0, 255, 0), 2);
	}
}

//* 加载报警区域
static void __LoadAlarmArea(vector<vector<Point>>& vvptRects)
{
	INT nRectNum = 0;
	FileStorage objFS("alarm_area.xml", FileStorage::READ);
	objFS["RECT_NUM"] >> nRectNum;

	ostringstream oss;
	for (INT i = 0; i < nRectNum; i++)
	{		
		vector<Point> vptRect;
		oss << "RECT_" << i;
		objFS[oss.str()] >> vptRect;
		vvptRects.push_back(vptRect);		
		oss.str("");
	}

	objFS.release();
}

//* 入侵检测
static INT __IntrusionDetect(vector<vector<Point>>& vvptRects, vector<RecogCategory>& vObjects)
{
	vector<RecogCategory>::iterator itObject = vObjects.begin();

	for (; itObject != vObjects.end(); itObject++)
	{
		RecogCategory object = *itObject;

		Point ptBottomCenter(object.nLeftTopX + (object.nRightBottomX - object.nLeftTopX) / 2, object.nRightBottomY - 50);
		//cout << "Center: " << ptBottomCenter << endl;
		for (INT i = 0; i < vvptRects.size(); i++)
		{
			//* 顺时针设置4个点
			Point ptLeftTop     = vvptRects[i][0];
			Point ptRightTop    = vvptRects[i][1];
			Point ptLeftBottom  = vvptRects[i][3];
			Point ptRightBottom = vvptRects[i][2];

			INT nMinX = 0x7FFFFFFF, nMaxX = 0, nMinY = 0x7FFFFFFF, nMaxY = 0;
			for (INT k = 0; k < 4; k++)
			{
				if (vvptRects[i][k].x < nMinX)
					nMinX = vvptRects[i][k].x;
				if (vvptRects[i][k].x > nMaxX)
					nMaxX = vvptRects[i][k].x;
				if (vvptRects[i][k].y < nMinY)
					nMinY = vvptRects[i][k].y;
				if (vvptRects[i][k].y > nMaxY)
					nMaxY = vvptRects[i][k].y;
			}

			//cout << "LT: " << ptLeftTop << " RT: " << ptRightTop << " LB: " << ptLeftBottom << " RB: " << ptRightBottom << endl;
			//cout << "X: [" << nMinX << ", " << nMaxX << "] Y: [" << nMinY << ", " << nMaxY << "]" << endl;

			if (ptBottomCenter.x > nMinX
				&& ptBottomCenter.x < nMaxX
				&& ptBottomCenter.y > nMinY
				&& ptBottomCenter.y < nMaxY)
			{
				return i;
			}
		}
	}

	return 0xFFFFFFFF;
}

//* OCV2 DNN接口之视频检测器，参数blIsVideoFile为"真"，则意味这是一个视频文件，"假"则意味着是rtsp流
static void __OCV2DNNVideoDetector(const string& strFile, iOCV2DNNObjectDetector *pobjDetector, BOOL blIsVideoFile)
{
	vector<vector<Point>> vvptRects;
	vector<Point> vptRect;	

	nAlarmAreaIdx = 0xFFFFFFFF;

	//* 声明一个VLC播放器，并启动播放
	VLCVideoPlayer objVideoPlayer;
	if (!__PlayVideo(objVideoPlayer, strFile, blIsVideoFile))
		return;

	//* 是否开启了设置报警区域项
	if (blIsSetAlarmArea)
	{
		//* 注册回调函数
		setMouseCallback(strFile, EHSetAlarmArea_OnMouse, &vptRect);
	}

	//* 开启了入侵检测，则只检测人体不检测其它物体了
	vector<string> vstrFilter;
	if (blIsIntrusionAlarm)
	{		
		vstrFilter.push_back(string("person"));

		//* 加载区域数据
		__LoadAlarmArea(vvptRects);
	}

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
			if (!blIsVideoFile)
				cout << "The connection with the RTSP stream is disconnected, unable to get the next frame, the process will exit." << endl;

			break;
		}

		Mat mSrcFrame = objVideoPlayer.GetNextFrame();
		if (mSrcFrame.empty())
			continue;

		if (!blIsSetAlarmArea)
		{
			vector<RecogCategory> vObjects;
			
			pobjDetector->detect(mSrcFrame, vObjects, vstrFilter);						

			//* 开启了入侵检测
			if (blIsIntrusionAlarm)
			{
				nAlarmAreaIdx = __IntrusionDetect(vvptRects, vObjects);
			}
			else
				__MarkDetectedObjectsOfVideo(mSrcFrame, vObjects, pobjDetector);
		}

		__ShowAlarmArea(mSrcFrame, vvptRects, vptRect);		

		imshow(strFile, mSrcFrame);
	}

	//* 其实不调用这个函数也可以，VLCVideoPlayer类的析构函数会主动调用的
	objVideoPlayer.stop();

	cv::destroyAllWindows();

	//* 如果是设置入侵检测区域则保存设置好的区域数据
	if (blIsSetAlarmArea)
	{
		__SaveAlarmArea(vvptRects);
	}
}

//* OCV2 DNN接口之SSD模型检测器
static void __OCV2DNNSSDModelDetector(string& strFile, BOOL blIsPicture, OCV2DNNObjectDetectorSSD::ENUM_DETECTOR enumSSDType)
{
	iOCV2DNNObjectDetector *pobjDetector;

	try {
		pobjDetector = new OCV2DNNObjectDetectorSSD(0.5F, enumSSDType);
	}
	catch (runtime_error& err) {
		cout << err.what() << endl;
		return;
	}
		
	if (blIsPicture)
	{
		__OCV2DNNPictureDetector(strFile, pobjDetector);
	}
	else
	{
		if(enumSSDType == OCV2DNNObjectDetectorSSD::VGGSSD)
			cout << "VGG model does not support video classifier." << endl;
		else
		{
			//* 看看是实时视频流还是视频文件，根据文件名前缀判断即可
			std::transform(strFile.begin(), strFile.end(), strFile.begin(), ::tolower);
			if (strFile.find("rtsp:", 0) == 0)
			{
				__OCV2DNNVideoDetector(strFile, pobjDetector, FALSE);
			}
			else
			{
				__OCV2DNNVideoDetector(strFile, pobjDetector, TRUE);
			}
		}
	}

	delete pobjDetector;
}

//* OCV2 DNN接口之Yolo2模型检测器
static void __OCV2DNNYolo2ModelDetector(string& strFile, BOOL blIsPicture, OCV2DNNObjectDetectorYOLO2::ENUM_DETECTOR enumYolo2Type)
{
	iOCV2DNNObjectDetector *pobjDetector;

	try {
		pobjDetector = new OCV2DNNObjectDetectorYOLO2(0.5F, enumYolo2Type);
	}
	catch (runtime_error& err) {
		cout << err.what() << endl;
		return;
	}
	
	if (blIsPicture)
	{
		__OCV2DNNPictureDetector(strFile, pobjDetector);
	}
	else
	{
		//* 看看是实时视频流还是视频文件，根据文件名前缀判断即可
		std::transform(strFile.begin(), strFile.end(), strFile.begin(), ::tolower);		
		if (strFile.find("rtsp:", 0) == 0)
		{					
			__OCV2DNNVideoDetector(strFile, pobjDetector, FALSE);
		}
		else
		{			
			__OCV2DNNVideoDetector(strFile, pobjDetector, TRUE);
		}
	}

	delete pobjDetector;
}

//* 通用分类器，对图像和视频进行物体分类，分类模型采用预训练模型和针对特定类别的自训练模型
int _tmain(int argc, _TCHAR* argv[])
{
	CommandLineParser objCmdLineParser(argc, argv, pszCmdLineArgs);

	if (!IsCommandLineArgsValid(pszCmdLineArgs, argc, argv) || objCmdLineParser.has("help"))
	{
		cout << "This is a classifier program, using the pre training model to detect and classify objects in the image. " << endl;
		objCmdLineParser.printMessage();

		return 0;
	}

	//* 读取输入文件名
	string strFile = "";
	try {
		strFile = objCmdLineParser.get<string>(0);				
	} catch(Exception& ex) {
		cout << ex.what() << endl;
	}	
	if (strFile.empty())
	{
		cout << "Please input the image name or rtsp stream address." << endl;

		return 0;
	}
	
	//* 读取模型设置
	string strModel = "VGG";
	if (objCmdLineParser.has("model"))
	{
		strModel = objCmdLineParser.get<string>("model");
	}

	//* 读取报警参数设置
	blIsSetAlarmArea   = (BOOL)objCmdLineParser.get<bool>("set_alarm_area");
	blIsIntrusionAlarm = (BOOL)objCmdLineParser.get<bool>("intrusion_alarm");	
	if (blIsSetAlarmArea && blIsIntrusionAlarm)
	{
		cout << "Parameters set_alarm_area and intrusion_alarm cannot be TRUE at the same time." << endl;
		return 0;
	}

	//* 根据模型设置调用不同的函数进行处理
	if (strModel == "VGG")
	{	
		__OCV2DNNSSDModelDetector(strFile, (BOOL)objCmdLineParser.get<bool>("picture"), OCV2DNNObjectDetectorSSD::VGGSSD);
	}
	else if (strModel == "Yolo2")
	{
		__OCV2DNNYolo2ModelDetector(strFile, (BOOL)objCmdLineParser.get<bool>("picture"), OCV2DNNObjectDetectorYOLO2::YOLO2);
	}
	else if (strModel == "Yolo2-Tiny")
	{
		__OCV2DNNYolo2ModelDetector(strFile, (BOOL)objCmdLineParser.get<bool>("picture"), OCV2DNNObjectDetectorYOLO2::YOLO2_TINY);
	}
	else if (strModel == "Yolo2-VOC")
	{
		__OCV2DNNYolo2ModelDetector(strFile, (BOOL)objCmdLineParser.get<bool>("picture"), OCV2DNNObjectDetectorYOLO2::YOLO2_VOC);
	}
	else if (strModel == "Yolo2-Tiny-VOC")
	{
		__OCV2DNNYolo2ModelDetector(strFile, (BOOL)objCmdLineParser.get<bool>("picture"), OCV2DNNObjectDetectorYOLO2::YOLO2_TINY_VOC);
	}
	else if (strModel == "MobileNetSSD")
	{
		__OCV2DNNSSDModelDetector(strFile, (BOOL)objCmdLineParser.get<bool>("picture"), OCV2DNNObjectDetectorSSD::MOBNETSSD);
	}
	else 
	{
		cout << "Invalid pre train model name: " << strModel.c_str() << endl;
	}

    return 0;
}

