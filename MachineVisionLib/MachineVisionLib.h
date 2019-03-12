// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� MACHINEVISIONLIB_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// MACHINEVISIONLIB ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
//* How to compile DLib:
//* https://blog.csdn.net/xingchenbingbuyu/article/details/53236541
//* ����VS2015
//* https://blog.csdn.net/flyyufenfei/article/details/79176136
//* 
//* OCR���:
//* https://blog.csdn.net/qq_37674858/article/details/80576978
//* tesseract D:\work\SC\DlibTest\x64\Release\ValidCodeImg.jpg out -psm 13
//* tesseract���䴦��Ϊԭʼ����
//* ѵ��Ҫʶ������ݼ��������ʶ��Ч�ʣ�
//* tesseract.exe --psm 7 captcha.tif captcha batch.nochop makebox
//* ����psm 7��ȥ�����󣬷����޷�ѵ��
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

#ifndef USE_OPENCV	//* ���AddMatVector()�����ڵ�����
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

//* ��¼������ÿ�����������ڻ������ı����С����λ��size(int)��������֮���������λÿ�����Ĳ���
#define LIBFACEDETECT_RESULT_STEP 142

//* define the buffer size. Do not change the size!
//* ���建������С ��Ҫ�ı��С��
#define LIBFACEDETECT_BUFFER_SIZE 0x20000

typedef struct _ST_IMG_RESIZE_ {
	INT nRows;
	INT nCols;
} ST_IMG_RESIZE, *PST_IMG_RESIZE;

//* �Խǵ�
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

//* ��������ʵʱ��Ƶ����Ļص�����ԭ������
typedef void (*PCB_VIDEOHANDLER)(Mat &mVideoData, DWORD64 dw64InputParam);

//* OpenCV�ӿ�
namespace cv2shell {
	enum ENUM_IMGRESIZE_METHOD {
		EIRSZM_EQUALRATIO = 0,	//* �ȱ�����С��Ŵ�ͼƬ
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

	//* ������DNN�����ϻ��ѵ�ʱ�䣬��λ����
	MACHINEVISIONLIB DOUBLE GetTimeSpentInNetDetection(Net& objDNNNet);

	MACHINEVISIONLIB void MergeOverlappingRect(vector<ST_DIAGONAL_POINTS> vSrcRects, vector<ST_DIAGONAL_POINTS>& vMergedRects);

	MACHINEVISIONLIB void ShowImageWindow(const CHAR *pszWindowTitle, BOOL blIsShowing);
	MACHINEVISIONLIB void CAPTCHAImgPreProcess(Mat& mSrcImg, Mat& mDstImg);	
	MACHINEVISIONLIB void CAPTCHAImgPreProcess(Mat& mSrcImg, Mat& mDstImg, const Size& size);	
};

//* caffe�ӿ�
//* ����������ǰ�򴫲��ͺ��򴫲�����������
//* https://blog.csdn.net/zhangjunhit/article/details/53501680
namespace caffe2shell {
	//* ģ�庯��һ��Ҫ����Ϊʵ����������������޷������������޷�������ʹ�ã��μ���
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

//* ����OCV2�ṩ��DNN�ӿ�API��Ƶ����������ӿ��࣬����ǰ���Сi��������һ���ӿ��࣬�������δʵ�ֵĽӿڶ��庯��
class MACHINEVISIONLIB iOCV2DNNObjectDetector {
public:
	iOCV2DNNObjectDetector(FLOAT flConfidenceThreshold) : o_flConfidenceThreshold(flConfidenceThreshold){}

	virtual void detect(Mat& mSrcImg, vector<RecogCategory>& vObjects) = 0;
	virtual void detect(Mat& mSrcImg, vector<RecogCategory>& vObjects, vector<string>& vstrFilter) = 0;
	virtual void MarkObject(Mat& mShowImg, vector<RecogCategory>& vObjects);
	virtual INT GetObjectNum(vector<RecogCategory>& vObjects, string strObjectName, FLOAT *pflConfidenceOfExist, FLOAT *pflConfidenceOfObjectNum);
	virtual ~iOCV2DNNObjectDetector(void) {};	//* ȱʡ����������

	Net GetNet(void) const
	{
		return o_objDNNNet;
	}

	vector<string> GetClassNames(void) const
	{
		return o_vClassNames;
	}

protected:
	FLOAT o_flConfidenceThreshold;	//* ���Ŷ���ֵ	
	Net o_objDNNNet;				//* ����ѵ��ģ�ͽ��������磬ʵ�����ڹ��캯����Ϊ�丳ֵ
	vector<string> o_vClassNames;	//* �����������
};

//* OCV2 DNN�ӿ�֮SSD�����
class MACHINEVISIONLIB OCV2DNNObjectDetectorSSD : public iOCV2DNNObjectDetector {
public:
	typedef enum {
		VGGSSD = 0,
		MOBNETSSD
	} ENUM_DETECTOR;

	//* ����ʹ��try-catch���Բ�׽��ʼ������
	OCV2DNNObjectDetectorSSD(FLOAT flConfidenceThreshold = 0.4F, ENUM_DETECTOR enumDetector = VGGSSD);

	//* �������ͼ���ҳ���ʶ������
	void detect(Mat& mSrcImg, vector<RecogCategory>& vObjects);
	void detect(Mat& mSrcImg, vector<RecogCategory>& vObjects, vector<string>& vstrFilter);

private:
	DOUBLE o_dblNormalCoef;			//* ��һ��ϵ��
	Scalar o_objMean;				//* Ҫ��ȥ�ľ�ֵ
	ENUM_DETECTOR o_enumDetector;	//* ���������
};

//* OCV2 DNN�ӿ�֮YOLO2�����
class MACHINEVISIONLIB OCV2DNNObjectDetectorYOLO2 : public iOCV2DNNObjectDetector {
public:
	typedef enum {
		YOLO2 = 0, 
		YOLO2_TINY, 
		YOLO2_VOC, 
		YOLO2_TINY_VOC
	} ENUM_DETECTOR;

#define YOLO2_PROBABILITY_DATA_INDEX	5

	//* ����ʹ��try-catch���Բ�׽��ʼ������
	OCV2DNNObjectDetectorYOLO2(FLOAT flConfidenceThreshold = 0.4F, ENUM_DETECTOR enumDetector = YOLO2);

	//* �������ͼ���ҳ���ʶ������
	void detect(Mat& mSrcImg, vector<RecogCategory>& vObjects);
	void detect(Mat& mSrcImg, vector<RecogCategory>& vObjects, vector<string>& vstrFilter);

private:	
	ENUM_DETECTOR o_enumDetector;	//* ���������
};

// �����Ǵ� MachineVisionLib.dll ������
class MACHINEVISIONLIB CMachineVisionLib {
public:
	CMachineVisionLib(void);
	// TODO:  �ڴ�������ķ�����
};

