#include "stdafx.h"
#include <iostream>  
#include <fstream> 
#include <list>  
#include <vector>  
#include <map>  
#include <stack>  
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/objdetect.hpp>

using namespace std;
using namespace cv;

//* 特征点匹配最直观的说明
//* https://blog.csdn.net/civiliziation/article/details/38370167
//* cv2和cv3函数改变的：
//* https://www.cnblogs.com/anqiang1995/p/7398218.html


//* 重要，能检测运动目标
//* https://www.jpjodoin.com/urbantracker/index.htm

#define WINDOW_NAME "『程序窗口1』"
#define WINDOW_NAME_RESULT "『分水岭结果窗口』"

Mat g_maskImg, g_srcImg;
Point prevPt(-1, -1);

static void on_Mouse(int event, int x, int y, int flags, void *)
{
	if (x < 0 || x >= g_srcImg.cols || y < 0 || y >= g_srcImg.rows)
		return;

	if (event == EVENT_LBUTTONUP || !(flags & EVENT_FLAG_LBUTTON))
		prevPt = Point(-1, -1);
	else if(event == EVENT_LBUTTONDOWN)
		prevPt = Point(x, y);
	else if (event == EVENT_MOUSEMOVE && (flags & EVENT_FLAG_LBUTTON))
	{
		Point pt(x, y);
		if (prevPt.x < 0)
			prevPt = pt;

		line(g_maskImg, prevPt, pt, Scalar::all(255), 5, 8, 0);
		line(g_srcImg, prevPt, pt, Scalar::all(255), 5, 8, 0);
		prevPt = pt;
		imshow(WINDOW_NAME, g_srcImg);
	}
}

int main(int argc, char** argv)
{
	g_srcImg = imread(argv[1]);
	imshow(WINDOW_NAME, g_srcImg);

	Mat srcImg, grayImg;
	//Mat MASK(g_srcImg.rows, g_srcImg.cols, CV_8UC3, Scalar(0, 0, 255));
	//g_srcImg.copyTo(srcImg, MASK);
	g_srcImg.copyTo(srcImg);
	cvtColor(g_srcImg, g_maskImg, COLOR_BGR2GRAY);
	cvtColor(g_maskImg, grayImg, COLOR_GRAY2BGR);
	g_maskImg = Scalar::all(0);

	setMouseCallback(WINDOW_NAME, on_Mouse, 0);

	while (1)
	{
		int c = waitKey(0);

		if ((char)c == 27)
			break;

		if((char)c == '2')
		{ 
			g_maskImg = Scalar::all(0);

			srcImg.copyTo(g_srcImg);
			imshow(WINDOW_NAME, g_srcImg);
			destroyWindow(WINDOW_NAME_RESULT);
		}

		if ((char)c == '1' || (char)c == ' ')
		{
			int i, j, compCount = 0;
			vector<vector<Point>> contours;
			vector<Vec4i> hierarchy;

			findContours(g_maskImg, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
			if (contours.empty())
				continue;

			Mat maskImg(g_maskImg.size(), CV_32S);
			maskImg = Scalar::all(0);

			for (int index = 0; index >= 0; index = hierarchy[index][0], compCount++)
				drawContours(maskImg, contours, index, Scalar::all(compCount + 1), -1, 8, hierarchy, INT_MAX);

			if (compCount == 0)
				continue;

			vector<Vec3b> colorTab;
			for (i = 0; i < compCount; i++)
			{
				int b = theRNG().uniform(0, 255);
				int g = theRNG().uniform(0, 255);
				int r = theRNG().uniform(0, 255);

				colorTab.push_back(Vec3b((UCHAR)b, (UCHAR)g, (UCHAR)r));
			}

			watershed(srcImg, maskImg);

			Mat watershedImg(maskImg.size(), CV_8UC3);
			for (i = 0; i < maskImg.rows; i++)
			{
				for (j = 0; j < maskImg.cols; j++)
				{
					int index = maskImg.at<int>(i, j);
					if (index == -1)					
						watershedImg.at<Vec3b>(i, j) = Vec3b(255, 255, 255);
					else if(index <= 0 || index > compCount)
						watershedImg.at<Vec3b>(i, j) = Vec3b(0, 0, 0);
					else
						watershedImg.at<Vec3b>(i, j) = colorTab[index - 1];
				}
			}

			watershedImg = watershedImg * 0.5 + grayImg * 0.5;
			imshow(WINDOW_NAME_RESULT, watershedImg);
		}
	}

	
	destroyAllWindows();

	return 0;
}