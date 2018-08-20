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

//* 关于VLC参数的详细说明参见：https://blog.csdn.net/jxbinwd/article/details/72722833
static CHAR* const l_pbaVLCBaseArgs[] =	//* const确保o_pbaVLCArg本身的值也就是地址不会被修改	
{
	"-I",
	"dummy",
	"--ignore-config"
	//"--longhelp",		//* 这两个参数可以打开VLC的参数说明，不过内容很多，最好在控制台将其重定向到TXT文件再看
	//"--advanced",
	//"--rtsp-frame-buffer-size=1500000",	//* 如果不设置这个，vlc会自动寻找一个合适的，建议不设置，只不过启动时会慢一些，如果视频的分辨率过大的话
};

//* VLC回调函数之锁定帧数据缓冲区函数，参数pvCBInputParam是传给回调函数的用户自定义参数
//* pvCBInputParam指定接收帧数据的缓冲区的首地址
void *MVLVideo::FCBLock(void *pvCBInputParam, void **ppvFrameBuf)
{	
	MVLVideo *pobjMVLVideo = (MVLVideo *)pvCBInputParam;
	*ppvFrameBuf = pobjMVLVideo->o_mVideoFrame.data;

	return *ppvFrameBuf;
}

//* VLC回调函数之解锁帧数据缓冲区函数，其实pvFrameData和ppvFrameBuf指向的是同一块内存，其值由FCBLock()函数返回
void MVLVideo::FCBUnlock(void *pvCBInputParam, void *pvFrameData, void* const *ppvFrameBuf)
{
	MVLVideo *pobjMVLVideo = (MVLVideo *)pvCBInputParam;

	pobjMVLVideo->o_mVideoFrame.copyTo(pobjMVLVideo->o_mDisplayFrame);
	pobjMVLVideo->o_unNextFrameIdx++;
}

//* VLC回调函数之显示函数
void MVLVideo::FCBDisplay(void *pvCBInputParam, void *pvFrameData)
{
	MVLVideo *pobjMVLVideo = (MVLVideo *)pvCBInputParam;

	if (pobjMVLVideo->o_pfcbDispPreprocessor)
	{
		pobjMVLVideo->o_pfcbDispPreprocessor(pobjMVLVideo->o_mDisplayFrame);
		imshow(pobjMVLVideo->o_strDisplayWinName, pobjMVLVideo->o_mDisplayFrame);
	}
}

Mat MVLVideo::GetNextFrame(void)
{
	//* 看看下一帧数据是否已经到达
	if (o_unPrevFrameIdx != o_unNextFrameIdx)
	{
		o_unPrevFrameIdx = o_unNextFrameIdx;
		return o_mVideoFrame;
	}
	else
	{
		Mat mDummy;
		return mDummy;
	}
}

//* 打开一个视频文件
BOOL MVLVideo::OpenVideoFromFile(const CHAR *pszFile, PFCB_DISPLAY_PREPROCESSOR pfcbDispPreprocessor, 
									const CHAR *pszDisplayWinName, ENUM_ASPECT_RATIO enumAspectRatio)
{
	UINT unArgc = sizeof(l_pbaVLCBaseArgs) / sizeof(l_pbaVLCBaseArgs[0]);
	CHAR **ppbaVLCArgs = new CHAR *[unArgc];
	for (UINT i = 0; i < unArgc; i++)	//* 对于播放视频文件，使用缺省参数即可
	{
		ppbaVLCArgs[i] = l_pbaVLCBaseArgs[i];
	}
	
	//* 初始化视频帧Matrix
	if (o_unAdjustedWidth)	
		o_unAdjustedWidth = DEFAULT_VIDEO_FRAME_WIDTH;
	if (enumAspectRatio == AR_16_9)	
		o_unAdjustedHeight = (o_unAdjustedWidth / 16) * 9;
	else
		o_unAdjustedHeight = (o_unAdjustedWidth / 4) * 3;
	o_mVideoFrame = Mat(Size(o_unAdjustedWidth, o_unAdjustedHeight), CV_8UC3);

	o_pstVLCInstance = libvlc_new(unArgc, ppbaVLCArgs);	
	delete[] ppbaVLCArgs;	//* 删除刚才申请的参数缓冲区，libvlc_new()调用后就不用了

	libvlc_media_t* pstVLCMedia = libvlc_media_new_path(o_pstVLCInstance, pszFile);
	o_pstVLCMediaPlayer = libvlc_media_player_new_from_media(pstVLCMedia);
	libvlc_media_release(pstVLCMedia);	//* libvlc_media_player_new_from_media()调用完毕后，释放就可以了

	libvlc_video_set_callbacks(o_pstVLCMediaPlayer, FCBLock, FCBUnlock, FCBDisplay, NULL);								//* 设定回调函数
	libvlc_video_set_format(o_pstVLCMediaPlayer, "RV24", o_unAdjustedWidth, o_unAdjustedHeight, o_unAdjustedWidth * 3);	//* 设定播放格式

	if(libvlc_media_player_play(o_pstVLCMediaPlayer) < 0)
		return FALSE;

	o_strDisplayWinName = pszDisplayWinName;
	o_pfcbDispPreprocessor = pfcbDispPreprocessor;

	return TRUE;
}

//* 打开一个网络RTSP串流
BOOL MVLVideo::OpenVideoFromeRtsp(const CHAR *pszURL, PFCB_DISPLAY_PREPROCESSOR pfcbDispPreprocessor,
									const CHAR *pszDisplayWinName, UINT unNetCachingTime, 
									BOOL blIsUsedTCP, ENUM_ASPECT_RATIO enumAspectRatio)
{
	UINT unBaseArgc = sizeof(l_pbaVLCBaseArgs) / sizeof(l_pbaVLCBaseArgs[0]);
	UINT unArgc;
	CHAR szNetCachingTime[30];
	CHAR **ppbaVLCArgs = new CHAR *[unBaseArgc + 2];

	sprintf_s(szNetCachingTime, "--network-caching=%d", unNetCachingTime);
	ppbaVLCArgs[unBaseArgc] = szNetCachingTime;
	if (blIsUsedTCP)
	{
		ppbaVLCArgs[unBaseArgc + 1] = "--rtsp-tcp";
		unArgc = unBaseArgc + 2;
	}
	else
		unArgc = unBaseArgc + 1;

	for (UINT i = 0; i < unBaseArgc; i++)	//* 对于播放视频文件，使用缺省参数即可
	{
		ppbaVLCArgs[i] = l_pbaVLCBaseArgs[i];
	}


	//* 初始化视频帧Matrix
	if (!o_unAdjustedWidth)
		o_unAdjustedWidth = DEFAULT_VIDEO_FRAME_WIDTH;
	if (enumAspectRatio == AR_16_9)
		o_unAdjustedHeight = (o_unAdjustedWidth / 16) * 9;
	else
		o_unAdjustedHeight = (o_unAdjustedWidth / 4) * 3;
	o_mVideoFrame = Mat(Size(o_unAdjustedWidth, o_unAdjustedHeight), CV_8UC3);

	cout << o_unAdjustedWidth << " * " << o_unAdjustedHeight << endl;

	o_pstVLCInstance = libvlc_new(unArgc, ppbaVLCArgs);
	delete[] ppbaVLCArgs;	//* 删除刚才申请的参数缓冲区，libvlc_new()调用后就不用了

	libvlc_media_t* pstVLCMedia = libvlc_media_new_location(o_pstVLCInstance, pszURL);
	o_pstVLCMediaPlayer = libvlc_media_player_new_from_media(pstVLCMedia);
	libvlc_media_release(pstVLCMedia);	//* libvlc_media_player_new_from_media()调用完毕后，释放就可以了

	libvlc_video_set_callbacks(o_pstVLCMediaPlayer, FCBLock, FCBUnlock, FCBDisplay, this);								//* 设定回调函数
	libvlc_video_set_format(o_pstVLCMediaPlayer, "RV24", o_unAdjustedWidth, o_unAdjustedHeight, o_unAdjustedWidth * 3);	//* 设定播放格式

	if (libvlc_media_player_play(o_pstVLCMediaPlayer) < 0)
		return FALSE;

	o_strDisplayWinName = pszDisplayWinName;
	o_pfcbDispPreprocessor = pfcbDispPreprocessor;

	return TRUE;
}

