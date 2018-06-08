#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <vector>
#include <dlib/image_processing.h>
#include <dlib/opencv.h>
#include <facedetect-dll.h>
#include "common_lib.h"
#include "MachineVisionLib.h"
#include "ImagePreprocess.h"

//* 增强图像清晰度，注意输入图像一定是灰度图，关于灰度级和动态范围的概念：
//* https://blog.csdn.net/hit2015spring/article/details/50448025
//* 直方图均衡化增强的理论地址（注意，文档里的L就是灰度值总数，也就是256个灰度值：0-255）：
//* https://www.cnblogs.com/newpanderking/articles/2950242.html
//* http://lib.csdn.net/article/aiframework/52656
IMGPREPROC_API void imgpreproc::HistogramEnhancedDefinition(Mat& matInputGrayImg, Mat& matDestImg)
{
	//* 总像素数
	DOUBLE dblPixelNum = matInputGrayImg.rows * matInputGrayImg.cols;

	INT naGrayLevelStatistic[256] = { 0 };			//* 灰度级统计数组，统计各灰度值在图像中的数量，初始化为0
	for (INT i = 0; i < matInputGrayImg.rows; i++)	//* 灰度值的范围为0-255，通过两个循环我们把256个灰度值在图像中出现的次数统计出来，并保存到数组中，也就是获取该图像的灰度级分布数据
	{
		UCHAR *pubData = matInputGrayImg.ptr<UCHAR>(i);
		for (INT j = 0; j < matInputGrayImg.cols; j++)
		{
			//* 累计该灰度值的数量，反正灰度值范围0-255，不会超过数组范围，通过统计我们就知道该幅图像的灰度分布情况了
			naGrayLevelStatistic[pubData[j]]++;
		}
	}

	//* 计算灰度级直方图,也就是前面统计出来的灰度级概率密度
	DOUBLE dblaGrayLevelRatio[256];
	for (INT i = 0; i < sizeof(dblaGrayLevelRatio) / sizeof(DOUBLE); i++)
	{
		dblaGrayLevelRatio[i] = (DOUBLE)naGrayLevelStatistic[i] / dblPixelNum;
	}

	//* 计算灰度变换系数，或者说累积分布函数
	DOUBLE dblaDestGrayLevelRatio[256];
	dblaDestGrayLevelRatio[0] = dblaGrayLevelRatio[0];
	for (INT i = 1; i < sizeof(dblaDestGrayLevelRatio) / sizeof(DOUBLE); i++)
	{
		dblaDestGrayLevelRatio[i] = dblaDestGrayLevelRatio[i-1] + dblaGrayLevelRatio[i];
	}

	//* 根据计算的灰度变换系数变换灰度得到目标图
	for (INT i = 0; i < matInputGrayImg.rows; i++)
	{
		UCHAR *pubData = matInputGrayImg.ptr<UCHAR>(i);
		UCHAR *pubDstData = matDestImg.ptr<UCHAR>(i);
		for (INT j = 0; j < matInputGrayImg.cols; j++)
		{
			pubDstData[j] = 255 * dblaDestGrayLevelRatio[pubData[j]];
		}
	}
}

