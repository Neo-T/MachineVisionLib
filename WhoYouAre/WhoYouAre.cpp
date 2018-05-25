// WhoYouAre.cpp : 定义控制台应用程序的入口点。
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
#include "FaceRecognition.h"

#define NEED_DEBUG_CONSOLE	1	//* 是否需要控制台窗口输出

#if !NEED_DEBUG_CONSOLE
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
#endif

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 3 && argc != 4)
	{
		cout << "Usage: " << endl << argv[0] << " Add [Img Path] [Person Name]" << endl;
		cout << argv[0] << "Predict [Img Path]";

		return -1;
	}

	FaceDatabase face_db;
	if (!face_db.LoadCaffeVGGNet("C:\\windows\\system32\\models\\vgg_face_caffe\\VGG_FACE_extract_deploy.prototxt",
		"C:\\windows\\system32\\models\\vgg_face_caffe\\VGG_FACE.caffemodel"))
	{
		cout << "Load Failed, the process will be exited!" << endl;
		return - 1;
	}

	string strOptType(argv[1]);
	if (string("Add") == strOptType)
	{
		if (face_db.AddFace(argv[2], argv[3]))
			cout << argv[3] << " was successfully added to the face database." << endl;
	}	

    return 0;
}

