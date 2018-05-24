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
	caflNet = caffe2shell::LoadNet<FLOAT>(strCaffePrototxtFile, strCaffeModelFile, caffe::TEST);

	if (caflNet)
		return TRUE;
	else
		return FALSE;
}

BOOL FaceDatabase::AddFace(Mat& matFace)
{
	return FALSE;
}

BOOL FaceDatabase::AddFace(const CHAR *pszImgName)
{
	Mat matFaceChips = cv2shell::ExtractFaceChips(pszImgName);

	imshow("pic", matFaceChips);
	waitKey(0);

	FaceChipsHandle(matFaceChips, 0);

	return AddFace(matFaceChips);
}

//FACE_PROCESSING_EXT Mat FaceProcessing(const Mat &img_, double gamma = 0.8, double sigma0 = 0, double sigma1 = 0, double mask = 0, double do_norm = 8);
Mat FaceDatabase::FaceChipsHandle(Mat& matFaceChips, DOUBLE dblPowerValue, 
									DOUBLE dblGamma, DOUBLE dblSigma0, DOUBLE dblSigma1, DOUBLE dblNorm)
{
	FLOAT *pflData;

	//* 矩阵数据转成浮点类型
	Mat matFloat;
	matFaceChips.convertTo(matFloat, CV_32F);
	
	INT nExtendedBoundaryLen = floor(3 * abs(dblSigma1));	//* 图形边界要扩充的尺寸
	Mat matExtended(Size(matFloat.cols + 2 * nExtendedBoundaryLen, matFloat.rows + 2 * nExtendedBoundaryLen), CV_32F, Scalar(0));	//* 根据原图尺寸建立一个四个边界等距扩展的矩阵，用于存储计算过程数据
	Mat matTransit(Size(matFloat.cols, matFloat.rows), CV_32F, Scalar(0));	//* 计算用的过渡矩阵
	
	//* 不允许gamma值为0，也就是强制提升阴影区域的对比度
	if (fabs(dblGamma) < 1e-6)
		dblGamma = 0.2;
	for (INT i = 0; i < matFloat.rows; i++)
	{
		for (INT j = 0; j < matFloat.cols; j++)
			matFloat.ptr<FLOAT>(i)[j] = pow(matFloat.ptr<float>(i)[j], dblGamma);		
	}

	//* 如果没有扩充边界，则直接进入下一步计算
	if (fabs(dblSigma1) < 1e-6)
		goto __lblDoNormalizes;

	//* 逐行读取图像数据到扩展矩阵，最终达成如下图所示的效果：
	/*
	             上方边界
	左上角 =================== 右上角
	       ||               ||
	     左||    扩充前的   ||右
	     边||    图像区域   ||边
	     界||               ||界
		   ||               ||
	左下角 =================== 右下角
	             下方边界
	*/
	for (INT i = 0; i < matExtended.rows; i++)
	{
		pflData = matExtended.ptr<FLOAT>(i);
		for (INT j = 0; j < matExtended.cols; j++)
		{
			//* 读取图像原有数据区，也就是扩充边界包围起来的实际图像数据区域（扩充前的图像数据）
			if (i >= nExtendedBoundaryLen && 
				i < matFloat.rows + nExtendedBoundaryLen && 
				j >= nExtendedBoundaryLen && 
				j < matFloat.cols + nExtendedBoundaryLen)
				pflData[j] = matFloat.ptr<FLOAT>(i - nExtendedBoundaryLen)[j - nExtendedBoundaryLen];
			//* 左上角，用原始图像左上角的数据填充
			else if (i < nExtendedBoundaryLen && j < nExtendedBoundaryLen)	
				pflData[j] = matFloat.ptr<FLOAT>(0)[0];
			//* 右上角，用原始图像右上角的数据填充
			else if (i < nExtendedBoundaryLen && j >= matFloat.cols + nExtendedBoundaryLen)
				pflData[j] = matFloat.ptr<FLOAT>(0)[matFloat.cols - 1];
			//* 左下角，用原始图像左下角的数据填充
			else if (i >= matFloat.rows + nExtendedBoundaryLen && j < nExtendedBoundaryLen)
				pflData[j] = matFloat.ptr<FLOAT>(matFloat.rows - 1)[0];
			//* 右下角，用原始图像右下角的数据填充
			else if (i >= matFloat.rows + nExtendedBoundaryLen && j >= matFloat.cols + nExtendedBoundaryLen)
				pflData[j] = matFloat.ptr<FLOAT>(matFloat.rows - 1)[matFloat.cols - 1];
			//* 上扩充边界，用原始图像最顶端一行的数据填充
			else if (i < nExtendedBoundaryLen && j >= nExtendedBoundaryLen && j < matFloat.cols + nExtendedBoundaryLen)
				pflData[j] = matFloat.ptr<FLOAT>(0)[j - nExtendedBoundaryLen];
			//* 下扩充边界,用原始图像最底端一行的数据填充
			else if (i >= matFloat.rows + nExtendedBoundaryLen && j >= nExtendedBoundaryLen && j < matFloat.cols + nExtendedBoundaryLen)
				pflData[j] = matFloat.ptr<FLOAT>(matFloat.rows - 1)[j - nExtendedBoundaryLen];
			//* 左扩充边界，用原始图像最左边一行的数据填充
			else if (j < nExtendedBoundaryLen && i >= nExtendedBoundaryLen && i < matFloat.rows + nExtendedBoundaryLen)
				pflData[j] = matFloat.ptr<FLOAT>(i - nExtendedBoundaryLen)[0];
			//* 右扩充边界，用原始图像最右边一行的数据填充
			else if (j >= matFloat.cols + nExtendedBoundaryLen && i >= nExtendedBoundaryLen && i < matFloat.rows + nExtendedBoundaryLen)
				pflData[j] = matFloat.ptr<FLOAT>(i - nExtendedBoundaryLen)[matFloat.cols - 1];
			else;
		}
	}

__lblDoNormalizes:
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

