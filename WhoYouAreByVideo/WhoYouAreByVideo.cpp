// WhoYouAreByVideo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <io.h>
#include <vector>
#include "common_lib.h"
#include "MachineVisionLib.h"
#include "FaceRecognition.h"
#include "MVLVideo.h"
#include "WhoYouAreByVideo.h"

static HANDLE hSHMFaceDB;
static CHAR *pbSHMFaceDB;

static BOOL blIsRunning;
BOOL WINAPI ConsoleCtrlHandler(DWORD dwEvent)
{
	blIsRunning = FALSE;
	printf("The process receives the CTL+C signal and will exit.\r\n");
	return TRUE;
}

//* 主进程初始化
static BOOL __MainProcInit(void)
{
	UINT unFrameWidth = DEFAULT_VIDEO_FRAME_WIDTH;
	UINT unFrameHeight = (unFrameWidth / 16) * 9;
	if (unFrameWidth > mVideoFrame.rows)
		mROI = mVideoFrame(Rect((mVideoFrame.cols - mVideoFrame.rows) / 2, 0, mVideoFrame.rows, mVideoFrame.rows));
	else if (mVideoFrame.cols < mVideoFrame.rows)
		mROI = mVideoFrame(Rect(0, (mVideoFrame.rows - mVideoFrame.cols) / 2, mVideoFrame.cols, mVideoFrame.cols));
	else
		mROI = mVideoFrame;

	if (NULL == (pbSHMFaceDB = IPCCreateSHM(SHM_FACE_DB_NAME,
		sizeof(ST_SHM_FACE_DB_HDR) + SHM_FACE_DB_SIZE_MAX * sizeof(ST_FACE),
		&hSHMFaceDB)))
		return 0;

	if (INVALID_PROC_ID == (dwSubProcID = StartProcess("WhoYouAreByVideo.exe", NULL)))
	{
		cout << "Sub process startup failure for face recognition, the process " << argv[0] << " exit!" << endl;
		return 0;
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	DWORD dwSubProcID;

	if (argc != 1 && argc != 2)
	{
		cout << "Usage: " << argv[0] << " [rtsp address]" << endl;
		cout << "Usage: " << argv[0] << " [video file path]" << endl;

		return 0;
	}

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleCtrlHandler, TRUE);

	blIsRunning = TRUE;

	//* 建立“脸库”
	if (argc == 2)
	{
		

		sprintf(pbSHMFaceDB, "Test123456");
		cout << "put Test123456" << endl;
		while (blIsRunning)
		{
			Sleep(1000);
		}

		cout << "Main process exit!" << endl;
	}
	else
	{
		if(NULL == (pbSHMFaceDB = IPCOpenSHM(SHM_FACE_DB_NAME, &hSHMFaceDB)))		
			return 0;
		
		while (blIsRunning)
		{			
			
			cout << "Get value: " << pbSHMFaceDB << endl;
			Sleep(1000);
		}

		cout << "Sub process exit!" << endl;
	}

	//* 关闭“脸库”
	IPCCloseSHM(pbSHMFaceDB, hSHMFaceDB);

    return 0;
}

