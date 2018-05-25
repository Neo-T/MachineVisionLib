// FaceRecognition.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <vector>
#include <facedetect-dll.h>
#include "common_lib.h"
#include "MachineVisionLib.h"
#include "FaceRecognition.h"

BOOL FaceDatabase::LoadCaffeVGGNet(string strCaffePrototxtFile, string strCaffeModelFile)
{
	pcaflNet = caffe2shell::LoadNet<FLOAT>(strCaffePrototxtFile, strCaffeModelFile, caffe::TEST);

	if (pcaflNet)
	{
		pflMemDataLayer = (caffe::MemoryDataLayer<FLOAT> *)pcaflNet->layers()[0].get();
		return TRUE;
	}
	else
		return FALSE;
}

//FACE_PROCESSING_EXT Mat FaceProcessing(const Mat &img_, double gamma = 0.8, double sigma0 = 0, double sigma1 = 0, double mask = 0, double do_norm = 8);
Mat FaceDatabase::FaceChipsHandle(Mat& matFaceChips, DOUBLE dblPowerValue, DOUBLE dblGamma, DOUBLE dblNorm)
{
	FLOAT *pflData;

	//* 矩阵数据转成浮点类型
	Mat matFloat;
	matFaceChips.convertTo(matFloat, CV_32F);
		
	Mat matTransit(Size(matFloat.cols, matFloat.rows), CV_32F, Scalar(0));	//* 计算用的过渡矩阵
	
	//* 不允许gamma值为0，也就是强制提升阴影区域的对比度
	if (fabs(dblGamma) < 1e-6)
		dblGamma = 0.2;
	for (INT i = 0; i < matFloat.rows; i++)
	{
		for (INT j = 0; j < matFloat.cols; j++)
			matFloat.ptr<FLOAT>(i)[j] = pow(matFloat.ptr<float>(i)[j], dblGamma);		
	}

	//* 如果为0.0则直接跳到最后一步
	DOUBLE dblTrim = fabs(dblNorm);
	if (dblTrim < 1e-6)
		goto __lblEnd;
	
	//* matFloat矩阵指数运算并计算均指
	DOUBLE dblMeanVal = 0.0;
	matTransit = cv::abs(matFloat);
	for (INT i = 0; i<matFloat.rows; i++)
	{
		pflData = matTransit.ptr<FLOAT>(i);
		for (INT j = 0; j < matFloat.cols; j++)
		{
			pflData[j] = pow(matTransit.ptr<FLOAT>(i)[j], dblPowerValue);
			dblMeanVal += pflData[j];
		}
	}
	dblMeanVal /= (matFloat.rows * matFloat.cols);

	//* 求得的均值指数运算后与矩阵相除
	DOUBLE dblDivisor = cv::pow(dblMeanVal, 1/ dblPowerValue);
	for (INT i = 0; i < matFloat.rows; i++)
	{
		pflData = matFloat.ptr<FLOAT>(i);
		for (INT j = 0; j<matFloat.cols; j++)
			pflData[j] /= dblDivisor;
	}

	//* 减去超标的数值，然后再一次求均值
	dblMeanVal = 0.0;
	matTransit = cv::abs(matFloat);
	for (INT i = 0; i<matFloat.rows; i++)
	{
		pflData = matTransit.ptr<FLOAT>(i);
		for (INT j = 0; j < matFloat.cols; j++)
		{
			if (pflData[j] > dblTrim)
				pflData[j] = dblTrim;

			pflData[j] = pow(pflData[j], dblPowerValue);
			dblMeanVal += pflData[j];
		}
	}
	dblMeanVal /= (matFloat.rows * matFloat.cols);

	//* 再一次将均值进行指数计算后与矩阵相除
	dblDivisor = cv::pow(dblMeanVal, 1 / dblPowerValue);
	for (INT i = 0; i < matFloat.rows; i++)
	{
		pflData = matFloat.ptr<FLOAT>(i);
		for (INT j = 0; j<matFloat.cols; j++)
			pflData[j] /= dblDivisor;
	}

	//* 如果大于0.0
	if(dblNorm > 1e-6)
	{ 
		for (INT i = 0; i<matFloat.rows; i++)
		{
			pflData = matFloat.ptr<FLOAT>(i);
			for (INT j = 0; j<matFloat.cols; j++)
				pflData[j] = dblTrim * tanh(pflData[j] / dblTrim);
		}
	}

__lblEnd:
	//* 找出矩阵的最小值
	FLOAT flMin = matFloat.ptr<FLOAT>(0)[0];
	for (INT i = 0; i <matFloat.rows; i++)
	{
		pflData = matFloat.ptr<FLOAT>(i);
		for (INT j = 0; j < matFloat.cols; j++)
		{
			if (pflData[j] < flMin)
				flMin = pflData[j];
		}
	}

	//* 矩阵加最小值
	for (INT i = 0; i <matFloat.rows; i++)
	{
		pflData = matFloat.ptr<FLOAT>(i);
		for (INT j = 0; j < matFloat.cols; j++)	
				pflData[j] += flMin;
	}

	//* 归一化
	normalize(matFloat, matFloat, 0, 255, NORM_MINMAX);
	matFloat.convertTo(matFloat, CV_8UC1);

	return matFloat;
}

BOOL FaceDatabase::AddFace(Mat& matImg)
{
	Mat matFaceChips = cv2shell::ExtractFaceChips(matImg);
	if (matFaceChips.empty())
	{
		cout << "No face was detected." << endl;
		return FALSE;
	}

	imshow("pic", matFaceChips);
	waitKey(0);

	//* ROI(region of interest)
	Mat matFaceROI = FaceChipsHandle(matFaceChips);

	vector<FLOAT> vImgFeature;
	caffe2shell::ExtractFeature(pcaflNet, pflMemDataLayer, matFaceROI, vImgFeature, FACE_FEATURE_DIMENSION, FACE_FEATURE_LAYER_NAME);


	return TRUE;
}

BOOL FaceDatabase::AddFace(const CHAR *pszImgName)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "AddFace() error：unable to read the picture, please confirm that the picture『" << pszImgName << "』 exists and the format is corrrect." << endl;

		return FALSE;
	}

	return AddFace(matImg);
}

