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

//* 计算两个点集之间的最短距离
MATHLGORITHM_API DOUBLE malib::ShortestDistance(vector<Point> *pvecBaseContour, vector<Point> *pvecTragetContour)
{
	DOUBLE dblMinDistance = 100000, dblDistance;
	for (INT i = 0; i < (*pvecBaseContour).size(); i++)
	{
		Point point_base = (*pvecBaseContour)[i];

		for (INT j = 0; j < (*pvecTragetContour).size(); j++)
		{
			Point point_target = (*pvecTragetContour)[j];

			INT x_distance = abs(point_target.x - point_base.x);
			INT y_distance = abs(point_target.y - point_base.y);
			if (x_distance > 50 && y_distance > 50)
				continue;

			dblDistance = sqrt((x_distance * x_distance) + (y_distance * y_distance));
			if (dblDistance < dblMinDistance)
				dblMinDistance = dblDistance;
		}
	}

	return dblMinDistance;
}

MATHLGORITHM_API BOOL malib::IsOverlappingRect(PST_DIAGONAL_POINTS pstRect1, PST_DIAGONAL_POINTS pstRect2)
{
	INT nMinX, nMaxX, nMinY, nMaxY;
	nMinX = max(pstRect1->point_left.x, pstRect2->point_left.x);
	nMaxX = min(pstRect1->point_right.x, pstRect2->point_right.x);
	nMinY = max(pstRect1->point_left.y, pstRect2->point_left.y);
	nMaxY = min(pstRect1->point_right.y, pstRect2->point_right.y);
	if (nMinX > nMaxX || nMinY > nMaxY)	//* 不相交
		return FALSE;

	return TRUE;
}