#pragma once

#include "resource.h"

class Image {
public:
	Image():o_blIsEdited(FALSE){}
	~Image() {}

	//* 打开图片
	BOOL open(CHAR *pszImgFileName) 
	{
		//* 加载图像		
		o_mOpenedImg = imread(pszImgFileName);
		if (o_mOpenedImg.empty())
			return FALSE;		
		
		o_mOpenedImg.copyTo(o_mResultImg);

		return TRUE;
	}

	//* 显示图片
	void show(CHAR *pszWindowTitle)
	{
		if (o_mResultImg.empty())
			return;
		
		if (fabs(o_dblScaleFactor - 1.0) < 1e-6)
		{
			o_mResultImg.copyTo(o_mShowImg);
		}
		else
		{
			resize(o_mResultImg, o_mShowImg, Size(o_mResultImg.cols * o_dblScaleFactor, o_mResultImg.rows * o_dblScaleFactor), 0, 0, INTER_AREA);
			//cout << "o_mResultImg.cols * o_dblScaleFactor: " << o_mResultImg.cols * o_dblScaleFactor << " - o_mResultImg.rows * o_dblScaleFactor: " << o_mResultImg.rows * o_dblScaleFactor << endl;
			//pyrDown(o_mResultImg, mShowImg, Size(o_mResultImg.cols / 2, o_mResultImg.rows / 2));
		}			

		imshow(pszWindowTitle, o_mShowImg);
	}

	Mat& GetSrcImg(void)
	{
		return o_mOpenedImg;
	}

	Mat& GetResultImg(void)
	{
		return o_mResultImg;
	}

	Mat& GetShowImg(void)
	{
		return o_mShowImg;
	}

	INT GetImgWidth(void)
	{
		if (o_mResultImg.empty())
			return 0;

		return o_mResultImg.cols;
	}

	INT GetImgHeight(void)
	{
		if (o_mResultImg.empty())
			return 0;

		return o_mResultImg.rows;
	}

	//* 设置缩放因子
	void SetScaleFactor(DOUBLE dblScaleFactor)
	{
		o_dblScaleFactor = dblScaleFactor;
	}

	DOUBLE GetScaleFactor(void) const
	{
		return o_dblScaleFactor;
	}

	void SetEditFlag(BOOL blFlag)
	{
		o_blIsEdited = blFlag;
	}

	BOOL GetEditFlag(void)
	{
		return o_blIsEdited;
	}

private:
	Mat o_mOpenedImg;			//* 读入的原始图片数据
	Mat o_mResultImg;			//* 处理后的结果数据
	Mat o_mShowImg;				//* 用于交互显示的源图像

	DOUBLE o_dblScaleFactor;	//* 对于超大分辨率的图像需要将其缩小到PC屏幕支持的区域范围内才能显示，否则操作起来会
								//* 很不方便，该参数记录这个比例因子

	BOOL o_blIsEdited;			//* 是否编辑过
};
