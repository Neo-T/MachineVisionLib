#pragma once

class ImagePerspectiveTransformation {
public:
	ImagePerspectiveTransformation():nDragingCornerPointIdx(INVALID_INDEX){}
	~ImagePerspectiveTransformation(){}
	void process(Mat& mSrcImg, Mat& mResutImg);

private:
	vector<Point2f> o_vptROI;
	INT nDragingCornerPointIdx;	//* 被拖拽的角点索引，这个索引就是其在o_vptROI中的位置
};
