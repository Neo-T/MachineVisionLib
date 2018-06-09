#pragma once
#ifdef MACHINEVISIONLIB_EXPORTS
#define IMGPREPROC __declspec(dllexport)
#else
#define IMGPREPROC __declspec(dllimport)
#endif

//* 轮廓节点
typedef struct _ST_CONTOUR_NODE_ {
	_ST_CONTOUR_NODE_ *pstPrevNode;
	_ST_CONTOUR_NODE_ *pstNextNode;

	INT nIndex;
	vector<Point> *pvecContour;
} ST_CONTOUR_NODE, *PST_CONTOUR_NODE;


typedef struct _ST_CONTOUR_GROUP_ {
	PST_CONTOUR_NODE pstContourLink;
	INT nContourNum;
} ST_CONTOUR_GROUP, *PST_CONTOUR_GROUP;

//* 图像轮廓类
class IMGPREPROC ImgContour {
public:
	ImgContour(Mat& matInputSrcImg) {
		
	}

	~ImgContour() {}

	//* 轮廓节点
	typedef struct _ST_CONTOUR_NODE_ {
		_ST_CONTOUR_NODE_ *pstPrevNode;
		_ST_CONTOUR_NODE_ *pstNextNode;

		INT nIndex;
		vector<Point> *pvecContour;
	} ST_CONTOUR_NODE, *PST_CONTOUR_NODE;


	typedef struct _ST_CONTOUR_GROUP_ {
		PST_CONTOUR_NODE pstContourLink;
		INT nContourNum;
	} ST_CONTOUR_GROUP, *PST_CONTOUR_GROUP;

private:
	Mat matPreProcedImg;
};
 

//* 图像预处理库
namespace imgpreproc {
	IMGPREPROC void HistogramEqualization(Mat& matInputGrayImg, Mat& matDestImg);	//* 利用直方图均衡算法增强图像的对比度和清晰度
	IMGPREPROC void ContrastEqualization(Mat& matInputGrayImg, Mat& matDestImg, DOUBLE dblGamma = 0.4,
												DOUBLE dblPowerValue = 0.1, DOUBLE dblNorm = 10);									//* 对比度均衡算法增强图像
	IMGPREPROC void ContrastEqualizationWithFilter(Mat& matInputGrayImg, Mat& matDestImg, Size size, FLOAT *pflaFilterKernel, 
														DOUBLE dblGamma = 0.4, DOUBLE dblPowerValue = 0.1, DOUBLE dblNorm = 10);	//* 带滤波的对比度均衡算法增强图像

	//IMGPREPROC_API void FindContourUsingCanny(Mat& matSrcImg, );
};

