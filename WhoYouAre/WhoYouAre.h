#pragma once

#include <map>
#include <unordered_map>

#define FACE_DETECT_MIN_CONFIDENCE_THRESHOLD	0.8f	//* 确定是一张人脸的最低可信度阈值

//* 传给VLC播放器之显示预处理函数的参数
typedef struct _ST_VLCPLAYER_FCBDISPPREPROC_PARAM_ {
	Net *pobjDNNNet;
	FaceDatabase *pobjFaceDB;
} ST_VLCPLAYER_FCBDISPPREPROC_PARAM, *PST_VLCPLAYER_FCBDISPPREPROC_PARAM;