// FaceRecognition.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <vector>
#include <dlib/image_processing.h>
#include <dlib/opencv.h>
#include <facedetect-dll.h>
#include "common_lib.h"
#include "MachineVisionLib.h"
#include "FaceRecognition.h"

BOOL FaceDatabase::LoadCaffeVGGNet(string strCaffePrototxtFile, string strCaffeModelFile)
{
	pcaflNet = caffe2shell::LoadNet<FLOAT>(strCaffePrototxtFile, strCaffeModelFile, caffe::TEST);

	if (pcaflNet)
	{
		pflMemDataLayer = (caffe::MemoryDataLayer<FLOAT> *)pcaflNet->layers()[0].get();
		return TRUE;
	}
	else
		return FALSE;
}

Mat FaceDatabase::ExtractFaceChips(Mat matImg, FLOAT flScale, INT nMinNeighbors, INT nMinPossibleFaceSize)
{
	Mat matDummy;

	INT *pnFaces = NULL;
	UCHAR *pubResultBuf = (UCHAR*)malloc(LIBFACEDETECT_BUFFER_SIZE);
	if (!pubResultBuf)
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 3 << ", malloc error code:" << GetLastError() << endl;
		return matDummy;
	}

	Mat matGray;
	cvtColor(matImg, matGray, CV_BGR2GRAY);

	//* 带特征点检测，这个函数要比DLib的特征点检测函数稍微快一些
	INT nLandmark = 1;
	pnFaces = facedetect_multiview_reinforce(pubResultBuf, (UCHAR*)(matGray.ptr(0)), matGray.cols, matGray.rows, (INT)matGray.step,
		flScale, nMinNeighbors, nMinPossibleFaceSize, 0, nLandmark);
	if (!pnFaces)
	{
		cout << "Error: No face was detected." << endl;
		return matDummy;
	}

	//* 单张图片只允许存在一个人脸
	if (*pnFaces != 1)
	{
		cout << "Error: Multiple faces were detected, and only one face was allowed." << endl;
		return matDummy;
	}

	//* 获取68个脸部特征点，脸部到鼻子:0-35, 眼睛:36-47,嘴形：48-60，嘴线：61-67
	SHORT *psScalar = ((SHORT*)(pnFaces + 1));
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

	dlib::array<dlib::array2d<dlib::rgb_pixel>> FaceChips;
	extract_image_chips(dlib::cv_image<uchar>(matGray), get_face_chip_details(shapes), FaceChips);
	Mat matFace = dlib::toMat(FaceChips[0]);

	//* 经过dlib处理后，matFace并不是灰度图像，需要转成灰度图像才可
	cvtColor(matFace, matFace, CV_BGR2GRAY);

	//* 按照网络要求调整为224,224大小
	resize(matFace, matFace, Size(224, 224));

	free(pubResultBuf);

	return matFace;
}

Mat FaceDatabase::ExtractFaceChips(const CHAR *pszImgName, FLOAT flScale, INT nMinNeighbors, INT nMinPossibleFaceSize)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "ExtractFaceChips() error：unable to read the picture, please confirm that the picture『" << pszImgName << "』 exists and the format is corrrect." << endl;

		return matImg;
	}

	return ExtractFaceChips(matImg, flScale, nMinNeighbors, nMinPossibleFaceSize);
}

//* 理论及公式，参见：
//* https://blog.csdn.net/wuzuyu365/article/details/51898714
//* 关于伽玛校正的理论资料：
//* https://blog.csdn.net/w450468524/article/details/51649651
//* 关于滤波核模板：
//* http://blog.sina.com.cn/s/blog_6ac784290101e47s.html
Mat FaceDatabase::FaceChipsHandle(Mat& matFaceChips, DOUBLE dblPowerValue, DOUBLE dblGamma, DOUBLE dblNorm)
{
	FLOAT *pflData;

	//* 矩阵数据转成浮点类型
	Mat matFloat;
	matFaceChips.convertTo(matFloat, CV_32F);
		
	Mat matTransit(Size(matFloat.cols, matFloat.rows), CV_32F, Scalar(0));	//* 计算用的过渡矩阵
	
	//* 不允许gamma值为0，也就是强制提升阴影区域的对比度
	if (fabs(dblGamma) < 1e-6)
		dblGamma = 0.2;
	for (INT i = 0; i < matFloat.rows; i++)
	{
		for (INT j = 0; j < matFloat.cols; j++)
			matFloat.ptr<FLOAT>(i)[j] = pow(matFloat.ptr<FLOAT>(i)[j], dblGamma);		
	}
	
	//* 高通滤波核，对比测试结果，目前这个核最好：
	//* 0.85-0.86 0.88-0.87 0.88-0.85 0.78-0.75
	FLOAT flaHighFilterKernel[9] = {-1, -1, -1, -1, 9, -1, -1, -1, -1}; 	
	Mat matFilterKernel = Mat(3, 3, CV_32FC1, flaHighFilterKernel);	
	filter2D(matFloat, matFloat, matFloat.depth(), matFilterKernel);

	//* 如果为0.0则直接跳到最后一步
	DOUBLE dblTrim = fabs(dblNorm);
	if (dblTrim < 1e-6)
		goto __lblEnd;
	
	//* matFloat矩阵指数运算并计算均指
	DOUBLE dblMeanVal = 0.0;
	matTransit = cv::abs(matFloat);
	for (INT i = 0; i<matFloat.rows; i++)
	{
		pflData = matTransit.ptr<FLOAT>(i);
		for (INT j = 0; j < matFloat.cols; j++)
		{
			pflData[j] = pow(matTransit.ptr<FLOAT>(i)[j], dblPowerValue);
			dblMeanVal += pflData[j];
		}
	}
	dblMeanVal /= (matFloat.rows * matFloat.cols);

	//* 求得的均值指数运算后与矩阵相除
	DOUBLE dblDivisor = cv::pow(dblMeanVal, 1/ dblPowerValue);
	for (INT i = 0; i < matFloat.rows; i++)
	{
		pflData = matFloat.ptr<FLOAT>(i);
		for (INT j = 0; j<matFloat.cols; j++)
			pflData[j] /= dblDivisor;
	}

	//* 减去超标的数值，然后再一次求均值
	dblMeanVal = 0.0;
	matTransit = cv::abs(matFloat);
	for (INT i = 0; i<matFloat.rows; i++)
	{
		pflData = matTransit.ptr<FLOAT>(i);
		for (INT j = 0; j < matFloat.cols; j++)
		{
			if (pflData[j] > dblTrim)
				pflData[j] = dblTrim;

			pflData[j] = pow(pflData[j], dblPowerValue);
			dblMeanVal += pflData[j];
		}
	}
	dblMeanVal /= (matFloat.rows * matFloat.cols);

	//* 再一次将均值进行指数计算后与矩阵相除
	dblDivisor = cv::pow(dblMeanVal, 1 / dblPowerValue);
	for (INT i = 0; i < matFloat.rows; i++)
	{
		pflData = matFloat.ptr<FLOAT>(i);
		for (INT j = 0; j<matFloat.cols; j++)
			pflData[j] /= dblDivisor;
	}

	//* 如果大于0.0
	if(dblNorm > 1e-6)
	{ 
		for (INT i = 0; i<matFloat.rows; i++)
		{
			pflData = matFloat.ptr<FLOAT>(i);
			for (INT j = 0; j<matFloat.cols; j++)
				pflData[j] = dblTrim * tanh(pflData[j] / dblTrim);
		}
	}

__lblEnd:
	//* 找出矩阵的最小值
	FLOAT flMin = matFloat.ptr<FLOAT>(0)[0];
	for (INT i = 0; i <matFloat.rows; i++)
	{
		pflData = matFloat.ptr<FLOAT>(i);
		for (INT j = 0; j < matFloat.cols; j++)
		{
			if (pflData[j] < flMin)
				flMin = pflData[j];
		}
	}

	//* 矩阵加最小值
	for (INT i = 0; i <matFloat.rows; i++)
	{
		pflData = matFloat.ptr<FLOAT>(i);
		for (INT j = 0; j < matFloat.cols; j++)	
				pflData[j] += flMin;
	}

	//* 归一化
	normalize(matFloat, matFloat, 0, 255, NORM_MINMAX);
	matFloat.convertTo(matFloat, CV_8UC1);

	//* 进入caffe提取特征之前，必须为RGB格式才可，否则caffe会报错
	cvtColor(matFloat, matFloat, CV_GRAY2RGB);

	imshow("FaceChipsHandle", matFloat);
	waitKey(600);

	//* 验证代码，正常逻辑不需要
	//FileStorage fs("mat.xml", FileStorage::WRITE);
	//fs << "MAT-DATA" << matFloat;	
	//fs.release();

	return matFloat;
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

BOOL FaceDatabase::AddFace(Mat& matImg, const string& strPersonName)
{
	if (matImg.empty())
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
	Mat matFaceChips = ExtractFaceChips(matImg);
	if (matFaceChips.empty())
	{
		cout << "No face was detected." << endl;
		return FALSE;
	}

	//imshow("pic", matFaceChips);
	//cv::waitKey(60);

	//* ROI(region of interest)，按照一定算法将脸部区域转换为caffe网络特征提取需要的输入数据
	Mat matFaceROI = FaceChipsHandle(matFaceChips);

	//* 通过caffe网络提取图像特征
	Mat matImgFeature(1, FACE_FEATURE_DIMENSION, CV_32F);
	caffe2shell::ExtractFeature(pcaflNet, pflMemDataLayer, matFaceROI, matImgFeature, FACE_FEATURE_DIMENSION, FACE_FEATURE_LAYER_NAME);
	
	//* 将特征数据存入文件
	string strXMLFile = string(FACE_DB_PATH) + "/" + strPersonName + ".xml";
	FileStorage fs(strXMLFile, FileStorage::WRITE);
	fs << "VGG-FACEFEATURE" << matImgFeature;
	fs.release();

	//* 保存人数和人名字节数，用于人脸数据库加载
	UpdateFaceDBStatisticFile(strPersonName);

	return TRUE;
}

BOOL FaceDatabase::AddFace(const CHAR *pszImgName, const string& strPersonName)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "AddFace() error：unable to read the picture, please confirm that the picture『" << pszImgName << "』 exists and the format is corrrect." << endl;

		return FALSE;
	}

	return AddFace(matImg, strPersonName);
}

//* 将人脸数据从文件中取出并放入指定内存
static BOOL __PutFaceDataFromFile(const string strFaceDataFile, FLOAT *pflFaceData)
{
	Mat matFaceFeatureData;

	FileStorage fs;
	if (fs.open(strFaceDataFile, FileStorage::READ))
	{
		fs["VGG-FACEFEATURE"] >> matFaceFeatureData;

		fs.release();

		for (INT i=0; i < FACE_FEATURE_DIMENSION; i++)
			pflFaceData[i] = matFaceFeatureData.at<FLOAT>(0, i);

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
	nActualNumOfPerson = 0;
	while ((unNameLen = CLIBReadDir(hDir, strFileName)) > 0)
	{
		//* 写入特征数据
		string strFaceDataFile = string(FACE_DB_PATH) + "/" + strFileName;
		if (!__PutFaceDataFromFile(strFaceDataFile, ((FLOAT *)stMemFileFaceData.pvMem) + nActualNumOfPerson * FACE_FEATURE_DIMENSION))
			continue;		
		nActualNumOfPerson++;

		//* 写入文件名
		size_t sztEndPos = strFileName.rfind(".xml");		
		string strPersonName = strFileName.substr(0, sztEndPos);
		sprintf(((CHAR*)stMemFilePersonName.pvMem) + unWriteBytes, "%s", strPersonName.c_str());
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
	if(!common_lib::CreateMemFile(&stMemFileFaceData, stInfo.nPersonNum * FACE_FEATURE_DIMENSION * sizeof(FLOAT)))
	{ 
		return FALSE;
	}

	//* 为人名建立内存文件
	if (!common_lib::CreateMemFile(&stMemFilePersonName, stInfo.nTotalLenOfPersonName))
	{
		common_lib::DeletMemFile(&stMemFileFaceData);

		return FALSE;
	}
	memset((CHAR*)stMemFilePersonName.pvMem, 0, stInfo.nTotalLenOfPersonName);

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

//* 返回相似度，参数strPersonName接收找到的人名，flConfidenceThreshold指定最低可信度值，低于这个值就认为不是这个人
//* flStopPredictThreshold指定停止查找的阈值，因为函数会检索整个数据库以找到最大相似度的人脸，有了这个值函数只要找到大于这个
//* 值的人脸就停止查找了，这样可有效提升效率
DOUBLE FaceDatabase::Predict(Mat& matImg, string& strPersonName, FLOAT flConfidenceThreshold, FLOAT flStopPredictThreshold)
{
	if (matImg.empty())
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 3 << ", the parameter matImg is empty." << GetLastError() << endl;
		return 0;
	}

	//* 从图片中提取脸部区域
	Mat matFaceChips = ExtractFaceChips(matImg);
	if (matFaceChips.empty())
	{
		cout << "No face was detected." << endl;
		return 0;
	}

	//imshow("pic", matFaceChips);
	//cv::waitKey(60);

	//* ROI(region of interest)，按照一定算法将脸部区域转换为caffe网络特征提取需要的输入数据
	Mat matFaceROI = FaceChipsHandle(matFaceChips);

	//* 通过caffe网络提取图像特征
	FLOAT flaFaceFeature[FACE_FEATURE_DIMENSION];
	caffe2shell::ExtractFeature(pcaflNet, pflMemDataLayer, matFaceROI, flaFaceFeature, FACE_FEATURE_DIMENSION, FACE_FEATURE_LAYER_NAME);

	//* 查找匹配的脸部特征
	const CHAR *pszPerson = (const CHAR *)stMemFilePersonName.pvMem;
	const FLOAT *pflaData = (FLOAT*)stMemFileFaceData.pvMem;
	DOUBLE dblMaxSimilarity = flConfidenceThreshold, dblSimilarity;
	const CHAR *pszMatchPersonName = NULL;
	for (INT i = 0; i < nActualNumOfPerson; i++)
	{		
		dblSimilarity = cv2shell::CosineSimilarity(pflaData + i * FACE_FEATURE_DIMENSION, flaFaceFeature, FACE_FEATURE_DIMENSION);
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

		cout << pszPerson << ":" << dblSimilarity << endl;
		
		//* 下一个
		pszPerson += strlen(pszPerson) + 1;
	}

	//* 超过了用户容忍的最小相似度值，则可以认为找到了
	if (dblMaxSimilarity > flConfidenceThreshold)	
		strPersonName = pszMatchPersonName;

	return dblMaxSimilarity;
}

DOUBLE FaceDatabase::Predict(const CHAR *pszImgName, string& strPersonName, FLOAT flConfidenceThreshold, FLOAT flStopPredictThreshold)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "Predict() error：unable to read the picture, please confirm that the picture『" << pszImgName << "』 exists and the format is corrrect." << endl;

		return FALSE;
	}

	return Predict(matImg, strPersonName, flConfidenceThreshold);
}

