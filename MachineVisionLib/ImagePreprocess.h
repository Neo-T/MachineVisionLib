#pragma once
#ifdef MACHINEVISIONLIB_EXPORTS
#define IMGPREPROC_API __declspec(dllexport)
#else
#define IMGPREPROC_API __declspec(dllimport)
#endif

//* 图像预处理库
namespace imgpreproc {
	IMGPREPROC_API void HistogramEnhancedDefinition(Mat& matInputGrayImg, Mat& matDestImg);	//* 增强图像的对比度和清晰度
};

