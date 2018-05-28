#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <time.h>
#include <Tlhelp32.h>
#include <Wtsapi32.h>
#include <winternl.h>
#include <iostream>
#include <vector>
#include "common_lib.h"

typedef LONG(WINAPI *PROCNTQSIP)(HANDLE, DWORD, PVOID, ULONG, PULONG);

//* 打开指定目录
COMMON_LIB_API HANDLE common_lib::CLIBOpenDirectory(const CHAR *pszDirectName)
{
	HANDLE hDir;
	WIN32_FIND_DATA stDirItem;
	string strPath;	
	
	strPath.append(pszDirectName);
	strPath += "\\*.*";

	hDir = FindFirstFile(strPath.c_str(), &stDirItem);	

	return hDir;
}

//* 关闭打开的目录
COMMON_LIB_API void common_lib::CLIBCloseDirectory(HANDLE hDir)
{
	FindClose(hDir); //* 注意一定是这个函数而不是CloseHandle
}

COMMON_LIB_API UINT common_lib::CLIBReadDir(HANDLE hDir, string &strFileName)
{
	WIN32_FIND_DATA stDirItem;
	UINT unNameLen;
	string strTmp;

__lblReadDir:
	if (!FindNextFile(hDir, &stDirItem))
		return 0;

	unNameLen = strlen((const char*)stDirItem.cFileName);

	//* 跳过目录表最前面的"."、".."
	if ((unNameLen == 1 && stDirItem.cFileName[0] == '.')
		|| (unNameLen == 2 && stDirItem.cFileName[0] == '.' && stDirItem.cFileName[1] == '.'))
		goto __lblReadDir;

	strTmp.append(stDirItem.cFileName);
	strFileName = strTmp;
	
	return unNameLen;
}

//* 获取指定文件夹下的文件总数
COMMON_LIB_API UINT common_lib::GetFileNumber(const CHAR *pszDirectName)
{
	WIN32_FIND_DATA stDirItem;
	UINT unNameLen;
	UINT unFileNum = 0;

	HANDLE hDir = CLIBOpenDirectory(pszDirectName);
	if (hDir == INVALID_HANDLE_VALUE)
		return 0;

__lblReadDir:
	if (0 == FindNextFile(hDir, &stDirItem))
		goto __lblEnd;		

	unFileNum++;
	goto __lblReadDir;

__lblEnd:
	CLIBCloseDirectory(hDir);

	//* 减去文件夹前面的".."，"."这个目录已经在CLIBOpenDirectory()函数中也就是调用FindFirstFile()函数时
	//* 读出来了，所以这里直接减一就行
	return unFileNum - 1;
}

COMMON_LIB_API HANDLE common_lib::IPCCreateSHM(CHAR *pszSHMemName, UINT unSize)
{
	HANDLE hSHM;

	hSHM = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, unSize, pszSHMemName);
	if (hSHM == NULL || hSHM == INVALID_HANDLE_VALUE)
	{
		cout << "error para in " << __FUNCTION__ <<"(), in file "<< __FILE__ << ", line " << __LINE__ - 3 << ", error code:" << GetLastError() << endl;

		return INVALID_HANDLE_VALUE;
	}

	return hSHM;
}

COMMON_LIB_API CHAR *common_lib::IPCCreateSHM(CHAR *pszSHMemName, UINT unSize, HANDLE *phSHM)
{
	HANDLE hSHM;
	void *pbMem;

	hSHM = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, unSize, pszSHMemName);
	if (hSHM == NULL || hSHM == INVALID_HANDLE_VALUE)
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 3 << ", error code:" << GetLastError() << endl;

		return NULL;
	}

	pbMem = MapViewOfFile(hSHM, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (pbMem == NULL)
	{
		CloseHandle(hSHM);

		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 5 << ", error code:" << GetLastError() << endl;

		return NULL;
	}

	if (phSHM != NULL)
		*phSHM = hSHM;

	return (CHAR*)pbMem;
}

COMMON_LIB_API CHAR *common_lib::IPCOpenSHM(CHAR *pszSHMemName, HANDLE *phSHM)
{
	HANDLE hSHM;
	void *pbMem;

	hSHM = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, pszSHMemName);
	if (hSHM == NULL)
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 3 << ", error code:" << GetLastError() << " SHMName:" << pszSHMemName << endl;

		return NULL;
	}

	pbMem = MapViewOfFile(hSHM, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (pbMem == NULL)
	{
		CloseHandle(hSHM);

		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 5 << ", error code:" << GetLastError() << " SHMName:" << pszSHMemName << endl;

		return NULL;
	}

	if (phSHM != NULL)
		*phSHM = hSHM;

	return (CHAR*)pbMem;
}

COMMON_LIB_API void common_lib::IPCCloseSHM(CHAR *pszSHM)
{
	UnmapViewOfFile(pszSHM);
}

COMMON_LIB_API void common_lib::IPCCloseSHM(CHAR *pszSHM, HANDLE hSHM)
{
	if (pszSHM != NULL)
	{
		UnmapViewOfFile(pszSHM);
		CloseHandle(hSHM);
	}
}

COMMON_LIB_API void common_lib::IPCDelSHM(HANDLE hSHM)
{
	CloseHandle(hSHM);
}

//* 建立内存文件
COMMON_LIB_API BOOL common_lib::CreateMemFile(PST_MEM_FILE pstMemFile, DWORD dwFileSize)
{
	HANDLE hMem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, dwFileSize, NULL);
	if (hMem == NULL)
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 3 << ", error code:" << GetLastError() << endl;
		return FALSE;
	}

	//* 完成实际的映射并获取首地址
	void *pvMem = MapViewOfFile(hMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (pvMem == NULL)
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 3 << ", error code:" << GetLastError() << endl;

		CloseHandle(hMem);		

		return FALSE;
	}

	pstMemFile->hMem = hMem;
	pstMemFile->pvMem = pvMem;

	return TRUE;
}

//* 删除内存文件
COMMON_LIB_API void common_lib::DeletMemFile(PST_MEM_FILE pstMemFile)
{
	if (pstMemFile->pvMem != NULL)
	{
		UnmapViewOfFile(pstMemFile->pvMem);
		CloseHandle(pstMemFile->hMem);

		pstMemFile->hMem = INVALID_HANDLE_VALUE;
		pstMemFile->pvMem = NULL;
	}
}

//* 获取指定进程的命令行输入信息
static INT __GetProcCmdLine(PROCNTQSIP pfunNTIf, UINT unPID, CHAR *pszCmdLine)
{
	HANDLE hProcess;
	LONG lStatus;
	PROCESS_BASIC_INFORMATION stPBI;
	PEB stPEB;
	RTL_USER_PROCESS_PARAMETERS stProcParam;
	SIZE_T ztDummy;
	ULONG ulSize;
	void *pvAddr;
	WCHAR wszCmdLine[MAX_PATH + 1];
	CHAR szCmdLine[MAX_PATH + 1];

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, unPID);
	if (!hProcess)
	{		
		return 0;
	}

	lStatus = pfunNTIf(hProcess, 0, (void *)&stPBI, sizeof(PROCESS_BASIC_INFORMATION), NULL);
	if (lStatus)
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 3 << ", error code:" << GetLastError() << endl;

		goto __lblEnd;
	}

	if (!ReadProcessMemory(hProcess, stPBI.PebBaseAddress, &stPEB, sizeof(PEB), &ztDummy))
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 3 << ", error code:" << GetLastError() << endl;

		goto __lblEnd;
	}

	if (!ReadProcessMemory(hProcess, stPEB.ProcessParameters, &stProcParam, sizeof(RTL_USER_PROCESS_PARAMETERS), &ztDummy))
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 2 << ", error code:" << GetLastError() << endl;

		goto __lblEnd;
	}

	pvAddr = stProcParam.CommandLine.Buffer;
	ulSize = stProcParam.CommandLine.Length;

	if (!ReadProcessMemory(hProcess, pvAddr, wszCmdLine, sizeof(wszCmdLine), &ztDummy))
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 2 << ", error code:" << GetLastError() << endl;

		goto __lblEnd;
	}

	WideCharToMultiByte(CP_ACP, 0, wszCmdLine, -1, szCmdLine, sizeof(szCmdLine), NULL, NULL);

	//* 以下代码段为截取出绝对路径后的可执行程序名及携带的参数
	//*--------------------------------------------------------
	INT nProcNameBytes = (INT)strlen(szCmdLine);
	INT i = nProcNameBytes - 1;

	//* 根据命令行第一个字符是否为引号来判断是否需要去掉命令行末尾的空格和引号，之所以这样处理是因为
	//* OS不知为何会给某些进程的命令行前后加个引号，甚至尾部还带个空格
	if (szCmdLine[0] == '\"')
	{
		for (; i>0; i--)
		{
			if (szCmdLine[i] == ' ')
				continue;
			else if (szCmdLine[i] == '\"')
				szCmdLine[i] = '\x00';
			else;

			break;
		}
	}

	//* 截取出实际的命令行参数
	for (; i>0; i--)
	{
		if (szCmdLine[i] == '\\')
		{
			i += 1;
			break;
		}
	}

	nProcNameBytes = strlen(&szCmdLine[i]);
	memcpy(pszCmdLine, &szCmdLine[i], nProcNameBytes);
	//*--------------------------------------------------------


__lblEnd:
	CloseHandle(hProcess);
	return nProcNameBytes;
}

COMMON_LIB_API UINT common_lib::IsProcExist(CHAR *pszProcName, INT nProcNameLen)
{
	HANDLE hProcSnap;
	PROCESSENTRY32 stPE32;
	INT nCmdLineBytes;
	CHAR szCmdLine[MAX_PATH + 1];

	PROCNTQSIP pfunNTIf;

	pfunNTIf = (PROCNTQSIP)GetProcAddress(GetModuleHandle("ntdll"), "NtQueryInformationProcess");
	if (!pfunNTIf)
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 3 << ", error code:" << GetLastError() << endl;

		return 0;
	}

	if ((hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) == INVALID_HANDLE_VALUE)
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 2 << ", error code:" << GetLastError() << endl;

		return 0;
	}

	//* 使用前必须先指定结构体的实际大小
	stPE32.dwSize = sizeof(PROCESSENTRY32);

	//* 首先调用Process32First()函数获取第一个进程信息后才能获取剩下的进程信息
	memset(stPE32.szExeFile, 0, sizeof(stPE32.szExeFile));
	if (!Process32First(hProcSnap, &stPE32))
	{
		cout << "error para in " << __FUNCTION__ << "(), in file " << __FILE__ << ", line " << __LINE__ - 2 << ", error code:" << GetLastError() << endl;

		CloseHandle(hProcSnap);
		return 0;
	}

	do {
		memset(szCmdLine, 0, sizeof(szCmdLine));
		if (stPE32.th32ProcessID > 100)
		{
			nCmdLineBytes = __GetProcCmdLine(pfunNTIf, stPE32.th32ProcessID, szCmdLine);
			if (!nCmdLineBytes)
				continue;

			//printf("%s\r\n", szCmdLine);
		}
		else
			continue;

		//* 如果进程名称长度相等则需要进一步确定名称是否匹配
		if (nCmdLineBytes == nProcNameLen)
		{
			if (strncmp((CHAR*)pszProcName, szCmdLine, nProcNameLen) == 0)
			{
				CloseHandle(hProcSnap);
				return (UINT)stPE32.th32ProcessID;
			}
		}
	} while (Process32Next(hProcSnap, &stPE32));

	CloseHandle(hProcSnap);
	return 0;
}

COMMON_LIB_API BOOL common_lib::IsProcExist(CHAR *pszProcName, ...)
{
	CHAR szProcCmdLine[MAX_PATH];
	va_list pvaArgList;

	va_start(pvaArgList, pszProcName);
	_vsntprintf_s(szProcCmdLine, sizeof(szProcCmdLine) - 1, _TRUNCATE, pszProcName, pvaArgList);
	va_end(pvaArgList);

	UINT unProcID = IsProcExist(szProcCmdLine, strlen(szProcCmdLine));
	if (unProcID != GetCurrentProcessId() && unProcID != 0)
		return TRUE;

	return FALSE;
}

COMMON_LIB_API UINT common_lib::GetWorkPath(CHAR *pszPath, UINT unPathBytes)
{
	UINT unFileNameBytes, i;

	memset(pszPath, 0, unPathBytes);
	unFileNameBytes = GetModuleFileName(NULL, pszPath, MAX_PATH);
	if (unFileNameBytes)
	{
		i = unFileNameBytes - 1;
		for (; i>0; i--)
		{
			if (pszPath[i] == '\\')
			{
				pszPath[i] = '\x00';
				return i;
			}
		}
	}

	return 0;
}

//* 吃掉自然数尾部的0，比如:
//* 720->72, 1280->128，或者6400->64等
COMMON_LIB_API INT common_lib::EatZeroOfTheNumberTail(INT nNum)
{
	if (nNum % 10 == 0)
		return EatZeroOfTheNumberTail(nNum / 10);
	else
		return nNum;
}

