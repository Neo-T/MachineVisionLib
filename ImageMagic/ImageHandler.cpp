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
void ImagePerspectiveTransformation::process(Mat& mSrcImg, Mat& mResultImg, Mat& mShowImg, DOUBLE dblScaleFactor)
{
	CHAR  *pszSrcImgWinName = "源图片";
	CHAR  *pszDestImgWinName = "目标图片";	

	namedWindow(pszSrcImgWinName, WINDOW_AUTOSIZE);
	namedWindow(pszDestImgWinName, WINDOW_AUTOSIZE);

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


	}	

	//* 销毁两个操作窗口
	destroyWindow(pszSrcImgWinName);
	destroyWindow(pszDestImgWinName);
}

//* 鼠标处理事件
static void EHImgPerspecTrans_OnMouse(INT nEvent, INT x, INT y, INT nFlags, void *pvIPT)
{
	vector<Point2f>& vptROI = ((ImagePerspectiveTransformation *)pvIPT)->GetROI();
	
	//* 鼠标是否按下了
	if (nEvent == EVENT_LBUTTONDOWN)
	{ 
		//* 4个点已经确定了
		if (vptROI.size() == 4)
		{
			//* 看看用户点击的是不是已经存在的角点，如果是，则支持用户拖拽以调整透视变换区域及角度
			for (INT i = 0; i < 4; i++)
			{
				
			}
		}
	}
}