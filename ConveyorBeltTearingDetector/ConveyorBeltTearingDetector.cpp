// ConveyorBeltTearingDetector.cpp : 定义控制台应用程序的入口点。
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
#include "MathAlgorithmLib.h"
#include "ImagePreprocess.h"

#if NEED_GPU
#pragma comment(lib,"cublas.lib")
#pragma comment(lib,"cuda.lib")
#pragma comment(lib,"cudart.lib")
#pragma comment(lib,"curand.lib")
#pragma comment(lib,"cudnn.lib")
#endif

#define NEED_DEBUG_CONSOLE	1	//* 是否需要控制台窗口输出

#if !NEED_DEBUG_CONSOLE
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
#endif

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 2)
	{
		cout << "Usage: " << argv[0] << " [image path]" << endl;
		exit(-1);
	}

	Mat matSrcImg = imread(argv[1]);
	if (matSrcImg.empty())
	{
		cout << "图片文件『" << argv[1] << "』不存在或者格式不正确，进程退出!" << endl;
		exit(-1);
	}

	//* 样本图片采集的并不好，需要人为指定ROI，去掉地板等无用区域
	matSrcImg = matSrcImg(Rect(10, 50, matSrcImg.cols - 130, matSrcImg.rows - 100));

	//* 原始图片太大，再缩小一下	
	pyrDown(matSrcImg, matSrcImg, Size(matSrcImg.cols / 2, matSrcImg.rows / 2));

	//* 在对图像进行裂缝检测之前先处理一下，实测对比度均衡算法效果不错
	Mat matGrayImg, matPreprocImg;
	cvtColor(matSrcImg, matGrayImg, COLOR_BGR2GRAY);
	imgpreproc::ContrastEqualization(matGrayImg, matPreprocImg);
	imshow("预处理结果", matPreprocImg);

	//* 对检测到的轮廓分组，找出裂缝
	ImgGroupedContour img_grp_contour = ImgGroupedContour(matPreprocImg, 110, 110 * 2, 3, TRUE);
	img_grp_contour.GroupContours(30);
	img_grp_contour.GetDiagonalPointsOfGroupContours(10);
	img_grp_contour.RectMarkGroupContours(matSrcImg, TRUE);

	imshow("裂缝检测结果", matSrcImg);
	waitKey(0);

    return 0;
}

