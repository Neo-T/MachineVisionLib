#include "stdafx.h"
#include <opencv.hpp>
#include <facedetect-dll.h>

using namespace cv;
using namespace std;

//define the buffer size. Do not change the size!
#define DETECT_BUFFER_SIZE 0x20000

int main(int argc, char** argv)
{
	INT *pnResults = NULL;
	UCHAR *pubResultBuf = (UCHAR*)malloc(DETECT_BUFFER_SIZE);
	if (!pubResultBuf)
	{
		cout << "malloc error:" << GetLastError() << endl;
		return -1;
	}
	
	Mat matSrc = imread("D:\\work\\SC\\DlibTest\\x64\\Release\\LenaHeadey-6.jpg");
	Mat matGray;
	cvtColor(matSrc, matGray, CV_BGR2GRAY);

	INT nLandmark = 1;
	pnResults = facedetect_multiview_reinforce(pubResultBuf, (UCHAR*)(matGray.ptr(0)), matGray.cols, matGray.rows, (INT)matGray.step,
												1.2f, 2, 16, 0, nLandmark);
	if (pnResults && *pnResults > 0)
	{
		for (INT i = 0; i < *pnResults; i++)
		{
			SHORT *psScalar = ((SHORT*)(pnResults + 1)) + 142 * i;
			INT x = psScalar[0];
			INT y = psScalar[1];
			INT nWidth = psScalar[2];
			INT nHeight = psScalar[3];
			INT nNeighbors = psScalar[4];

			Point left(x, y);
			Point right(x + nWidth, y + nHeight);
			rectangle(matSrc, left, right, Scalar(230, 255, 0), 1);

			if (nLandmark)
			{
				//* 官方文档给出的例子就是68个点
				for (INT j = 0; j < 68; j++)
				{
					circle(matSrc, Point((INT)psScalar[6 + 2 * j], (INT)psScalar[6 + 2 * j + 1]), 1, Scalar(0, 0, 255), 2);
				}
			}
		}		
	}
		
	free(pubResultBuf);

	imshow("LandMark", matSrc);
	waitKey(0);

	return 1;
}