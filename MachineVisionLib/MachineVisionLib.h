// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 MACHINEVISIONLIB_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// MACHINEVISIONLIB 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
//* How to compile DLib:
//* https://blog.csdn.net/xingchenbingbuyu/article/details/53236541
//* 配置VS2015
//* https://blog.csdn.net/flyyufenfei/article/details/79176136
//* 
//* OCR相关:
//* https://blog.csdn.net/qq_37674858/article/details/80576978
//* tesseract D:\work\SC\DlibTest\x64\Release\ValidCodeImg.jpg out -psm 13
//* tesseract将其处理为原始数据
//* 训练要识别的数据集才能提高识别效率：
//* tesseract.exe --psm 7 captcha.tif captcha batch.nochop makebox
//* 增加psm 7会去掉错误，否则无法训练
//* https://www.jianshu.com/p/5c8c6b170f6f
//* http://lib.csdn.net/article/deeplearning/50608
#ifdef MACHINEVISIONLIB_EXPORTS
#define MACHINEVISIONLIB __declspec(dllexport)
#else
#define MACHINEVISIONLIB __declspec(dllimport)
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

#if NEED_GPU
#pragma comment(lib,"cublas.lib")
#pragma comment(lib,"cuda.lib")
#pragma comment(lib,"cudart.lib")
#pragma comment(lib,"curand.lib")
#pragma comment(lib,"cudnn.lib")
#endif

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

//* 对角点
typedef struct _ST_DIAGONAL_POINTS_ {
	Point point_left;
	Point point_right;
} ST_DIAGONAL_POINTS, *PST_DIAGONAL_POINTS;

class MACHINEVISIONLIB RecogCategory {
public:
	RecogCategory()
	{
		flConfidenceVal = 0;
	};
	float flConfidenceVal;
	string strCategoryName;

	INT nLeftTopX;
	INT nLeftTopY;
	INT nRightBottomX;
	INT nRightBottomY;
};

class MACHINEVISIONLIB Face {
public:
	Face() : o_flConfidenceVal(0){}
	Face(INT nLeftTopX, INT nLeftTopY, INT nRightBottomX, INT nRightBottomY) : o_flConfidenceVal(0) {
		o_nLeftTopX = nLeftTopX;
		o_nLeftTopY = nLeftTopY;
		o_nRightBottomX = nRightBottomX;
		o_nRightBottomY = nRightBottomY;
	}
	FLOAT o_flConfidenceVal;

	INT o_nLeftTopX;
	INT o_nLeftTopY;
	INT o_nRightBottomX;
	INT o_nRightBottomY;
};

//* 用于网络实时视频处理的回调函数原型声明
typedef void (*PCB_VIDEOHANDLER)(Mat &mVideoData, DWORD64 dw64InputParam);

//* OpenCV接口
namespace cv2shell {
	enum ENUM_IMGRESIZE_METHOD {
		EIRSZM_EQUALRATIO = 0,	//* 等比例缩小或放大图片
		EIRSZM_EQUILATERAL
	};

	MACHINEVISIONLIB void CV2Canny(Mat& mSrc, Mat& mOut);
	MACHINEVISIONLIB void CV2Canny(const CHAR *pszImgName, Mat& mOut);

	template <typename DType> void CV2ShowVideo(DType dtVideoSrc, BOOL blIsNeedToReplay);
	template MACHINEVISIONLIB void CV2ShowVideo(const CHAR *pszNetURL, BOOL blIsContinueToPlay);
	template MACHINEVISIONLIB void CV2ShowVideo(INT nCameraIdx, BOOL blIsContinueToPlay);

	template <typename DType> void CV2ShowVideo(DType dtVideoSrc, PCB_VIDEOHANDLER pfunNetVideoHandler, DWORD64 dw64InputParam, BOOL blIsNeedToReplay);
	template MACHINEVISIONLIB void CV2ShowVideo(const CHAR *pszNetURL, PCB_VIDEOHANDLER pfunNetVideoHandler, DWORD64 dw64InputParam, BOOL blIsNeedToReplay);
	template MACHINEVISIONLIB void CV2ShowVideo(INT nCameraIdx, PCB_VIDEOHANDLER pfunNetVideoHandler, DWORD64 dw64InputParam, BOOL blIsNeedToReplay);

	MACHINEVISIONLIB void CV2CreateAlphaMat(Mat &mat);

	MACHINEVISIONLIB string CV2HashValue(Mat& mSrc, PST_IMG_RESIZE pstResize);
	MACHINEVISIONLIB string CV2HashValue(const CHAR *pszImgName, PST_IMG_RESIZE pstResize);

	MACHINEVISIONLIB ST_IMG_RESIZE CV2GetResizeValue(const CHAR *pszImgName);
	MACHINEVISIONLIB ST_IMG_RESIZE CV2GetResizeValue(Mat& mImg);

	MACHINEVISIONLIB void ImgEquilateral(Mat& mImg, Mat& mResizeImg, INT nResizeLen, Size& objAddedEdgeSize, const Scalar& border_color = Scalar(0, 0, 0));
	MACHINEVISIONLIB void ImgEquilateral(Mat& mImg, Mat& mResizeImg, Size& objAddedEdgeSize, const Scalar& border_color = Scalar(0, 0, 0));

	MACHINEVISIONLIB INT *FaceDetect(const CHAR *pszImgName, FLOAT flScale = 1.05f, INT nMinNeighbors = 5, INT nMinPossibleFaceSize = 16);
	MACHINEVISIONLIB INT *FaceDetect(Mat& mImg, FLOAT flScale = 1.05f, INT nMinNeighbors = 5, INT nMinPossibleFaceSize = 16);
	MACHINEVISIONLIB void MarkFaceWithRectangle(Mat& mImg, INT *pnFaces);
	MACHINEVISIONLIB void MarkFaceWithRectangle(Mat& mImg, FLOAT flScale = 1.05f, INT nMinNeighbors = 5, INT nMinPossibleFaceSize = 16);
	MACHINEVISIONLIB void MarkFaceWithRectangle(const CHAR *pszImgName, FLOAT flScale = 1.05f, INT nMinNeighbors = 5, INT nMinPossibleFaceSize = 16);

	MACHINEVISIONLIB Net InitFaceDetectDNNet(void);	
	MACHINEVISIONLIB Mat FaceDetect(Net& dnnNet, Mat& mImg, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUALRATIO, FLOAT flScale = 1.0f, const Scalar& mean = Scalar(104.0, 177.0, 123.0));
	MACHINEVISIONLIB Mat FaceDetect(Net& dnnNet, const CHAR *pszImgName, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUALRATIO, FLOAT flScale = 1.0f, const Scalar& mean = Scalar(104.0, 177.0, 123.0));
	MACHINEVISIONLIB Mat FaceDetect(Net& dnnNet, const CHAR *pszImgName, const Size& size, FLOAT flScale = 1.0f, const Scalar& mean = Scalar(104.0, 177.0, 123.0));	
	MACHINEVISIONLIB void FaceDetect(Net& dnnNet, Mat& mImg, vector<Face>& vFaces, FLOAT flConfidenceThreshold = 0.3, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUALRATIO, FLOAT flScale = 1.0f, const Scalar& mean = Scalar(104.0, 177.0, 123.0));
	MACHINEVISIONLIB void FaceDetect(Net& dnnNet, const CHAR *pszImgName, vector<Face>& vFaces, FLOAT flConfidenceThreshold = 0.3, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUALRATIO, FLOAT flScale = 1.0f, const Scalar& mean = Scalar(104.0, 177.0, 123.0));
	MACHINEVISIONLIB void FaceDetect(Net& dnnNet, const CHAR *pszImgName, vector<Face>& vFaces, const Size& size, FLOAT flConfidenceThreshold = 0.3, FLOAT flScale = 1.0f, const Scalar& mean = Scalar(104.0, 177.0, 123.0));


	MACHINEVISIONLIB void MarkFaceWithRectangle(Mat& mImg, Mat& mFaces, FLOAT flConfidenceThreshold = 0.3, BOOL blIsShow = FALSE);
	MACHINEVISIONLIB void MarkFaceWithRectangle(Mat& mImg, vector<Face>& vFaces, BOOL blIsShow = FALSE);
	MACHINEVISIONLIB void MarkFaceWithRectangle(Net& dnnNet, const CHAR *pszImgName, const Size& size, FLOAT flConfidenceThreshold = 0.3, FLOAT flScale = 1.0f, const Scalar& mean = Scalar(104.0, 177.0, 123.0));
	MACHINEVISIONLIB void MarkFaceWithRectangle(Net& dnnNet, const CHAR *pszImgName, FLOAT flConfidenceThreshold = 0.3, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUALRATIO, FLOAT flScale = 1.0f, const Scalar& mean = Scalar(104.0, 177.0, 123.0));

	MACHINEVISIONLIB Net InitLightClassifier(vector<string>& vClassNames);	
	MACHINEVISIONLIB void ObjectDetect(Mat& mImg, Net& dnnNet, vector<string>& vClassNames, vector<RecogCategory>& vObjects, FLOAT flConfidenceThreshold = 0.4, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUILATERAL, FLOAT flScale = 1.0f, const Scalar& mean = Scalar(104.0, 117.0, 123.0));
	MACHINEVISIONLIB void ObjectDetect(const CHAR *pszImgName, Net& dnnNet, vector<string>& vClassNames, vector<RecogCategory>& vObjects, const Size& size, FLOAT flConfidenceThreshold = 0.4, FLOAT flScale = 1.0f, const Scalar& mean = Scalar(104.0, 117.0, 123.0));
	MACHINEVISIONLIB void ObjectDetect(const CHAR *pszImgName, Net& dnnNet, vector<string>& vClassNames, vector<RecogCategory>& vObjects, FLOAT flConfidenceThreshold = 0.4, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUILATERAL, FLOAT flScale = 1.0f, const Scalar& mean = Scalar(104.0, 117.0, 123.0));
	MACHINEVISIONLIB void MarkObjectWithRectangle(Mat& mImg, vector<RecogCategory>& vObjects);
	MACHINEVISIONLIB void MarkObjectWithRectangle(const CHAR *pszImgName, Net& dnnNet, vector<string>& vClassNames, const Size& size, FLOAT flConfidenceThreshold = 0.4, FLOAT flScale = 1.0f, const Scalar& mean = Scalar(104.0, 117.0, 123.0));
	MACHINEVISIONLIB void MarkObjectWithRectangle(const CHAR *pszImgName, Net& dnnNet, vector<string>& vClassNames, FLOAT flConfidenceThreshold = 0.4, ENUM_IMGRESIZE_METHOD enumMethod = EIRSZM_EQUILATERAL, FLOAT flScale = 1.0f, const Scalar& mean = Scalar(104.0, 117.0, 123.0));
	MACHINEVISIONLIB INT GetObjectNum(vector<RecogCategory>& vObjects, string strObjectName, FLOAT *pflConfidenceOfExist, FLOAT *pflConfidenceOfObjectNum); 

	typedef enum {
		YOLO2, YOLO2_TINY, YOLO2_VOC, YOLO2_TINY_VOC
	} ENUM_YOLO2_MODEL_TYPE;

#define YOLO2_PROBABILITY_DATA_INDEX	5
	MACHINEVISIONLIB Net InitYolo2Classifier(vector<string>& vClassNames, ENUM_YOLO2_MODEL_TYPE enumModelType = YOLO2);
	MACHINEVISIONLIB void Yolo2ObjectDetect(Mat& mImg, Net& objDNNNet, vector<string>& vClassNames, vector<RecogCategory>& vObjects, FLOAT flConfidenceThreshold = 0.4);	


	MACHINEVISIONLIB void MergeOverlappingRect(vector<ST_DIAGONAL_POINTS> vSrcRects, vector<ST_DIAGONAL_POINTS>& vMergedRects);

	MACHINEVISIONLIB void ShowImageWindow(CHAR *pszWindowTitle, BOOL blIsShowing);
	MACHINEVISIONLIB void CAPTCHAImgPreProcess(Mat& mSrcImg, Mat& mDstImg);	
	MACHINEVISIONLIB void CAPTCHAImgPreProcess(Mat& mSrcImg, Mat& mDstImg, const Size& size);
};

//* caffe接口
//* 关于神经网络前向传播和后向传播的入门文章
//* https://blog.csdn.net/zhangjunhit/article/details/53501680
namespace caffe2shell {
	//* 模板函数一定要被人为实例化，否则编译器无法导出，函数无法被真正使用，参见：
	//* https://blog.csdn.net/liyuanbhu/article/details/50363670
	template <typename DType> caffe::Net<DType> *LoadNet(std::string strParamFile, std::string strModelFile, caffe::Phase phase);	
	template MACHINEVISIONLIB caffe::Net<FLOAT> *caffe2shell::LoadNet(std::string strParamFile, std::string strModelFile, caffe::Phase phase);
	template MACHINEVISIONLIB caffe::Net<DOUBLE> *caffe2shell::LoadNet(std::string strParamFile, std::string strModelFile, caffe::Phase phase);

	template <typename DType> void ExtractFeature(caffe::Net<DType> *pNet, caffe::MemoryDataLayer<DType> *pMemDataLayer, Mat& matImgROI, DType *pdtaImgFeature, INT nFeatureDimension, const CHAR *pszLayerName);
	template MACHINEVISIONLIB void ExtractFeature(caffe::Net<FLOAT> *pflNet, caffe::MemoryDataLayer<FLOAT> *pMemDataLayer, Mat& matImgROI, FLOAT *pflaImgFeature, INT nFeatureDimension, const CHAR *pszLayerName);
	template MACHINEVISIONLIB void ExtractFeature(caffe::Net<DOUBLE> *pdblNet, caffe::MemoryDataLayer<DOUBLE> *pMemDataLayer, Mat& matImgROI, DOUBLE *pdblaImgFeature, INT nFeatureDimension, const CHAR *pszBlobName);

	template <typename DType> void ExtractFeature(caffe::Net<DType> *pNet, caffe::MemoryDataLayer<DType> *pMemDataLayer, Mat& matImgROI, Mat& matImgFeature, INT nFeatureDimension, const CHAR *pszLayerName);
	template MACHINEVISIONLIB void ExtractFeature(caffe::Net<FLOAT> *pflNet, caffe::MemoryDataLayer<FLOAT> *pMemDataLayer, Mat& matImgROI, Mat& matImgFeature, INT nFeatureDimension, const CHAR *pszLayerName);
	template MACHINEVISIONLIB void ExtractFeature(caffe::Net<DOUBLE> *pdblNet, caffe::MemoryDataLayer<DOUBLE> *pMemDataLayer, Mat& matImgROI, Mat& matImgFeature, INT nFeatureDimension, const CHAR *pszBlobName);
};

class MACHINEVISIONLIB ImgMatcher {
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
class MACHINEVISIONLIB CMachineVisionLib {
public:
	CMachineVisionLib(void);
	// TODO:  在此添加您的方法。
};

