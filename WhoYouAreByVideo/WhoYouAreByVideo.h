#pragma once

#include <map>
#include <unordered_map>

#define SHM_FACE_DB_SIZE_MAX	30						//* 脸库支持保存最多多少张脸
#define SHM_FACE_DB_NAME		"SHM_FACE_DB"			//* 进程间共享内存脸库之名称
#define IPC_MUTEX_FACEDB_NAME	"IPC_MUTEX_FACEDB"		//* 脸库读写锁

#define INVALID_FACE_NODE_INDEX	0xFFFF					//* 无效的脸部节点索引

#define MIN_PIXEL_DISTANCE_FOR_NEW_FACE			50		//* 两帧之间只要脸部坐标不超过该数值则认为还是原来的脸
#define FACE_DETECT_MIN_CONFIDENCE_THRESHOLD	0.5f	//* 确定是一张人脸的最低可信度阈值

//* 传给播放器之显示预处理函数的参数
typedef struct _ST_PLAYER_FCBDISPPREPROC_PARAM_ {
	Net *pobjDNNNet;	
} ST_PLAYER_FCBDISPPREPROC_PARAM, *PST_PLAYER_FCBDISPPREPROC_PARAM;

//* “脸库”头部数据结构
typedef struct _ST_SHM_FACE_DB_HDR_ {
	USHORT usFaceLink;				//* “脸”双向链表
	USHORT usFreeLink;				//* 空闲链表
	struct {
		UINT x;
		UINT y;
		UINT unWidth;
		UINT unHeight;
	} stROIRect;
	UINT unFrameROIDataBytes;		//* 保存当前视频帧ROI区域数据的共享内存的大小
	BOOL blIsSubProcInitOK;			//* 子进程是否已初始化完成
} ST_SHM_FACE_DB_HDR, *PST_SHM_FACE_DB_HDR;

//* 保存检测到的单张人脸信息，包括坐标、范围等数据
typedef struct _ST_FACE_ {
	USHORT usPrevNode;		//* 建立“脸”的双向链表需要的前后节点地址
	USHORT usNextNode;

	UINT unFaceID;			//* 唯一的标识一张脸

	UINT unFrameIdx;		//* 脸部数据对应的帧序号

	string strPersonName;			//* 预测出的人名
	FLOAT flPredictConfidence;		//* 预测置信度
	UINT unFrameIdxForPrediction;	//* 做出预测时的帧序号
	BOOL blIsPredicted;				//* 是否已作出预测
									
	//* 人脸位置坐标
	INT nLeftTopX;
	INT nLeftTopY;
	INT nRightBottomX;
	INT nRightBottomY;
} ST_FACE, *PST_FACE;
