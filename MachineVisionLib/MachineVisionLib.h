// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 MACHINEVISIONLIB_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// MACHINEVISIONLIB_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
//* How to compile DLib:
//* https://blog.csdn.net/xingchenbingbuyu/article/details/53236541
//* 配置VS2015
//* https://blog.csdn.net/flyyufenfei/article/details/79176136
#ifdef MACHINEVISIONLIB_EXPORTS
#define MACHINEVISIONLIB_API __declspec(dllexport)
#else
#define MACHINEVISIONLIB_API __declspec(dllimport)
#endif

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp> 
#include <opencv2/dnn/shape_utils.hpp>
#include <opencv.hpp>
#include <opencv2/core/utils/trace.hpp> 
#include <caffe/caffe.hpp>

#define NEED_GPU	0

using namespace cv;
using namespace common_lib;
using namespace cv::ml;
using namespace cv::dnn;
using namespace std;

//* 记录检测出的每张人脸数据在缓冲区的保存大小，单位（size(int)），换言之就是逐个定位每张脸的步长
#define LIBFACEDETECT_RESULT_STEP 142

//* define the buffer size. Do not change the size!
//* 定义缓冲区大小 不要改变大小！
#define LIBFACEDETECT_BUFFER_SIZE 0x20000

typedef struct _ST_IMG_RESIZE_ {
	INT nRows;
	INT nCols;
} ST_IMG_RESIZE, *PST_IMG_RESIZE;

class MACHINEVISIONLIB_API RecogCategory {
public:
	RecogCategory()
	{
		flConfidenceVal = 0;
	};
	float flConfidenceVal;
	string strCategoryName;

	INT xLeftBottom;
	INT yLeftBottom;
	INT xRightTop;
	INT yRightTop;
};

class MACHINEVISIONLIB_API Face {
public:
	Face() : flConfidenceVal(0){}
	float flConfidenceVal;

	INT xLeftBottom;
	INT yLeftBottom;
	INT xRightTop;
	INT yRightTop;
};

//* 用于网络实时视频处理的回调函数原型声明
typedef Mat(*PCB_NETVIDEOHANDLER)(Mat &mVideoData);

//* OpenCV接口
namespace cv2shell {
	enum ENUM_IMGRESIZE_METHOD {
		EIRSZM_EQUALRATIO = 0,	//* 等比例缩小或放大图片
		EIRSZM_EQUILATERAL
	};

	MACHINEVISIONLIB_API void CV2Canny(Mat &mSrc, Mat &matOut);
	MACHINEVISIONLIB_API void CV2Canny(const CHAR *pszImgName, Mat &matOut);

	MACHINEVISIONLIB_API void CV2ShowVideoFromNetCamera(const CHAR *pszNetURL);
	MACHINEVISIONLIB_API void CV2ShowVideoFromNetCamera(const CHAR *pszNetURL, PCB_NETVIDEOHANDLER pfunNetVideoHandler);

	MACHINEVISIONLIB_API void CV2CreateAlphaMat(Mat &mat);

	MACHINEVISIONLIB_API string CV2HashValue(Mat &matSrc, PST_IMG_RESIZE pstResize);
	MACHINEVISIONLIB_API string CV2HashValue(const CHAR *pszImgName, PST_IMG_RESIZE pstResize);

	MACHINEVISIONLIB_API ST_IMG_RESIZE CV2GetResizeValue(const CHAR *pszImgName);
	MACHINEVISIONLIB_API ST_IMG_RESIZE CV2GetResizeValue(Mat &matImg);

	MACHINEVISIONLIB_API void ImgEquilateral(Mat &matImg, Mat &matResizeImg, INT nResizeLen, const Scalar &border_color = Scalar(0, 0, 0));
	MACHINEVISIONLIB_API void ImgEquilateral(Mat &matImg, Mat &matResizeImg, const Scalar &border_color = Scalar(0, 0, 0));

	MACHINEVISIONLIB_API INT *FaceDetect(const CHAR *pszImgName, FLOAT flScale = 1.05f, INT nMinNeighbors = 5, INT nMinPossibleFaceSize = 16);
	MACHINEVISIONLIB_API INT *FaceDetect(Mat &matImg, FLOAT flScale = 1.05f, INT nMinNeighbors = 5, INT nMinPossibleFaceSize = 16);
	MACHINEVISIONLIB_API void MarkFaceWithRectangle(Mat &matImg, INT *pnFaces);
	MACHINEVISIONLIB_API void MarkFaceWithRectangle(const CHAR *pszImgName, FLOAT flScale = 1.05f, INT nMinNeighbors = 5, INT nMinPossibleFaceSize = 16);

	MACHINEVISIONLIB_API Net InitFaceDetectDNNet(void);
	MACHINEVISIONLIB_API Mat FaceDetect(Net &dnnNet, Mat &matImg, const Size &size, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 177.0, 123.0));
	MACHINEVISIONLIB_API Mat FaceDetect(Net &dnnNet, Mat &matImg, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUALRATIO, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 177.0, 123.0));
	MACHINEVISIONLIB_API Mat FaceDetect(Net &dnnNet, const CHAR *pszImgName, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUALRATIO, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 177.0, 123.0));
	MACHINEVISIONLIB_API Mat FaceDetect(Net &dnnNet, const CHAR *pszImgName, const Size &size, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 177.0, 123.0));
	MACHINEVISIONLIB_API void FaceDetect(Net &dnnNet, Mat &matImg, vector<Face> &vFaces, const Size &size, FLOAT flConfidenceThreshold = 0.3, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 177.0, 123.0));
	MACHINEVISIONLIB_API void FaceDetect(Net &dnnNet, Mat &matImg, vector<Face> &vFaces, FLOAT flConfidenceThreshold = 0.3, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUALRATIO, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 177.0, 123.0));
	MACHINEVISIONLIB_API void FaceDetect(Net &dnnNet, const CHAR *pszImgName, vector<Face> &vFaces, FLOAT flConfidenceThreshold = 0.3, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUALRATIO, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 177.0, 123.0));
	MACHINEVISIONLIB_API void FaceDetect(Net &dnnNet, const CHAR *pszImgName, vector<Face> &vFaces, const Size &size, FLOAT flConfidenceThreshold = 0.3, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 177.0, 123.0));


	MACHINEVISIONLIB_API void MarkFaceWithRectangle(Mat &matImg, Mat &matFaces, FLOAT flConfidenceThreshold = 0.3);
	MACHINEVISIONLIB_API void MarkFaceWithRectangle(Mat &matImg, vector<Face> &vFaces);
	MACHINEVISIONLIB_API void MarkFaceWithRectangle(Net &dnnNet, const CHAR *pszImgName, const Size &size, FLOAT flConfidenceThreshold = 0.3, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 177.0, 123.0));
	MACHINEVISIONLIB_API void MarkFaceWithRectangle(Net &dnnNet, const CHAR *pszImgName, FLOAT flConfidenceThreshold = 0.3, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUALRATIO, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 177.0, 123.0));

	MACHINEVISIONLIB_API Net InitLightClassifier(vector<string> &vClassNames);
	MACHINEVISIONLIB_API void ObjectDetect(Mat &matImg, Net &dnnNet, vector<string> &vClassNames, vector<RecogCategory> &vObjects, const Size &size, FLOAT flConfidenceThreshold = 0.4, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 117.0, 123.0));
	MACHINEVISIONLIB_API void ObjectDetect(Mat &matImg, Net &dnnNet, vector<string> &vClassNames, vector<RecogCategory> &vObjects, FLOAT flConfidenceThreshold = 0.4, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUILATERAL, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 117.0, 123.0));
	MACHINEVISIONLIB_API void ObjectDetect(const CHAR *pszImgName, Net &dnnNet, vector<string> &vClassNames, vector<RecogCategory> &vObjects, const Size &size, FLOAT flConfidenceThreshold = 0.4, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 117.0, 123.0));
	MACHINEVISIONLIB_API void ObjectDetect(const CHAR *pszImgName, Net &dnnNet, vector<string> &vClassNames, vector<RecogCategory> &vObjects, FLOAT flConfidenceThreshold = 0.4, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUILATERAL, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 117.0, 123.0));
	MACHINEVISIONLIB_API void MarkObjectWithRectangle(Mat &matImg, vector<RecogCategory> &vObjects);
	MACHINEVISIONLIB_API void MarkObjectWithRectangle(const CHAR *pszImgName, Net &dnnNet, vector<string> &vClassNames, const Size &size, FLOAT flConfidenceThreshold = 0.4, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 117.0, 123.0));
	MACHINEVISIONLIB_API void MarkObjectWithRectangle(const CHAR *pszImgName, Net &dnnNet, vector<string> &vClassNames, FLOAT flConfidenceThreshold = 0.4, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUILATERAL, FLOAT flScale = 1.0f, const Scalar &mean = Scalar(104.0, 117.0, 123.0));
	MACHINEVISIONLIB_API INT GetObjectNum(vector<RecogCategory> &vObjects, string strObjectName, FLOAT *pflConfidenceOfExist, FLOAT *pflConfidenceOfObjectNum);
};

//* caffe接口
//* 关于神经网络前向传播和后向传播的入门文章
//* https://blog.csdn.net/zhangjunhit/article/details/53501680
namespace caffe2shell {
	template <typename DType>
	MACHINEVISIONLIB_API caffe::Net<DType>* LoadNet(std::string strParamFile, std::string strModelFile, caffe::Phase phase);
};

class MACHINEVISIONLIB_API ImgMatcher {
public:
	ImgMatcher(INT nSetMatchThresholdValue);
	BOOL InitImgMatcher(const CHAR *pszModelImg);
	BOOL InitImgMatcher(vector<string *> &vInputModelImgs);
	void UninitImgMatcher(void);
	INT ImgSimilarity(Mat &matImg);
	INT ImgSimilarity(const CHAR *pszImgName);
	INT ImgSimilarity(Mat &matImg, INT *pnDistance);
	INT ImgSimilarity(const CHAR *pszImgName, INT *pnDistance);

private:
	INT nMatchThresholdValue;
	ST_IMG_RESIZE stImgResize;
	vector<string *> vModelImgHashValues = vector<string *>();
};

// 此类是从 MachineVisionLib.dll 导出的
class MACHINEVISIONLIB_API CMachineVisionLib {
public:
	CMachineVisionLib(void);
	// TODO:  在此添加您的方法。
};

