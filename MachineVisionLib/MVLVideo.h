#pragma once
#include <vlc/vlc.h>

#pragma comment(lib,"libvlc.lib")
#pragma comment(lib,"libvlccore.lib")

//* 画面宽高比，即画面比例
typedef enum {
	AR_16_9 = 0,	//* 16:9
	AR_4_3			//* 4:3
} ENUM_ASPECT_RATIO;

class MVLVideo {
public:
	MVLVideo() : o_unVideoWidth(0), 
				 o_unVideoHeight(0), 				 
				 o_unAdjustedWidth(0),				 
				 o_pstVLCInstance(NULL), 
				 o_pstVLCMediaPlayer(NULL) 
	{
		InitThreadMutex(&o_thMutex);
	};

	//* 手动指定固定宽度
	MVLVideo(UINT unAdjustedWidth) : o_unVideoWidth(0), 
									 o_unVideoHeight(0), 									 
									 o_pstVLCInstance(NULL), 
									 o_pstVLCMediaPlayer(NULL)
	{
		o_unAdjustedWidth = unAdjustedWidth;
		InitThreadMutex(&o_thMutex);
	};

	//* 释放申请的相关资源
	~MVLVideo() {
		o_mVideoFrame.release();
		libvlc_media_player_stop(o_pstVLCMediaPlayer);
		libvlc_media_player_release(o_pstVLCMediaPlayer);
		libvlc_release(o_pstVLCInstance);

		UninitThreadMutex(&o_thMutex);
	};

	//* 参数unCachingTime为VLC播放缓存的时间，也就是缓存一段时间的视频再开始播放，单位为毫秒
	BOOL OpenVideoFromFile(string strFile, ENUM_ASPECT_RATIO enumAspectRatio = AR_16_9);
	BOOL OpenVideoFromeRtsp(string strURL, UINT unNetCachingTime = 200, BOOL blIsUsedTCP = FALSE, ENUM_ASPECT_RATIO enumAspectRatio = AR_16_9);

private:
	void *FCBLock(void *pvCBInputParam, void **ppvFrameBuf);							//* VLC回调函数之锁定帧数据缓冲区函数，参数pvCBInputParam是传给回调函数的用户自定义参数
	void FCBUnlock(void *pvCBInputParam, void *pvFrameData, void *const *ppvFrameBuf);	//* VLC回调函数之解锁帧数据缓冲区函数，其实pvFrameData和ppvFrameBuf指向的是同一块内存，其值由FCBLock()函数返回	

	libvlc_instance_t *o_pstVLCInstance;			//* VLC实例
	libvlc_media_player_t *o_pstVLCMediaPlayer;		//* VLC播放器

	THMUTEX o_thMutex;

	Mat o_mVideoFrame;			//* 读取到的视频帧	

	UINT o_unVideoWidth;		//* 视频帧的原始宽度
	UINT o_unVideoHeight;		//* 视频帧的原始高度

	UINT o_unAdjustedWidth;		//* 调用者指定要调整到的图像宽度，系统将据此调整图像的分辨率到该宽度，图像高度按原有比例调整
	UINT o_unAdjustedHeight;	

	//* 关于VLC参数的详细说明参见：https://blog.csdn.net/jxbinwd/article/details/72722833
	CHAR *o_pbaVLCArgs[] =
	{
		"-I",
		"dummy",
		"--ignore-config"
		//"--longhelp",		//* 这两个参数可以打开VLC的参数说明，不过内容很多，最好在控制台将其重定向到TXT文件再看
		//"--advanced",
		//"--rtsp-frame-buffer-size=1500000",	//* 如果不设置这个，vlc会自动寻找一个更合适，建议不设置，只不过启动时会慢一些，如果视频的分辨率过大的话
	};
};