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
	Mat matImg = cv2shell::ExtractFaceChips(pszImgName);

	//imshow("pic", matImg);
	//waitKey(0);

	return AddFace(matImg);
}

//FACE_PROCESSING_EXT Mat FaceProcessing(const Mat &img_, double gamma = 0.8, double sigma0 = 0, double sigma1 = 0, double mask = 0, double do_norm = 8);
Mat FaceDatabase::FaceChipsHandle(Mat& matFaceChips)
{
	Mat matFloat;
	matFaceChips.convertTo(matFloat, CV_32F);


	return matFloat;
}

