// ObjectClassifier.cpp : �������̨Ӧ�ó������ڵ㡣
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
BOOL blIsSetAlarmArea;		//* �Ƿ�������Ա���ֱ�������
BOOL blIsIntrusionAlarm;	//* �Ƿ�����Ա���ֱ���
INT nAlarmAreaIdx;

//* Usage��ʹ��˵��
static const CHAR *pszCmdLineArgs = {
	"{help            h |     | print help message}"
	"{@file             |     | input data source, image file path or rtsp stream address}"
	"{model           m | VGG | which model to use, value is VGG��Yolo2��Yolo2-Tiny��Yolo2-VOC��Yolo2-Tiny-VOC��MobileNetSSD}"
	"{picture         p |     | image file detector, non video detector}"
	"{set_alarm_area  s |     | set up an alarm area for intrusion}"
	"{intrusion_alarm a |     | intrusion alarm}"
};

//* ��ʾ��⵽���壨ͼƬ��
static void __ShowDetectedObjectsOfPicture(Mat mShowImg, vector<RecogCategory>& vObjects, iOCV2DNNObjectDetector *pobjDetector)
{
	//* ʶ��ķѵ�ʱ��	
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

//* ��ʾ��⵽���壨��Ƶ��
static void __MarkDetectedObjectsOfVideo(Mat mShowFrame, vector<RecogCategory>& vObjects, iOCV2DNNObjectDetector *pobjDetector)
{
	DOUBLE dblTimeSpent = GetTimeSpentInNetDetection(pobjDetector->GetNet());
	ostringstream oss;
	oss << "The time spent: " << dblTimeSpent << " ms";
	putText(mShowFrame, oss.str(), Point(mShowFrame.cols - 250, 20), 0, 0.5, Scalar(0, 0, 255), 2);

	pobjDetector->MarkObject(mShowFrame, vObjects);	
}

//* ������Ƶ
static BOOL __PlayVideo(VLCVideoPlayer& objVideoPlayer, const string& strFile, BOOL blIsVideoFile)
{
	//* ����һ��VLC�����������������Ŵ���	
	cvNamedWindow(strFile.c_str(), CV_WINDOW_AUTOSIZE);

	if (blIsVideoFile)
		objVideoPlayer.OpenVideoFromFile(strFile.c_str(), NULL, strFile.c_str());
	else
		objVideoPlayer.OpenVideoFromeRtsp(strFile.c_str(), NULL, strFile.c_str(), 500);

	//* ��ʼ����
	if (!objVideoPlayer.start())
	{
		cout << "Video playback failed!" << endl;
		return FALSE;
	}

	return TRUE;
}

//* OCV2 DNN�ӿ�֮ͼƬ�����
static void __OCV2DNNPictureDetector(const string& strFile, iOCV2DNNObjectDetector *pobjDetector)
{
	Mat mSrcImg = imread(strFile);

	vector<RecogCategory> vObjects;
	pobjDetector->detect(mSrcImg, vObjects);

	__ShowDetectedObjectsOfPicture(mSrcImg, vObjects, pobjDetector);
}

//* �����������ּ�����������¼��ص�����
static void EHSetAlarmArea_OnMouse(INT nEvent, INT x, INT y, INT nFlags, void *pvRect)
{
	//* ע���������ּ������ʱһ��Ҫ˳ʱ������4����
	if (nEvent == EVENT_LBUTTONUP)
	{		
		((vector<Point> *)pvRect)->push_back(Point(x, y));						
	}
}

//* �������õ����ּ������
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

//* ��ʾ���õ����ּ�ⱨ������
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

		//* �������ּ������
		if (vptRect.size() > 3)
		{			
			vvptRects.push_back(vptRect);

			vptRect.clear();
		}
	}

	//* �������ּ������
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

//* ���ر�������
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

//* ���ּ��
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
			//* ˳ʱ������4����
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

//* OCV2 DNN�ӿ�֮��Ƶ�����������blIsVideoFileΪ"��"������ζ����һ����Ƶ�ļ���"��"����ζ����rtsp��
static void __OCV2DNNVideoDetector(const string& strFile, iOCV2DNNObjectDetector *pobjDetector, BOOL blIsVideoFile)
{
	vector<vector<Point>> vvptRects;
	vector<Point> vptRect;	

	nAlarmAreaIdx = 0xFFFFFFFF;

	//* ����һ��VLC������������������
	VLCVideoPlayer objVideoPlayer;
	if (!__PlayVideo(objVideoPlayer, strFile, blIsVideoFile))
		return;

	//* �Ƿ��������ñ���������
	if (blIsSetAlarmArea)
	{
		//* ע��ص�����
		setMouseCallback(strFile, EHSetAlarmArea_OnMouse, &vptRect);
	}

	//* ���������ּ�⣬��ֻ������岻�������������
	vector<string> vstrFilter;
	if (blIsIntrusionAlarm)
	{		
		vstrFilter.push_back(string("person"));

		//* ������������
		__LoadAlarmArea(vvptRects);
	}

	CHAR bKey;
	BOOL blIsPaused = FALSE;
	while (TRUE)
	{
		bKey = waitKey(10);
		if (27 == bKey)
			break;

		//* �ո���ͣ�������ͣ������ͣ���������Ỻ��һ��ʱ���ʵʱ��Ƶ��������֮����ָ����ź���Ƶ�뵱ǰʱ�̴���һ����ʱ���
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

			//* ���������ּ��
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

	//* ��ʵ�������������Ҳ���ԣ�VLCVideoPlayer��������������������õ�
	objVideoPlayer.stop();

	cv::destroyAllWindows();

	//* ������������ּ�������򱣴����úõ���������
	if (blIsSetAlarmArea)
	{
		__SaveAlarmArea(vvptRects);
	}
}

//* OCV2 DNN�ӿ�֮SSDģ�ͼ����
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
			//* ������ʵʱ��Ƶ��������Ƶ�ļ��������ļ���ǰ׺�жϼ���
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

//* OCV2 DNN�ӿ�֮Yolo2ģ�ͼ����
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
		//* ������ʵʱ��Ƶ��������Ƶ�ļ��������ļ���ǰ׺�жϼ���
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

//* ͨ�÷���������ͼ�����Ƶ����������࣬����ģ�Ͳ���Ԥѵ��ģ�ͺ�����ض�������ѵ��ģ��
int _tmain(int argc, _TCHAR* argv[])
{
	CommandLineParser objCmdLineParser(argc, argv, pszCmdLineArgs);

	if (!IsCommandLineArgsValid(pszCmdLineArgs, argc, argv) || objCmdLineParser.has("help"))
	{
		cout << "This is a classifier program, using the pre training model to detect and classify objects in the image. " << endl;
		objCmdLineParser.printMessage();

		return 0;
	}

	//* ��ȡ�����ļ���
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
	
	//* ��ȡģ������
	string strModel = "VGG";
	if (objCmdLineParser.has("model"))
	{
		strModel = objCmdLineParser.get<string>("model");
	}

	//* ��ȡ������������
	blIsSetAlarmArea   = (BOOL)objCmdLineParser.get<bool>("set_alarm_area");
	blIsIntrusionAlarm = (BOOL)objCmdLineParser.get<bool>("intrusion_alarm");	
	if (blIsSetAlarmArea && blIsIntrusionAlarm)
	{
		cout << "Parameters set_alarm_area and intrusion_alarm cannot be TRUE at the same time." << endl;
		return 0;
	}

	//* ����ģ�����õ��ò�ͬ�ĺ������д���
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

