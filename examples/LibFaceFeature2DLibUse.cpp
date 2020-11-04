#include "stdafx.h"
#include <opencv.hpp>
#include <facedetect-dll.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/opencv/cv_image.h>
#include <dlib/opencv.h>

using namespace cv;
using namespace std;
using namespace dlib;

//define the buffer size. Do not change the size!
#define DETECT_BUFFER_SIZE 0x20000

int main(int argc, char** argv)
{
	LARGE_INTEGER freq;
	LARGE_INTEGER start_t, fst_t, mid_t, stop_t;
	double exe_time;
	QueryPerformanceFrequency(&freq);

	INT *pnResults = NULL;
	UCHAR *pubResultBuf = (UCHAR*)malloc(DETECT_BUFFER_SIZE);
	if (!pubResultBuf)
	{
		cout << "malloc error:" << GetLastError() << endl;
		return -1;
	}
	
	Mat matSrc = imread("D:\\work\\SC\\DlibTest\\x64\\Release\\LenaHeadey-2.jpg");
	Mat matGray;
	cvtColor(matSrc, matGray, CV_BGR2GRAY);

	QueryPerformanceCounter(&start_t);
	INT nLandmark = 1;
	pnResults = facedetect_multiview_reinforce(pubResultBuf, (UCHAR*)(matGray.ptr(0)), matGray.cols, matGray.rows, (INT)matGray.step,
													1.2f, 2, 16, 0, nLandmark);
	QueryPerformanceCounter(&mid_t);
	mid_t.QuadPart -= start_t.QuadPart;

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
			cv::rectangle(matSrc, left, right, Scalar(230, 255, 0), 1);			

			QueryPerformanceCounter(&start_t);
			std::vector<point> vFaceFeature;
			for (INT j = 0; j < 68; j++)
				vFaceFeature.push_back(point((INT)psScalar[6 + 2 * j], (INT)psScalar[6 + 2 * j + 1]));

			cv::Rect opencvRect(x, y, nWidth, nHeight);
			dlib::rectangle dlibRect((LONG)opencvRect.tl().x, (LONG)opencvRect.tl().y, (LONG)opencvRect.br().x - 1, (long)opencvRect.br().y - 1);

			full_object_detection shape(dlibRect, vFaceFeature);
			std::vector<full_object_detection> shapes;
			shapes.push_back(shape);
			QueryPerformanceCounter(&stop_t);			
			exe_time = 1e3*(stop_t.QuadPart - start_t.QuadPart + mid_t.QuadPart) / freq.QuadPart;
			printf("Your program executed time is %fms.\n", exe_time);
							
			//* 官方文档给出的例子就是68个点,脸部到鼻子:0-35, 眼睛:36-47,嘴形：48-60，嘴线：
			for (INT j = 0; j < 68; j++)
			{
				circle(matSrc, Point((INT)psScalar[6 + 2 * j], (INT)psScalar[6 + 2 * j + 1]), 1, Scalar(0, 0, 255), 2);				
			}
			

			dlib::array<array2d<rgb_pixel>>  face_chips;
			extract_image_chips(dlib::cv_image<uchar>(matGray), get_face_chip_details(shapes), face_chips);
			Mat pic = toMat(face_chips[0]);

			cv::imshow("pic", pic);
			cv::waitKey(0);

			resize(pic, pic, Size(224, 224));
			cv::imshow("pic-resize", pic);
			cv::waitKey(0);
		}		
	}
		
	free(pubResultBuf);

	cv::imshow("LandMark", matSrc);
	cv::waitKey(0);

	return 1;
}