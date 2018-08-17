#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <vector>
#include "common_lib.h"
#include "MachineVisionLib.h"
#include "MathAlgorithmLib.h"
#include "ImagePreprocess.h"
#include "MVLVideo.h"

void *MVLVideo::FCBLock(void *pvCBInputParam, void **ppvFrameBuf)
{
	EnterThreadMutex(&o_thMutex);


}

void MVLVideo::FCBUnlock(void *pvCBInputParam, void *pvFrameData, void *const *ppvFrameBuf)
{
	ExitThreadMutex(&o_thMutex);
}

//* 打开一个视频文件
BOOL MVLVideo::OpenVideoFromFile(string strFile, ENUM_ASPECT_RATIO enumAspectRatio)
{
	libvlc_media_t *pstVLCMedia;


}

//* 打开一个网络RTSP串流
BOOL MVLVideo::OpenVideoFromeRtsp(string strURL, UINT unNetCachingTime, BOOL blIsUsedTCP, ENUM_ASPECT_RATIO enumAspectRatio)
{

}

