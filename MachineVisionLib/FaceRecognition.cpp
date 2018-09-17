// FaceRecognition.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <vector>
#include <facedetect-dll.h>
#include "common_lib.h"
#include "MachineVisionLib.h"
#include "MathAlgorithmLib.h"
#include "ImagePreprocess.h"
#include "FaceRecognition.h"

BOOL FaceDatabase::LoadCaffeVGGNet(string strCaffePrototxtFile, string strCaffeModelFile)
{
	o_pcaflNet = caffe2shell::LoadNet<FLOAT>(strCaffePrototxtFile, strCaffeModelFile, caffe::TEST);

	if (o_pcaflNet)
	{
		o_pflMemDataLayer = (caffe::MemoryDataLayer<FLOAT> *)o_pcaflNet->layers()[0].get();
		return TRUE;
	}
	else
		return FALSE;
}

//* 从一张完整图片中提取脸部特征图，该函数只能提取一张脸的，参数mFaceFeatureChips为出口参数，接收提取的特征数据
static void __ExtractFaceFeatureChips(Mat& mGray, SHORT *psScalar, Mat& mFaceFeatureChips)
{
	//* 获取68个脸部特征点，脸部到鼻子:0-35, 眼睛:36-47,嘴形：48-60，嘴线：61-67
	vector<dlib::point> vFaceFeature;
	for (INT j = 0; j < 68; j++)
		vFaceFeature.push_back(dlib::point((INT)psScalar[6 + 2 * j], (INT)psScalar[6 + 2 * j + 1]));

	//* 为了速度，牺牲了部分可读性：
	//* psScalar的0、1、2、3分别代表坐标x、y、width、height，如此：
	//* rect()的四个参数分别是左上角坐标和右下角坐标
	dlib::rectangle rect(psScalar[0], psScalar[1], psScalar[0] + psScalar[2] - 1, psScalar[1] + psScalar[3] - 1);

	//* 将68个特征点转为dlib数据
	dlib::full_object_detection shape(rect, vFaceFeature);
	vector<dlib::full_object_detection> shapes;
	shapes.push_back(shape);

	//dlib::array<dlib::array2d<dlib::rgb_pixel>> FaceChips;	//* rgb_pixel指定按照RGB三通道分配内存
	dlib::array<dlib::array2d<uchar>> FaceChips;
	extract_image_chips(dlib::cv_image<uchar>(mGray), get_face_chip_details(shapes), FaceChips);
	mFaceFeatureChips = dlib::toMat(FaceChips[0]);		

	//* 按照网络要求调整为224,224大小
	resize(mFaceFeatureChips, mFaceFeatureChips, Size(224, 224), INTER_AREA);
	//imshow("dlib", mFaceFeatureChips);
	//waitKey(0);
}

//* 同上，从一张完整图片中提取脸部特征图，该函数只能提取一张脸的，参数mFaceFeatureChips为出口参数，接收提取的特征数据
static void __ExtractFaceFeatureChips(dlib::shape_predictor& objShapePredictor, Mat& mFaceGray, Face& objFace, Mat& mFaceFeatureChips)
{
	vector<dlib::full_object_detection> vobjShapes;	

	//* 提取68个特征点
	dlib::rectangle objDLIBRect(objFace.o_nLeftTopX, objFace.o_nLeftTopY, objFace.o_nRightBottomX, objFace.o_nRightBottomY);
	vobjShapes.push_back(objShapePredictor(dlib::cv_image<uchar>(mFaceGray), objDLIBRect));
	if (vobjShapes.empty())
		return;	

	dlib::array<dlib::array2d<uchar>> objFaceChips;
	extract_image_chips(dlib::cv_image<uchar>(mFaceGray), get_face_chip_details(vobjShapes), objFaceChips);
	mFaceFeatureChips = dlib::toMat(objFaceChips[0]);

	
	//* 按照网络要求调整为224,224大小
	resize(mFaceFeatureChips, mFaceFeatureChips, Size(224, 224), INTER_AREA);
}

//* 从一张完整图片中提取脸部特征图
Mat FaceDatabase::ExtractFaceChips(Mat& mImg, FLOAT flScale, INT nMinNeighbors, INT nMinPossibleFaceSize)
{
	Mat mDummy;

	INT *pnFaces = NULL;
	UCHAR *pubResultBuf = (UCHAR*)malloc(LIBFACEDETECT_BUFFER_SIZE);
	if (!pubResultBuf)
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 3 << ", malloc error code:" << GetLastError() << endl;
		return mDummy;
	}

	Mat mGray;
	cvtColor(mImg, mGray, CV_BGR2GRAY);

	//* 带特征点检测，这个函数要比DLib的特征点检测函数稍微快一些
	INT nLandmark = 1;
	pnFaces = facedetect_multiview_reinforce(pubResultBuf, (UCHAR*)(mGray.ptr(0)), mGray.cols, mGray.rows, (INT)mGray.step,
												flScale, nMinNeighbors, nMinPossibleFaceSize, 0, nLandmark);
	if (!pnFaces)
	{
		cout << "Error: No face was detected." << endl;
		return mDummy;
	}	

	//* 单张图片只允许存在一个人脸
	if (*pnFaces != 1)
	{
		cout << "Error: Multiple faces were detected, and only one face was allowed." << endl;
		return mDummy;
	}

	//* 获取68个脸部特征点，脸部到鼻子:0-35, 眼睛:36-47,嘴形：48-60，嘴线：61-67
	SHORT *psScalar = ((SHORT*)(pnFaces + 1));

	Mat mFace;
	__ExtractFaceFeatureChips(mGray, psScalar, mFace);
	
	free(pubResultBuf);

	return mFace;
}

//* 同上
Mat FaceDatabase::ExtractFaceChips(const CHAR *pszImgName, FLOAT flScale, INT nMinNeighbors, INT nMinPossibleFaceSize)
{
	Mat mImg = imread(pszImgName);
	if (mImg.empty())
	{
		cout << "ExtractFaceChips() error：unable to read the picture, please confirm that the picture『" << pszImgName << "』 exists and the format is corrrect." << endl;

		return mImg;
	}

	return ExtractFaceChips(mImg, flScale, nMinNeighbors, nMinPossibleFaceSize);
}

//* 针对光照、采集设备设备变化等问题，我们针对人脸使用了结合gamma校正、差分高斯滤波(DoG)、对比度均衡化三种技术的光照预处理算法，理论及公式参见：
//* https://blog.csdn.net/wuzuyu365/article/details/51898714
Mat FaceDatabase::FaceChipsHandle(Mat& mFaceChips, DOUBLE dblPowerValue, DOUBLE dblGamma, DOUBLE dblNorm)
{
	Mat mDstChips;

	//* 高通滤波核，对比测试结果，目前这个核最好：
	//* 0.85-0.86 0.88-0.87 0.88-0.85 0.78-0.75
	FLOAT flaHighFilterKernel[9] = { -1, -1, -1, -1, 9, -1, -1, -1, -1 };
	imgpreproc::ContrastEqualizationWithFilter(mFaceChips, mDstChips, Size(3, 3), flaHighFilterKernel, dblGamma, dblPowerValue, dblNorm);

	//* 进入caffe提取特征之前，必须为RGB格式才可，否则caffe会报错
	cvtColor(mDstChips, mDstChips, CV_GRAY2RGB);

	//imshow("FaceChipsHandle", mDstChips);
	//waitKey(0);

	//* 验证代码，正常逻辑不需要
	//FileStorage fs("mat.xml", FileStorage::WRITE);
	//fs << "MAT-DATA" << matFloat;	
	//fs.release();

	return mDstChips;
}

//* 看看该人是否已经添加到数据库中，避免重复添加
BOOL FaceDatabase::IsPersonAdded(const string& strPersonName)
{
	HANDLE hDir = CLIBOpenDirectory(FACE_DB_PATH);
	if (hDir == INVALID_HANDLE_VALUE)
	{
		cout << "Unabled to open the folder " << FACE_DB_PATH << ", the process will be exit." << endl;
		exit(-1);
	}

	UINT unNameLen;
	string strFileName, strPersonFileName = strPersonName + ".xml";
	while ((unNameLen = CLIBReadDir(hDir, strFileName)) > 0)
	{		
		if (strFileName == strPersonFileName)
		{
			CLIBCloseDirectory(hDir);
			return TRUE;
		}
	}

	CLIBCloseDirectory(hDir);

	return FALSE;
}

//* 更新人脸库统计文件
void FaceDatabase::UpdateFaceDBStatisticFile(const string& strPersonName)
{
	ST_FACE_DB_STATIS_INFO stInfo;

	//* 先读出
	GetFaceDBStatisInfo(&stInfo);

	//* 更新数值，其中stInfo.dwTotalLenOfPersonName预留出"\x00"来，以方便读取
	stInfo.nPersonNum += 1;
	stInfo.nTotalLenOfPersonName += strPersonName.size() + 1;

	//* 写入
	FileStorage fs(FACE_DB_STATIS_FILE, FileStorage::WRITE);
	fs << FDBSTATIS_LABEL_PERSON_NUM << stInfo.nPersonNum;
	fs << FDBSTATIS_LABEL_PERSONNAME_TOTAL_LEN << stInfo.nTotalLenOfPersonName;

	fs.release();
}

void FaceDatabase::GetFaceDBStatisInfo(PST_FACE_DB_STATIS_INFO pstInfo)
{
	FileStorage fs;
	if (fs.open(FACE_DB_STATIS_FILE, FileStorage::READ))
	{
		fs[FDBSTATIS_LABEL_PERSON_NUM] >> pstInfo->nPersonNum;
		fs[FDBSTATIS_LABEL_PERSONNAME_TOTAL_LEN] >> pstInfo->nTotalLenOfPersonName;

		fs.release();
	}
	else
	{		
		cout << "The statistic file 『" << FACE_DB_STATIS_FILE << "』 of the face database does not exist, and has not added face data yet?" << endl;

		pstInfo->nPersonNum = 0;
		pstInfo->nTotalLenOfPersonName = 0;
	}
}

BOOL FaceDatabase::AddFace(Mat& mImg, const string& strPersonName)
{
	if (mImg.empty())
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 3 << ", the parameter matImg is empty." << GetLastError() << endl;
		return FALSE;
	}

	if (IsPersonAdded(strPersonName))
	{
		cout << strPersonName  << " has been added to the face database." << endl;
		return TRUE;
	}

	//* 从图片中提取脸部区域
	Mat mFaceChips = ExtractFaceChips(mImg);
	if (mFaceChips.empty())
	{
		cout << "No face was detected." << endl;
		return FALSE;
	}

	//* ROI(region of interest)，按照一定算法将脸部区域转换为caffe网络特征提取需要的输入数据
	Mat mFaceROI = FaceChipsHandle(mFaceChips);

	//* 通过caffe网络提取图像特征
	Mat mImgFeature(1, FACE_FEATURE_DIMENSION, CV_32F);
	caffe2shell::ExtractFeature(o_pcaflNet, o_pflMemDataLayer, mFaceROI, mImgFeature, FACE_FEATURE_DIMENSION, FACE_FEATURE_LAYER_NAME);	
	
	//* 将特征数据存入文件
	string strXMLFile = string(FACE_DB_PATH) + "/" + strPersonName + ".xml";
	FileStorage fs(strXMLFile, FileStorage::WRITE);
	fs << "VGG-FACEFEATURE" << mImgFeature;
	fs.release();

	//* 保存人数和人名字节数，用于人脸数据库加载
	UpdateFaceDBStatisticFile(strPersonName);

	return TRUE;
}

BOOL FaceDatabase::AddFace(const CHAR *pszImgName, const string& strPersonName)
{
	Mat mImg = imread(pszImgName);
	if (mImg.empty())
	{
		cout << "AddFace() error：unable to read the picture, please confirm that the picture『" << pszImgName << "』 exists and the format is corrrect." << endl;

		return FALSE;
	}

	return AddFace(mImg, strPersonName);
}

//* 添加人脸，不同于上面两个函数，它使用DLIB提供的68个特征点模型提取脸部特征
BOOL FaceDatabase::AddFace(Mat& mImgGray, Face& objFace, const string& strPersonName)
{
	if (mImgGray.empty())
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 3 << ", the parameter matImg is empty." << GetLastError() << endl;
		return FALSE;
	}

	if (IsPersonAdded(strPersonName))
	{
		cout << strPersonName << " has been added to the face database." << endl;
		return TRUE;
	}

	Mat mFaceChips;
	__ExtractFaceFeatureChips(o_objShapePredictor, mImgGray, objFace, mFaceChips);
	if (mFaceChips.empty())
	{
		cout << "No face was detected." << endl;
		return FALSE;
	}

	//* ROI(region of interest)，按照一定算法将脸部区域转换为caffe网络特征提取需要的输入数据
	Mat mFaceROI = FaceChipsHandle(mFaceChips);

	//* 通过caffe网络提取图像特征
	Mat mImgFeature(1, FACE_FEATURE_DIMENSION, CV_32F);
	caffe2shell::ExtractFeature(o_pcaflNet, o_pflMemDataLayer, mFaceROI, mImgFeature, FACE_FEATURE_DIMENSION, FACE_FEATURE_LAYER_NAME);

	//* 将特征数据存入文件
	string strXMLFile = string(FACE_DB_PATH) + "/" + strPersonName + ".xml";
	FileStorage fs(strXMLFile, FileStorage::WRITE);
	fs << "VGG-FACEFEATURE" << mImgFeature;
	fs.release();

	//* 保存人数和人名字节数，用于人脸数据库加载
	UpdateFaceDBStatisticFile(strPersonName);

	return TRUE;
}

//* 将人脸数据从文件中取出并放入指定内存
static BOOL __PutFaceDataFromFile(const string strFaceDataFile, FLOAT *pflFaceData)
{
	Mat mFaceFeatureData;

	FileStorage fs;
	if (fs.open(strFaceDataFile, FileStorage::READ))
	{
		fs["VGG-FACEFEATURE"] >> mFaceFeatureData;

		fs.release();

		for (INT i=0; i < FACE_FEATURE_DIMENSION; i++)
			pflFaceData[i] = mFaceFeatureData.at<FLOAT>(0, i);

		return TRUE;
	}
	else
	{
		cout << "File 『" << strFaceDataFile << "』 not exist." << endl;

		return FALSE;
	}	
}

//* 将人脸数据写入内存文件
void FaceDatabase::PutFaceToMemFile(void)
{
	HANDLE hDir = CLIBOpenDirectory(FACE_DB_PATH);
	if (hDir == INVALID_HANDLE_VALUE)
	{
		cout << "Unabled to open the folder " << FACE_DB_PATH << ", the process will be exit." << endl;
		exit(-1);
	}

	UINT unNameLen, unWriteBytes;
	string strFileName;
	unWriteBytes = 0;
	o_nActualNumOfPerson = 0;
	while ((unNameLen = CLIBReadDir(hDir, strFileName)) > 0)
	{
		//* 写入特征数据
		string strFaceDataFile = string(FACE_DB_PATH) + "/" + strFileName;
		if (!__PutFaceDataFromFile(strFaceDataFile, ((FLOAT *)o_stMemFileFaceData.pvMem) + o_nActualNumOfPerson * FACE_FEATURE_DIMENSION))
			continue;		
		o_nActualNumOfPerson++;

		//* 写入文件名
		size_t sztEndPos = strFileName.rfind(".xml");		
		string strPersonName = strFileName.substr(0, sztEndPos);
		sprintf(((CHAR*)o_stMemFilePersonName.pvMem) + unWriteBytes, "%s", strPersonName.c_str());
		unWriteBytes += strPersonName.size() + 1;		
	}

	CLIBCloseDirectory(hDir);
}

//* 从数据库中加载人脸数据到内存
BOOL FaceDatabase::LoadFaceData(void)
{
	ST_FACE_DB_STATIS_INFO stInfo;

	//* 读出统计信息
	GetFaceDBStatisInfo(&stInfo);
	if (!stInfo.nPersonNum)
	{
		cout << "No face data has been added, the process will be exit." << endl;
		exit(-1);
	}

	//* 为人脸数据建立内存文件
	if(!common_lib::CreateMemFile(&o_stMemFileFaceData, stInfo.nPersonNum * FACE_FEATURE_DIMENSION * sizeof(FLOAT)))
	{ 
		return FALSE;
	}

	//* 为人名建立内存文件
	if (!common_lib::CreateMemFile(&o_stMemFilePersonName, stInfo.nTotalLenOfPersonName))
	{
		common_lib::DeletMemFile(&o_stMemFileFaceData);

		return FALSE;
	}
	memset((CHAR*)o_stMemFilePersonName.pvMem, 0, stInfo.nTotalLenOfPersonName);

	//* 将人脸数据放入内存文件
	PutFaceToMemFile();

	//* 验证代码，正常逻辑不需要
	//const CHAR *pszPerson = (const CHAR *)stMemFilePersonName.pvMem;
	//const FLOAT *pflaData = (FLOAT*)stMemFileFaceData.pvMem;
	//for (INT i = 0; i < nActualNumOfPerson; i++)
	//{
	//	cout << "『Name』 " << pszPerson << " ";
	//	pszPerson += strlen(pszPerson) + 1;

	//	cout << pflaData[i * FACE_FEATURE_DIMENSION] << " " << pflaData[i * FACE_FEATURE_DIMENSION + 1]
	//		<< " " << pflaData[i * FACE_FEATURE_DIMENSION + 2] << " " << pflaData[i * FACE_FEATURE_DIMENSION + 3] << endl;
	//}
	
	return TRUE;
}

BOOL FaceDatabase::LoadDLIB68FaceLandmarksModel(const CHAR *pszModelFileName)
{
	try {
		dlib::deserialize(pszModelFileName) >> o_objShapePredictor;
		o_blIsLoadDLIB68FaceLandmarksModel = TRUE;
		return TRUE;
	}
	catch (runtime_error& e) {		
		cerr << e.what() << endl;
		return FALSE;
	}
	catch (exception& e) {
		cerr << e.what() << endl;
		return FALSE;
	}
}

//* 完成实际的预测
static DOUBLE __Predict(FaceDatabase *pobjFaceDB, Mat& matFaceChips, string& strPersonName, 
							FLOAT flConfidenceThreshold, FLOAT flStopPredictThreshold)
{	
	//* ROI(region of interest)，按照一定算法(比如gamma校正、滤波、归一化等处理)将脸部区域转换为caffe网络特征提取需要的输入数据
	Mat matFaceROI = pobjFaceDB->FaceChipsHandle(matFaceChips);	
	
	//* 通过caffe网络提取图像特征
	FLOAT flaFaceFeature[FACE_FEATURE_DIMENSION];	
	caffe2shell::ExtractFeature(pobjFaceDB->o_pcaflNet, pobjFaceDB->o_pflMemDataLayer, matFaceROI, flaFaceFeature, FACE_FEATURE_DIMENSION, FACE_FEATURE_LAYER_NAME);		

	//* 查找匹配的脸部特征	
	const CHAR *pszPerson = (const CHAR *)pobjFaceDB->o_stMemFilePersonName.pvMem;
	const FLOAT *pflaData = (FLOAT*)pobjFaceDB->o_stMemFileFaceData.pvMem;
	DOUBLE dblMaxSimilarity = flConfidenceThreshold, dblSimilarity;
	const CHAR *pszMatchPersonName = NULL;
	for (INT i = 0; i < pobjFaceDB->o_nActualNumOfPerson; i++)
	{
		dblSimilarity = malib::CosineSimilarity(pflaData + i * FACE_FEATURE_DIMENSION, flaFaceFeature, FACE_FEATURE_DIMENSION);
		if (dblSimilarity > dblMaxSimilarity)
		{
			//* 大于停止查找的阈值，基本就可以确定是这个人了，所以不再查找了
			if (dblSimilarity > flStopPredictThreshold)
			{
				strPersonName = pszPerson;

				return dblSimilarity;
			}

			dblMaxSimilarity = dblSimilarity;
			pszMatchPersonName = pszPerson;
		}

		//cout << pszPerson << ":" << dblSimilarity << endl;

		//* 下一个
		pszPerson += strlen(pszPerson) + 1;
	}

	//* 超过了用户容忍的最小相似度值，则可以认为找到了
	if (dblMaxSimilarity > flConfidenceThreshold)
		strPersonName = pszMatchPersonName;
	
	return dblMaxSimilarity;
}

//* 返回相似度，参数strPersonName接收找到的人名，flConfidenceThreshold指定最低可信度值，低于这个值就认为不是这个人
//* flStopPredictThreshold指定停止查找的阈值，因为函数会检索整个数据库以找到最大相似度的人脸，有了这个值函数只要找到大于这个
//* 值的人脸就停止查找了，这样可有效提升效率
DOUBLE FaceDatabase::Predict(Mat& mImg, string& strPersonName, FLOAT flConfidenceThreshold, FLOAT flStopPredictThreshold)
{
	if (mImg.empty())
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 3 << ", the parameter matImg is empty." << GetLastError() << endl;
		return 0;
	}

	//* 从图片中提取脸部区域
	Mat mFaceChips = ExtractFaceChips(mImg);	
	if (mFaceChips.empty())
	{
		//cout << "No face was detected." << endl;
		return 0;
	}

	//imshow("pic", matFaceChips);
	//cv::waitKey(60);

	return __Predict(this, mFaceChips, strPersonName, flConfidenceThreshold, flStopPredictThreshold);
}

DOUBLE FaceDatabase::Predict(const CHAR *pszImgName, string& strPersonName, FLOAT flConfidenceThreshold, FLOAT flStopPredictThreshold)
{
	Mat mImg = imread(pszImgName);
	if (mImg.empty())
	{
		cout << "Predict() error：unable to read the picture, please confirm that the picture『" << pszImgName << "』 exists and the format is corrrect." << endl;

		return 0;
	}

	return Predict(mImg, strPersonName, flConfidenceThreshold);
}

//* 使用DLIB提供68个特征点模型提取脸部特征
DOUBLE FaceDatabase::Predict(Mat& mImgGray, Face& objFace, string& strPersonName, FLOAT flConfidenceThreshold, FLOAT flStopPredictThreshold)
{
	if (mImgGray.empty())
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 3 << ", the parameter matImgGray is empty." << GetLastError() << endl;
		return 0;
	}

	Mat mFaceChips;
	__ExtractFaceFeatureChips(o_objShapePredictor, mImgGray, objFace, mFaceChips);

	DOUBLE dblConfidence = 0;
	if (!mFaceChips.empty())
		dblConfidence = __Predict(this, mFaceChips, strPersonName, flConfidenceThreshold, flStopPredictThreshold);

	return dblConfidence;
}

//* 视频预测模块
vector<ST_PERSON> FaceDatabase::VideoPredict::Predict(Mat& mVideoImg, FLOAT flConfidenceThreshold, FLOAT flStopPredictThreshold)
{
	vector<ST_PERSON> vPersons;

	Mat mGray;
	cvtColor(mVideoImg, mGray, CV_BGR2GRAY);

	//* 带特征点检测，这个函数要比DLib的特征点检测函数稍微快一些
	INT nLandmark = 1;
	INT *pnFaces = facedetect_multiview_reinforce(o_pubFeaceDetectResultBuf, (UCHAR*)(mGray.ptr(0)), mGray.cols, mGray.rows, (INT)mGray.step,
													o_flScale, o_nMinNeighbors, o_nMinPossibleFaceSize, 0, nLandmark);
	if (!pnFaces)
		return vPersons;

	for (INT i = 0; i < *pnFaces; i++)
	{
		SHORT *psScalar = ((SHORT*)(pnFaces + 1)) + LIBFACEDETECT_RESULT_STEP * i;		

		//* 人脸位置
		INT x = psScalar[0];
		INT y = psScalar[1];
		INT nWidth = psScalar[2];
		INT nHeight = psScalar[3];		

		//* 矩形框出人脸
		Point left(x, y);
		Point right(x + nWidth, y + nHeight);
		rectangle(mVideoImg, left, right, Scalar(0, 255, 0), 1);	
		
		Mat mFaceChips;
		__ExtractFaceFeatureChips(mGray, psScalar, mFaceChips);			

		//* 预测并标记
		INT nBaseLine = 0;
		String strPersonLabel;
		string strConfidenceLabel;
		Rect rect;		
		ST_PERSON stPerson;		
		stPerson.dblConfidence = __Predict(o_pobjFaceDB, mFaceChips, stPerson.strPersonName, flConfidenceThreshold, flStopPredictThreshold);
		if (stPerson.dblConfidence > flConfidenceThreshold)
		{
			vPersons.push_back(stPerson);

			//* 标出人名和可信度
			strPersonLabel = "Name: " + stPerson.strPersonName;
			strConfidenceLabel = "Confidence: " + static_cast<ostringstream*>(&(ostringstream() << stPerson.dblConfidence))->str();
			
			Size personLabelSize = getTextSize(strPersonLabel, FONT_HERSHEY_SIMPLEX, 0.5, 1, &nBaseLine);
			Size confidenceLabelSize = getTextSize(strConfidenceLabel, FONT_HERSHEY_SIMPLEX, 0.5, 1, &nBaseLine);
			rect = Rect(Point(x, y - (personLabelSize.height + confidenceLabelSize.height + 3)), 
							Size(personLabelSize.width > confidenceLabelSize.width ? personLabelSize.width : confidenceLabelSize.width, 
								personLabelSize.height + confidenceLabelSize.height + nBaseLine + 3));
			rectangle(mVideoImg, rect, Scalar(255, 255, 255), CV_FILLED);
			putText(mVideoImg, strPersonLabel, Point(x, y - confidenceLabelSize.height - 3), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(107, 194, 53));
			putText(mVideoImg, strConfidenceLabel, Point(x, y), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(107, 194, 53));
		}
		else
		{
			strPersonLabel = "No matching face was found";

			Size personLabelSize = getTextSize(strPersonLabel, FONT_HERSHEY_SIMPLEX, 0.5, 1, &nBaseLine);
			rect = Rect(Point(x, y - personLabelSize.height), Size(personLabelSize.width, personLabelSize.height + nBaseLine));

			rectangle(mVideoImg, rect, Scalar(255, 255, 255), CV_FILLED);
			putText(mVideoImg, strPersonLabel, Point(x, y), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(107, 194, 53));
		}
	}

	return vPersons;
}

//* 视频预测模块，该函数需要上层调用函数将检测到的脸部数据传递给它，设计该函数的目的是让用户选择快速的脸部检测函数，上面同名函数使用的检测函数很耗时
DOUBLE FaceDatabase::VideoPredict::Predict(Mat& mVideoImgGray, Face& objFace, dlib::shape_predictor& objShapePredictor,
											string& strPersonName, FLOAT flConfidenceThreshold, FLOAT flStopPredictThreshold)
{
	Mat mFaceChips;	
	__ExtractFaceFeatureChips(objShapePredictor, mVideoImgGray, objFace, mFaceChips);

	DOUBLE dblConfidence = 0;
	if (!mFaceChips.empty())
		dblConfidence = __Predict(o_pobjFaceDB, mFaceChips, strPersonName, flConfidenceThreshold, flStopPredictThreshold);

	return dblConfidence;
}

