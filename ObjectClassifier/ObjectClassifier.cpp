// ObjectClassifier.cpp : 定义控制台应用程序的入口点。
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
#include "MVLVideo.h"

using namespace common_lib;
using namespace cv2shell;

//* Usage，使用说明
static const string straUsage[] = {"ObjectClassifier [model type]",""
};

//* 通用分类器，对图像和视频进行物体分类，分类模型采用预训练模型和针对特定类别的自训练模型
int _tmain(int argc, _TCHAR* argv[])
{
	vector<string> vClassNames;
	Net objDNNNet = InitLightClassifier(vClassNames);
	if (objDNNNet.empty())
	{
		cout << "The initialization lightweight classifier failed, probably because the model file or voc.names file was not found." << endl;
		return -1;
	}

	cout << "The lightweight classifier load successfully." << endl;

    return 0;
}

