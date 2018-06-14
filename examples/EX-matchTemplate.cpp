#include "stdafx.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>  
#include <fstream> 

using namespace std;
using namespace cv;


//* 重要，能检测运动目标
//* https://www.jpjodoin.com/urbantracker/index.htm
int main(int argc, char** argv)
{
	Mat matGrayImg, matResult;

	Mat matSrcImg = imread(argv[1]);
	pyrDown(matSrcImg, matSrcImg, Size(matSrcImg.cols / 2, matSrcImg.rows / 2));
	cvtColor(matSrcImg, matGrayImg, COLOR_BGR2GRAY);
	equalizeHist(matGrayImg, matGrayImg);
	imshow("直方图均衡图片", matGrayImg);

	Mat matTemplateImg = imread(argv[2]);
	cvtColor(matTemplateImg, matTemplateImg, COLOR_BGR2GRAY);

	matResult.create(matGrayImg.rows - matTemplateImg.rows + 1, matGrayImg.cols - matTemplateImg.cols + 1, CV_32FC1);
	matchTemplate(matGrayImg, matTemplateImg, matResult, TM_CCOEFF_NORMED);
	for (int i = 0; i < matResult.rows; i++)
	{
		FLOAT *pflData = matResult.ptr<FLOAT>(i);
		for (int j = 0; j < matResult.cols; j++)
		{
			if (pflData[j] > 0.68)
			{
				rectangle(matSrcImg, Point(j, i), Point(j + matTemplateImg.cols, i + matTemplateImg.rows), Scalar::all(0), 1, 8, 0);
			}
		}
	}

	imshow("结果图片", matResult);
	imshow("标记图片", matSrcImg);
	
	waitKey(0);

	destroyAllWindows();

	return 0;
}