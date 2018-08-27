#pragma once
#ifdef MACHINEVISIONLIB_EXPORTS
#define MVLVIDEO __declspec(dllexport)
#else
#define MVLVIDEO __declspec(dllimport)
#endif

#include <vlc/vlc.h>

#pragma comment(lib,"libvlc.lib")
#pragma comment(lib,"libvlccore.lib")

#define DEFAULT_VIDEO_FRAME_WIDTH	1280	//* 缺省帧宽

typedef void(*PFCB_DISPLAY_PREPROCESSOR)(Mat& mVideoFrame, void *pvParam, UINT unCurFrameIdx);

//* 画面宽高比，即画面比例
typedef enum {
	AR_16_9 = 0,	//* 16:9
	AR_4_3			//* 4:3
} ENUM_ASPECT_RATIO;

class MVLVIDEO VLCVideoPlayer {
public:
	VLCVideoPlayer() : o_unAdjustedWidth(0),
					   o_pstVLCInstance(NULL), 
					   o_pstVLCMediaPlayer(NULL), 
					   o_unNextFrameIdx(0), 
					   o_unPrevFrameIdx(0)
	{
	};

	//* 手动指定固定宽度
	VLCVideoPlayer(UINT unAdjustedWidth) : o_pstVLCInstance(NULL),
										   o_pstVLCMediaPlayer(NULL), 
										   o_unNextFrameIdx(0), 
										   o_unPrevFrameIdx(0)
	{
		o_unAdjustedWidth = unAdjustedWidth;		
	};

	//* 释放申请的相关资源
	~VLCVideoPlayer();
	
	void OpenVideoFromFile(const CHAR *pszFile, PFCB_DISPLAY_PREPROCESSOR pfcbDispPreprocessor, const CHAR *pszDisplayWinName, ENUM_ASPECT_RATIO enumAspectRatio = AR_16_9);
	void OpenVideoFromeRtsp(const CHAR *pszURL, PFCB_DISPLAY_PREPROCESSOR pfcbDispPreprocessor,
							const CHAR *pszDisplayWinName, UINT unNetCachingTime = 200, 
							BOOL blIsUsedTCP = FALSE, ENUM_ASPECT_RATIO enumAspectRatio = AR_16_9);	//* 参数unNetCachingTime为VLC播放缓存的时间，也就是缓存一段时间的视频再开始播放，单位为毫秒

	void SetDispPreprocessorInputParam(void *pvParam);

	Mat GetNextFrame(void);
	UINT GetCurFrameIndex(void)
	{
		return o_unNextFrameIdx;
	}
	
	BOOL start(void);				//* 开始播放
	void pause(BOOL blIsPaused);	//* 暂停播放
	void stop(void);				//* 关闭视频播放
	BOOL IsPlayEnd(void);			//* 是否播送完毕

private:
	static void *FCBLock(void *pvCBInputParam, void **ppvFrameBuf);								//* VLC回调函数之锁定帧数据缓冲区函数，参数pvCBInputParam是传给回调函数的用户自定义参数
	static void FCBUnlock(void *pvCBInputParam, void *pvFrameData, void *const *ppvFrameBuf);	//* VLC回调函数之解锁帧数据缓冲区函数，其实pvFrameData和ppvFrameBuf指向的是同一块内存，其值由FCBLock()函数返回
	static void FCBDisplay(void *pvCBInputParam, void *pvFrameData);							//* VLC回调函数之显示函数

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
};