#pragma once
#ifdef MACHINEVISIONLIB_EXPORTS
#define MATHLGORITHM_API __declspec(dllexport)
#else
#define MATHLGORITHM_API __declspec(dllimport)
#endif

//* ÊýÑ§Ëã·¨¿â
namespace malib {
	MATHLGORITHM_API DOUBLE CosineSimilarity(const FLOAT *pflaBaseData, const FLOAT *pflaTargetData, UINT unDimension);
	MATHLGORITHM_API DOUBLE ShortestDistance(vector<Point> *pvecBaseContour, vector<Point> *pvecTragetContour);
	MATHLGORITHM_API BOOL IsOverlappingRect(PST_DIAGONAL_POINTS pstRect1, PST_DIAGONAL_POINTS pstRect2);
};
