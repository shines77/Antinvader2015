///////////////////////////////////////////////////////////////////////////////
//
// 版权所有 (c) 2016 - 2017
//
// 原始文件名称     : KeLog.h
// 工程名称         : AntinvaderDriver
// 创建时间         : 2017-01-09
//
//
// 描述             : 内核驱动的 Log 文件输出模块
//
// 更新维护:
//  0000 [2017-01-09] 最初版本.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ntifs.h>
#include <fltKernel.h>

#ifndef KeLog_TracePrint
#define KeLog_TracePrint(_x_)     DbgPrint _x_
#endif

NTSTATUS
KeLog_Init();

NTSTATUS
KeLog_Unload();

void KeLog_GetCurrentTime(PTIME_FIELDS timeFileds);

void KeLog_GetCurrentTimeString(LPSTR time);

BOOLEAN
KeLog_Print(LPCSTR lpszLog, ...);

BOOLEAN
KeLog_FltPrint(PFLT_INSTANCE pfiInstance, LPCSTR lpszLog, ...);
