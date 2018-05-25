// MachineVisionLib.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <io.h>
#include <vector>

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/opencv/cv_image.h>
#include <dlib/opencv.h>

#include <facedetect-dll.h>
#include "common_lib.h"
#include "MachineVisionLib.h"

#if NEED_GPU
#pragma comment(lib,"cublas.lib")
#pragma comment(lib,"cuda.lib")
#pragma comment(lib,"cudart.lib")
#pragma comment(lib,"curand.lib")
#pragma comment(lib,"cudnn.lib")
#endif

//* 执行Canny边缘检测
MACHINEVISIONLIB_API void cv2shell::CV2Canny(Mat &matSrc, Mat &matOut)
{
	Mat matGray, matBlur;

	//* 灰化处理
	cvtColor(matSrc, matGray, COLOR_BGR2GRAY);

	//* 模糊处理
	blur(matGray, matBlur, Size(4, 4));

	//* 边缘
	Canny(matBlur, matOut, 0, 60, 3);
}

//* 执行Canny边缘检测
MACHINEVISIONLIB_API void cv2shell::CV2Canny(const CHAR *pszImgName, Mat &matOut)
{
	Mat matImg = imread(pszImgName);
	if (!matImg.empty())
	{
		CV2Canny(matImg, matOut);
	}
}

//* 播放一段网路摄像头实时视频，参数pszNetURL用于指定实时视频播放地址，比如：
//* rtsp://admin:abcd1234@192.168.0.250/mjpeg/ch1
MACHINEVISIONLIB_API void cv2shell::CV2ShowVideoFromNetCamera(const CHAR *pszNetURL)
{
	Mat mSrc;
	VideoCapture video;
	BOOL blIsNotOpen = TRUE;

	while (true)
	{
		if (blIsNotOpen)
		{
			if (video.open(pszNetURL))
				blIsNotOpen = FALSE;
			else
			{
				Sleep(1000);
				continue;
			}
		}

		if (video.read(mSrc))
		{
			imshow(pszNetURL, mSrc);
			if (waitKey(40) >= 0)
				break;
		}
		else
		{
			blIsNotOpen = TRUE;
			video.release();
			continue;
		}
	}
}

//* 获取网络摄像头的实时视频数据，并允许用户通过回调函数形式处理收到的实时视频数据，调用例子如下：
//* CV2ShowVideoFromNetCamera("rtsp://admin:abcd1234@192.168.0.250/mjpeg/ch1", CLSCV2Shell::CV2Canny);
MACHINEVISIONLIB_API void cv2shell::CV2ShowVideoFromNetCamera(const CHAR *pszNetURL, PCB_NETVIDEOHANDLER pfunNetVideoHandler)
{
	Mat mSrc, mHandleData;
	VideoCapture video;

	bool blIsNotOpen = TRUE;

	while (TRUE)
	{
		if (blIsNotOpen)
		{
			if (video.open(pszNetURL))
				blIsNotOpen = FALSE;
			else
			{
				Sleep(1000);
				continue;
			}
		}

		if (video.read(mSrc))
		{
			mHandleData = mSrc;
			if (NULL != pfunNetVideoHandler)
			{
				mHandleData = pfunNetVideoHandler(mSrc);
			}

			imshow(pszNetURL, mHandleData);

			if (waitKey(40) >= 0)
				break;
		}
		else
		{
			blIsNotOpen = TRUE;
			video.release();
			continue;
		}
	}
}

//* 建立模拟alpha模拟图像，该函数的例子如下：
/*
Mat mat(480, 640, CV_8UC4);	//* 带alpha通道的mat

CV2CreateAlphaMat(mat);

vector<int> compression_params;
compression_params.push_back(IMWRITE_PNG_COMPRESSION);
compression_params.push_back(0);

try {
imwrite("alpha_test.png", mat, compression_params);
imshow("alpha_test.png", mat);
waitKey(0);
} catch (runtime_error &ex) {
cout << "图像转换成PNG格式出错，错误原因：" << ex.what() << endl;
}
*/
MACHINEVISIONLIB_API void cv2shell::CV2CreateAlphaMat(Mat &mat)
{
	INT i, j;

	for (i = 0; i < mat.rows; i++)
	{
		for (j = 0; j < mat.cols; j++)
		{
			Vec4b &rgba = mat.at<Vec4b>(i, j);
			rgba[0] = UCHAR_MAX;	//* 蓝色分量
			rgba[1] = saturate_cast<uchar>((FLOAT(mat.cols - j)) / ((FLOAT)mat.cols) * UCHAR_MAX);	//* 绿色分量,saturate_cast的作用防止后面的算法出现小于0或者大于255的数(uchar不能大于255)
			rgba[2] = saturate_cast<uchar>((FLOAT(mat.rows - j)) / ((FLOAT)mat.rows) * UCHAR_MAX);	//* 红色分量
			rgba[3] = saturate_cast<uchar>(0.5 * (rgba[1] + rgba[2]));	//* alpha分量，透明度
		}
	}
}

//* 均值Hash算法
MACHINEVISIONLIB_API string cv2shell::CV2HashValue(Mat &matSrc, PST_IMG_RESIZE pstResize)
{
	string strValues(pstResize->nRows * pstResize->nCols, '\0');

	Mat matImg;
	if (matSrc.channels() == 3)
		cvtColor(matSrc, matImg, CV_BGR2GRAY);
	else
		matImg = matSrc.clone();

	//* 第一步，缩小尺寸。将图片缩小到不能被8整出且大于8的尺寸，以去除图片的细节（长宽互换正好压缩掉门打开等细节）
	resize(matImg, matImg, Size(pstResize->nRows, pstResize->nCols));
	//resize(matImg, matImg, Size(pstResize->nCols * 10, pstResize->nRows * 10));
	//imshow("ce", matImg);
	//waitKey(0);

	//* 第二步，简化色彩(Color Reduce)。将缩小后的图片，转为64级灰度。
	UCHAR *pubData;
	for (INT i = 0; i<matImg.rows; i++)
	{
		pubData = matImg.ptr<UCHAR>(i);
		for (INT j = 0; j<matImg.cols; j++)
		{
			pubData[j] = pubData[j] / 4;
		}
	}

	//imshow("『缩小色彩』", matImg);

	//* 第三步，计算平均值。计算所有像素的灰度平均值。
	INT nAverage = (INT)mean(matImg).val[0];

	//* 第四步，比较像素的灰度。将每个像素的灰度，与平均值进行比较。大于或等于平均值记为1,小于平均值记为0
	//* C++的神奇之处，竟然可以这样操作
	Mat matMask = (matImg >= (UCHAR)nAverage);

	//* 第五步，计算哈希值，其实就是将16进制的0和1转成ASCII的‘0’和‘1’，即0x00->0x30、0x01->0x31，将其存储到一个线性字符串中
	INT nIdx = 0;
	for (INT i = 0; i<matMask.rows; i++)
	{
		pubData = matMask.ptr<UCHAR>(i);
		for (INT j = 0; j<matMask.cols; j++)
		{
			if (pubData[j] == 0)
				strValues[nIdx++] = '0';
			else
				strValues[nIdx++] = '1';
		}
	}

	matImg.release();	//* 其实不释放也可以，OpenCV会根据引用计数主动释放的，说白了是C++的内存管理机制释放的

	return strValues;
}

//* 上一个CV2HashValue函数的重载函数
MACHINEVISIONLIB_API string cv2shell::CV2HashValue(const CHAR *pszImgName, PST_IMG_RESIZE pstResize)
{
	string strDuff;

	Mat matImg = imread(pszImgName);
	if (matImg.data == NULL)
		return strDuff;

	return CV2HashValue(matImg, pstResize);
}

//* 计算等比例（步长为8）缩小图像尺寸至其中一维（2D图片的长宽两维）不能被8整出且大于8的最小图片尺寸，比如：
//* 72 x 128 -> 9 x 16
static void __GetResizeValue(PST_IMG_RESIZE pstResize)
{
	if (pstResize->nRows > 64 && pstResize->nCols > 64)
	{
		if (pstResize->nRows % 8 == 0 && pstResize->nCols % 8 == 0)
		{
			pstResize->nRows /= 8;
			pstResize->nCols /= 8;

			__GetResizeValue(pstResize);
		}
	}
}

//* 根据哈希计算的要求，需要将图片缩小以去除图片细节，该函数将计算按比例缩小至不能被8整出的最小图片尺寸
MACHINEVISIONLIB_API ST_IMG_RESIZE cv2shell::CV2GetResizeValue(Mat &matImg)
{
	ST_IMG_RESIZE stSize;

	stSize.nRows = EatZeroOfTheNumberTail(matImg.rows);
	stSize.nCols = EatZeroOfTheNumberTail(matImg.cols);

	__GetResizeValue(&stSize);

	return stSize;
}

//* 按照指定尺寸调整等比例调整图像大小，对于长宽不一致但要求调整为长宽一致的图片，函数会为短边不上单一像素的边以使其等长
MACHINEVISIONLIB_API void cv2shell::ImgEquilateral(Mat &matImg, Mat &matResizeImg, INT nResizeLen, const Scalar &border_color)
{
	INT nTop = 0, nBottom = 0, nLeft = 0, nRight = 0, nDifference;
	Mat matBorderImg;

	INT nLongestEdge = max(matImg.rows, matImg.cols);

	if (matImg.rows < nLongestEdge)
	{
		nDifference = nLongestEdge - matImg.rows;
		nTop = nDifference / 2;
		nBottom = nDifference - nTop;
	}
	else if (matImg.cols < nLongestEdge)
	{
		nDifference = nLongestEdge - matImg.cols;
		nLeft = nDifference / 2;
		nRight = nDifference - nLeft;
	}
	else;

	//* 给图像增加边界，是图片长、宽等长，cv2.BORDER_CONSTANT指定边界颜色由value指定
	copyMakeBorder(matImg, matBorderImg, nTop, nBottom, nLeft, nRight, BORDER_CONSTANT, border_color);

	Size size(nResizeLen, nResizeLen);

	if (nResizeLen < nLongestEdge)
		resize(matBorderImg, matResizeImg, size, INTER_AREA);
	else
		resize(matBorderImg, matResizeImg, size, INTER_CUBIC);
}

//* 将图片尺寸调整为等边图片，其实就是短边填充指定颜色的像素使其与长边等长
MACHINEVISIONLIB_API void cv2shell::ImgEquilateral(Mat &matImg, Mat &matResizeImg, const Scalar &border_color)
{
	INT nTop = 0, nBottom = 0, nLeft = 0, nRight = 0, nDifference;

	INT nLongestEdge = max(matImg.rows, matImg.cols);

	if (matImg.rows < nLongestEdge)
	{
		nDifference = nLongestEdge - matImg.rows;
		nTop = nDifference / 2;
		nBottom = nDifference - nTop;
	}
	else if (matImg.cols < nLongestEdge)
	{
		nDifference = nLongestEdge - matImg.cols;
		nLeft = nDifference / 2;
		nRight = nDifference - nLeft;
	}
	else;

	//* 给图像增加边界，是图片长、宽等长，cv2.BORDER_CONSTANT指定边界颜色由value指定
	copyMakeBorder(matImg, matResizeImg, nTop, nBottom, nLeft, nRight, BORDER_CONSTANT, border_color);
}

//* 根据哈希计算的要求，需要将图片缩小以去除图片细节，该函数将计算按比例缩小至不能被8整出的最小图片尺寸
MACHINEVISIONLIB_API ST_IMG_RESIZE cv2shell::CV2GetResizeValue(const CHAR *pszImgName)
{
	Mat matImg = imread(pszImgName);
	if (matImg.data == NULL)
		return{ 0, 0 };

	return CV2GetResizeValue(matImg);
}

ImgMatcher::ImgMatcher(INT nSetMatchThresholdValue)
{
	nMatchThresholdValue = nSetMatchThresholdValue;
}

//汉明距离计算
static int __HanmingDistance(string &str1, string &str2, PST_IMG_RESIZE pstResize)
{
	INT nPixelNum = pstResize->nRows * pstResize->nCols;

	if ((str1.size() != nPixelNum) || (str2.size() != nPixelNum))
		return -1;

	INT nDifference = 0;
	for (INT i = 0; i<nPixelNum; i++)
	{
		if (str1[i] != str2[i])
			nDifference++;
	}

	return nDifference;
}

BOOL ImgMatcher::InitImgMatcher(vector<string *> &vModelImgs)
{
	//* 先获取图片尺寸
	string *pstrImgName = vModelImgs.at(0);
	stImgResize = cv2shell::CV2GetResizeValue(pstrImgName->c_str());

	//* 逐个取出所有图片
	vector<string*>::iterator itImgName = vModelImgs.begin();
	for (; itImgName != vModelImgs.end(); itImgName++)
	{
		string strHashValue;

		//* 计算图片的哈希值
		strHashValue = cv2shell::CV2HashValue(((string)**itImgName).c_str(), &stImgResize);
		if (strHashValue.empty())
		{
			//* 释放掉之前申请的string
			vector<string *>::iterator itHashValues = vModelImgHashValues.begin();
			for (; itHashValues != vModelImgHashValues.end(); itHashValues++)
				delete *itHashValues;

			return FALSE;
		}

		string *pstrHashValue = new string;
		*pstrHashValue = strHashValue;
		vModelImgHashValues.push_back(pstrHashValue);

		//* 直接取值，如果是一个*则取到的是保存地，也就是pstrImgName，只有再一个*才能是string
		//cout << **itImgName << endl;

		delete *itImgName;	//* 删除时这里必须是地址
	}

	return TRUE;
}

BOOL ImgMatcher::InitImgMatcher(const CHAR *pszModelImg)
{
	vector<string *> vModelImgs;
	string *pstrImgName = new string(pszModelImg);

	vModelImgs.push_back(pstrImgName);

	return InitImgMatcher(vModelImgs);
}

void ImgMatcher::UninitImgMatcher(void)
{
	//* 释放掉之前申请的string
	vector<string *>::iterator itHashValues = vModelImgHashValues.begin();
	for (; itHashValues != vModelImgHashValues.end(); itHashValues++)
		delete *itHashValues;
}

//* 根据模板图片计算图片相似度，只要与其中一个模板图片的汉明距离小于阈值则认为图片相似，返回值如下：
//* 0, 图片相似度很小
//* 1, 图片相似度很大
INT ImgMatcher::ImgSimilarity(Mat &matImg)
{
	//* 计算图片的哈希值
	string strHashValue = cv2shell::CV2HashValue(matImg, &stImgResize);

	vector<string *>::iterator itHashValues = vModelImgHashValues.begin();
	for (; itHashValues != vModelImgHashValues.end(); itHashValues++)
	{
		INT nDistance = __HanmingDistance(**itHashValues, strHashValue, &stImgResize);
		if (nDistance < nMatchThresholdValue)
			return 1;
	}

	return 0;
}

//* 根据模板图片计算图片相似度，只要与其中一个模板图片的汉明距离小于阈值则认为图片相似，返回值如下：
//* 0,  图片相似度太小
//* 1,  图片相似度很大
//* -1，目标图片打开错误
INT ImgMatcher::ImgSimilarity(const CHAR *pszImgName)
{
	Mat matImg = imread(pszImgName);
	if (matImg.data == NULL)
		return -1;

	return ImgSimilarity(matImg);
}

//* 根据模板图片计算图片相似度，只要与其中一个模板图片的汉明距离小于阈值则认为图片相似，返回值如下：
//* 0, 图片相似度很小
//* 1, 图片相似度很大
//* 参数pnDistance会把与模板图片汉明距离最大的那个值输出出来
INT ImgMatcher::ImgSimilarity(Mat &matImg, INT *pnDistance)
{
	INT nMaxDistance = 0, nRtnVal = 0;

	//* 计算图片的哈希值
	string strHashValue = cv2shell::CV2HashValue(matImg, &stImgResize);

	vector<string *>::iterator itHashValues = vModelImgHashValues.begin();
	for (; itHashValues != vModelImgHashValues.end(); itHashValues++)
	{
		INT nDistance = __HanmingDistance(**itHashValues, strHashValue, &stImgResize);
		if (nDistance > nMaxDistance)
			nMaxDistance = nDistance;

		if (nDistance < nMatchThresholdValue)
		{
			nMaxDistance = nDistance;
			nRtnVal = 1;
			goto __lblEnd;
		}
	}

__lblEnd:
	*pnDistance = nMaxDistance;

	return nRtnVal;
}

INT ImgMatcher::ImgSimilarity(const CHAR *pszImgName, INT *pnDistance)
{
	Mat matImg = imread(pszImgName);
	if (matImg.data == NULL)
		return -1;

	return ImgSimilarity(matImg, pnDistance);
}

//* 利用Shiqi Yu开发的开源人脸检测库实现的人脸检测函数
//* 检测出人脸，入口参数matImg是已经读入的被检测的图片数据，出口参数是找到的结果集
//* 注意出口参数指向的缓冲区是malloc得到的，上层调用函数必须主动释放才可，该缓冲
//* 区第一个INT字节为检测到的人脸数量，这之后就是具体的人脸在图片中的坐标位置
//* flScale:              缩放因子，其实就是人脸检测算法会用不同大小的窗口进行扫
//*                       描，每两个扫描窗口之间的缩放倍数，倍数越小，检测速度越
//*                       慢，当然精度会越高
//*
//* nMinNeighbors:        有多少个扫描窗口检测到了人脸，少于nMinNeighbors，则认为
//*                       不是人脸
//* 
//* nMinPossibleFaceSize: 人脸最小可能的尺寸，换言之就是人脸检测算法寻找人脸的最小区域
MACHINEVISIONLIB_API INT *cv2shell::FaceDetect(Mat &matImg, FLOAT flScale, INT nMinNeighbors, INT nMinPossibleFaceSize)
{
	UCHAR *pubResultBuf = (UCHAR*)malloc(LIBFACEDETECT_BUFFER_SIZE);
	if (!pubResultBuf)
	{
		cout << "cv2shell::FaceDetect()错误：" << GetLastError() << endl;
		return NULL;
	}

	Mat matGrayImg;
	cvtColor(matImg, matGrayImg, CV_BGR2GRAY);

	INT *pnResult = NULL;
	pnResult = facedetect_multiview_reinforce(pubResultBuf, (unsigned char*)(matGrayImg.ptr(0)),
		matGrayImg.cols, matGrayImg.rows, matGrayImg.step,
		flScale, nMinNeighbors, nMinPossibleFaceSize);
	if (pnResult && *pnResult > 0)
		return pnResult;

	free(pubResultBuf);

	return NULL;
}

MACHINEVISIONLIB_API INT *cv2shell::FaceDetect(const CHAR *pszImgName, FLOAT flScale, INT nMinNeighbors, INT nMinPossibleFaceSize)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "cv2shell::FaceDetect()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		return NULL;
	}

	return FaceDetect(matImg, flScale, nMinNeighbors, nMinPossibleFaceSize);
}

//* 用矩形框标记出人脸
MACHINEVISIONLIB_API void cv2shell::MarkFaceWithRectangle(Mat &matImg, INT *pnFaces)
{
	for (INT i = 0; i < *pnFaces; i++)
	{
		SHORT *psScalar = ((SHORT*)(pnFaces + 1)) + LIBFACEDETECT_RESULT_STEP * i;
		INT x = psScalar[0];
		INT y = psScalar[1];
		INT nWidth = psScalar[2];
		INT nHeight = psScalar[3];
		INT nNeighbors = psScalar[4];

		Point left(x, y);
		Point right(x + nWidth, y + nHeight);
		rectangle(matImg, left, right, Scalar(230, 255, 0), 1);
	}

	free(pnFaces);
}

MACHINEVISIONLIB_API void cv2shell::MarkFaceWithRectangle(const CHAR *pszImgName, FLOAT flScale, INT nMinNeighbors, INT nMinPossibleFaceSize)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "cv2shell::MarkFaceWithRectangle()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;
		return;
	}

	INT *pnFaces = FaceDetect(matImg, flScale, nMinNeighbors, nMinPossibleFaceSize);
	if (!pnFaces)
		return;

	MarkFaceWithRectangle(matImg, pnFaces);

	imshow("Face Detect Result", matImg);
}

//* 初始化用于人脸检测的DNN网络
MACHINEVISIONLIB_API Net cv2shell::InitFaceDetectDNNet(void)
{
	String strModelCfgFile = "C:\\Windows\\System32\\models\\resnet\\deploy.prototxt";
	String strModelFile = "C:\\Windows\\System32\\models\\resnet\\res10_300x300_ssd_iter_140000.caffemodel";

	dnn::Net dnnNet = readNetFromCaffe(strModelCfgFile, strModelFile);
	if (dnnNet.empty())
		cout << "DNN网络建立失败，请确认输入的网络配置文件『deploy.prototxt』和模型文件『resnet_ssd.caffemodel』存在且格式正确" << endl;

	return dnnNet;
}

//* 使用DNN网络已训练好的模型开发的人脸检测函数
MACHINEVISIONLIB_API Mat cv2shell::FaceDetect(Net &dnnNet, Mat &matImg, const Size &size, FLOAT flScale, const Scalar &mean)
{
	//* 加载图片文件并归一化，size参数指定图片要缩放的目标尺寸，mena指定要减去的平均值的平均标量（红蓝绿三个颜色通道都要减）
	Mat matInputBlob = blobFromImage(matImg, flScale, size, mean, false, false);

	//* 设置网络输入
	dnnNet.setInput(matInputBlob, "data");

	//* 计算网络输出
	Mat matDetection = dnnNet.forward("detection_out");

	Mat matFaces(matDetection.size[2], matDetection.size[3], CV_32F, matDetection.ptr<float>());

	return matFaces;
}

//* 将图片等比例缩小至指定像素
static Size __ResizeImgToSpecPixel(Mat &matImg, INT nMinPixel)
{
	INT *pnMin, *pnMax, nRows, nCols;

	//* 已经小于指定的最小值了，那就没必要再缩减了
	if (matImg.rows <= nMinPixel || matImg.cols <= nMinPixel)
		return Size(matImg.cols, matImg.rows);

	//* 相等则直接返回指定的最小值就可以了
	if (matImg.rows == matImg.cols)
		return Size(nMinPixel, nMinPixel);

	nRows = matImg.rows;
	nCols = matImg.cols;

	pnMin = &nCols;
	pnMax = &nRows;

	if (matImg.rows < matImg.cols)
	{
		pnMin = &nRows;
		pnMax = &nCols;
	}

	if (nMinPixel < 260)
		nMinPixel = 260;

	FLOAT flScale = ((FLOAT)nMinPixel) / ((FLOAT)(*pnMin));
	*pnMax = (INT)(((FLOAT)*pnMax) * flScale);
	*pnMin = nMinPixel;

	return Size(nCols, nRows);
}

//* 该函数可以自动等比例resize图片尺寸到300像素左右，以确保最佳识别效果
MACHINEVISIONLIB_API Mat cv2shell::FaceDetect(Net &dnnNet, Mat &matImg, ENUM_IMGRESIZE_METHOD enumMethod, FLOAT flScale, const Scalar &mean)
{
	if (enumMethod == EIRSZM_EQUALRATIO)
	{
		Size size = __ResizeImgToSpecPixel(matImg, 288); //* 实测288效果最好

		return FaceDetect(dnnNet, matImg, size, flScale, mean);
	}
	else
	{
		Mat matEquilateralImg;

		ImgEquilateral(matImg, matEquilateralImg);
		//namedWindow("Equilateral", 0);
		//imshow("Equilateral", matEquilateralImg);	
		//imwrite("D:\\work\\OpenCV\\resize_test.jpg", matEquilateralImg);

		return FaceDetect(dnnNet, matEquilateralImg, Size(300, 300), flScale, mean);
	}
}

MACHINEVISIONLIB_API Mat cv2shell::FaceDetect(Net &dnnNet, const CHAR *pszImgName, ENUM_IMGRESIZE_METHOD enumMethod, FLOAT flScale, const Scalar &mean)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "cv2shell::FaceDetect()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		Mat mat;

		return mat;
	}

	return FaceDetect(dnnNet, matImg, enumMethod, flScale, mean);
}

MACHINEVISIONLIB_API Mat cv2shell::FaceDetect(Net &dnnNet, const CHAR *pszImgName, const Size &size, FLOAT flScale, const Scalar &mean)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "cv2shell::FaceDetect()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		Mat mat;

		return mat;
	}

	return FaceDetect(dnnNet, matImg, size, flScale, mean);
}

//* 参数vFaces用于保存检测到的人脸信息，是一个输出参数
MACHINEVISIONLIB_API void cv2shell::FaceDetect(Net &dnnNet, Mat &matImg, vector<Face> &vFaces, const Size &size,
	FLOAT flConfidenceThreshold, FLOAT flScale, const Scalar &mean)
{
	//* 加载图片文件并归一化，size参数指定图片要缩放的目标尺寸，mena指定要减去的平均值的平均标量（红蓝绿三个颜色通道都要减）
	Mat matInputBlob = blobFromImage(matImg, flScale, size, mean, false, false);

	//* 设置网络输入
	dnnNet.setInput(matInputBlob, "data");

	//* 计算网络输出
	Mat matDetection = dnnNet.forward("detection_out");

	Mat matFaces(matDetection.size[2], matDetection.size[3], CV_32F, matDetection.ptr<float>());

	for (int i = 0; i < matFaces.rows; i++)
	{
		FLOAT flConfidenceVal = matFaces.at<FLOAT>(i, 2);
		if (flConfidenceVal < flConfidenceThreshold)
			continue;

		Face face;

		face.xLeftBottom = static_cast<INT>(matFaces.at<FLOAT>(i, 3) * matImg.cols);
		face.yLeftBottom = static_cast<INT>(matFaces.at<FLOAT>(i, 4) * matImg.rows);
		face.xRightTop = static_cast<INT>(matFaces.at<FLOAT>(i, 5) * matImg.cols);
		face.yRightTop = static_cast<INT>(matFaces.at<FLOAT>(i, 6) * matImg.rows);

		face.flConfidenceVal = flConfidenceVal;
		vFaces.push_back(face);
	}
}

MACHINEVISIONLIB_API void cv2shell::FaceDetect(Net &dnnNet, Mat &matImg, vector<Face> &vFaces, FLOAT flConfidenceThreshold,
												ENUM_IMGRESIZE_METHOD enumMethod, FLOAT flScale, const Scalar &mean)
{
	if (enumMethod == EIRSZM_EQUALRATIO)
	{
		Size size = __ResizeImgToSpecPixel(matImg, 288); //* 实测288效果最好

		FaceDetect(dnnNet, matImg, vFaces, size, flConfidenceThreshold, flScale, mean);
	}
	else
	{
		Mat matEquilateralImg;

		ImgEquilateral(matImg, matEquilateralImg);
		//namedWindow("Equilateral", 0);
		//imshow("Equilateral", matEquilateralImg);	
		//imwrite("D:\\work\\OpenCV\\resize_test.jpg", matEquilateralImg);

		FaceDetect(dnnNet, matEquilateralImg, vFaces, Size(300, 300), flConfidenceThreshold, flScale, mean);
	}
}

MACHINEVISIONLIB_API void cv2shell::FaceDetect(Net &dnnNet, const CHAR *pszImgName, vector<Face> &vFaces,
												FLOAT flConfidenceThreshold, ENUM_IMGRESIZE_METHOD enumMethod, 
												FLOAT flScale, const Scalar &mean)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "cv2shell::FaceDetect()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		return;
	}

	FaceDetect(dnnNet, matImg, vFaces, flConfidenceThreshold, enumMethod, flScale, mean);
}

MACHINEVISIONLIB_API void cv2shell::FaceDetect(Net &dnnNet, const CHAR *pszImgName, vector<Face> &vFaces, const Size &size,
	FLOAT flConfidenceThreshold, FLOAT flScale, const Scalar &mean)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "cv2shell::FaceDetect()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		return;
	}

	FaceDetect(dnnNet, matImg, vFaces, size, flConfidenceThreshold, flScale, mean);
}

//* 将测结果在图片上展示出来：用矩形框标记出人脸并输出预测概率
//* 参数flConfidenceThreshold指定最小置信度阈值，也就是预测是人脸的最小概率值，大于此概率的被认作是人脸并标记
MACHINEVISIONLIB_API void cv2shell::MarkFaceWithRectangle(Mat &matImg, Mat &matFaces, FLOAT flConfidenceThreshold)
{
	for (int i = 0; i < matFaces.rows; i++)
	{
		FLOAT flConfidenceVal = matFaces.at<FLOAT>(i, 2);
		if (flConfidenceVal < flConfidenceThreshold)
			continue;

		INT xLeftBottom = static_cast<INT>(matFaces.at<FLOAT>(i, 3) * matImg.cols);
		INT yLeftBottom = static_cast<INT>(matFaces.at<FLOAT>(i, 4) * matImg.rows);
		INT xRightTop = static_cast<INT>(matFaces.at<FLOAT>(i, 5) * matImg.cols);
		INT yRightTop = static_cast<INT>(matFaces.at<FLOAT>(i, 6) * matImg.rows);

		//* 画出矩形
		Rect rectObj(xLeftBottom, yLeftBottom, (xRightTop - xLeftBottom), (yRightTop - yLeftBottom));
		rectangle(matImg, rectObj, Scalar(0, 255, 0));

		//* 在被监测图片上输出可信度概率
		//* ======================================================================================
		ostringstream oss;
		oss << flConfidenceVal;
		String strConfidenceVal(oss.str());
		String strLabel = "Face: " + strConfidenceVal;

		INT nBaseLine = 0;
		Size labelSize = getTextSize(strLabel, FONT_HERSHEY_SIMPLEX, 0.5, 1, &nBaseLine);
		rectangle(matImg, Rect(Point(xLeftBottom, yLeftBottom - labelSize.height),
			Size(labelSize.width, labelSize.height + nBaseLine)),
			Scalar(255, 255, 255), CV_FILLED);
		putText(matImg, strLabel, Point(xLeftBottom, yLeftBottom), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0));
		//* ======================================================================================
	}

	imshow("Face Detect Result", matImg);
}

//* 用矩形框标出人脸并输出概率
MACHINEVISIONLIB_API void cv2shell::MarkFaceWithRectangle(Net &dnnNet, const CHAR *pszImgName, const Size &size, 
															FLOAT flConfidenceThreshold, FLOAT flScale, const Scalar &mean)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "cv2shell::MarkFaceWithRectangle()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		return;
	}

	Mat matFaces = FaceDetect(dnnNet, matImg, size, flScale, mean);
	if (matFaces.empty())
		return;

	MarkFaceWithRectangle(matImg, matFaces, flConfidenceThreshold);
}

//* 用矩形框标出人脸并输出概率
MACHINEVISIONLIB_API void cv2shell::MarkFaceWithRectangle(Net &dnnNet, const CHAR *pszImgName, FLOAT flConfidenceThreshold,
	ENUM_IMGRESIZE_METHOD enumMethod, FLOAT flScale, const Scalar &mean)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "cv2shell::MarkFaceWithRectangle()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		return;
	}

	Mat &matFaces = FaceDetect(dnnNet, matImg, enumMethod, flScale, mean);
	if (matFaces.empty())
		return;

	MarkFaceWithRectangle(matImg, matFaces, flConfidenceThreshold);
}

MACHINEVISIONLIB_API void cv2shell::MarkFaceWithRectangle(Mat &matImg, vector<Face> &vFaces)
{
	vector<Face>::iterator itFace = vFaces.begin();
	for (; itFace != vFaces.end(); itFace++)
	{
		Face face = *itFace;

		//* 画出矩形
		Rect rectObj(face.xLeftBottom, face.yLeftBottom, (face.xRightTop - face.xLeftBottom), (face.yRightTop - face.yLeftBottom));
		rectangle(matImg, rectObj, Scalar(0, 255, 0));

		//* 在被监测图片上输出可信度概率
		//* ======================================================================================
		ostringstream oss;
		oss << face.flConfidenceVal;
		String strConfidenceVal(oss.str());
		String strLabel = "Face: " + strConfidenceVal;

		INT nBaseLine = 0;
		Size labelSize = getTextSize(strLabel, FONT_HERSHEY_SIMPLEX, 0.5, 1, &nBaseLine);
		rectangle(matImg, Rect(Point(face.xLeftBottom, face.yLeftBottom - labelSize.height),
			Size(labelSize.width, labelSize.height + nBaseLine)),
			Scalar(255, 255, 255), CV_FILLED);
		putText(matImg, strLabel, Point(face.xLeftBottom, face.yLeftBottom), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0));
		//* ======================================================================================
	}

	namedWindow("Face Detect Result", 0);
	imshow("Face Detect Result", matImg);
}

MACHINEVISIONLIB_API Mat cv2shell::ExtractFaceChips(Mat matImg, FLOAT flScale, INT nMinNeighbors, INT nMinPossibleFaceSize)
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

	//* 按照网络要求调整为224,224大小
	resize(matFace, matFace, Size(224, 224));

	free(pubResultBuf);

	return matFace;
}

MACHINEVISIONLIB_API Mat cv2shell::ExtractFaceChips(const CHAR *pszImgName, FLOAT flScale, INT nMinNeighbors, INT nMinPossibleFaceSize)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "ExtractFaceChips() error：unable to read the picture, please confirm that the picture『" << pszImgName << "』 exists and the format is corrrect." << endl;

		return matImg;
	}

	return ExtractFaceChips(matImg, flScale, nMinNeighbors, nMinPossibleFaceSize);
}

//* 初始化轻型分类器，其实就是把DNN网络配置文件和训练好的模型加载到内存并据此建立DNN网络
MACHINEVISIONLIB_API Net cv2shell::InitLightClassifier(vector<string> &vClassNames)
{
	dnn::Net dnnNet;

	ifstream ifsClassNamesFile("C:\\Windows\\System32\\models\\vgg_ssd\\voc.names");
	if (ifsClassNamesFile.is_open())
	{
		string strClassName = "";
		while (getline(ifsClassNamesFile, strClassName))
			vClassNames.push_back(strClassName);
	}
	else
		return dnnNet;

	String strModelCfgFile("C:\\Windows\\System32\\models\\vgg_ssd\\deploy.prototxt"),
		strModelFile("C:\\Windows\\System32\\models\\vgg_ssd\\VGG_VOC0712_SSD_300x300_iter_120000.caffemodel");

	dnnNet = readNetFromCaffe(strModelCfgFile, strModelFile);
	if (dnnNet.empty())
	{
		cout << "DNN网络建立失败，请确认输入的网络配置文件『deploy.prototxt』和模型文件『VGG_SSD.caffemodel』存在且格式正确" << endl;
	}

	return dnnNet;
}

//* 使用DNN网络识别目标属于哪种物体
MACHINEVISIONLIB_API void cv2shell::ObjectDetect(Mat &matImg, Net &dnnNet, vector<string> &vClassNames, vector<RecogCategory> &vObjects, 
													const Size &size, FLOAT flConfidenceThreshold, FLOAT flScale, const Scalar &mean)
{
	if (matImg.channels() == 4)
		cvtColor(matImg, matImg, COLOR_BGRA2BGR);

	//* 加载图片文件并归一化，size参数指定图片要缩放的目标尺寸，mena指定要减去的平均值的平均标量（红蓝绿三个颜色通道都要减）
	Mat matInputBlob = blobFromImage(matImg, flScale, size, mean, false, false);

	//* 设置网络输入
	dnnNet.setInput(matInputBlob, "data");

	//* 计算网络输出
	Mat matDetection = dnnNet.forward("detection_out");

	Mat matIdentifyObjects(matDetection.size[2], matDetection.size[3], CV_32F, matDetection.ptr<float>());

	for (int i = 0; i < matIdentifyObjects.rows; i++)
	{
		FLOAT flConfidenceVal = matIdentifyObjects.at<FLOAT>(i, 2);

		if (flConfidenceVal < flConfidenceThreshold)
			continue;

		RecogCategory category;

		category.xLeftBottom = static_cast<int>(matIdentifyObjects.at<float>(i, 3) * matImg.cols);
		category.yLeftBottom = static_cast<int>(matIdentifyObjects.at<float>(i, 4) * matImg.rows);
		category.xRightTop = static_cast<int>(matIdentifyObjects.at<float>(i, 5) * matImg.cols);
		category.yRightTop = static_cast<int>(matIdentifyObjects.at<float>(i, 6) * matImg.rows);

		category.flConfidenceVal = flConfidenceVal;

		size_t tObjClass = (size_t)matIdentifyObjects.at<float>(i, 1);
		category.strCategoryName = vClassNames[tObjClass];
		vObjects.push_back(category);
	}
}

MACHINEVISIONLIB_API void cv2shell::ObjectDetect(Mat &matImg, Net &dnnNet, vector<string> &vClassNames, vector<RecogCategory> &vObjects, 
													FLOAT flConfidenceThreshold, ENUM_IMGRESIZE_METHOD enumMethod, FLOAT flScale, const Scalar &mean)
{
	if (enumMethod == EIRSZM_EQUALRATIO)
	{
		Size size = __ResizeImgToSpecPixel(matImg, 260);

		ObjectDetect(matImg, dnnNet, vClassNames, vObjects, size, flConfidenceThreshold, flScale, mean);
	}
	else
	{
		Mat matEquilateralImg;

		ImgEquilateral(matImg, matEquilateralImg);

		//namedWindow("Equilateral", 0);
		//imshow("Equilateral", matEquilateralImg);	
		//imwrite("D:\\work\\OpenCV\\resize_test.jpg", matEquilateralImg);

		ObjectDetect(matEquilateralImg, dnnNet, vClassNames, vObjects, Size(300, 300), flConfidenceThreshold, flScale, mean);
	}
}

MACHINEVISIONLIB_API void cv2shell::ObjectDetect(const CHAR *pszImgName, Net &dnnNet, vector<string> &vClassNames, vector<RecogCategory> &vObjects,
													const Size &size, FLOAT flConfidenceThreshold, FLOAT flScale, const Scalar &mean)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "cv2shell::ObjectDetect()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		return;
	}

	ObjectDetect(matImg, dnnNet, vClassNames, vObjects, size, flConfidenceThreshold, flScale, mean);
}

MACHINEVISIONLIB_API void cv2shell::ObjectDetect(const CHAR *pszImgName, Net &dnnNet, vector<string> &vClassNames, vector<RecogCategory> &vObjects,
													FLOAT flConfidenceThreshold, ENUM_IMGRESIZE_METHOD enumMethod, FLOAT flScale, const Scalar &mean)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "cv2shell::ObjectDetect()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		return;
	}

	ObjectDetect(matImg, dnnNet, vClassNames, vObjects, flConfidenceThreshold, enumMethod, flScale, mean);
}

//* 将检测出的目标对象在原图上用矩形框标记出来
MACHINEVISIONLIB_API void cv2shell::MarkObjectWithRectangle(Mat &matImg, vector<RecogCategory> &vObjects)
{
	vector<RecogCategory>::iterator itObject = vObjects.begin();

	for (; itObject != vObjects.end(); itObject++)
	{
		RecogCategory object = *itObject;

		//* 画出矩形
		Rect rectObj(object.xLeftBottom, object.yLeftBottom, (object.xRightTop - object.xLeftBottom), (object.yRightTop - object.yLeftBottom));
		rectangle(matImg, rectObj, Scalar(0, 255, 0));

		//* 在被监测图片上输出可信度概率
		//* ======================================================================================
		ostringstream oss;
		oss << object.flConfidenceVal;
		String strConfidenceVal(oss.str());
		String strLabel = object.strCategoryName + ": " + strConfidenceVal;

		INT nBaseLine = 0;
		Size labelSize = getTextSize(strLabel, FONT_HERSHEY_SIMPLEX, 0.5, 1, &nBaseLine);
		rectangle(matImg, Rect(Point(object.xLeftBottom, object.yLeftBottom - labelSize.height),
			Size(labelSize.width, labelSize.height + nBaseLine)),
			Scalar(255, 255, 255), CV_FILLED);
		putText(matImg, strLabel, Point(object.xLeftBottom, object.yLeftBottom), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0));
		//* ======================================================================================
	}

	//namedWindow("Object Detect Result", 0);	
	if (matImg.rows > 768 && matImg.rows == matImg.cols)
	{
		Mat matResizeImg;
		resize(matImg, matResizeImg, Size(720, 720), 0, 0, INTER_AREA);
		imshow("Object Detect Result", matResizeImg);
	}
	else
		imshow("Object Detect Result", matImg);
}

MACHINEVISIONLIB_API void cv2shell::MarkObjectWithRectangle(const CHAR *pszImgName, Net &dnnNet, vector<string> &vClassNames, 
															const Size &size, FLOAT flConfidenceThreshold, FLOAT flScale, 
															const Scalar &mean)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "cv2shell::MarkObjectWithRectangle()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		return;
	}

	vector<RecogCategory> vObjects;
	ObjectDetect(matImg, dnnNet, vClassNames, vObjects, size, flConfidenceThreshold, flScale, mean);
	MarkObjectWithRectangle(matImg, vObjects);
}

MACHINEVISIONLIB_API void cv2shell::MarkObjectWithRectangle(const CHAR *pszImgName, Net &dnnNet, vector<string> &vClassNames, FLOAT flConfidenceThreshold,
															ENUM_IMGRESIZE_METHOD enumMethod, FLOAT flScale, const Scalar &mean)
{
	Mat matImg = imread(pszImgName);
	if (matImg.empty())
	{
		cout << "cv2shell::MarkObjectWithRectangle()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		return;
	}

	vector<RecogCategory> vObjects;
	ObjectDetect(matImg, dnnNet, vClassNames, vObjects, flConfidenceThreshold, enumMethod, flScale, mean);
	MarkObjectWithRectangle(matImg, vObjects);
}

//* 获取检测到的目标对象的数量，如果检测到了目标对象，则参数pflConfidenceOfExist保存可信度值，不存在的话pflConfidenceOfExist返回一个没有意义的值
//* 参数strObjectName必须从InitLightClassifier()函数读入的voc.names文件中获取，因为这个分类器只能分类voc.names定义的那些目标物体
MACHINEVISIONLIB_API INT cv2shell::GetObjectNum(vector<RecogCategory> &vObjects, string strObjectName, FLOAT *pflConfidenceOfExist, 
												FLOAT *pflConfidenceOfObjectNum)
{
	vector<RecogCategory>::iterator itObject = vObjects.begin();
	FLOAT flConfidenceSum = 0.0f, flMaxConfidence = 0.0f;
	INT nObjectNum = 0;

	for (; itObject != vObjects.end(); itObject++)
	{
		RecogCategory object = *itObject;

		if (object.strCategoryName == strObjectName)
		{
			nObjectNum++;

			if (flMaxConfidence < object.flConfidenceVal)
				flMaxConfidence = object.flConfidenceVal;

			flConfidenceSum += object.flConfidenceVal;
		}
	}

	if (nObjectNum)
	{
		if (pflConfidenceOfObjectNum)
			*pflConfidenceOfObjectNum = flConfidenceSum / ((FLOAT)nObjectNum);

		if (pflConfidenceOfExist)
			*pflConfidenceOfExist = flMaxConfidence;
	}

	return nObjectNum;
}

CMachineVisionLib::CMachineVisionLib()
{
    return;
}

template <typename DType>
caffe::Net<DType>* caffe2shell::LoadNet(std::string strParamFile, std::string strModelFile, caffe::Phase phase)
{
	//* _access()函数的第2个参数0代表对文件的访问权限，0：检查文件是否存在，2：写，4：读，6：读写
	if (_access(strParamFile.c_str(), 0) == -1)
	{
		cout << "file " << strParamFile << " not exist!" << endl;
		return NULL;
	}

	if (_access(strModelFile.c_str(), 0) == -1)
	{
		cout << "file " << strModelFile << " not exist!" << endl;
		return NULL;
	}

	caffe::Net<DType>* caNet(new caffe::Net<DType>(strParamFile, phase));
	caNet->CopyTrainedLayersFrom(strModelFile);

	return caNet;
}

//* 提取图像特征
template <typename DType> 
void caffe2shell::ExtractFeature(caffe::Net<DType> *pNet, caffe::MemoryDataLayer<DType> *pMemDataLayer, 
									Mat& matImgROI, vector<DType>& vImgFeature, INT nFeatureDimension, const CHAR *pszBlobName)
{
	//* 将数据和标签放入网络
	vector<Mat> vmatImgROI;
	vector<INT> vnLabel;
	vmatImgROI.push_back(matImgROI);
	vnLabel.push_back(0);
	pMemDataLayer->AddMatVector(vmatImgROI, vnLabel);

	//* 前向传播，获取特征数据
	pNet->Forward();

	//* 关于Blob讲得最详细的Blog地址如下：
	//* https://blog.csdn.net/junmuzi/article/details/52761379
	//* 关于boost共享指针最详细的地址如下：
	//* https://www.cnblogs.com/helloamigo/p/3575098.html
	boost::shared_ptr<caffe::Blob<DType>> blobImgFeature = pNet->blob_by_name(pszBlobName);
#if NEED_GPU
	//* 使用GPU时直接从GPU读取数据可以跳过GPU->CPU的同步操作，这样能快一点
	const DType *pFeatureData = blobImgFeature->gpu_data();
#else
	//* 使用GPU时也可以使用cpu_data()，因为调用cpu_data()时，caffe会自动同步GPU->CPU，但这样就需要耗费一些同步时间，所以只有单纯CPU时才调用cpu_data()
	const DType *pFeatureData = blobImgFeature->gpu_data();

#endif

	//* 将特征数据存入出口参数，供上层调用函数使用
	for (INT i = 0; i < nFeatureDimension; i++)
		vImgFeature.push_back(pFeatureData[i]);
}
