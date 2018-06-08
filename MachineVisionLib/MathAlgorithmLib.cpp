#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <vector>
#include <dlib/image_processing.h>
#include <dlib/opencv.h>
#include <facedetect-dll.h>
#include "common_lib.h"
#include "MachineVisionLib.h"
#include "MathAlgorithmLib.h"

//* 利用余弦计算相似度，关于此，理论描述及公式参见：
//* https://blog.csdn.net/u012160689/article/details/15341303
//* C++算法实现参见：
//* https://blog.csdn.net/akadiao/article/details/79767113
MATHLGORITHM_API DOUBLE malib::CosineSimilarity(const FLOAT *pflaBaseData, const FLOAT *pflaTargetData, UINT unDimension)
{
	DOUBLE dblDotProduct = 0.0f;
	DOUBLE dblQuadraticSumOfBase = 0.0f;
	DOUBLE dblQuadraticSumOfTarget = 0.0f;

	for (UINT i = 0; i < unDimension; i++)
	{
		//* 点积
		dblDotProduct += pflaBaseData[i] * pflaTargetData[i];

		dblQuadraticSumOfBase += (pflaBaseData[i] * pflaBaseData[i]);
		dblQuadraticSumOfTarget += (pflaTargetData[i] * pflaTargetData[i]);
	}

	return dblDotProduct / (sqrt(dblQuadraticSumOfBase) * sqrt(dblQuadraticSumOfTarget));
}