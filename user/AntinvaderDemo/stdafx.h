//
// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

// Windows 头文件
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN     // 从 Windows 头中排除极少使用的资料
#endif
#include <windows.h>
//#include <ntdef.h>
#include <tchar.h>
#include <stdio.h>

// TODO: 在此处引用程序需要的其他头文件

#pragma comment(lib, "fltLib.lib")
#pragma comment(lib, "user32.lib")

#if defined(_WIN64)
#pragma comment(lib, "KernelControler64.lib")
#else
#pragma comment(lib, "KernelControler32.lib")
#endif
