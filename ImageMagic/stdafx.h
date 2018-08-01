// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#define OPEN_DEBUG_CONSOLE 1	//* 打开调试控制台输出

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
// Windows 头文件:
//* 解决DialogBox()函数报：
//* error C2664: “INT_PTR DialogBoxParamA(HINSTANCE,LPCSTR,HWND,DLGPROC,LPARAM)”: 无法将参数 4 从“INT_PTR (__cdecl *)(HWND,UINT,WPARAM,LPARAM)”转换为“DLGPROC”
//* 这个NO_STRICT必须在预处理器定义中增加，因为caffe需要用到这个，否则编译时caffe会报错，而DLGPROC的原型生命却是STRICT的，你说讨厌吗？讨厌的软件兼容问题
#define STRICT
#include <windows.h>
#undef STRICT

//* 注意一定要包含这个头文件，DOUBLE等数据类型的定义均在此文件中，否则编译时会报common_lib.h中end()重载错误
#include <wtypes.h>

// C 运行时头文件
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// TODO:  在此处引用程序需要的其他头文件
