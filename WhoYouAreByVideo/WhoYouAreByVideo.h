#pragma once

#define SHM_FACE_DB_SIZE_MAX	10				//* 脸库支持保存最多多少张脸
#define SHM_FACE_DB_NAME		"SHM_FACE_DB"	//* 进程间共享内存脸库之名称

//* “脸库”头部数据结构
typedef struct _ST_SHM_FACE_DB_HDR_ {
	UINT unFrameROIWidth;	//* 视频帧ROI区域的宽度
	UINT unFrameROIHeight;	//* 视频帧ROI区域的高度
} ST_SHM_FACE_DB_HDR, *PST_SHM_FACE_DB_HDR;

//* 保存检测到的单张人脸信息，包括坐标、范围等数据
typedef struct _ST_FACE_ {
	UINT unFaceID;			//* 唯一的标识一张脸

	UINT unFrameIdx;		//* 脸部数据对应的帧序号

	string strPersonName;			//* 预测出的人名
	FLOAT flPredictConfidence;		//* 预测置信度
	UINT unFrameIdxForPrediction;	//* 做出预测时的帧序号
	
	Face objFace;					//* 人脸位置坐标
} ST_FACE, *PST_FACE;
