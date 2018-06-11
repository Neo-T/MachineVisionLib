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
#include "MathAlgorithmLib.h"
#include "ImagePreprocess.h"

//* 利用直方图均衡算法增强图像对比度和清晰度，注意输入图像一定是灰度图，关于灰度级和动态范围的概念：
//* https://blog.csdn.net/hit2015spring/article/details/50448025
//* 直方图均衡化增强的理论地址（注意，文档里的L就是灰度值总数，也就是256个灰度值：0-255）：
//* https://www.cnblogs.com/newpanderking/articles/2950242.html
//* http://lib.csdn.net/article/aiframework/52656
IMGPREPROC void imgpreproc::HistogramEqualization(Mat& matInputGrayImg, Mat& matDestImg)
{
	matDestImg = Mat(matInputGrayImg.rows, matInputGrayImg.cols, CV_8UC1);

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
	DOUBLE dblaGrayLevelProb[256];
	for (INT i = 0; i < sizeof(dblaGrayLevelProb) / sizeof(DOUBLE); i++)
	{
		dblaGrayLevelProb[i] = (DOUBLE)naGrayLevelStatistic[i] / dblPixelNum;
	}

	//* 我觉得把下面的计算称为计算灰度变换系数更直观一些，专业的说法叫做累积分布函数
	DOUBLE dblaDestGrayLevelProb[256];
	dblaDestGrayLevelProb[0] = dblaGrayLevelProb[0];
	for (INT i = 1; i < sizeof(dblaDestGrayLevelProb) / sizeof(DOUBLE); i++)
	{
		dblaDestGrayLevelProb[i] = dblaDestGrayLevelProb[i-1] + dblaGrayLevelProb[i];
	}

	//* 根据计算的灰度变换系数变换灰度得到目标图
	for (INT i = 0; i < matInputGrayImg.rows; i++)
	{
		UCHAR *pubData = matInputGrayImg.ptr<UCHAR>(i);
		UCHAR *pubDstData = matDestImg.ptr<UCHAR>(i);
		for (INT j = 0; j < matInputGrayImg.cols; j++)
		{
			pubDstData[j] = 255 * dblaDestGrayLevelProb[pubData[j]];
		}
	}
}

//* 利用对比度均衡算法增强图像，理论地址：
//* https://blog.csdn.net/wuzuyu365/article/details/51898714
//* 其中，我们用到了gamma校正，相关资料如下：
//* https://blog.csdn.net/w450468524/article/details/51649651
IMGPREPROC void imgpreproc::ContrastEqualization(Mat& matInputGrayImg, Mat& matDestImg, DOUBLE dblGamma, DOUBLE dblPowerValue, DOUBLE dblNorm)
{
	FLOAT *pflData;

	//* 矩阵数据转成浮点类型
	matInputGrayImg.convertTo(matDestImg, CV_32F);

	Mat matTransit(Size(matDestImg.cols, matDestImg.rows), CV_32F, Scalar(0));	//* 计算用的过渡矩阵

	//* 不允许gamma值为0，也就是强制提升阴影区域的对比度
	if (fabs(dblGamma) < 1e-6)
		dblGamma = 0.2;
	for (INT i = 0; i < matDestImg.rows; i++)
	{
		pflData = matDestImg.ptr<FLOAT>(i);
		for (INT j = 0; j < matDestImg.cols; j++)
			pflData[j] = pow(pflData[j], dblGamma);
	}

	//* 如果为0.0则直接跳到最后一步
	DOUBLE dblTrim = fabs(dblNorm);
	if (dblTrim < 1e-6)
		goto __lblEnd;

	//* matDestImg矩阵指数运算并计算均指
	DOUBLE dblMeanVal = 0.0;
	matTransit = cv::abs(matDestImg);
	for (INT i = 0; i<matDestImg.rows; i++)
	{
		pflData = matTransit.ptr<FLOAT>(i);
		for (INT j = 0; j < matDestImg.cols; j++)
		{
			pflData[j] = pow(matTransit.ptr<FLOAT>(i)[j], dblPowerValue);
			dblMeanVal += pflData[j];
		}
	}
	dblMeanVal /= (matDestImg.rows * matDestImg.cols);

	//* 求得的均值指数运算后与矩阵相除
	DOUBLE dblDivisor = cv::pow(dblMeanVal, 1 / dblPowerValue);
	for (INT i = 0; i < matDestImg.rows; i++)
	{
		pflData = matDestImg.ptr<FLOAT>(i);
		for (INT j = 0; j<matDestImg.cols; j++)
			pflData[j] /= dblDivisor;
	}

	//* 减去超标的数值，然后再一次求均值
	dblMeanVal = 0.0;
	matTransit = cv::abs(matDestImg);
	for (INT i = 0; i<matDestImg.rows; i++)
	{
		pflData = matTransit.ptr<FLOAT>(i);
		for (INT j = 0; j < matDestImg.cols; j++)
		{
			if (pflData[j] > dblTrim)
				pflData[j] = dblTrim;

			pflData[j] = pow(pflData[j], dblPowerValue);
			dblMeanVal += pflData[j];
		}
	}
	dblMeanVal /= (matDestImg.rows * matDestImg.cols);

	//* 再一次将均值进行指数计算后与矩阵相除
	dblDivisor = cv::pow(dblMeanVal, 1 / dblPowerValue);
	for (INT i = 0; i < matDestImg.rows; i++)
	{
		pflData = matDestImg.ptr<FLOAT>(i);
		for (INT j = 0; j<matDestImg.cols; j++)
			pflData[j] /= dblDivisor;
	}

	//* 如果大于0.0
	if (dblNorm > 1e-6)
	{
		for (INT i = 0; i<matDestImg.rows; i++)
		{
			pflData = matDestImg.ptr<FLOAT>(i);
			for (INT j = 0; j<matDestImg.cols; j++)
				pflData[j] = dblTrim * tanh(pflData[j] / dblTrim);
		}
	}

__lblEnd:
	//* 找出矩阵的最小值
	FLOAT flMin = matDestImg.ptr<FLOAT>(0)[0];
	for (INT i = 0; i <matDestImg.rows; i++)
	{
		pflData = matDestImg.ptr<FLOAT>(i);
		for (INT j = 0; j < matDestImg.cols; j++)
		{
			if (pflData[j] < flMin)
				flMin = pflData[j];
		}
	}

	//* 矩阵加最小值
	for (INT i = 0; i <matDestImg.rows; i++)
	{
		pflData = matDestImg.ptr<FLOAT>(i);
		for (INT j = 0; j < matDestImg.cols; j++)
			pflData[j] += flMin;
	}

	//* 归一化
	normalize(matDestImg, matDestImg, 0, 255, NORM_MINMAX);
	matDestImg.convertTo(matDestImg, CV_8UC1);
}

//* 关于滤波核模板：
//* http://blog.sina.com.cn/s/blog_6ac784290101e47s.html
IMGPREPROC void imgpreproc::ContrastEqualizationWithFilter(Mat& matInputGrayImg, Mat& matDestImg, Size size, FLOAT *pflaFilterKernel, 
																DOUBLE dblGamma, DOUBLE dblPowerValue, DOUBLE dblNorm)
{
	FLOAT *pflData;

	//* 矩阵数据转成浮点类型
	matInputGrayImg.convertTo(matDestImg, CV_32F);

	Mat matTransit(Size(matDestImg.cols, matDestImg.rows), CV_32F, Scalar(0));	//* 计算用的过渡矩阵

	//* 不允许gamma值为0，也就是强制提升阴影区域的对比度
	if (fabs(dblGamma) < 1e-6)
		dblGamma = 0.2;
	for (INT i = 0; i < matDestImg.rows; i++)
	{
		pflData = matDestImg.ptr<FLOAT>(i);
		for (INT j = 0; j < matDestImg.cols; j++)
			pflData[j] = pow(pflData[j], dblGamma);
	}

	//* 滤波
	Mat matFilterKernel = Mat(size, CV_32FC1, pflaFilterKernel);
	filter2D(matDestImg, matDestImg, matDestImg.depth(), matFilterKernel);

	//* 如果为0.0则直接跳到最后一步
	DOUBLE dblTrim = fabs(dblNorm);
	if (dblTrim < 1e-6)
		goto __lblEnd;

	//* matDestImg矩阵指数运算并计算均指
	DOUBLE dblMeanVal = 0.0;
	matTransit = cv::abs(matDestImg);
	for (INT i = 0; i<matDestImg.rows; i++)
	{
		pflData = matTransit.ptr<FLOAT>(i);
		for (INT j = 0; j < matDestImg.cols; j++)
		{
			pflData[j] = pow(matTransit.ptr<FLOAT>(i)[j], dblPowerValue);
			dblMeanVal += pflData[j];
		}
	}
	dblMeanVal /= (matDestImg.rows * matDestImg.cols);

	//* 求得的均值指数运算后与矩阵相除
	DOUBLE dblDivisor = cv::pow(dblMeanVal, 1 / dblPowerValue);
	for (INT i = 0; i < matDestImg.rows; i++)
	{
		pflData = matDestImg.ptr<FLOAT>(i);
		for (INT j = 0; j<matDestImg.cols; j++)
			pflData[j] /= dblDivisor;
	}

	//* 减去超标的数值，然后再一次求均值
	dblMeanVal = 0.0;
	matTransit = cv::abs(matDestImg);
	for (INT i = 0; i<matDestImg.rows; i++)
	{
		pflData = matTransit.ptr<FLOAT>(i);
		for (INT j = 0; j < matDestImg.cols; j++)
		{
			if (pflData[j] > dblTrim)
				pflData[j] = dblTrim;

			pflData[j] = pow(pflData[j], dblPowerValue);
			dblMeanVal += pflData[j];
		}
	}
	dblMeanVal /= (matDestImg.rows * matDestImg.cols);

	//* 再一次将均值进行指数计算后与矩阵相除
	dblDivisor = cv::pow(dblMeanVal, 1 / dblPowerValue);
	for (INT i = 0; i < matDestImg.rows; i++)
	{
		pflData = matDestImg.ptr<FLOAT>(i);
		for (INT j = 0; j<matDestImg.cols; j++)
			pflData[j] /= dblDivisor;
	}

	//* 如果大于0.0
	if (dblNorm > 1e-6)
	{
		for (INT i = 0; i<matDestImg.rows; i++)
		{
			pflData = matDestImg.ptr<FLOAT>(i);
			for (INT j = 0; j<matDestImg.cols; j++)
				pflData[j] = dblTrim * tanh(pflData[j] / dblTrim);
		}
	}

__lblEnd:
	//* 找出矩阵的最小值
	FLOAT flMin = matDestImg.ptr<FLOAT>(0)[0];
	for (INT i = 0; i <matDestImg.rows; i++)
	{
		pflData = matDestImg.ptr<FLOAT>(i);
		for (INT j = 0; j < matDestImg.cols; j++)
		{
			if (pflData[j] < flMin)
				flMin = pflData[j];
		}
	}

	//* 矩阵加最小值
	for (INT i = 0; i <matDestImg.rows; i++)
	{
		pflData = matDestImg.ptr<FLOAT>(i);
		for (INT j = 0; j < matDestImg.cols; j++)
			pflData[j] += flMin;
	}

	//* 归一化
	normalize(matDestImg, matDestImg, 0, 255, NORM_MINMAX);
	matDestImg.convertTo(matDestImg, CV_8UC1);
}

//* 预处理函数，首先边缘检测、然后找出轮廓，最后将轮廓放入链表以备后续分组处理
void ImgGroupedContour::Preprocess(Mat& matSrcImg, DOUBLE dblThreshold1, DOUBLE dblThreshold2, INT nApertureSize, 
									DOUBLE dblGamma, DOUBLE dblPowerValue, DOUBLE dblNorm)
{
	Mat matGrayImg, matContrastEqualImg;

	cvtColor(matSrcImg, matGrayImg, COLOR_BGR2GRAY);
	imgpreproc::ContrastEqualization(matGrayImg, matContrastEqualImg, dblGamma, dblPowerValue, dblNorm);

	//* 使用Canny算法进行边缘检测，理论地址：
	//* https://blog.csdn.net/jia20003/article/details/41173767
	//* Opencv提供的Canny()函数说明:
	//* https://www.cnblogs.com/mypsq/p/4983566.html
	Canny(matContrastEqualImg, matGrayImg, dblThreshold1, dblThreshold2, nApertureSize);

	//* 寻找轮廓，相关资料：
	//* https://blog.csdn.net/dcrmg/article/details/51987348
	findContours(matGrayImg, vContours, vHierarchy, RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0));

	pstMallocMemForLink = pstNotGroupedContourLink = (PST_CONTOUR_NODE)malloc(vContours.size() * sizeof(ST_CONTOUR_NODE));
	if(!pstMallocMemForLink)
	{
		string strError = string("Preprocess()函数执行错误，无法为轮廓链表的建立申请一块动态内存！");
		throw runtime_error(strError);
	}

	//* 建立轮廓链表
	for (int i = 0; i < vContours.size(); i++)
	{
		//vector<Point> vPoints = vContours[i];
		//for (int j = 0; j < vPoints.size(); j++)
		//{
		//	cout << i << "-" << j << ": (" << vPoints[j].x << ", " << vPoints[j].y << ")" << endl;
		//}
		//cout << endl << endl;

		pstNotGroupedContourLink[i].pvecContour = &vContours[i];
		pstNotGroupedContourLink[i].nIndex = i;

		if (i)
		{
			pstNotGroupedContourLink[i - 1].pstNextNode = pstNotGroupedContourLink + i;
			pstNotGroupedContourLink[i].pstPrevNode = pstNotGroupedContourLink + (i - 1);
			pstNotGroupedContourLink[i].pstNextNode = NULL;
		}
		else
			pstNotGroupedContourLink[i].pstPrevNode = NULL;
	}
}

//* 将临近轮廓胶合在一起
void ImgGroupedContour::GlueAdjacentContour(INT nContourGroupIndex, DOUBLE dblDistanceThreshold)
{
	PST_CONTOUR_NODE pstNextContourNode = pstNotGroupedContourLink, pstFoundContourNode;
	BOOL blIsFoundGroupNode = FALSE;

__lblLoop:
	if (NULL == pstNextContourNode)
	{
		if (blIsFoundGroupNode) //* 重新开始找，看还有聚集的吗，之所这样设计是因为我们通过opencv获取的轮廓数据有可能分布并不连续，需要多次查找才能确定分布路径
		{
			blIsFoundGroupNode = FALSE;
			pstNextContourNode = pstNotGroupedContourLink;

			//* 胶合玩当前一波，再重新找一遍，看看未分组轮廓中是否还有与新胶合的轮廓相近的轮廓
			//* 注意这种情况是存在的，因为链表后面的轮廓可能与链表前面的轮廓相近
			goto __lblLoop;
		}
		else
		{
			return;
		}
	}

	PST_CONTOUR_NODE pstGroupNode = vGroupContour[nContourGroupIndex].pstContourLink;
	while (NULL != pstGroupNode)
	{
		DOUBLE dblDistance = malib::ShortestDistance(pstGroupNode->pvecContour, pstNextContourNode->pvecContour);
		if (dblDistance < dblDistanceThreshold)	//* 在许可的距离范围内，那么组团就行了
		{
			//* 先从原始链表中摘出来
			if (pstNextContourNode->pstPrevNode)
				pstNextContourNode->pstPrevNode->pstNextNode = pstNextContourNode->pstNextNode;
			else
			{
				//* 链表第一个节点没有上一个节点，此时要调整链表首节点
				pstNotGroupedContourLink = pstNextContourNode->pstNextNode;
			}

			if (pstNextContourNode->pstNextNode)
				pstNextContourNode->pstNextNode->pstPrevNode = pstNextContourNode->pstPrevNode;

			pstFoundContourNode = pstNextContourNode;
			pstNextContourNode = pstNextContourNode->pstNextNode;

			//* 然后将其放入该轮廓组中
			pstFoundContourNode->pstNextNode = pstGroupNode->pstNextNode;
			pstFoundContourNode->pstPrevNode = pstGroupNode;
			if (pstGroupNode->pstNextNode)
				pstGroupNode->pstNextNode->pstPrevNode = pstFoundContourNode;
			pstGroupNode->pstNextNode = pstFoundContourNode;

			vGroupContour[nContourGroupIndex].nContourNum++;

			blIsFoundGroupNode = TRUE;

			goto __lblLoop;
		}

		pstGroupNode = pstGroupNode->pstNextNode;
	}

	pstNextContourNode = pstNextContourNode->pstNextNode;
	goto __lblLoop;
}

//* 对轮廓分组，采用的算法很简单，就是逐个比价两个轮廓的距离，只要距离小于参数dblDistanceThreshold指定的像素值，则认为两个轮廓相近，属于同一组
void ImgGroupedContour::GroupContours(DOUBLE dblDistanceThreshold)
{
	INT nContourGroupIndex = 0;
	ST_CONTOUR_GROUP stContourGroup;
	PST_CONTOUR_NODE pstNextContourNode = pstNotGroupedContourLink;

__lblLoop:
	if (NULL == pstNextContourNode)
		return;

	//* 将链表剩余的的第一个节点作为下一个分组的起始节点
	stContourGroup.pstContourLink = pstNextContourNode;
	stContourGroup.nContourNum = 1;
	vGroupContour.push_back(stContourGroup);
	pstNotGroupedContourLink = pstNextContourNode->pstNextNode;
	if (pstNotGroupedContourLink)
		pstNotGroupedContourLink->pstPrevNode = NULL;

	pstNextContourNode->pstNextNode = NULL;
	pstNextContourNode->pstPrevNode = NULL;

	GlueAdjacentContour(nContourGroupIndex, dblDistanceThreshold);

	nContourGroupIndex++;

	pstNextContourNode = pstNotGroupedContourLink;

	goto __lblLoop;
}

//* 获取分组轮廓的对角点
void ImgGroupedContour::GetDiagonalPointsOfGroupContours(INT nMinContourNumThreshold)
{
	for (INT i = 0; i < vGroupContour.size(); i++)
	{
		ST_CONTOUR_GROUP stContourGroup = vGroupContour[i];

		//cout << "Contour Num: " << stContourGroup.nContourNum << endl;

		if (stContourGroup.nContourNum < nMinContourNumThreshold)
			continue;

		INT x_left_top = 10000, y_left_top = 10000, x_right_bottom = 0, y_right_bottom = 0;

		//* 找出整个轮廓左上角和右下角的坐标
		PST_CONTOUR_NODE pstGroupNode = vGroupContour[i].pstContourLink;
		while (pstGroupNode != NULL)
		{
			//cout << pstGroupNode->nIndex << " ";

			for (INT j = 0; j < (*pstGroupNode->pvecContour).size(); j++)
			{
				Point point = (*pstGroupNode->pvecContour)[j];
				if (point.x < x_left_top)
					x_left_top = point.x;
				if (point.y < y_left_top)
					y_left_top = point.y;

				if (point.x > x_right_bottom)
					x_right_bottom = point.x;
				if (point.y > y_right_bottom)
					y_right_bottom = point.y;
			}

			pstGroupNode = pstGroupNode->pstNextNode;
		}

		cout << endl;

		Point left(x_left_top, y_left_top);
		Point right(x_right_bottom, y_right_bottom);

		//cout << "rect: " << left << ", " << right << endl;

		ST_DIAGONAL_POINTS stDiagPoints;
		stDiagPoints.point_left = left;
		stDiagPoints.point_right = right;
		vDiagonalPointsOfGroupContour.push_back(stDiagPoints);
	}
}

//* 同上
void ImgGroupedContour::GetDiagonalPointsOfGroupContours(INT nMinContourNumThreshold, vector<ST_DIAGONAL_POINTS>& vOutputDiagonalPoints)
{
	GetDiagonalPointsOfGroupContours(nMinContourNumThreshold);

	vOutputDiagonalPoints = vDiagonalPointsOfGroupContour;
}

//* 用矩形标记分组轮廓
void ImgGroupedContour::RectMarkGroupContours(Mat& matMarkImg, BOOL blIsMergeOverlappingRect, Scalar scalar)
{
	vector<ST_DIAGONAL_POINTS> vRects;

	//* 是否需要合并重叠的矩形
	if (blIsMergeOverlappingRect)
	{
		cv2shell::MergeOverlappingRect(vDiagonalPointsOfGroupContour, vRects);
	}
	else
		vRects = vDiagonalPointsOfGroupContour;

	for (INT i = 0; i < vRects.size(); i++)
	{
		ST_DIAGONAL_POINTS stRect = vRects[i];
		rectangle(matMarkImg, stRect.point_left, stRect.point_right, scalar, 1);
	}
}

