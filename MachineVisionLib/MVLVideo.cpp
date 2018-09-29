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
	"-I"
	, "dummy"
	"--ignore-config" 	
	//, "--rtsp-frame-buffer-size=1500000"	//* 如果不设置这个，vlc会自动寻找一个合适的，建议不设置，只不过启动时会慢一些，如果视频的分辨率过大的话
	//, "--longhelp"						//* 这两个参数可以打开VLC的参数说明，不过内容很多，最好在控制台将其重定向到TXT文件再看
	//, "--advanced"
};

//* VLC回调函数之锁定帧数据缓冲区函数，参数pvCBInputParam是传给回调函数的用户自定义参数
//* pvCBInputParam指定接收帧数据的缓冲区的首地址
void *VLCVideoPlayer::FCBLock(void *pvCBInputParam, void **ppvFrameBuf)
{	
	VLCVideoPlayer *pobjVideoPlayer = (VLCVideoPlayer *)pvCBInputParam;
	//*ppvFrameBuf = pobjVideoPlayer->o_mVideoFrameRGB.data;
	*ppvFrameBuf = pobjVideoPlayer->o_pubaVideoFrameData;		

	//* 获取视频尺寸
	if (!pobjVideoPlayer->o_blIsInited)
	{
		UINT unWidth, unHeight;
		libvlc_video_get_size(pobjVideoPlayer->o_pstVLCMediaPlayer, 0, &unWidth, &unHeight);
		pobjVideoPlayer->o_objOriginalResolution = Size(unWidth, unHeight);

		cout << "Video Size: " << unWidth << " x " << unHeight << " : " << pobjVideoPlayer->o_objOriginalResolution.empty() << endl;
	}	

	return *ppvFrameBuf;
}

//* VLC回调函数之解锁帧数据缓冲区函数，其实pvFrameData和ppvFrameBuf指向的是同一块内存，其值由FCBLock()函数返回
void VLCVideoPlayer::FCBUnlock(void *pvCBInputParam, void *pvFrameData, void* const *ppvFrameBuf)
{
	VLCVideoPlayer *pobjVideoPlayer = (VLCVideoPlayer *)pvCBInputParam;

	if (!pobjVideoPlayer->o_blIsInited)
		return;
	
	Mat mVideoFrameRGB = Mat(pobjVideoPlayer->o_unAdjustedHeight, pobjVideoPlayer->o_unAdjustedWidth, CV_8UC3, pobjVideoPlayer->o_pubaVideoFrameData);
	cvtColor(mVideoFrameRGB, pobjVideoPlayer->o_mVideoFrame, CV_RGB2BGR);

	pobjVideoPlayer->o_unNextFrameIdx++;	
}

//* VLC回调函数之显示函数
void VLCVideoPlayer::FCBDisplay(void *pvCBInputParam, void *pvFrameData)
{
	VLCVideoPlayer *pobjVideoPlayer = (VLCVideoPlayer *)pvCBInputParam;

	if (!pobjVideoPlayer->o_blIsInited)
		return;

	if (pobjVideoPlayer->o_pfcbDispPreprocessor)
	{		
		pobjVideoPlayer->o_pfcbDispPreprocessor(pobjVideoPlayer->o_mVideoFrame, pobjVideoPlayer->o_pvFunCBDispPreprocParam, pobjVideoPlayer->o_unNextFrameIdx);
		imshow(pobjVideoPlayer->o_strDisplayWinName, pobjVideoPlayer->o_mVideoFrame);
	}	
}

Mat VLCVideoPlayer::GetNextFrame(void)
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

//* 获取视频的原始尺寸
BOOL VLCVideoPlayer::__GetOriginalResolution(const CHAR *pszDisplayWinName)
{
	if (!o_pstVLCMediaPlayer)
		return FALSE;

	if(pszDisplayWinName)
		cv2shell::ShowImageWindow(pszDisplayWinName, FALSE);

#define DUMMY_BORDER_LEN	10
	UCHAR ubaDummy[DUMMY_BORDER_LEN * DUMMY_BORDER_LEN * 3];
	o_pubaVideoFrameData = ubaDummy;
	libvlc_video_set_format(o_pstVLCMediaPlayer, "RV24", DUMMY_BORDER_LEN, DUMMY_BORDER_LEN, DUMMY_BORDER_LEN * 3);
	if (libvlc_media_player_play(o_pstVLCMediaPlayer) < 0)
	{
		cout << "The libvlc_media_player_play() function failed, Please confirm that the video address is correct." << endl;

		return FALSE;
	}

	//* 最长等待20秒，以期获得视频原始解析度
	UINT unWaitTimeOut = 0;
	while (unWaitTimeOut < 2000)
	{
		if (!o_objOriginalResolution.empty())
		{
			if (o_objOriginalResolution.width && o_objOriginalResolution.height)
			{
				//* 停止播放
				libvlc_media_player_stop(o_pstVLCMediaPlayer);				

				return TRUE;
			}
		}

		Sleep(10);
		unWaitTimeOut++;
	}
#undef DUMMY_BORDER_LEN

	cout << "The VLCVideoPlayer::GetOriginalResolution() function timeout." << endl;

	return FALSE;
}

//* 设置视频的播放尺寸
void VLCVideoPlayer::__SetPlaybackResolution(const CHAR *pszDisplayWinName)
{
	ENUM_ASPECT_RATIO enumAspectRatio;	

	//* 计算视频长宽比
	if ((o_objOriginalResolution.width / 16) == (o_objOriginalResolution.height / 9))
		enumAspectRatio = AR_16_9;
	else if((o_objOriginalResolution.width / 4) == (o_objOriginalResolution.height / 3))
		enumAspectRatio = AR_4_3;
	else if ((o_objOriginalResolution.width / 5) == (o_objOriginalResolution.height / 3))
		enumAspectRatio = AR_5_3;
	else
		enumAspectRatio = AR_OTHER;

	//* 然后看看用户是否指定了要调整的尺寸
	if (o_unAdjustedWidth)
	{
		switch (enumAspectRatio)
		{
		case AR_16_9:
			o_unAdjustedHeight = (o_unAdjustedWidth / 16) * 9;
			break;

		case AR_4_3:
			o_unAdjustedHeight = (o_unAdjustedWidth / 4) * 3;
			break;

		case AR_5_3:
			o_unAdjustedHeight = (o_unAdjustedWidth / 5) * 3;
			break;

		default:
			o_unAdjustedHeight = (UINT)(((FLOAT)o_objOriginalResolution.height) * (((FLOAT)o_unAdjustedWidth) / ((FLOAT)o_objOriginalResolution.width)));
			break;
		}		
	}
	else
	{
		//* 获取屏幕大小
		INT nPCWidth = GetSystemMetrics(SM_CXSCREEN);
		INT nPCHeight = GetSystemMetrics(SM_CYSCREEN);

		//* 视频高度大于屏幕高度，则缩小至屏幕高度，基本山普通PC都是16：9或4：3或极少数5：3的，所以只要看视频高度是否超出即可，高度不超出，宽度肯定不超出
		if (o_objOriginalResolution.height > nPCHeight)
		{
			o_unAdjustedHeight = nPCHeight;

			switch (enumAspectRatio)
			{
			case AR_16_9:
				o_unAdjustedWidth = (o_unAdjustedHeight / 9) * 16;
				break;

			case AR_4_3:
				o_unAdjustedWidth = (o_unAdjustedHeight / 3) * 4;
				break;

			case AR_5_3:
				o_unAdjustedWidth = (o_unAdjustedHeight / 3) * 5;
				break;

			default:
				o_unAdjustedWidth = (UINT)(((FLOAT)o_objOriginalResolution.width) * (((FLOAT)o_unAdjustedHeight) / ((FLOAT)o_objOriginalResolution.height)));
				break;
			}
		}
		else
		{
			o_unAdjustedWidth = o_objOriginalResolution.width;
			o_unAdjustedHeight = o_objOriginalResolution.height;
		}
	}

	//* 分配视频数据接收缓冲区
	o_unFrameDataBufSize = o_unAdjustedWidth * o_unAdjustedHeight * 3;
	o_pubaVideoFrameData = new UCHAR[o_unFrameDataBufSize];
	libvlc_video_set_format(o_pstVLCMediaPlayer, "RV24", o_unAdjustedWidth, o_unAdjustedHeight, o_unAdjustedWidth * 3);	//* 设定正常的播放格式

	if (pszDisplayWinName)
		cv2shell::ShowImageWindow(pszDisplayWinName, TRUE);

	o_blIsInited = TRUE;
}

//* 打开一个视频文件
BOOL VLCVideoPlayer::OpenVideoFromFile(const CHAR *pszFile, PFCB_DISPLAY_PREPROCESSOR pfcbDispPreprocessor, const CHAR *pszDisplayWinName)
{
	UINT unArgc = sizeof(l_pbaVLCBaseArgs) / sizeof(l_pbaVLCBaseArgs[0]);
	CHAR **ppbaVLCArgs = new CHAR *[unArgc];
	for (UINT i = 0; i < unArgc; i++)	//* 对于播放视频文件，使用缺省参数即可
	{
		ppbaVLCArgs[i] = l_pbaVLCBaseArgs[i];
	}

	o_pstVLCInstance = libvlc_new(unArgc, ppbaVLCArgs);	
	delete[] ppbaVLCArgs;	//* 删除刚才申请的参数缓冲区，libvlc_new()调用后就不用了

	libvlc_media_t* pstVLCMedia = libvlc_media_new_path(o_pstVLCInstance, pszFile);
	o_pstVLCMediaPlayer = libvlc_media_player_new_from_media(pstVLCMedia);
	libvlc_media_release(pstVLCMedia);	//* libvlc_media_player_new_from_media()调用完毕后，释放就可以了

	libvlc_video_set_callbacks(o_pstVLCMediaPlayer, FCBLock, FCBUnlock, FCBDisplay, this);	//* 设定回调函数

	//* 获取视频的原始解析度（尺寸）
	if (!__GetOriginalResolution(pszDisplayWinName))
		return FALSE;
	
	//* 根据用户指定的宽度或显示屏尺寸自动调整视频播放尺寸
	__SetPlaybackResolution(pszDisplayWinName);

	if (pfcbDispPreprocessor)
	{
		o_strDisplayWinName = pszDisplayWinName;
		o_pfcbDispPreprocessor = pfcbDispPreprocessor;
	}

	return TRUE;
}

//* 打开一个网络RTSP串流
BOOL VLCVideoPlayer::OpenVideoFromeRtsp(const CHAR *pszURL, PFCB_DISPLAY_PREPROCESSOR pfcbDispPreprocessor,
									const CHAR *pszDisplayWinName, UINT unNetCachingTime, BOOL blIsUsedTCP)
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
	
	o_pstVLCInstance = libvlc_new(unArgc, ppbaVLCArgs);
	delete[] ppbaVLCArgs;	//* 删除刚才申请的参数缓冲区，libvlc_new()调用后就不用了

	libvlc_media_t* pstVLCMedia = libvlc_media_new_location(o_pstVLCInstance, pszURL);
	o_pstVLCMediaPlayer = libvlc_media_player_new_from_media(pstVLCMedia);
	libvlc_media_release(pstVLCMedia);	//* libvlc_media_player_new_from_media()调用完毕后，释放就可以了

	libvlc_video_set_callbacks(o_pstVLCMediaPlayer, FCBLock, FCBUnlock, FCBDisplay, this);	//* 设定回调函数																													//* 获取视频的原始解析度（尺寸）
	
	//* 获取视频的原始解析度（尺寸）
	if (!__GetOriginalResolution(pszDisplayWinName))
		return FALSE;

	//* 根据用户指定的宽度或显示屏尺寸自动调整视频播放尺寸
	__SetPlaybackResolution(pszDisplayWinName);

	if (pfcbDispPreprocessor)
	{
		o_strDisplayWinName = pszDisplayWinName;
		o_pfcbDispPreprocessor = pfcbDispPreprocessor;
	}

	return TRUE;
}

//* 设置显示预处理函数的输入参数
void VLCVideoPlayer::SetDispPreprocessorInputParam(void *pvParam)
{
	o_pvFunCBDispPreprocParam = pvParam;
}

//* 暂停/恢复当前视频/网络摄像机的播放
void VLCVideoPlayer::pause(BOOL blIsPaused)
{
	libvlc_media_player_set_pause(o_pstVLCMediaPlayer, blIsPaused);
}

//* 停止播放当前视频/网络摄像
void VLCVideoPlayer::stop(void)
{
	libvlc_media_player_stop(o_pstVLCMediaPlayer);

	//* 等待结束
	libvlc_state_t enumVLCPlayState;
	while (TRUE)
	{
		enumVLCPlayState = libvlc_media_player_get_state(o_pstVLCMediaPlayer);
		if (enumVLCPlayState == libvlc_Stopped || enumVLCPlayState == libvlc_Ended || enumVLCPlayState == libvlc_Error)
			break;
	}
}

BOOL VLCVideoPlayer::start(void)
{
	if (libvlc_media_player_play(o_pstVLCMediaPlayer) < 0)
	{
		cout << "libvlc_media_player_play()函数执行失败，请确认视频文件或RTSP流媒体地址正确！" << endl;

		return FALSE;
	}		

	return TRUE;
}

//* 视频文件是否已经播放完毕，这个函数对网络摄像机无效
BOOL VLCVideoPlayer::IsPlayEnd(void)
{
	libvlc_state_t enumVLCPlayState = libvlc_media_player_get_state(o_pstVLCMediaPlayer);	
	if (enumVLCPlayState == libvlc_Ended || enumVLCPlayState == libvlc_Stopped)
		return TRUE;

	return FALSE;
}

//* 释放申请的相关资源
VLCVideoPlayer::~VLCVideoPlayer() {
	if (libvlc_media_player_get_state(o_pstVLCMediaPlayer) != libvlc_Stopped)
		libvlc_media_player_stop(o_pstVLCMediaPlayer);

	libvlc_media_player_release(o_pstVLCMediaPlayer);
	libvlc_release(o_pstVLCInstance);

	//* 注意这个顺序，libvlc库在stop之前一直在使用它们，因此至少是stop后才能释放	
	o_mVideoFrame.release();
	delete[] o_pubaVideoFrameData;

	cout << "VLC player has successfully quit!" << endl;
};
