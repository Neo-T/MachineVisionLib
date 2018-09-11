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

//* Usage，使用说明
static const CHAR *pszCmdLineArgs = {
	"{help    h   |     | print help message}"
	"{@file       |     | input data source, image file path or rtsp stream address}"
	"{model   m   | VGG | which model to use, value is VGG、Yolo2 or Yolo3}"
	"{picture p   |     | image file classification, non video calssification}"
};

//* VGG模型之图片分类器
static void __VGGModelClassifierOfPicture(Net& objDNNNet, vector<string>& vClassNames, const string& strFile)
{
	Mat mSrcImg = imread(strFile);

	vector<RecogCategory> vObjects;
	ObjectDetect(mSrcImg, objDNNNet, vClassNames, vObjects);

	MarkObjectWithRectangle(mSrcImg, vObjects);

	Mat mDstImg;
	resize(mSrcImg, mDstImg, Size(mSrcImg.cols / 2, mSrcImg.rows / 2), 0, 0, INTER_AREA);
	imshow("Object Classifier", mDstImg);
	waitKey(0);
}

//* VGG模型之视频分类器，参数blIsVideoFile为"真"，则意味这是一个视频文件，"假"则意味着是rtsp流
static void __VGGModelClassifierOfVideo(Net& objDNNNet, vector<string>& vClassNames, const string& strFile, BOOL blIsVideoFile)
{

}

//* VGG模型分类器
static void __VGGModelClassifier(string& strFile, BOOL blIsPicture)
{
	vector<string> vClassNames;
	Net objDNNNet = InitLightClassifier(vClassNames);
	if (objDNNNet.empty())
	{
		cout << "The initialization lightweight classifier failed, probably because the model file or voc.names file was not found." << endl;
		return;
	}

	
	if (blIsPicture)
	{
		__VGGModelClassifierOfPicture(objDNNNet, vClassNames, strFile);
	}
	else
	{
		//* 看看是实时视频流还是视频文件，根据文件名前缀判断即可
		std::transform(strFile.begin(), strFile.end(), strFile.begin(), ::tolower);
		if (strFile.find("rtsp:", 0) == 0)
		{
			__VGGModelClassifierOfVideo(objDNNNet, vClassNames, strFile, FALSE);
		}
		else
			__VGGModelClassifierOfVideo(objDNNNet, vClassNames, strFile, TRUE);
	}
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
	}catch(Exception& ex){}	
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

	//* 根据模型设置调用不同的函数进行处理
	if (strModel == "VGG")
	{	
		__VGGModelClassifier(strFile, (BOOL)objCmdLineParser.get<bool>("picture"));
	}
	else if (strModel == "Yolo2")
	{
		
	}
	else if (strModel == "Yolo3")
	{
		
	}
	else 
	{
		cout << "Invalid pre train model name: " << strModel.c_str() << endl;
	}

    return 0;
}

