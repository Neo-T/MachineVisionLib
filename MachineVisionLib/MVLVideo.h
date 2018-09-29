#pragma once
#ifdef MACHINEVISIONLIB_EXPORTS
#define MVLVIDEO __declspec(dllexport)
#else
#define MVLVIDEO __declspec(dllimport)
#endif

#include <vlc/vlc.h>

#pragma comment(lib,"libvlc.lib")
#pragma comment(lib,"libvlccore.lib")

//* 画面宽高比，即画面比例
typedef enum {
	AR_16_9 = 0,	//* 16:9
	AR_4_3, 		//* 4:3
	AR_5_3,			//* 5:3
	AR_OTHER
} ENUM_ASPECT_RATIO;

typedef void(*PFCB_DISPLAY_PREPROCESSOR)(Mat& mVideoFrame, void *pvParam, UINT unCurFrameIdx);

class MVLVIDEO VLCVideoPlayer {
public:
	VLCVideoPlayer() : o_unAdjustedWidth(0),
					   o_pstVLCInstance(NULL), 
					   o_pstVLCMediaPlayer(NULL), 
					   o_unNextFrameIdx(0), 
					   o_unPrevFrameIdx(0), 
					   o_pfcbDispPreprocessor(NULL), 
					   o_blIsInited(FALSE)
	{		
	};

	//* 手动指定固定宽度
	VLCVideoPlayer(UINT unAdjustedWidth) : o_pstVLCInstance(NULL),
										   o_pstVLCMediaPlayer(NULL), 
										   o_unNextFrameIdx(0), 
										   o_unPrevFrameIdx(0), 
										   o_pfcbDispPreprocessor(NULL), 
										   o_blIsInited(FALSE), 
										   o_unAdjustedWidth(unAdjustedWidth)
	{
	};

	//* 释放申请的相关资源
	~VLCVideoPlayer();
	BOOL OpenVideoFromFile(const CHAR *pszFile, PFCB_DISPLAY_PREPROCESSOR pfcbDispPreprocessor, const CHAR *pszDisplayWinName);
	BOOL OpenVideoFromeRtsp(const CHAR *pszURL, PFCB_DISPLAY_PREPROCESSOR pfcbDispPreprocessor,
							const CHAR *pszDisplayWinName, UINT unNetCachingTime = 200, BOOL blIsUsedTCP = FALSE);	//* 参数unNetCachingTime为VLC播放缓存的时间，也就是缓存一段时间的视频再开始播放，单位为毫秒

	void SetDispPreprocessorInputParam(void *pvParam);

	Mat GetNextFrame(void);
	UINT GetCurFrameIndex(void) const
	{
		return o_unNextFrameIdx;
	}

	Size GetVideoResolution(void) const
	{
		return Size(o_unAdjustedWidth, o_unAdjustedHeight);
	}
	
	BOOL start(void);				//* 开始播放
	void pause(BOOL blIsPaused);	//* 暂停播放
	void stop(void);				//* 关闭视频播放
	BOOL IsPlayEnd(void);			//* 是否播送完毕

private:
	static void *FCBLock(void *pvCBInputParam, void **ppvFrameBuf);								//* VLC回调函数之锁定帧数据缓冲区函数，参数pvCBInputParam是传给回调函数的用户自定义参数
	static void FCBUnlock(void *pvCBInputParam, void *pvFrameData, void *const *ppvFrameBuf);	//* VLC回调函数之解锁帧数据缓冲区函数，其实pvFrameData和ppvFrameBuf指向的是同一块内存，其值由FCBLock()函数返回
	static void FCBDisplay(void *pvCBInputParam, void *pvFrameData);							//* VLC回调函数之显示函数

	BOOL __GetOriginalResolution(const CHAR *pszDisplayWinName);	//* 获取视频的原始尺寸
	void __SetPlaybackResolution(const CHAR *pszDisplayWinName);	//* 设置视频的播放尺寸

	libvlc_instance_t *o_pstVLCInstance;			//* VLC实例
	libvlc_media_player_t *o_pstVLCMediaPlayer;		//* VLC播放器

	UCHAR *o_pubaVideoFrameData;	//* 视频帧数据缓冲区
	UINT o_unFrameDataBufSize;		//* 帧数据缓冲区容量

	Mat o_mVideoFrame;				//* Opencv视频帧

	PFCB_DISPLAY_PREPROCESSOR o_pfcbDispPreprocessor;	//* 视频显示前的预处理函数
	string o_strDisplayWinName;							//* 显示显示窗口的名称
	void *o_pvFunCBDispPreprocParam;					//* 传递给预处理函数的参数

	UINT o_unNextFrameIdx;
	UINT o_unPrevFrameIdx;

	UINT o_unAdjustedWidth;		//* 调用者指定要调整到的图像宽度，系统将据此调整图像的分辨率到该宽度，图像高度按指定画面比例调整
	UINT o_unAdjustedHeight;	

	Size o_objOriginalResolution;	//* 视频尺寸

	BOOL o_blIsInited;				//* 是否已初始化完毕
};