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

#ifndef USE_OPENCV	//* 解决AddMatVector()不存在的问题
#define USE_OPENCV
#include <caffe/caffe.hpp>
#include <caffe/layers/input_layer.hpp>
#include <caffe/layers/inner_product_layer.hpp>
#include <caffe/layers/dropout_layer.hpp>
#include <caffe/layers/conv_layer.hpp>
#include <caffe/layers/relu_layer.hpp>
#include <caffe/layers/memory_data_layer.hpp>
#include <caffe/layers/pooling_layer.hpp>
#include <caffe/layers/lrn_layer.hpp>
#include <caffe/layers/softmax_layer.hpp>
#endif

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
typedef void (*PCB_VIDEOHANDLER)(Mat &mVideoData, UINT unInputParam);

//* OpenCV接口
namespace cv2shell {
	enum ENUM_IMGRESIZE_METHOD {
		EIRSZM_EQUALRATIO = 0,	//* 等比例缩小或放大图片
		EIRSZM_EQUILATERAL
	};

	MACHINEVISIONLIB_API void CV2Canny(Mat &mSrc, Mat &matOut);
	MACHINEVISIONLIB_API void CV2Canny(const CHAR *pszImgName, Mat &matOut);

	template <typename DType> void CV2ShowVideo(DType dtVideoSrc);
	template MACHINEVISIONLIB_API void CV2ShowVideo(const CHAR *pszNetURL);
	template MACHINEVISIONLIB_API void CV2ShowVideo(INT nCameraIdx);

	template <typename DType> void CV2ShowVideo(DType dtVideoSrc, PCB_VIDEOHANDLER pfunNetVideoHandler, UINT unInputParam);
	template MACHINEVISIONLIB_API void CV2ShowVideo(const CHAR *pszNetURL, PCB_VIDEOHANDLER pfunNetVideoHandler, UINT unInputParam);
	template MACHINEVISIONLIB_API void CV2ShowVideo(INT nCameraIdx, PCB_VIDEOHANDLER pfunNetVideoHandler, UINT unInputParam);

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
	MACHINEVISIONLIB_API void MarkFaceWithRectangle(Mat &matImg, FLOAT flScale = 1.05f, INT nMinNeighbors = 5, INT nMinPossibleFaceSize = 16);
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
	
	MACHINEVISIONLIB_API DOUBLE CosineSimilarity(const FLOAT *pflaBaseData, const FLOAT *pflaTargetData, UINT unDimension);
};

//* caffe接口
//* 关于神经网络前向传播和后向传播的入门文章
//* https://blog.csdn.net/zhangjunhit/article/details/53501680
namespace caffe2shell {
	//* 模板函数一定要被人为实例化，否则编译器无法导出，函数无法被真正使用，参见：
	//* https://blog.csdn.net/liyuanbhu/article/details/50363670
	template <typename DType> caffe::Net<DType> *LoadNet(std::string strParamFile, std::string strModelFile, caffe::Phase phase);	
	template MACHINEVISIONLIB_API caffe::Net<FLOAT> *caffe2shell::LoadNet(std::string strParamFile, std::string strModelFile, caffe::Phase phase);
	template MACHINEVISIONLIB_API caffe::Net<DOUBLE> *caffe2shell::LoadNet(std::string strParamFile, std::string strModelFile, caffe::Phase phase);

	template <typename DType> void ExtractFeature(caffe::Net<DType> *pNet, caffe::MemoryDataLayer<DType> *pMemDataLayer, Mat& matImgROI, DType *pdtaImgFeature, INT nFeatureDimension, const CHAR *pszLayerName);
	template MACHINEVISIONLIB_API void ExtractFeature(caffe::Net<FLOAT> *pflNet, caffe::MemoryDataLayer<FLOAT> *pMemDataLayer, Mat& matImgROI, FLOAT *pflaImgFeature, INT nFeatureDimension, const CHAR *pszLayerName);
	template MACHINEVISIONLIB_API void ExtractFeature(caffe::Net<DOUBLE> *pdblNet, caffe::MemoryDataLayer<DOUBLE> *pMemDataLayer, Mat& matImgROI, DOUBLE *pdblaImgFeature, INT nFeatureDimension, const CHAR *pszBlobName);

	template <typename DType> void ExtractFeature(caffe::Net<DType> *pNet, caffe::MemoryDataLayer<DType> *pMemDataLayer, Mat& matImgROI, Mat& matImgFeature, INT nFeatureDimension, const CHAR *pszLayerName);
	template MACHINEVISIONLIB_API void ExtractFeature(caffe::Net<FLOAT> *pflNet, caffe::MemoryDataLayer<FLOAT> *pMemDataLayer, Mat& matImgROI, Mat& matImgFeature, INT nFeatureDimension, const CHAR *pszLayerName);
	template MACHINEVISIONLIB_API void ExtractFeature(caffe::Net<DOUBLE> *pdblNet, caffe::MemoryDataLayer<DOUBLE> *pMemDataLayer, Mat& matImgROI, Mat& matImgFeature, INT nFeatureDimension, const CHAR *pszBlobName);
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

