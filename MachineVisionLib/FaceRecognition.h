#pragma once
#ifdef MACHINEVISIONLIB_EXPORTS
#define FACERECOGNITION __declspec(dllexport)
#else
#define FACERECOGNITION __declspec(dllimport)
#endif

#include "caffe/caffe.hpp"

namespace caffe
{
	extern INSTANTIATE_CLASS(InputLayer);
	extern INSTANTIATE_CLASS(InnerProductLayer);
	extern INSTANTIATE_CLASS(DropoutLayer);
	extern INSTANTIATE_CLASS(ConvolutionLayer);
	extern INSTANTIATE_CLASS(ReLULayer);
	extern INSTANTIATE_CLASS(PoolingLayer);
	extern INSTANTIATE_CLASS(LRNLayer);
	extern INSTANTIATE_CLASS(SoftmaxLayer);
	extern INSTANTIATE_CLASS(MemoryDataLayer);
}

#define FACE_FEATURE_DIMENSION					2622						//* 要提取的特征维数
#define FACE_FEATURE_LAYER_NAME					"fc8"						//* 特征层的名称
#define FACE_DB_PATH							"./PERSONS"					//* 数据库保存路径
#define FACE_DB_STATIS_FILE						"./FACEDB_STATISTIC.xml"	//* 人脸库统计文件
#define FDBSTATIS_LABEL_PERSON_NUM				"PERSON_NUM"				//* 人脸库统计文件标签之人数
#define FDBSTATIS_LABEL_PERSONNAME_TOTAL_LEN	"PERSONNAME_TOTAL_LENGTH"	//* 人脸库统计文件标签之人名总长度，结束符\x00亦被统计在内


#define FACE_DATA_FILE_NAME		"NEO-FACEFEATURE-DATA.DAT"	//* 加载人脸特征数据到内存时，内存文件的名称
#define PERSON_NAME_FILE_NAME	"NEO-PERSON-NAME.TXT"		//* 加载人名数据到内存时，内存文件的名称

//* 识别出的人名、可信度等信息保存到该结构体中
typedef struct _ST_PERSON_ {
	string strPersonName;	//* 人名
	DOUBLE dblConfidence;	//* 可信度
} ST_PERSON, *PST_PERSON;

//* 人脸数据库统计信息
typedef struct _ST_FACE_DB_STATIS_INFO_ {
	INT nPersonNum;				//* 人数
	INT nTotalLenOfPersonName;	//* 所有人名的总长度
} ST_FACE_DB_STATIS_INFO, *PST_FACE_DB_STATIS_INFO;

//* 人脸数据库类
class FACERECOGNITION FaceDatabase {
public:
	FaceDatabase() {
#if NEED_GPU
		caffe::Caffe::set_mode(caffe::Caffe::GPU);
#else
		caffe::Caffe::set_mode(caffe::Caffe::CPU);
#endif		

		stMemFileFaceData.pvMem = NULL;
		stMemFileFaceData.hMem = INVALID_HANDLE_VALUE;

		stMemFilePersonName.pvMem = NULL;
		stMemFilePersonName.hMem = INVALID_HANDLE_VALUE;

		pvideo_predict = NULL;
	}

	~FaceDatabase() {	
		common_lib::DeletMemFile(&stMemFileFaceData);
		common_lib::DeletMemFile(&stMemFilePersonName);

		if (pvideo_predict)
			delete pvideo_predict;
	}

	BOOL LoadCaffeVGGNet(string strCaffePrototxtFile, string strCaffeModelFile);

	BOOL IsPersonAdded(const string& strPersonName);
	BOOL AddFace(const CHAR *pszImgName, const string& strPersonName);
	BOOL AddFace(Mat& matImg, const string& strPersonName);

	void GetFaceDBStatisInfo(PST_FACE_DB_STATIS_INFO pstInfo);
	BOOL LoadFaceData(void);

	Mat FaceChipsHandle(Mat& matFaceChips, DOUBLE dblPowerValue = 0.1, DOUBLE dblGamma = 0.8, DOUBLE dblNorm = 10);

	DOUBLE Predict(Mat& matImg, string& strPersonName, FLOAT flConfidenceThreshold = 0.55, FLOAT flStopPredictThreshold = 0.95);
	DOUBLE Predict(const CHAR *pszImgName, string& strPersonName, FLOAT flConfidenceThreshold = 0.55, FLOAT flStopPredictThreshold = 0.95);

	class FACERECOGNITION VideoPredict {
	public:
		VideoPredict(FaceDatabase *pinput_face_db, FLOAT flInputScale = 1.05f, INT nInputMinNeighbors = 5, INT nInputMinPossibleFaceSize = 16) {
			pubFeaceDetectResultBuf = (UCHAR*)malloc(LIBFACEDETECT_BUFFER_SIZE);
			if (!pubFeaceDetectResultBuf)
			{
				perror(NULL);
				cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 4 << ", malloc error code:" << GetLastError() << endl;

				exit(-1);
			}

			flScale = flInputScale;
			nMinNeighbors = nInputMinNeighbors;
			nMinPossibleFaceSize = nInputMinPossibleFaceSize;

			pface_db = pinput_face_db;
		}

		~VideoPredict() {
			if (pubFeaceDetectResultBuf)
				free(pubFeaceDetectResultBuf);

			cout << "The memory allocated for the libfacedetecion library has been released." << endl;
		}

		vector<ST_PERSON> Predict(Mat& matVideoImg, FLOAT flConfidenceThreshold = 0.80, FLOAT flStopPredictThreshold = 0.95);

	private:
		UCHAR *pubFeaceDetectResultBuf;
		FLOAT flScale;
		INT nMinNeighbors;
		INT nMinPossibleFaceSize;
		FaceDatabase *pface_db;
	} *pvideo_predict;

	caffe::Net<FLOAT> *pcaflNet;
	caffe::MemoryDataLayer<FLOAT> *pflMemDataLayer;

	ST_MEM_FILE stMemFileFaceData;
	ST_MEM_FILE stMemFilePersonName;
	INT nActualNumOfPerson;

private:
	//* 提取脸部图像，注意只能提取一张脸部图像
	Mat ExtractFaceChips(Mat& matImg, FLOAT flScale = 1.05f, INT nMinNeighbors = 5, INT nMinPossibleFaceSize = 16);
	Mat ExtractFaceChips(const CHAR *pszImgName, FLOAT flScale = 1.05f, INT nMinNeighbors = 5, INT nMinPossibleFaceSize = 16);	
	void UpdateFaceDBStatisticFile(const string& strPersonName);
	void PutFaceToMemFile(void);
};
