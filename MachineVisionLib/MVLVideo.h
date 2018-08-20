#pragma once
#ifdef MACHINEVISIONLIB_EXPORTS
#define MVLVIDEO __declspec(dllexport)
#else
#define MVLVIDEO __declspec(dllimport)
#endif

#include <vlc/vlc.h>

#pragma comment(lib,"libvlc.lib")
#pragma comment(lib,"libvlccore.lib")

#define DEFAULT_VIDEO_FRAME_WIDTH	960	//* 缺省帧宽

typedef void(*PFCB_DISPLAY_PREPROCESSOR)(Mat& mVideoFrame);

//* 画面宽高比，即画面比例
typedef enum {
	AR_16_9 = 0,	//* 16:9
	AR_4_3			//* 4:3
} ENUM_ASPECT_RATIO;

class MVLVIDEO MVLVideo {
public:
	MVLVideo() : o_unAdjustedWidth(0),				 
				 o_pstVLCInstance(NULL), 
				 o_pstVLCMediaPlayer(NULL), 
				 o_unNextFrameIdx(0), 
				 o_unPrevFrameIdx(0)
	{
	};

	//* 手动指定固定宽度
	MVLVideo(UINT unAdjustedWidth) : o_pstVLCInstance(NULL), 
									 o_pstVLCMediaPlayer(NULL), 
									 o_unNextFrameIdx(0), 
									 o_unPrevFrameIdx(0)
	{
		o_unAdjustedWidth = unAdjustedWidth;		
	};

	//* 释放申请的相关资源
	~MVLVideo() {
		o_mVideoFrame.release();
		libvlc_media_player_stop(o_pstVLCMediaPlayer);
		libvlc_media_player_release(o_pstVLCMediaPlayer);
		libvlc_release(o_pstVLCInstance);		
	};
	
	BOOL OpenVideoFromFile(const CHAR *pszFile, PFCB_DISPLAY_PREPROCESSOR pfcbDispPreprocessor, const CHAR *pszDisplayWinName, ENUM_ASPECT_RATIO enumAspectRatio = AR_16_9);
	BOOL OpenVideoFromeRtsp(const CHAR *pszURL, PFCB_DISPLAY_PREPROCESSOR pfcbDispPreprocessor,
							const CHAR *pszDisplayWinName, UINT unNetCachingTime = 200, 
							BOOL blIsUsedTCP = FALSE, ENUM_ASPECT_RATIO enumAspectRatio = AR_16_9);	//* 参数unNetCachingTime为VLC播放缓存的时间，也就是缓存一段时间的视频再开始播放，单位为毫秒

	Mat GetNextFrame(void);
	

private:
	static void *FCBLock(void *pvCBInputParam, void **ppvFrameBuf);								//* VLC回调函数之锁定帧数据缓冲区函数，参数pvCBInputParam是传给回调函数的用户自定义参数
	static void FCBUnlock(void *pvCBInputParam, void *pvFrameData, void *const *ppvFrameBuf);	//* VLC回调函数之解锁帧数据缓冲区函数，其实pvFrameData和ppvFrameBuf指向的是同一块内存，其值由FCBLock()函数返回
	static void FCBDisplay(void *pvCBInputParam, void *pvFrameData);							//* VLC回调函数之显示函数

	libvlc_instance_t *o_pstVLCInstance;			//* VLC实例
	libvlc_media_player_t *o_pstVLCMediaPlayer;		//* VLC播放器

	Mat o_mVideoFrame;			//* 读取到的视频帧
	Mat o_mDisplayFrame;		//* 用于显示的帧数据

	PFCB_DISPLAY_PREPROCESSOR o_pfcbDispPreprocessor;
	string o_strDisplayWinName;	//* 显示显示窗口的名称

	UINT o_unNextFrameIdx;
	UINT o_unPrevFrameIdx;

	UINT o_unAdjustedWidth;		//* 调用者指定要调整到的图像宽度，系统将据此调整图像的分辨率到该宽度，图像高度按指定画面比例调整
	UINT o_unAdjustedHeight;
};