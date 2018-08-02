// ImageMagic.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include <Commdlg.h>
#include <shellapi.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <vector>
#include "common_lib.h"
#include "MachineVisionLib.h"
#include "ImageHandler.h"
#include "ImageMagic.h"

//* 进制_s之类的安全警告
#pragma warning(disable:4996)

#define MAX_LOADSTRING 100

HINSTANCE hInst;					//* 当前实例
HWND hWndMain;						//* 主窗口句柄
HWND hWndStatic;					//* 静态文本框，用于OCV显示窗口的父窗口
CHAR szTitle[MAX_LOADSTRING];		//* 标题栏文本
CHAR szMainWindowTitle[MAX_LOADSTRING + MAX_PATH];	//* 运行状态程序标题栏
CHAR szWindowClass[MAX_LOADSTRING];	//* 主窗口类名
CHAR szImgFileName[MAX_PATH + 1];	//* 打开的图像文件名
Image *pobjOpenedImage;				//* 当前正在编辑的图像

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void CreateTextStatic(void);

//* 相关业务函数
//* ----------------------------------------------------------------------
BOOL GetImageFile(CHAR *pszImgFileName, UINT unImgFileNameSize);	//* 选择一个图像文件
void OpenImgeFile(CHAR *pszImgFileName);							//* 打开并在主窗口绘制该图片
void SetWindowSize(INT nWidth, INT nHeight);						//* 调整窗口大小
void DrawImage(void);												//* 绘制图片
void ForcedUpdateCleintRect(HWND hWnd);								//* 强制更新指定窗口的用户交互区域
void EnableMenuItemForEdit(void);									//* 使能编辑菜单
void SaveEditedImg(void);

//* 为图像处理菜单提供的功能入口函数
void ImageHandler(INT nMenuID);

//*	图像处理相关的函数
void IMGHANDLER_Transformation(void);		//* 透视变换
//* ----------------------------------------------------------------------

//* 图像处理相关的菜单ID数组
static INT naImgHandleMenuList[] = { 
	IDM_TRANSFORMATION, 
	IDM_MAP, 
	IDM_SEPARATE, 
	IDM_CHANGE_BG 
};

//* 储量处理相关的函数均放到此数组中
static ST_IMGHANDLER staImgHandlerList[] = {
	{ IDM_TRANSFORMATION , IMGHANDLER_Transformation }
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此放置代码。

    // 初始化全局字符串
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_IMAGEMAGIC, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

#if OPEN_DEBUG_CONSOLE
	if (!AllocConsole())
	{
		MessageBox(NULL, "无法为进程分配一个调试控制台，请确认该进程拥有相关权限，或者联系Neo！", "错误", MB_ICONERROR | MB_OK);

		return FALSE;
	}

	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
#endif

    // 执行应用程序初始化: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

	hWndStatic = NULL;
	pobjOpenedImage = nullptr;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_IMAGEMAGIC));

    MSG msg;

    // 主消息循环: 
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

	if (pobjOpenedImage)
		delete pobjOpenedImage;

#if OPEN_DEBUG_CONSOLE	
	FreeConsole();
#endif

    return (int) msg.wParam;
}

//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IMAGEMAGIC));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_IMAGEMAGIC);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   hWndMain = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWndMain)
   {
      return FALSE;
   }

   //* 使能窗口接收文件拖放
   DragAcceptFiles(hWndMain, TRUE);

   ShowWindow(hWndMain, nCmdShow);
   UpdateWindow(hWndMain);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {	
	//* 屏幕分辨率更改通知
	case WM_DISPLAYCHANGE:
		DrawImage();
		break;

	//* 文件拖动通知
	case WM_DROPFILES:
		SaveEditedImg();
		DragQueryFile((HDROP)wParam, 0, szImgFileName, sizeof(szImgFileName));
		OpenImgeFile(szImgFileName);
		DragFinish((HDROP)wParam);
		break;

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择: 
            switch (wmId)
            {
			case IDM_OPEN:
				SaveEditedImg();

				if (GetImageFile(szImgFileName, sizeof(szImgFileName)))
				{
					//* 等比例显示图片，超大图片会等比例缩小，小图则维持1：1比例
					OpenImgeFile(szImgFileName);
				}
				break;

			case IDM_SAVE:
				break;

			//case IDM_TRANSFORMATION:
			//case IDM_MAP:
			//case IDM_SEPARATE:
			//case IDM_CHANGE_BG:
			//	ImageHandler(wmId);
			//	break;

            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;

            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;

            default:
				ImageHandler(wmId);
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
			DrawImage();
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		SaveEditedImg();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

//* 建立一个静态文本区域
void CreateTextStatic(void)
{
	if (hWndStatic != NULL)
	{
		destroyAllWindows();
		ShowWindow(hWndStatic, SW_HIDE);
		DestroyWindow(hWndStatic);		
	}

	RECT rect_client;

	GetClientRect(hWndMain, &rect_client);
	
	hWndStatic = CreateWindow(TEXT("STATIC"), TEXT(""),
								WS_CHILD | WS_VISIBLE | SS_SUNKEN | SS_CENTERIMAGE | WS_EX_STATICEDGE,
								0, 0, rect_client.right, rect_client.bottom,
								hWndMain,
								(HMENU)NULL,
								hInst, NULL);	

	namedWindow(szMainWindowTitle, WINDOW_NORMAL);

	HWND hWndOCVImgShow = (HWND)cvGetWindowHandle(szMainWindowTitle);
	HWND hWndParentOCVImgShow = GetParent(hWndOCVImgShow);
	SetParent(hWndOCVImgShow, hWndStatic);		//* 将父窗口切换为STATIC控件
	ShowWindow(hWndParentOCVImgShow, SW_HIDE);	//* 隐藏OpenCV的自有窗口
}

//* 选择一个要编辑的图像文件，参数pszImgFileName保存图像文件的路径及名称，unImgFileNameSize指定pszImgFileName缓冲区的大小，单位：字节
BOOL GetImageFile(CHAR *pszImgFileName, UINT unImgFileNameSize)
{
	OPENFILENAME ofn;

	//* Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWndMain;
	ofn.lpstrFile = pszImgFileName;
	ofn.lpstrFile[0] = '\0';	//* 设置为0，则不使用pszImagFileName中的内容作为初始路径

	ofn.nMaxFile = unImgFileNameSize;
	ofn.lpstrFilter = _T("Jpeg Files (*.jpg)\0*.jpg\0PNG Files (*.png)\0*.png\0Bitmap Files (*.bmp)\0*.bmp\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.lpstrTitle = _T("打开");

	return GetOpenFileName(&ofn);
}

//* 打开并在主窗口绘制该图片
void OpenImgeFile(CHAR *pszImgFileName)
{
	//* 打开图像
	Image *pobjImage = new Image();
	if (!pobjImage->open(pszImgFileName))
	{
		MessageBox(hWndMain, "无法读取图片文件，请确认图片文件存在且格式正确！", "错误", MB_ICONERROR | MB_OK);
		return;
	}

	//* 先删除原先的图像
	if (pobjOpenedImage)
		delete pobjOpenedImage;

	pobjOpenedImage = pobjImage;

	//* 更改主程序标题栏，将当前打开的图片文件路径显示到标题栏中
	sprintf(szMainWindowTitle, "%s - %s", szTitle, pszImgFileName);
	SetWindowText(hWndMain, szMainWindowTitle);

	//* 顺序不能调换，必须先设置完标题栏后再执行这句，只有如此才能将图片显示在WINDOWS的STATIC控件上
	CreateTextStatic();

	//* 使能编辑菜单项
	EnableMenuItemForEdit();

	pobjOpenedImage->SetEditFlag(FALSE);

	//* 强制更新，不需要调用该函数了，因为前面重建了STATIC控件，OS肯定会主动重绘该区域的
	//ForcedUpdateCleintRect(hWndMain);
}

//* 将窗口调整至与图片一样的大小
void SetWindowSize(INT nWidth, INT nHeight)
{
	RECT rect_window, rect_old_client;
	INT nBorderWidth, nBorderHeight, nWindowWidth, nWindowHeight;

	//* 获取屏幕大小
	INT nPCWidth = GetSystemMetrics(SM_CXSCREEN);
	INT nPCHeight = GetSystemMetrics(SM_CYSCREEN);

	//* 获取窗口和用户区域的RECT数据
	GetWindowRect(hWndMain, &rect_window);
	GetClientRect(hWndMain, &rect_old_client);

	nBorderWidth = (rect_window.right - rect_window.left) - (rect_old_client.right - rect_old_client.left);
	nBorderHeight = (rect_window.bottom - rect_window.top) - (rect_old_client.bottom - rect_old_client.top);

	nWindowWidth = nBorderWidth + nWidth;
	nWindowHeight = nBorderHeight + nHeight;

	SetWindowPos(hWndMain, NULL, (nPCWidth - nWindowWidth) / 2, (nPCHeight - nWindowHeight) / 2, nWindowWidth, nWindowHeight, SWP_NOMOVE | SWP_NOZORDER);
	//MoveWindow(hWndMain, (nPCWidth - nWindowWidth) / 2, (nPCHeight - nWindowHeight) / 2, nWindowWidth, nWindowHeight, TRUE);
	SetWindowPos(hWndStatic, NULL, 0, 0, nWidth, nHeight, SWP_NOMOVE | SWP_NOZORDER);

	resizeWindow(szMainWindowTitle, nWidth, nHeight);
}

//* 绘制图片
void DrawImage(void)
{
	if (pobjOpenedImage)
	{
		INT nImgWidth, nImgHeight, nPCWidth, nPCHeight;
		
		nImgWidth = pobjOpenedImage->GetImgWidth();
		nImgHeight = pobjOpenedImage->GetImgHeight();

		//* 看看是否超过当前屏幕大小
		nPCWidth = GetSystemMetrics(SM_CXSCREEN) / 2;
		nPCHeight = GetSystemMetrics(SM_CYSCREEN) / 2;
		if (nImgWidth > nPCWidth || nImgHeight > nPCHeight)
		{
			DOUBLE dblScaleFactor;

			//* 如果宽度相差大，则以宽度作为计算缩放因子的基数，否则，高度作为基数
			if (nImgWidth - nPCWidth > nImgHeight - nPCHeight)
			{
				dblScaleFactor = ((DOUBLE)nPCWidth) / (DOUBLE)nImgWidth;
			}
			else
				dblScaleFactor = ((DOUBLE)nPCHeight) / (DOUBLE)nImgHeight;			
			
			nImgWidth *= dblScaleFactor;
			nImgHeight *= dblScaleFactor;			

			pobjOpenedImage->SetScaleFactor(dblScaleFactor);
		}
		else
			pobjOpenedImage->SetScaleFactor(1.0);

		//* 调整显示区域大小
		SetWindowSize(nImgWidth, nImgHeight);

		pobjOpenedImage->show(szMainWindowTitle);
	}
}

//* 强制更新指定窗口的用户交互区域
void ForcedUpdateCleintRect(HWND hWnd)
{
	RECT rect_client;
	GetClientRect(hWndMain, &rect_client);

	InvalidateRect(hWndMain, &rect_client, TRUE);
}

void EnableMenuItemForEdit(void)
{
	for(INT i=0; i<sizeof(naImgHandleMenuList) / sizeof(INT); i++)
		EnableMenuItem(GetMenu(hWndMain), naImgHandleMenuList[i], MF_ENABLED);

	//* 禁止保存菜单
	EnableMenuItem(GetMenu(hWndMain), IDM_SAVE, MF_DISABLED);
}

//* 改变所有菜单的状态，参数unMenuFlags有两个值：MF_ENABLED或MF_DISABLED
void ChangeAllMenuStatus(UINT unMenuFlags)
{
	for (INT i = 0; i<sizeof(naImgHandleMenuList) / sizeof(INT); i++)
		EnableMenuItem(GetMenu(hWndMain), naImgHandleMenuList[i], unMenuFlags);

	//* 系统关闭菜单
	EnableMenuItem(GetSystemMenu(hWndMain, FALSE), SC_CLOSE, unMenuFlags);
}

//* 保存编辑过的图片
void SaveEditedImg(void)
{

}

void ImageHandler(INT nMenuID)
{
	for (INT i = 0; i < sizeof(staImgHandlerList) / sizeof(ST_IMGHANDLER); i++)
	{
		if (nMenuID == staImgHandlerList[i].nMenuID)
		{
			ChangeAllMenuStatus(MF_DISABLED);			

			staImgHandlerList[i].pfunHadler();

			//* 变换完毕，更新显示区域
			ForcedUpdateCleintRect(hWndMain);

			ChangeAllMenuStatus(MF_ENABLED);

			pobjOpenedImage->SetEditFlag(TRUE);

			//* 使能保存菜单
			EnableMenuItem(GetMenu(hWndMain), IDM_SAVE, MF_ENABLED);

			break;
		}
	}
}

//* 对用户选择的部分图像区域进行透视变换
void IMGHANDLER_Transformation(void)
{
	ImagePerspectiveTransformation objImgPerspecTrans;

	//* 透视变换
	objImgPerspecTrans.process(pobjOpenedImage->GetSrcImg(), 
								pobjOpenedImage->GetResultImg(), 
								pobjOpenedImage->GetShowImg(), 
								pobjOpenedImage->GetScaleFactor());	
}

