#pragma once
#ifdef MACHINEVISIONLIB_EXPORTS
#define IMGPREPROC __declspec(dllexport)
#else
#define IMGPREPROC __declspec(dllimport)
#endif

//* 使用近邻算法对图像轮廓分组
class IMGPREPROC ImgGroupedContour {
public:
	ImgGroupedContour(Mat& matGrayImg, DOUBLE dblThreshold1, DOUBLE dblThreshold2, INT nApertureSize = 3, BOOL blIsMarkContour = FALSE) {
		if (matGrayImg.empty())
		{
			string strError = string("ImgContour类构造函数错误：传入的参数matSrcImg为空！");
			throw runtime_error(strError);
		}

		Preprocess(matGrayImg, dblThreshold1, dblThreshold2, nApertureSize, blIsMarkContour);
	}

	~ImgGroupedContour() {
		if (pstMallocMemForLink)
		{
			free(pstMallocMemForLink);
			cout << "The memory allocated for the contour grouping has been released." << endl;
		}
	}

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

	void GetContours(vector<vector<Point>>& vOutputContours, vector<Vec4i>& vOutputHierarchy) const {
		vOutputContours = vContours;
		vOutputHierarchy = vHierarchy;
	}

	void GetContours(vector<vector<Point>>& vOutputContours) const {
		vOutputContours = vContours;
	}

	void GroupContours(DOUBLE dblDistanceThreshold = 10);
	void GetDiagonalPointsOfGroupContours(INT nMinContourNumThreshold);
	void GetDiagonalPointsOfGroupContours(INT nMinContourNumThreshold, vector<ST_DIAGONAL_POINTS>& vOutputDiagonalPoints);
	void RectMarkGroupContours(Mat& matMarkImg, BOOL blIsMergeOverlappingRect = FALSE, Scalar scalar = Scalar(230, 255, 0));

private:
	ImgGroupedContour() {}
	void Preprocess(Mat& matGrayImg, DOUBLE dblThreshold1, DOUBLE dblThreshold2, INT nApertureSize, BOOL blIsMarkContour);
	void GlueAdjacentContour(INT nContourGroupIndex, DOUBLE dblDistanceThreshold);

	vector<vector<Point>> vContours;
	vector<Vec4i> vHierarchy;

	PST_CONTOUR_NODE pstNotGroupedContourLink, pstMallocMemForLink;
	vector<ST_CONTOUR_GROUP> vGroupContour;
	vector<ST_DIAGONAL_POINTS> vDiagonalPointsOfGroupContour;
};
 

//* 图像预处理库
namespace imgpreproc {
	IMGPREPROC void HistogramEqualization(Mat& matInputGrayImg, Mat& matDestImg);	//* 利用直方图均衡算法增强图像的对比度和清晰度
	IMGPREPROC void ContrastEqualization(Mat& matInputGrayImg, Mat& matDestImg, DOUBLE dblGamma = 0.4,
												DOUBLE dblPowerValue = 0.1, DOUBLE dblNorm = 10);									//* 对比度均衡算法增强图像
	IMGPREPROC void ContrastEqualizationWithFilter(Mat& matInputGrayImg, Mat& matDestImg, Size size, FLOAT *pflaFilterKernel, 
														DOUBLE dblGamma = 0.4, DOUBLE dblPowerValue = 0.1, DOUBLE dblNorm = 10);	//* 带滤波的对比度均衡算法增强图像
};

