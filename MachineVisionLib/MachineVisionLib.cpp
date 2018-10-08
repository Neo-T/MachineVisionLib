// MachineVisionLib.cpp : 定义 DLL 应用程序的导出函数。
//
#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <io.h>
#include <vector>
#include <facedetect-dll.h>
#include "common_lib.h"
#include "MachineVisionLib.h"

//* 执行Canny边缘检测
MACHINEVISIONLIB void cv2shell::CV2Canny(Mat &matSrc, Mat &matOut)
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
MACHINEVISIONLIB void cv2shell::CV2Canny(const CHAR *pszImgName, Mat &matOut)
{
	Mat matImg = imread(pszImgName);
	if (!matImg.empty())
	{
		CV2Canny(matImg, matOut);
	}
}

//* 既可以播放一段rtsp视频流、video文件还可以播放本地摄像头拍摄的实时视频，比如传入如下字符串作为参数：
//* rtsp://admin:abcd1234@192.168.0.250/mjpeg/ch1，指定播放网络摄像头拍摄的rtsp实时视频流
//* 参数blIsNeedToReplay用于指定播放意外终止比如网络中断时，是否尝试重新连接并继续播放，对于视频文件来说则是是否循环播放
template <typename DType>
void cv2shell::CV2ShowVideo(DType dtVideoSrc, BOOL blIsNeedToReplay)
{
	Mat mSrc;
	VideoCapture video;
	BOOL blIsNotOpen = TRUE;
	DOUBLE dblFPS = 40.;

	while (TRUE)
	{
		if (blIsNotOpen)
		{
			if (video.open(dtVideoSrc))
			{
				dblFPS = video.get(CV_CAP_PROP_FPS);

				//video.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
				//video.set(CV_CAP_PROP_FRAME_HEIGHT, 960);

				cout << video.get(CV_CAP_PROP_FRAME_WIDTH) << " x " << video.get(CV_CAP_PROP_FRAME_HEIGHT) << " FPS: " << dblFPS << endl;

				blIsNotOpen = FALSE;
			}
			else
			{
				Sleep(1000);
				continue;
			}
		}

		if (video.read(mSrc))
		{
			String strWinName = String("视频流【") + dtVideoSrc +  String("】");
			imshow(strWinName, mSrc);
			if (waitKey(1000.0 / dblFPS) == 27)
				break;
		}
		else
		{
			if (!blIsNeedToReplay)
			{
				video.release();
				return;
			}

			blIsNotOpen = TRUE;
			video.release();
			continue;
		}
	}
}

//* 同上，只是增加了回调处理函数
template <typename DType>
void cv2shell::CV2ShowVideo(DType dtVideoSrc, PCB_VIDEOHANDLER pfunNetVideoHandler, DWORD64 dw64InputParam, BOOL blIsNeedToReplay)
{
	Mat mSrc;
	VideoCapture video;
	DOUBLE dblFPS = 40.0;

	bool blIsNotOpen = TRUE;

	while (TRUE)
	{
		if (blIsNotOpen)
		{
			if (video.open(dtVideoSrc))
			{
				dblFPS = video.get(CV_CAP_PROP_FPS);

				cout << video.get(CV_CAP_PROP_FRAME_WIDTH) << " x " << video.get(CV_CAP_PROP_FRAME_HEIGHT) << " FPS: " << dblFPS << endl;

				blIsNotOpen = FALSE;
			}
			else
			{
				Sleep(1000);
				continue;
			}
		}

		if (video.read(mSrc))
		{
			if (NULL != pfunNetVideoHandler)
			{
				pfunNetVideoHandler(mSrc, dw64InputParam);
			}

			if (waitKey(1000.0 / dblFPS) == 27)
				break;
		}
		else
		{
			if (!blIsNeedToReplay)
			{
				video.release();
				return;
			}

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
MACHINEVISIONLIB void cv2shell::CV2CreateAlphaMat(Mat& mat)
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
MACHINEVISIONLIB string cv2shell::CV2HashValue(Mat& mSrc, PST_IMG_RESIZE pstResize)
{
	string strValues(pstResize->nRows * pstResize->nCols, '\0');

	Mat mImg;
	if (mSrc.channels() == 3)
		cvtColor(mSrc, mImg, CV_BGR2GRAY);
	else
		mImg = mSrc.clone();

	//* 第一步，缩小尺寸。将图片缩小到不能被8整出且大于8的尺寸，以去除图片的细节（长宽互换正好压缩掉门打开等细节）
	resize(mImg, mImg, Size(pstResize->nRows, pstResize->nCols));
	//resize(matImg, matImg, Size(pstResize->nCols * 10, pstResize->nRows * 10));
	//imshow("ce", matImg);
	//waitKey(0);

	//* 第二步，简化色彩(Color Reduce)。将缩小后的图片，转为64级灰度。
	UCHAR *pubData;
	for (INT i = 0; i<mImg.rows; i++)
	{
		pubData = mImg.ptr<UCHAR>(i);
		for (INT j = 0; j<mImg.cols; j++)
		{
			pubData[j] = pubData[j] / 4;
		}
	}

	//imshow("『缩小色彩』", matImg);

	//* 第三步，计算平均值。计算所有像素的灰度平均值。
	INT nAverage = (INT)mean(mImg).val[0];

	//* 第四步，比较像素的灰度。将每个像素的灰度，与平均值进行比较。大于或等于平均值记为1,小于平均值记为0
	//* C++的神奇之处，竟然可以这样操作
	Mat mMask = (mImg >= (UCHAR)nAverage);

	//* 第五步，计算哈希值，其实就是将16进制的0和1转成ASCII的‘0’和‘1’，即0x00->0x30、0x01->0x31，将其存储到一个线性字符串中
	INT nIdx = 0;
	for (INT i = 0; i<mMask.rows; i++)
	{
		pubData = mMask.ptr<UCHAR>(i);
		for (INT j = 0; j<mMask.cols; j++)
		{
			if (pubData[j] == 0)
				strValues[nIdx++] = '0';
			else
				strValues[nIdx++] = '1';
		}
	}

	mImg.release();	//* 其实不释放也可以，OpenCV会根据引用计数主动释放的，说白了是C++的内存管理机制释放的

	return strValues;
}

//* 上一个CV2HashValue函数的重载函数
MACHINEVISIONLIB string cv2shell::CV2HashValue(const CHAR *pszImgName, PST_IMG_RESIZE pstResize)
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
MACHINEVISIONLIB ST_IMG_RESIZE cv2shell::CV2GetResizeValue(Mat& mImg)
{
	ST_IMG_RESIZE stSize;

	stSize.nRows = EatZeroOfTheNumberTail(mImg.rows);
	stSize.nCols = EatZeroOfTheNumberTail(mImg.cols);

	__GetResizeValue(&stSize);

	return stSize;
}

//* 按照指定尺寸调整等比例调整图像大小，对于长宽不一致但要求调整为长宽一致的图片，函数会为短边加上单一像素的边以使其等长
MACHINEVISIONLIB void cv2shell::ImgEquilateral(Mat& mImg, Mat& mResizeImg, INT nResizeLen, Size& objAddedEdgeSize, const Scalar& border_color)
{
	INT nTop = 0, nBottom = 0, nLeft = 0, nRight = 0, nDifference;
	Mat mBorderImg;

	if (mImg.rows == mImg.cols)
	{
		if (mImg.rows == nResizeLen)
		{
			objAddedEdgeSize.width  = 0;
			objAddedEdgeSize.height = 0;

			mResizeImg = mImg;

			return;
		}
	}

	INT nLongestEdge = max(mImg.rows, mImg.cols);

	if (mImg.rows < nLongestEdge)
	{
		nDifference = nLongestEdge - mImg.rows;
		nTop = nBottom = nDifference / 2;
	}
	else if (mImg.cols < nLongestEdge)
	{
		nDifference = nLongestEdge - mImg.cols;
		nLeft = nRight = nDifference / 2;
	}
	else
		goto __lblSize;

	//* 给图像增加边界，使得图片等边，cv2.BORDER_CONSTANT指定边界颜色由value指定
	copyMakeBorder(mImg, mBorderImg, nTop, nBottom, nLeft, nRight, BORDER_CONSTANT, border_color);

__lblSize:
	Size size(nResizeLen, nResizeLen);

	objAddedEdgeSize.width  = ((DOUBLE)nLeft) * ((DOUBLE)nResizeLen / (DOUBLE)mBorderImg.rows);
	objAddedEdgeSize.height = ((DOUBLE)nTop)  * ((DOUBLE)nResizeLen / (DOUBLE)mBorderImg.cols);

	if (nResizeLen < nLongestEdge)
		resize(mBorderImg, mResizeImg, size, INTER_AREA);
	else
		resize(mBorderImg, mResizeImg, size, INTER_CUBIC);
}

//* 将图片尺寸调整为等边图片，其实就是短边填充指定颜色的像素使其与长边等长
MACHINEVISIONLIB void cv2shell::ImgEquilateral(Mat& mImg, Mat& mResizeImg, Size& objAddedEdgeSize, const Scalar& border_color)
{
	INT nTop = 0, nBottom = 0, nLeft = 0, nRight = 0, nDifference;

	if (mImg.rows == mImg.cols)
	{
		objAddedEdgeSize.width = 0;
		objAddedEdgeSize.height = 0;

		mResizeImg = mImg;
		return;
	}

	INT nLongestEdge = max(mImg.rows, mImg.cols);

	if (mImg.rows < nLongestEdge)
	{
		nDifference = nLongestEdge - mImg.rows;
		nTop = nBottom = nDifference / 2;
	}
	else if (mImg.cols < nLongestEdge)
	{
		nDifference = nLongestEdge - mImg.cols;
		nLeft = nRight = nDifference / 2;
	}
	else;

	objAddedEdgeSize.width = nLeft;
	objAddedEdgeSize.height = nTop;

	//* 给图像增加边界，是图片长、宽等长，cv2.BORDER_CONSTANT指定边界颜色由value指定
	copyMakeBorder(mImg, mResizeImg, nTop, nBottom, nLeft, nRight, BORDER_CONSTANT, border_color);
}

//* 根据哈希计算的要求，需要将图片缩小以去除图片细节，该函数将计算按比例缩小至不能被8整出的最小图片尺寸
MACHINEVISIONLIB ST_IMG_RESIZE cv2shell::CV2GetResizeValue(const CHAR *pszImgName)
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
static int __HanmingDistance(string& str1, string& str2, PST_IMG_RESIZE pstResize)
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

BOOL ImgMatcher::InitImgMatcher(vector<string *>& vModelImgs)
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
INT ImgMatcher::ImgSimilarity(Mat& mImg)
{
	//* 计算图片的哈希值
	string strHashValue = cv2shell::CV2HashValue(mImg, &stImgResize);

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
INT ImgMatcher::ImgSimilarity(Mat& mImg, INT *pnDistance)
{
	INT nMaxDistance = 0, nRtnVal = 0;

	//* 计算图片的哈希值
	string strHashValue = cv2shell::CV2HashValue(mImg, &stImgResize);

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
MACHINEVISIONLIB INT *cv2shell::FaceDetect(Mat& mImg, FLOAT flScale, INT nMinNeighbors, INT nMinPossibleFaceSize)
{
	UCHAR *pubResultBuf = (UCHAR*)malloc(LIBFACEDETECT_BUFFER_SIZE);
	if (!pubResultBuf)
	{
		cout << "cv2shell::FaceDetect()错误：" << GetLastError() << endl;
		return NULL;
	}

	Mat mGrayImg;
	cvtColor(mImg, mGrayImg, CV_BGR2GRAY);

	INT *pnResult = NULL;
	pnResult = facedetect_multiview_reinforce(pubResultBuf, (unsigned char*)(mGrayImg.ptr(0)),
												mGrayImg.cols, mGrayImg.rows, mGrayImg.step,
		flScale, nMinNeighbors, nMinPossibleFaceSize);
	if (pnResult && *pnResult > 0)
		return pnResult;

	free(pubResultBuf);

	return NULL;
}

MACHINEVISIONLIB INT *cv2shell::FaceDetect(const CHAR *pszImgName, FLOAT flScale, INT nMinNeighbors, INT nMinPossibleFaceSize)
{
	Mat mImg = imread(pszImgName);
	if (mImg.empty())
	{
		cout << "cv2shell::FaceDetect()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		return NULL;
	}

	return FaceDetect(mImg, flScale, nMinNeighbors, nMinPossibleFaceSize);
}

//* 用矩形框标记出人脸
MACHINEVISIONLIB void cv2shell::MarkFaceWithRectangle(Mat& mImg, INT *pnFaces)
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
		rectangle(mImg, left, right, Scalar(230, 255, 0), 1);
	}

	free(pnFaces);
}

MACHINEVISIONLIB void cv2shell::MarkFaceWithRectangle(Mat& mImg, FLOAT flScale, INT nMinNeighbors, INT nMinPossibleFaceSize)
{
	INT *pnFaces = FaceDetect(mImg, flScale, nMinNeighbors, nMinPossibleFaceSize);
	if (!pnFaces)
		return;

	MarkFaceWithRectangle(mImg, pnFaces);

	imshow("Face Detect Result", mImg);
}

MACHINEVISIONLIB void cv2shell::MarkFaceWithRectangle(const CHAR *pszImgName, FLOAT flScale, INT nMinNeighbors, INT nMinPossibleFaceSize)
{
	Mat mImg = imread(pszImgName);
	if (mImg.empty())
	{
		cout << "cv2shell::MarkFaceWithRectangle()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;
		return;
	}

	MarkFaceWithRectangle(mImg, flScale, nMinNeighbors, nMinPossibleFaceSize);
}

//* 初始化用于人脸检测的DNN网络
MACHINEVISIONLIB Net cv2shell::InitFaceDetectDNNet(void)
{
	String strModelCfgFile = "C:\\Windows\\System32\\models\\resnet\\deploy.prototxt";
	String strModelFile = "C:\\Windows\\System32\\models\\resnet\\res10_300x300_ssd_iter_140000.caffemodel";

	dnn::Net dnnNet = readNetFromCaffe(strModelCfgFile, strModelFile);
	if (dnnNet.empty())
		cout << "DNN网络建立失败，请确认输入的网络配置文件『deploy.prototxt』和模型文件『resnet_ssd.caffemodel』存在且格式正确" << endl;

	return dnnNet;
}

//* 使用DNN网络已训练好的模型开发的人脸检测函数
static Mat __DNNFaceDetect(Net& dnnNet, Mat& mImg, const Size& size, Size& objAddedEdgeSize, FLOAT flScale, const Scalar& mean)
{
	//* 加载图片文件并归一化，size参数指定图片要缩放的目标尺寸，mena指定要减去的平均值的平均标量（红蓝绿三个颜色通道都要减）
	//* 关于blobFromImage:
	//* image: 输入图像
	//* scalefactor： 如果模型训练时归一化到了0-1之间，那么这个参数就应该是1.0f/256.0f，否则为1
	//* size: 应该与训练时的输入图像尺寸保持一致，这里就应该是300 x 300
	//* mean：均值，与模型训练时的值一致，由于我们使用的是预训练模型，该值固定
	//* swapRB: 是否交换图像第1个通道和最后一个通道的顺序，这里不需要
	//* crop: TRUE，则依据size裁剪图像，否则直接将图像调整至Size尺寸
	Mat mInputBlob = blobFromImage(mImg, flScale, size, mean, FALSE, FALSE);

	//* 设置网络输入
	dnnNet.setInput(mInputBlob, "data");

	//* 计算网络输出
	Mat mDetection = dnnNet.forward("detection_out");

	Mat mFaces(mDetection.size[2], mDetection.size[3], CV_32F, mDetection.ptr<FLOAT>());

	//* 没有增加边框，则不需要重映射坐标位置
	if (!objAddedEdgeSize.width && !objAddedEdgeSize.height)
		return mFaces;

	//* 计算增加的边框宽度系数，并将坐标位置转换为原始图像的位置
	FLOAT flSrcImgWidth = (FLOAT)(mImg.cols - 2 * objAddedEdgeSize.width);
	FLOAT flSrcImgHeight = (FLOAT)(mImg.rows - 2 * objAddedEdgeSize.height);
	for (INT i = 0; i < mFaces.rows; i++)
	{
		FLOAT flConfidenceVal = mFaces.at<FLOAT>(i, 2);
		if (flConfidenceVal < 0.3)	//* 低于0.3的就不再理会
			continue;

		mFaces.at<FLOAT>(i, 3) = ((FLOAT)(static_cast<INT>(mFaces.at<FLOAT>(i, 3) * mImg.cols) - objAddedEdgeSize.width))  / flSrcImgWidth;
		mFaces.at<FLOAT>(i, 4) = ((FLOAT)(static_cast<INT>(mFaces.at<FLOAT>(i, 4) * mImg.rows) - objAddedEdgeSize.height)) / flSrcImgHeight;
		mFaces.at<FLOAT>(i, 5) = ((FLOAT)(static_cast<INT>(mFaces.at<FLOAT>(i, 5) * mImg.cols) - objAddedEdgeSize.width))  / flSrcImgWidth;
		mFaces.at<FLOAT>(i, 6) = ((FLOAT)(static_cast<INT>(mFaces.at<FLOAT>(i, 6) * mImg.rows) - objAddedEdgeSize.height)) / flSrcImgHeight;
	}

	return mFaces;
}

//* 将图片等比例缩小至指定像素
static Size __ResizeImgToSpecPixel(Mat& mImg, INT nMinPixel)
{
	INT *pnMin, *pnMax, nRows, nCols;

	//* 已经小于指定的最小值了，那就没必要再缩减了
	if (mImg.rows <= nMinPixel || mImg.cols <= nMinPixel)
		return Size(mImg.cols, mImg.rows);

	//* 相等则直接返回指定的最小值就可以了
	if (mImg.rows == mImg.cols)
		return Size(nMinPixel, nMinPixel);

	nRows = mImg.rows;
	nCols = mImg.cols;

	pnMin = &nCols;
	pnMax = &nRows;

	if (mImg.rows < mImg.cols)
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
MACHINEVISIONLIB Mat cv2shell::FaceDetect(Net& dnnNet, Mat& mImg, ENUM_IMGRESIZE_METHOD enumMethod, FLOAT flScale, const Scalar& mean)
{
	if (enumMethod == EIRSZM_EQUALRATIO)
	{
		//Size size = __ResizeImgToSpecPixel(matImg, 288); //* 实测288效果最好		
		
		return __DNNFaceDetect(dnnNet, mImg, Size(300, 300), Size(0, 0), flScale, mean);
	}
	else
	{
		Mat matEquilateralImg;
		Size objAddedEdgeSize;

		ImgEquilateral(mImg, matEquilateralImg, objAddedEdgeSize);

		return __DNNFaceDetect(dnnNet, matEquilateralImg, Size(300, 300), objAddedEdgeSize, flScale, mean);
	}
}

MACHINEVISIONLIB Mat cv2shell::FaceDetect(Net& dnnNet, const CHAR *pszImgName, ENUM_IMGRESIZE_METHOD enumMethod, FLOAT flScale, const Scalar& mean)
{
	Mat mImg = imread(pszImgName);
	if (mImg.empty())
	{
		cout << "cv2shell::FaceDetect()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		Mat mat;

		return mat;
	}

	return FaceDetect(dnnNet, mImg, enumMethod, flScale, mean);
}

MACHINEVISIONLIB Mat cv2shell::FaceDetect(Net& dnnNet, const CHAR *pszImgName, const Size& size, FLOAT flScale, const Scalar& mean)
{
	Mat mImg = imread(pszImgName);
	if (mImg.empty())
	{
		cout << "cv2shell::FaceDetect()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		Mat mat;

		return mat;
	}

	return __DNNFaceDetect(dnnNet, mImg, size, Size(0, 0), flScale, mean);
}

//* 参数vFaces用于保存检测到的人脸信息，是一个输出参数
static void __DNNFaceDetect(Net& dnnNet, Mat& mImg, vector<Face> &vFaces, const Size& size, Size& objAddedEdgeSize, 
											FLOAT flConfidenceThreshold, FLOAT flScale, const Scalar& mean)
{
	//* 加载图片文件并归一化，size参数指定图片要缩放的目标尺寸，mena指定要减去的平均值的平均标量（红蓝绿三个颜色通道都要减）
	Mat matInputBlob = blobFromImage(mImg, flScale, size, mean, false, false);

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

		Face objFace;

		objFace.o_nLeftTopX = static_cast<INT>(matFaces.at<FLOAT>(i, 3) * mImg.cols) - objAddedEdgeSize.width;
		objFace.o_nLeftTopY = static_cast<INT>(matFaces.at<FLOAT>(i, 4) * mImg.rows) - objAddedEdgeSize.height;
		objFace.o_nRightBottomX = static_cast<INT>(matFaces.at<FLOAT>(i, 5) * mImg.cols) - objAddedEdgeSize.width;
		objFace.o_nRightBottomY = static_cast<INT>(matFaces.at<FLOAT>(i, 6) * mImg.rows) - objAddedEdgeSize.height;

		objFace.o_flConfidenceVal = flConfidenceVal;
		vFaces.push_back(objFace);
	}
}

MACHINEVISIONLIB void cv2shell::FaceDetect(Net& dnnNet, Mat& mImg, vector<Face>& vFaces, FLOAT flConfidenceThreshold,
												ENUM_IMGRESIZE_METHOD enumMethod, FLOAT flScale, const Scalar& mean)
{
	if (enumMethod == EIRSZM_EQUALRATIO)
	{
		//Size size = __ResizeImgToSpecPixel(matImg, 288); //* 实测288效果最好

		__DNNFaceDetect(dnnNet, mImg, vFaces, Size(300, 300), Size(0, 0), flConfidenceThreshold, flScale, mean);
	}
	else
	{
		Mat mEquilateralImg;
		Size objAddedEdgeSize;

		ImgEquilateral(mImg, mEquilateralImg, objAddedEdgeSize);

		__DNNFaceDetect(dnnNet, mEquilateralImg, vFaces, Size(300, 300), objAddedEdgeSize, flConfidenceThreshold, flScale, mean);
	}
}

MACHINEVISIONLIB void cv2shell::FaceDetect(Net& dnnNet, const CHAR *pszImgName, vector<Face>& vFaces,
												FLOAT flConfidenceThreshold, ENUM_IMGRESIZE_METHOD enumMethod, 
												FLOAT flScale, const Scalar& mean)
{
	Mat mImg = imread(pszImgName);
	if (mImg.empty())
	{
		cout << "cv2shell::FaceDetect()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		return;
	}

	FaceDetect(dnnNet, mImg, vFaces, flConfidenceThreshold, enumMethod, flScale, mean);
}

MACHINEVISIONLIB void cv2shell::FaceDetect(Net& dnnNet, const CHAR *pszImgName, vector<Face>& vFaces, const Size& size,
											FLOAT flConfidenceThreshold, FLOAT flScale, const Scalar& mean)
{
	Mat mImg = imread(pszImgName);
	if (mImg.empty())
	{
		cout << "cv2shell::FaceDetect()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		return;
	}

	__DNNFaceDetect(dnnNet, mImg, vFaces, size, Size(0, 0), flConfidenceThreshold, flScale, mean);
}

//* 将测结果在图片上展示出来：用矩形框标记出人脸并输出预测概率
//* 参数flConfidenceThreshold指定最小置信度阈值，也就是预测是人脸的最小概率值，大于此概率的被认作是人脸并标记
MACHINEVISIONLIB void cv2shell::MarkFaceWithRectangle(Mat& mImg, Mat& mFaces, FLOAT flConfidenceThreshold, BOOL blIsShow)
{
	for (INT i = 0; i < mFaces.rows; i++)
	{
		FLOAT flConfidenceVal = mFaces.at<FLOAT>(i, 2);
		if (flConfidenceVal < flConfidenceThreshold)
			continue;

		INT nLeftTopX = static_cast<INT>(mFaces.at<FLOAT>(i, 3) * mImg.cols);
		INT nLeftTopY = static_cast<INT>(mFaces.at<FLOAT>(i, 4) * mImg.rows);
		INT nRightBottomX = static_cast<INT>(mFaces.at<FLOAT>(i, 5) * mImg.cols);
		INT nRightBottomY = static_cast<INT>(mFaces.at<FLOAT>(i, 6) * mImg.rows);

		//cout << nLeftTopX << " " << nLeftTopY << " " << nRightBottomX << " " << nRightBottomY << endl;

		//* 画出矩形
		Rect rectObj(nLeftTopX, nLeftTopY, (nRightBottomX - nLeftTopX), (nRightBottomY - nLeftTopY));
		rectangle(mImg, rectObj, Scalar(0, 255, 0));

		//* 在被监测图片上输出可信度概率
		//* ======================================================================================
		ostringstream oss;
		oss << flConfidenceVal;
		String strConfidenceVal(oss.str());
		String strLabel = "Face: " + strConfidenceVal;

		INT nBaseLine = 0;
		Size labelSize = getTextSize(strLabel, FONT_HERSHEY_SIMPLEX, 0.5, 1, &nBaseLine);
		rectangle(mImg, Rect(Point(nLeftTopX, nLeftTopY - labelSize.height),
			Size(labelSize.width, labelSize.height + nBaseLine)),
			Scalar(255, 255, 255), CV_FILLED);
		putText(mImg, strLabel, Point(nLeftTopX, nLeftTopY), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0));
		//* ======================================================================================
	}

	if(blIsShow)
		imshow("Face Detect Result", mImg);
}

//* 用矩形框标出人脸并输出概率
MACHINEVISIONLIB void cv2shell::MarkFaceWithRectangle(Net& dnnNet, const CHAR *pszImgName, const Size& size, 
															FLOAT flConfidenceThreshold, FLOAT flScale, const Scalar& mean)
{
	Mat mImg = imread(pszImgName);
	if (mImg.empty())
	{
		cout << "cv2shell::MarkFaceWithRectangle()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		return;
	}

	Mat mFaces = __DNNFaceDetect(dnnNet, mImg, size, Size(0, 0), flScale, mean);
	if (mFaces.empty())
		return;

	MarkFaceWithRectangle(mImg, mFaces, flConfidenceThreshold);
}

//* 用矩形框标出人脸并输出概率
MACHINEVISIONLIB void cv2shell::MarkFaceWithRectangle(Net& dnnNet, const CHAR *pszImgName, FLOAT flConfidenceThreshold,
														ENUM_IMGRESIZE_METHOD enumMethod, FLOAT flScale, const Scalar& mean)
{
	Mat mImg = imread(pszImgName);
	if (mImg.empty())
	{
		cout << "cv2shell::MarkFaceWithRectangle()错误：无法读入图片，请确认图片『" << pszImgName << "』存在或者格式正确" << endl;

		return;
	}

	Mat& mFaces = FaceDetect(dnnNet, mImg, enumMethod, flScale, mean);
	if (mFaces.empty())
		return;

	MarkFaceWithRectangle(mImg, mFaces, flConfidenceThreshold);
}

MACHINEVISIONLIB void cv2shell::MarkFaceWithRectangle(Mat& mImg, vector<Face>& vFaces, BOOL blIsShow)
{
	vector<Face>::iterator itFace = vFaces.begin();
	for (; itFace != vFaces.end(); itFace++)
	{
		Face objFace = *itFace;

		//* 画出矩形
		Rect rectObj(objFace.o_nLeftTopX, objFace.o_nLeftTopY, (objFace.o_nRightBottomX - objFace.o_nLeftTopX), (objFace.o_nRightBottomY - objFace.o_nLeftTopY));
		rectangle(mImg, rectObj, Scalar(0, 255, 0));

		//* 在被监测图片上输出可信度概率
		//* ======================================================================================
		ostringstream oss;
		oss << objFace.o_flConfidenceVal;
		String strConfidenceVal(oss.str());
		String strLabel = "Face: " + strConfidenceVal;

		INT nBaseLine = 0;
		Size labelSize = getTextSize(strLabel, FONT_HERSHEY_SIMPLEX, 0.5, 1, &nBaseLine);
		rectangle(mImg, Rect(Point(objFace.o_nLeftTopX, objFace.o_nLeftTopY - labelSize.height),
			Size(labelSize.width, labelSize.height + nBaseLine)),
			Scalar(255, 255, 255), CV_FILLED);
		putText(mImg, strLabel, Point(objFace.o_nLeftTopX, objFace.o_nLeftTopY), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0));
		//* ======================================================================================
	}

	if (blIsShow)
	{
		namedWindow("Face Detect Result", 0);
		imshow("Face Detect Result", mImg);
	}	
}

//* 合并重叠的矩形，参数vSrcRects为原始矩形数据，函数检查该矩形集是否存在重叠的矩形，重叠的矩形将被合并后保存到参数vMergedRects指向的内存中
MACHINEVISIONLIB void cv2shell::MergeOverlappingRect(vector<ST_DIAGONAL_POINTS> vSrcRects, vector<ST_DIAGONAL_POINTS>& vMergedRects)
{
	INT nMergeRectIndex = 0;

__lblLoop:
	if (!vSrcRects.size())
		return;

	vector<ST_DIAGONAL_POINTS>::iterator itSrcRect = vSrcRects.begin();

	vMergedRects.push_back(*itSrcRect);
	vSrcRects.erase(itSrcRect);

	itSrcRect = vSrcRects.begin();
	for (; itSrcRect != vSrcRects.end();)
	{
		//* 看看矩形是否相交
		ST_DIAGONAL_POINTS stRectTarget = *itSrcRect;
		ST_DIAGONAL_POINTS stRectBase = vMergedRects[nMergeRectIndex];

		INT nMinX, nMaxX, nMinY, nMaxY;
		nMinX = max(stRectBase.point_left.x, stRectTarget.point_left.x);
		nMaxX = min(stRectBase.point_right.x, stRectTarget.point_right.x);
		nMinY = max(stRectBase.point_left.y, stRectTarget.point_left.y);
		nMaxY = min(stRectBase.point_right.y, stRectTarget.point_right.y);
		if (nMinX > nMaxX || nMinY > nMaxY)	//* 不相交
		{
			itSrcRect++;
			continue;
		}
		else
		{
			vMergedRects[nMergeRectIndex].point_left.x = min(stRectBase.point_left.x, stRectTarget.point_left.x);
			vMergedRects[nMergeRectIndex].point_left.y = min(stRectBase.point_left.y, stRectTarget.point_left.y);
			vMergedRects[nMergeRectIndex].point_right.x = max(stRectBase.point_right.x, stRectTarget.point_right.x);
			vMergedRects[nMergeRectIndex].point_right.y = max(stRectBase.point_right.y, stRectTarget.point_right.y);

			vSrcRects.erase(itSrcRect);
			itSrcRect = vSrcRects.begin();	//* 重新开始查找相交的矩形
		}
	}

	nMergeRectIndex++;
	goto __lblLoop;
}

//* 在原始图像上标记识别出的物体
void iOCV2DNNObjectDetector::MarkObject(Mat& mShowImg, vector<RecogCategory>& vObjects)
{
	vector<RecogCategory>::iterator itObject = vObjects.begin();

	for (; itObject != vObjects.end(); itObject++)
	{
		RecogCategory object = *itObject;

		//* 画出矩形
		Rect rectObj(object.nLeftTopX, object.nLeftTopY, (object.nRightBottomX - object.nLeftTopX), (object.nRightBottomY - object.nLeftTopY));
		rectangle(mShowImg, rectObj, Scalar(0, 255, 0), 2);

		//cout << object.nLeftTopX << ", " << object.nLeftTopY << " - " << object.nRightBottomX << ", " << object.nRightBottomY << endl;

		//* 在被监测图片上输出可信度概率
		//* ======================================================================================
		ostringstream oss;
		oss << object.flConfidenceVal;
		String strConfidenceVal(oss.str());
		String strLabel = object.strCategoryName + ": " + strConfidenceVal;

		INT nBaseLine = 0;
		Size labelSize = getTextSize(strLabel, FONT_HERSHEY_SIMPLEX, 0.5, 1, &nBaseLine);
		rectangle(mShowImg, Rect(Point(object.nLeftTopX, object.nLeftTopY - labelSize.height),
					Size(labelSize.width, labelSize.height + nBaseLine)),
					Scalar(255, 255, 255), CV_FILLED);
		putText(mShowImg, strLabel, Point(object.nLeftTopX, object.nLeftTopY), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0));
		//* ======================================================================================
	}
}

//* 获取检测到的目标对象的数量，如果检测到了目标对象，则参数pflConfidenceOfExist保存可信度值，不存在的话pflConfidenceOfExist返回一个没有意义的值
//* 参数strObjectName必须从InitLightClassifier()函数读入的voc.names文件中获取，因为这个分类器只能分类voc.names定义的那些目标物体
INT iOCV2DNNObjectDetector::GetObjectNum(vector<RecogCategory>& vObjects, string strObjectName, FLOAT *pflConfidenceOfExist, FLOAT *pflConfidenceOfObjectNum)
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

//* OCV2 DNN接口之SSD检测器
OCV2DNNObjectDetectorSSD::OCV2DNNObjectDetectorSSD(FLOAT flConfidenceThreshold, ENUM_DETECTOR enumDetector) :
													iOCV2DNNObjectDetector(flConfidenceThreshold), o_enumDetector(enumDetector)
{	
	string strVOCFile, strCfgFile, strModelFile;

	//* 根据检测器类型设定要加载模型文件
	switch (enumDetector)
	{
	case MOBNETSSD:
		strVOCFile = "C:\\Windows\\System32\\models\\mobile_net_ssd\\voc.names";
		strCfgFile = "C:\\Windows\\System32\\models\\mobile_net_ssd\\MobileNetSSD_deploy.prototxt";
		strModelFile = "C:\\Windows\\System32\\models\\mobile_net_ssd\\MobileNetSSD_deploy.caffemodel";

		o_dblNormalCoef = 0.007843;
		o_objMean = Scalar(127.5, 127.5, 127.5);
		break;

	default:
		strVOCFile = "C:\\Windows\\System32\\models\\vgg_ssd\\voc.names";
		strCfgFile = "C:\\Windows\\System32\\models\\vgg_ssd\\deploy.prototxt";
		strModelFile = "C:\\Windows\\System32\\models\\vgg_ssd\\VGG_VOC0712_SSD_300x300_iter_120000.caffemodel";

		o_dblNormalCoef = 1.F;
		o_objMean = Scalar(104.F, 117.F, 123.F);

		break;
	}

	ifstream ifsClassNamesFile(strVOCFile);
	if (ifsClassNamesFile.is_open())
	{
		string strClassName = "";
		while (getline(ifsClassNamesFile, strClassName))
			o_vClassNames.push_back(strClassName);
	}
	else
	{
		throw runtime_error("Failed to open the voc.names file, please confirm if the file exist(the path is " + strVOCFile + ").");
	}

	o_objDNNNet = readNetFromCaffe(strCfgFile, strModelFile);	
}

//* OCV2 DNN网络之SSD模型物体检测
static void __SSDObjectDetect(Mat& mImg, Net& objDNNNet, vector<string>& vClassNames, vector<RecogCategory>& vObjects, const Size& objSize,
	Size& objAddedEdgeSize, FLOAT flConfidenceThreshold, FLOAT flScale, const Scalar& objMean)
{
	if (mImg.channels() == 4)
		cvtColor(mImg, mImg, COLOR_BGRA2BGR);

	//* 加载图片文件并归一化，size参数指定图片要缩放的目标尺寸，mean指定要减去的平均值的平均标量（红蓝绿三个颜色通道都要减）
	Mat mInputBlob = blobFromImage(mImg, flScale, objSize, objMean, FALSE, FALSE);

	//* 设置网络输入
	objDNNNet.setInput(mInputBlob, "data");

	//* 计算网络输出
	Mat mDetection = objDNNNet.forward("detection_out");

	Mat mIdentifyObjects(mDetection.size[2], mDetection.size[3], CV_32F, mDetection.ptr<FLOAT>());

	for (int i = 0; i < mIdentifyObjects.rows; i++)
	{
		FLOAT flConfidenceVal = mIdentifyObjects.at<FLOAT>(i, 2);

		if (flConfidenceVal < flConfidenceThreshold)
			continue;

		RecogCategory category;

		category.nLeftTopX = static_cast<INT>(mIdentifyObjects.at<FLOAT>(i, 3) * mImg.cols) - objAddedEdgeSize.width;
		category.nLeftTopY = static_cast<INT>(mIdentifyObjects.at<FLOAT>(i, 4) * mImg.rows) - objAddedEdgeSize.height;
		category.nRightBottomX = static_cast<INT>(mIdentifyObjects.at<FLOAT>(i, 5) * mImg.cols) - objAddedEdgeSize.width;
		category.nRightBottomY = static_cast<INT>(mIdentifyObjects.at<FLOAT>(i, 6) * mImg.rows) - objAddedEdgeSize.height;

		category.flConfidenceVal = flConfidenceVal;

		size_t tObjClass = (size_t)mIdentifyObjects.at<FLOAT>(i, 1);
		category.strCategoryName = vClassNames[tObjClass];
		vObjects.push_back(category);
	}
}

//* 检测整幅图像找出认识的物体
void OCV2DNNObjectDetectorSSD::detect(Mat& mSrcImg, vector<RecogCategory>& vObjects)
{
	Mat mEquilateralImg;
	Size objAddedEdgeSize;

	cv2shell::ImgEquilateral(mSrcImg, mEquilateralImg, objAddedEdgeSize);

	__SSDObjectDetect(mEquilateralImg, o_objDNNNet, o_vClassNames, vObjects, Size(300, 300), objAddedEdgeSize, o_flConfidenceThreshold, o_dblNormalCoef, o_objMean);
}

void OCV2DNNObjectDetectorSSD::detect(Mat& mSrcImg, vector<RecogCategory>& vObjects, vector<string>& vstrFilter)
{
	Mat mEquilateralImg;
	Size objAddedEdgeSize;

	cv2shell::ImgEquilateral(mSrcImg, mEquilateralImg, objAddedEdgeSize);

	vector<RecogCategory> vOriginalObjects;
	__SSDObjectDetect(mEquilateralImg, o_objDNNNet, o_vClassNames, vOriginalObjects, Size(300, 300), objAddedEdgeSize, o_flConfidenceThreshold, o_dblNormalCoef, o_objMean);

	if (!vstrFilter.empty())
	{
		//* 取出参数vstrFilter指定的类别，其余的丢弃
		for (INT i = 0; i < vstrFilter.size(); i++)
		{
			for (INT k = 0; k < vOriginalObjects.size(); k++)
			{
				if (vOriginalObjects[k].strCategoryName == vstrFilter[i])
				{
					vObjects.push_back(vOriginalObjects[k]);
					break;
				}
			}
		}
	}
	else
	{
		vObjects = vOriginalObjects;
	}	
}

//* 初始化Yolo2分类器模型
static Net __InitYolo2Detector(const CHAR *pszClassNameFile, const CHAR *pszModelCfgFile, const CHAR *pszModelWeightFile, vector<string>& vClassNames)
{
	Net objDNNNet;

	//* 加载类别名称
	ifstream ifsClassNamesFile(pszClassNameFile);
	if (ifsClassNamesFile.is_open())
	{
		string strClassName = "";
		while (getline(ifsClassNamesFile, strClassName))
			vClassNames.push_back(strClassName);
	}
	else
	{
		throw runtime_error("Failed to open the voc.names file, please confirm if the file exist(the path is " + string(pszClassNameFile) + ").");
	}

	return readNetFromDarknet(pszModelCfgFile, pszModelWeightFile);
}

//* OCV2 DNN接口之YOLO2检测器
OCV2DNNObjectDetectorYOLO2::OCV2DNNObjectDetectorYOLO2(FLOAT flConfidenceThreshold, ENUM_DETECTOR enumDetector) : 
														iOCV2DNNObjectDetector(flConfidenceThreshold), o_enumDetector(enumDetector)
{
	switch (enumDetector)
	{
	case YOLO2_TINY_VOC:
		o_objDNNNet = __InitYolo2Detector("C:\\Windows\\System32\\models\\yolov2\\voc.names",
										  "C:\\Windows\\System32\\models\\yolov2\\yolov2-tiny-voc.cfg",
										  "C:\\Windows\\System32\\models\\yolov2\\yolov2-tiny-voc.weights",
										  o_vClassNames);
		break;

	case YOLO2_VOC:
		o_objDNNNet = __InitYolo2Detector("C:\\Windows\\System32\\models\\yolov2\\voc.names",
										  "C:\\Windows\\System32\\models\\yolov2\\yolov2-voc.cfg",
										  "C:\\Windows\\System32\\models\\yolov2\\yolov2-voc.weights",
										  o_vClassNames);
		break;

	case YOLO2_TINY:
		o_objDNNNet = __InitYolo2Detector("C:\\Windows\\System32\\models\\yolov2\\coco.names",
										  "C:\\Windows\\System32\\models\\yolov2\\yolov2-tiny.cfg",
										  "C:\\Windows\\System32\\models\\yolov2\\yolov2-tiny.weights",
										  o_vClassNames);
		break;

	case YOLO2:
	default:
		o_objDNNNet = __InitYolo2Detector("C:\\Windows\\System32\\models\\yolov2\\coco.names",
										  "C:\\Windows\\System32\\models\\yolov2\\yolov2.cfg",
										  "C:\\Windows\\System32\\models\\yolov2\\yolov2.weights",
										  o_vClassNames);
		break;
	}
}

void OCV2DNNObjectDetectorYOLO2::detect(Mat& mSrcImg, vector<RecogCategory>& vObjects)
{
	if (mSrcImg.channels() == 4)
		cvtColor(mSrcImg, mSrcImg, COLOR_BGRA2BGR);

#define IMG_EQUILATERAL 0	//* 是否需要将图像等边处理，经过实测发现，这个模型不需要将图片等边后再输入，等边后的预测效果反而不好，奇怪

#if IMG_EQUILATERAL
	Mat mEquilateralImg;
	Size objAddedEdgeSize;

	if (mSrcImg.cols == mSrcImg.rows)
	{
		mEquilateralImg = mSrcImg;
		objAddedEdgeSize = Size(0, 0);
	}
	else
	{
		ImgEquilateral(mSrcImg, mEquilateralImg, objAddedEdgeSize);
	}
#endif	

#if IMG_EQUILATERAL
	//* 按网络要求输入数据并预测
	Mat mInputBlob = blobFromImage(mEquilateralImg, 1 / 255.F, Size(416, 416), Scalar(), FALSE, FALSE);
#else
	//* 按网络要求输入数据并预测
	Mat mInputBlob = blobFromImage(mSrcImg, 1 / 255.F, Size(416, 416), Scalar(), FALSE, FALSE);
#endif

	o_objDNNNet.setInput(mInputBlob, "data");
	Mat mDetection = o_objDNNNet.forward();

	//* 取出分类结果
	/*
	* mDetection保存着分类数据，以行为单位，一行一个物体，行结构相同，列的存储单位为FLOAT，行结构如下：
	* 第0、1列：目标物体区域中心点的坐标系数，这个系数并不是实际的坐标位置，而是实际位置[x，y]分别除以图像列宽cols和行宽rows得到的
	* 第2、3列：目标物体所占矩形区域的宽度width和高度height
	* 第4   列：未使用，含义不可知
	* 第5-N 列: 预训练模型支持多少种分类，就有多少列，Yolo2模型支持识别80种物体，则这里就有80列，每一列与coco.names文件给出的分类列表由上到下，一一对应，列值为该物体属于coco.names列表所对应分类的概率
	*/
	for (INT i = 0; i<mDetection.rows; i++)
	{
		//* 获取概率数据首地址
		FLOAT *pflProbData = &mDetection.at<FLOAT>(i, YOLO2_PROBABILITY_DATA_INDEX);

		//* 遍历整列概率数据，找出最大概率分类的位置索引，注意这是相对索引，是从pflProbData开始的
		UINT unMaxProbabilityIndex = max_element(pflProbData, pflProbData + mDetection.cols - YOLO2_PROBABILITY_DATA_INDEX) - pflProbData;

		//* 获取概率值，并抛弃概率值较低的分类
		FLOAT flConfidenceVal = mDetection.at<FLOAT>(i, unMaxProbabilityIndex + YOLO2_PROBABILITY_DATA_INDEX);
		if (flConfidenceVal < o_flConfidenceThreshold)
			continue;

		//* 中心点坐标位置
		FLOAT flCenterX = mDetection.at<FLOAT>(i, 0);
		FLOAT flCenterY = mDetection.at<FLOAT>(i, 1);
		FLOAT flObjWidth = mDetection.at<FLOAT>(i, 2);
		FLOAT flObjHeight = mDetection.at<FLOAT>(i, 3);

		RecogCategory category;

#if IMG_EQUILATERAL
		category.nLeftTopX = static_cast<INT>((flCenterX - flObjWidth / 2) * mEquilateralImg.cols) - objAddedEdgeSize.width;
		category.nLeftTopY = static_cast<INT>((flCenterY - flObjHeight / 2) * mEquilateralImg.rows) - objAddedEdgeSize.height;
		category.nRightBottomX = static_cast<INT>((flCenterX + flObjWidth / 2) * mEquilateralImg.cols) - objAddedEdgeSize.width;
		category.nRightBottomY = static_cast<INT>((flCenterY + flObjHeight / 2) * mEquilateralImg.rows) - objAddedEdgeSize.height;
#else
		category.nLeftTopX = static_cast<INT>((flCenterX - flObjWidth / 2) * mSrcImg.cols);
		category.nLeftTopY = static_cast<INT>((flCenterY - flObjHeight / 2) * mSrcImg.rows);
		category.nRightBottomX = static_cast<INT>((flCenterX + flObjWidth / 2) * mSrcImg.cols);
		category.nRightBottomY = static_cast<INT>((flCenterY + flObjHeight / 2) * mSrcImg.rows);
#endif

		category.flConfidenceVal = flConfidenceVal;
		category.strCategoryName = o_vClassNames[unMaxProbabilityIndex];
		vObjects.push_back(category);
	}

#undef IMG_EQUILATERAL
}

void OCV2DNNObjectDetectorYOLO2::detect(Mat& mSrcImg, vector<RecogCategory>& vObjects, vector<string>& vstrFilter)
{
	vector<RecogCategory> vOriginalObjects;
	detect(mSrcImg, vOriginalObjects);

	if (!vstrFilter.empty())
	{
		//* 取出参数vstrFilter指定的类别，其余的丢弃
		for (INT i = 0; i < vstrFilter.size(); i++)
		{
			for (INT k = 0; k < vOriginalObjects.size(); k++)
			{
				if (vOriginalObjects[k].strCategoryName == vstrFilter[i])
				{
					vObjects.push_back(vOriginalObjects[k]);
					break;
				}
			}
		}
	}
	else
		vObjects = vOriginalObjects;
}

//* 返回在DNN网络上花费的时间，单位毫秒
MACHINEVISIONLIB DOUBLE cv2shell::GetTimeSpentInNetDetection(Net& objDNNNet)
{
	vector<DOUBLE> vdblLayersTimings;
	DOUBLE dblFreq = getTickFrequency() / 1000;
	DOUBLE dblTime = objDNNNet.getPerfProfile(vdblLayersTimings) / dblFreq;

	return dblTime;
}

//* 显示或者隐藏OpenCV的窗口，参数blIsShowing：TRUE，显示；FALSE，隐藏
MACHINEVISIONLIB void cv2shell::ShowImageWindow(const CHAR *pszWindowTitle, BOOL blIsShowing)
{
	HWND hWndOCVImgShow = (HWND)cvGetWindowHandle(pszWindowTitle);
	HWND hWndParentOCVImgShow = GetParent(hWndOCVImgShow);

	if(blIsShowing)
		ShowWindow(hWndParentOCVImgShow, SW_SHOW);
	else
		ShowWindow(hWndParentOCVImgShow, SW_HIDE);
}

//* 认证码图片预处理操作，其实就是去掉图片的背景和颜色特征
MACHINEVISIONLIB void cv2shell::CAPTCHAImgPreProcess(Mat& mSrcImg, Mat& mDstImg)
{
	if (mSrcImg.empty())
	{
		cout << "检测到参数mSrcImg为空，CAPTCHAImgPreProcess()函数无法对图片进行预处理操作！" << endl;
		return;
	}

	Mat mGrayImg;
	if (mSrcImg.channels() == 3)
		cvtColor(mSrcImg, mGrayImg, COLOR_BGR2GRAY);
	else
		mSrcImg.copyTo(mGrayImg);

	//* 将灰度图颜色反转，也就是偏亮的颜色变暗，偏暗的变亮，这样才能在二值化时让认证码部分变成白色
	mGrayImg = 255 - mGrayImg;

	//* 计算图像的灰度均值，并二值化，低于均值的黑色，高于均值的白色
	DOUBLE dblMean = mean(mGrayImg)[0];
	threshold(mGrayImg, mDstImg, dblMean, 255, THRESH_BINARY);
}

//* 参数size用于指定进行开运算以消除小的噪点时使用的核大小，推荐值为Size(3, 3)
MACHINEVISIONLIB void cv2shell::CAPTCHAImgPreProcess(Mat& mSrcImg, Mat& mDstImg, const Size& size)
{
	Mat mBinaryImg;
	CAPTCHAImgPreProcess(mSrcImg, mBinaryImg);

	Mat element = getStructuringElement(MORPH_RECT, size);
	morphologyEx(mBinaryImg, mDstImg, MORPH_OPEN, element);
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
									Mat& matImgROI, DType *pdtaImgFeature, INT nFeatureDimension, const CHAR *pszBlobName)
{
	//* 将数据和标签放入网络
	vector<Mat> vmatImgROI;
	vector<INT> vnLabel;
	vmatImgROI.push_back(matImgROI);
	vnLabel.push_back(0);
	pMemDataLayer->AddMatVector(vmatImgROI, vnLabel);

	//* 前向传播，获取特征数据
	PerformanceTimer objPerformTimer;
	objPerformTimer.start();
	pNet->Forward();	
	cout << "Execution time of caffe net Forward(): " << objPerformTimer.end() / 1000 << "ms." << endl;

	//* 关于Blob讲得最详细的Blog地址如下：
	//* https://blog.csdn.net/junmuzi/article/details/52761379
	//* 关于boost共享指针最详细的地址如下：
	//* https://www.cnblogs.com/helloamigo/p/3575098.html
	boost::shared_ptr<caffe::Blob<DType>> blobImgFeature = pNet->blob_by_name(pszBlobName);

	//* 一旦调用cpu_data()，caffe会自动同步GPU->CPU（当然没有使用GPU，那么数据一直在CPU这边，caffe就不会自动同步了）
	const DType *pFeatureData = blobImgFeature->cpu_data();
	memcpy(pdtaImgFeature, pFeatureData, nFeatureDimension * sizeof(DType));
}

//* 提取图像特征
template <typename DType>
void caffe2shell::ExtractFeature(caffe::Net<DType> *pNet, caffe::MemoryDataLayer<DType> *pMemDataLayer,
									Mat& matImgROI, Mat& matImgFeature, INT nFeatureDimension, const CHAR *pszBlobName)
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

	//* 一旦调用cpu_data()，caffe会自动同步GPU->CPU（当然没有使用GPU，那么数据一直在CPU这边，caffe就不会自动同步了）
	const DType *pFeatureData = blobImgFeature->cpu_data();	
	memcpy(matImgFeature.data, pFeatureData, nFeatureDimension * sizeof(DType));
}
