#pragma once

#include <map>
#include <unordered_map>

#define VLCPLAYER_DISPLAY_PREDICT_RESULT		0		//* 是否在播放器显示窗口显示预测结果，其实就是显示和预测两个线程分别处理，以提升性能
#define FACE_DETECT_USE_DNNNET					0		//* 使用DNN网络检测人脸
#define VLCPLAYER_DISPLAY_EN					1		//* VLC播放器显示使能

#define FACE_DISAPPEAR_FRAME_NUM				60		//* 判定人脸已经消失的帧数，这个用于释放人脸数据占据的内存
#define MIN_PIXEL_DISTANCE_FOR_NEW_FACE			50		//* 两帧之间只要脸部坐标不超过该数值则认为还是原来的脸
#define FACE_DETECT_MIN_CONFIDENCE_THRESHOLD	0.8f	//* 确定是一张人脸的最低可信度阈值

//* 保存检测到的单张人脸信息，包括坐标、范围等数据
typedef struct _ST_FACE_ {
	UINT unFaceID;			//* 唯一的标识一张脸

	UINT unFrameIdx;		//* 脸部数据对应的帧序号

	//* 脸部坐标位置
	INT nLeftTopX;
	INT nLeftTopY;
	INT nRightBottomX;
	INT nRightBottomY;
	
	string strPersonName;			//* 预测出的人名
	FLOAT flPredictConfidence;		//* 预测置信度
	UINT unFrameIdxForPrediction;	//* 做出预测时的帧序号

	Mat mFace;
} ST_FACE, *PST_FACE;

//* 传给VLC播放器之显示预处理函数的参数
typedef struct _ST_VLCPLAYER_FCBDISPPREPROC_PARAM_ {
	Net *pobjDNNNet;
	unordered_map<UINT, ST_FACE> *pmapFaces;
	THMUTEX *pthMutexMapFaces;
} ST_VLCPLAYER_FCBDISPPREPROC_PARAM, *PST_VLCPLAYER_FCBDISPPREPROC_PARAM;