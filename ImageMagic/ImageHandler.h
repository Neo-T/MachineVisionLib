#pragma once

#define MOUSE_SHOT_OFFSET	10	//* 确定鼠标点在某个点上时允许的最大偏移，单位像素，只要小于这个值，就认为鼠标已经在这个点上了

class ImagePerspectiveTransformation {
public:
	ImagePerspectiveTransformation():o_nDragingCornerPointIdx(INVALID_INDEX), o_blIsNeedPaint(FALSE) {}
	~ImagePerspectiveTransformation(){}
	void process(Mat& mSrcImg, Mat& mResultImg, Mat& mSrcShowImg, DOUBLE dblScaleFactor);

	vector<Point2f>& GetROI(void)
	{		
		return o_vptROI;
	}

	void SetDragingCornerPointIndex(INT nDragingCornerPointIdx)
	{
		o_nDragingCornerPointIdx = nDragingCornerPointIdx;
	}

	INT GetDragingCornerPointIndex(void) const
	{
		return o_nDragingCornerPointIdx;
	}

	//* 设置绘制标记，告诉process()函数有新的点击加入，需要再次绘制角点区域
	void NeedToDrawCornerPoint(void)
	{
		o_blIsNeedPaint = TRUE;
	}

	DOUBLE GetScaleFactor(void)
	{
		return o_dblScaleFactor;
	}

private:
	vector<Point2f> o_vptROI;
	INT o_nDragingCornerPointIdx;	//* 被拖拽的角点索引，这个索引就是其在o_vptROI中的位置
	BOOL o_blIsNeedPaint;
	DOUBLE o_dblScaleFactor;
};

//* 菜单处理函数
typedef void(*PFUN_IMGHANDLER)(void);
typedef struct _ST_IMGHANDLER_ {
	INT nMenuID;
	PFUN_IMGHANDLER pfunHadler;
} ST_IMGHANDLER, *PST_IMGHANDLER;
