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

BOOL FaceDatabase::LoadVGGNet(string strCaffeModelFile, string strPrototxt)
{
	ca_fl_net = new caffe::Net<float>(strPrototxt, caffe::TEST);
	ca_fl_net->CopyTrainedLayersFrom(strCaffeModelFile);
}

