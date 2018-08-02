#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <vector>
#include "common_lib.h"
#include "MachineVisionLib.h"
#include "MathAlgorithmLib.h"
#include "ImagePreprocess.h"
#include "ImageHandler.h"

#if NEED_GPU
#pragma comment(lib,"cublas.lib")
#pragma comment(lib,"cuda.lib")
#pragma comment(lib,"cudart.lib")
#pragma comment(lib,"curand.lib")
#pragma comment(lib,"cudnn.lib")
#endif

static void EHImgPerspecTrans_OnMouse(INT nEvent, INT x, INT y, INT nFlags, void *pvIPT);

//* 图像透视变换
void ImagePerspectiveTransformation::process(Mat& mSrcImg, Mat& mResultImg, Mat& mSrcShowImg, DOUBLE dblScaleFactor)
{
	CHAR *pszSrcImgWinName = "源图片";
	CHAR *pszDestImgWinName = "目标图片";

	o_dblScaleFactor = dblScaleFactor;

	namedWindow(pszDestImgWinName, WINDOW_AUTOSIZE);
	//moveWindow(pszDestImgWinName, 30, 30);		//* 将目标窗口移动到左上角
	resizeWindow(pszDestImgWinName, 300, 300);	//* 目标窗口调整为一个固定值，省得程序一启动就很大
	namedWindow(pszSrcImgWinName, WINDOW_AUTOSIZE);	

	imshow(pszSrcImgWinName, mSrcShowImg);
	//imshow(pszDestImgWinName, mShowImg);

	//* 处理鼠标事件的回调函数
	setMouseCallback(pszSrcImgWinName, EHImgPerspecTrans_OnMouse, this);

	BOOL blIsNotEndOpt = TRUE;
	BOOL blIsPut = FALSE;
	while (blIsNotEndOpt && cvGetWindowHandle(pszSrcImgWinName) && cvGetWindowHandle(pszDestImgWinName))
	{
		//* 等待按键
		CHAR bInputKey = waitKey(10);
		if (bInputKey == 'q' || bInputKey == 'Q' || bInputKey == 27)
			blIsNotEndOpt = FALSE;	

		//* 是否需要绘制角点区域
		if (o_blIsNeedPaint)
		{
			o_blIsNeedPaint = FALSE;

			//* 重新获取原始数据
			Mat mShowImg = mSrcShowImg.clone();

			//* 绘制角点
			for (INT i = 0; i < o_vptROI.size(); i++)
			{
				Point2f point = o_vptROI[i] * o_dblScaleFactor;	

				circle(mShowImg, point, 4, Scalar(0, 255, 0), 2);
				if (i)
				{
					line(mShowImg, o_vptROI[i - 1] * o_dblScaleFactor, point, Scalar(0, 0, 255), 2);
					circle(mShowImg, point, 5, Scalar(0, 255, 0), 3);
				}
			}

			//* 4个点选择完毕了
			if (o_vptROI.size() == 4)
			{
				//* 将头尾两个角点连起来
				Point2f point = o_vptROI[0] * o_dblScaleFactor;
				line(mShowImg, point, o_vptROI[3] * o_dblScaleFactor, Scalar(0, 0, 255), 2);
				circle(mShowImg, point, 5, Scalar(0, 255, 0), 3);

				//* 进行透视转换
				//* ===================================================================================================================
				vector<Point2f> vptDstCorners(4);

				//* 目标角点左上点，以下4个点的位置确定是按照顺时针方向定义的，换言之，用户必须先按照左上、右上、右下、左下的顺序
				//* 选择变换区域才能变换出用户希望的图片样子
				vptDstCorners[0].x = 0;
				vptDstCorners[0].y = 0;

				//* norm()函数的公式为:sqrt(x^2 + y^2)，计算得到两个二维点之间的欧几里德距离
				//* 比较0、1和2、3点之间那个欧式距离最大，大的那个就是目标角点中的右上点
				vptDstCorners[1].x = (FLOAT)max(norm(o_vptROI[0] - o_vptROI[1]), norm(o_vptROI[2] - o_vptROI[3]));
				vptDstCorners[1].y = 0;

				//* 目标角点的右下点
				vptDstCorners[2].x = (float)max(norm(o_vptROI[0] - o_vptROI[1]), norm(o_vptROI[2] - o_vptROI[3]));
				vptDstCorners[2].y = (float)max(norm(o_vptROI[1] - o_vptROI[2]), norm(o_vptROI[3] - o_vptROI[0]));

				//* 目标角点的左下点
				vptDstCorners[3].x = 0;
				vptDstCorners[3].y = (float)max(norm(o_vptROI[1] - o_vptROI[2]), norm(o_vptROI[3] - o_vptROI[0]));

				//* 计算转换矩阵，并转换
				Mat H = findHomography(o_vptROI, vptDstCorners);
				warpPerspective(mSrcImg, mResultImg, H, Size(cvRound(vptDstCorners[2].x), cvRound(vptDstCorners[2].y)));
				imshow(pszDestImgWinName, mResultImg);
				//* ===================================================================================================================
			}

			imshow(pszSrcImgWinName, mShowImg);
		}		
	}	

	//* 销毁两个操作窗口
	destroyWindow(pszSrcImgWinName);
	destroyWindow(pszDestImgWinName);
}

//* 鼠标处理事件
static void EHImgPerspecTrans_OnMouse(INT nEvent, INT x, INT y, INT nFlags, void *pvIPT)
{
	ImagePerspectiveTransformation *pobjIPT = (ImagePerspectiveTransformation *)pvIPT;
	vector<Point2f>& vptROI = pobjIPT->GetROI();

	DOUBLE dblScaleFactor = pobjIPT->GetScaleFactor();

	//* 转变成原始图片坐标
	FLOAT flSrcImgX = ((FLOAT)x) / dblScaleFactor;
	FLOAT flSrcImgY = ((FLOAT)y) / dblScaleFactor;

	//* 鼠标已按下
	if (nEvent == EVENT_LBUTTONDOWN)
	{
		//* 看看用户点击的是不是已经存在的角点，如果是，则支持用户拖拽以调整透视变换区域及角度
		for (INT i = 0; i < vptROI.size(); i++)
		{
			if (abs(vptROI[i].x - flSrcImgX) < MOUSE_SHOT_OFFSET && abs(vptROI[i].y - flSrcImgY) < MOUSE_SHOT_OFFSET)
			{
				pobjIPT->SetDragingCornerPointIndex(i);
				break;
			}
		}
	}

	//* 在这个位置处理鼠标松开事件，三个if语句块的顺序不能变，这样才可避免程序继续相应用户针对某个角点的拖拽事件
	if (nEvent == EVENT_LBUTTONUP)
	{ 
		//* 还没选择完4个点，则继续添加之
		if (vptROI.size() < 4)
		{
			vptROI.push_back(Point2f(flSrcImgX, flSrcImgY));
			pobjIPT->NeedToDrawCornerPoint();
		}

		//* 只要松开鼠标，则不再支持拖拽
		pobjIPT->SetDragingCornerPointIndex(INVALID_INDEX);
	}

	//* 鼠标移动事件
	if (nEvent == EVENT_MOUSEMOVE)
	{
		//* 看看用户是否在某个角点上点击的
		INT nCornerPointIdx = pobjIPT->GetDragingCornerPointIndex();
		if (nCornerPointIdx != INVALID_INDEX)
		{
			vptROI[nCornerPointIdx].x = flSrcImgX;
			vptROI[nCornerPointIdx].y = flSrcImgY;
			pobjIPT->NeedToDrawCornerPoint();
		}		
	}
}