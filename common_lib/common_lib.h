// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 COMMON_LIB_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// COMMON_LIB_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef COMMON_LIB_EXPORTS
#define COMMON_LIB_API __declspec(dllexport)
#else
#define COMMON_LIB_API __declspec(dllimport)
#endif

#include <winsock2.h>
#include <mswsock.h>
#include <iostream>
#include <vector>

using namespace std;

//* 内存文件
typedef struct _ST_MEM_FILE_ {	
	HANDLE hMem;
	void *pvMem;
} ST_MEM_FILE, *PST_MEM_FILE;

class COMMON_LIB_API PerformanceTimer {
public:
	PerformanceTimer() {
		QueryPerformanceFrequency(&uniOSFreq);
	}

	void start(void) {
		QueryPerformanceCounter(&uniStartCount);
	}

	//* 返回单位为微秒
	DOUBLE end(void) {
		LARGE_INTEGER uniStopCount;

		QueryPerformanceCounter(&uniStopCount);

		return (1e6 / ((DOUBLE)uniOSFreq.QuadPart)) * (DOUBLE)(uniStopCount.QuadPart - uniStartCount.QuadPart);
	}

private:
	LARGE_INTEGER uniOSFreq;
	LARGE_INTEGER uniStartCount;
};

//* 公共函数库
namespace common_lib {
	COMMON_LIB_API HANDLE CLIBOpenDirectory(const CHAR *pszDirectName);
	COMMON_LIB_API void CLIBCloseDirectory(HANDLE hDir);
	COMMON_LIB_API UINT CLIBReadDir(HANDLE hDir, string &strFileName);
	COMMON_LIB_API UINT GetFileNumber(const CHAR *pszDirectName);

	COMMON_LIB_API HANDLE IPCCreateSHM(CHAR *pszSHMemName, UINT unSize);
	COMMON_LIB_API CHAR *IPCCreateSHM(CHAR *pszSHMemName, UINT unSize, HANDLE *phSHM);
	COMMON_LIB_API CHAR *IPCOpenSHM(CHAR *pszSHMemName, HANDLE *phSHM);
	COMMON_LIB_API void IPCCloseSHM(CHAR *pszSHM);
	COMMON_LIB_API void IPCCloseSHM(CHAR *pszSHM, HANDLE hSHM);
	COMMON_LIB_API void IPCDelSHM(HANDLE hSHM);

	COMMON_LIB_API BOOL CreateMemFile(PST_MEM_FILE pstMemFile, DWORD dwFileSize);
	COMMON_LIB_API void DeletMemFile(PST_MEM_FILE pstMemFile);

	COMMON_LIB_API UINT IsProcExist(CHAR *pszProcName, INT nProcNameLen);
	COMMON_LIB_API BOOL IsProcExist(CHAR *pszProcName, ...);		

	COMMON_LIB_API UINT GetWorkPath(CHAR *pszPath, UINT unPathBytes);

	COMMON_LIB_API INT EatZeroOfTheNumberTail(INT nNum);
};