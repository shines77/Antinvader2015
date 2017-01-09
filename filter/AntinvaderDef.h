///////////////////////////////////////////////////////////////////////////////
//
// 版权所有 (c) 2011 - 2012
//
// 原始文件名称     : AntinvaderDef.h
// 工程名称         : AntinvaderDef
// 创建时间         : 2011-03-20
//
//
// 描述             : Antinvader驱动程序主要头文件,包括函数和静态类型定义
//
// 更新维护:
//  0000 [2011-03-20] 最初版本.
//
///////////////////////////////////////////////////////////////////////////////

// $Id$

#ifndef __ANTINVADERDRIVER_H_VERSION__
#define __ANTINVADERDRIVER_H_VERSION__ 100

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "drvCommon.h"
#include "drvVersion.h"

#include <ntdef.h>
#include <ntifs.h>

// 命令约定
#include "Common.h"

#ifndef FILE_DEVICE_ANTINVADERDRIVER
#define FILE_DEVICE_ANTINVADERDRIVER    0x8000
#endif

// 调试支持
// #if DBG

#define FILE_OBJECT_NAME_BUFFER(_file_object)       (_file_object)->FileName.Buffer
#define CURRENT_PROCESS_NAME_BUFFER                 ((PCHAR)PsGetCurrentProcess() + stGlobalProcessNameOffset)

// 追踪方式
#define DEBUG_TRACE_ERROR               0x00000001  // Errors - whenever we return a failure code
#define DEBUG_TRACE_LOAD_UNLOAD         0x00000002  // Loading/unloading of the filter
#define DEBUG_TRACE_INSTANCES           0x00000004  // Attach / detatch of instances
#define DEBUG_TRACE_DATA_OPERATIONS     0x00000008  // Operation to access / modify in memory metadata
#define DEBUG_TRACE_ALL_IO              0x00000010  // All IO operations tracked by this filter
#define DEBUG_TRACE_NORMAL_INFO         0x00000020  // Misc. information
#define DEBUG_TRACE_IMPORTANT_INFO      0x00000040  // Important information
#define DEBUG_TRACE_CONFIDENTIAL        0x00000080
#define DEBUG_TRACE_TEMPORARY           0x00000100
#define DEBUG_TRACE_ALL                 0xFFFFFFFF  // All flags

// 当前方式
#define DEBUG_TRACE_MASK                DEBUG_TRACE_ALL | DEBUG_TRACE_TEMPORARY | DEBUG_TRACE_ERROR | DEBUG_TRACE_CONFIDENTIAL    // DEBUG_TRACE_ALL

#define DebugTrace(_Level, _ProcedureName, _Data)       \
    if ((_Level) & (DEBUG_TRACE_MASK)) {                \
        DbgPrint("[Antinvader:"_ProcedureName"]\n\t\t\t"); \
        DbgPrint _Data;                                 \
        DbgPrint("\n");                                 \
    }

#define DebugTraceFile(_Level, _ProcedureName, _FileName, _Data)    \
    if ((_Level) & (DEBUG_TRACE_MASK)) {                            \
        DbgPrint("[Antinvader:"_ProcedureName"] %s\n\t\t\t", _FileName); \
        DbgPrint _Data;                                             \
        DbgPrint("\n");                                             \
    }

#define DebugTraceFileAndProcess(_Level, _ProcedureName, _FileName, _Data)  \
    if ((_Level) & (DEBUG_TRACE_MASK)) {                                    \
        DbgPrint("[Antinvader:"_ProcedureName"] %ws: %s\n\t\t\t", _FileName, CURRENT_PROCESS_NAME_BUFFER); \
        DbgPrint _Data;                                                     \
        DbgPrint("\n");                                                     \
    }

/*
//
// 测试
//
    KdPrint(("DebugTestRead: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
        "\t\tIRP_BUFFERED_IO:           %d\n"
        "\t\tIRP_CLOSE_OPERATION:       %d\n"
        "\t\tIRP_DEALLOCATE_BUFFER:     %d\n"
        "\t\tIRP_INPUT_OPERATION:       %d\n"
        "\t\tIRP_NOCACHE:               %d\n"
        "\t\tIRP_PAGING_IO:             %d\n"
        "\t\tIRP_SYNCHRONOUS_API:       %d\n"
        "\t\tIRP_SYNCHRONOUS_PAGING_IO: %d\n"
        "\t\tLength:                    %d\n"
        "\t\tByteOffset                 %d\n",
        pIoParameterBlock->IrpFlags&(IRP_BUFFERED_IO),
        pIoParameterBlock->IrpFlags&(IRP_CLOSE_OPERATION),
        pIoParameterBlock->IrpFlags&(IRP_DEALLOCATE_BUFFER),
        pIoParameterBlock->IrpFlags&(IRP_INPUT_OPERATION),
        pIoParameterBlock->IrpFlags&(IRP_NOCACHE),
        pIoParameterBlock->IrpFlags&(IRP_PAGING_IO),
        pIoParameterBlock->IrpFlags&(IRP_SYNCHRONOUS_API),
        pIoParameterBlock->IrpFlags&(IRP_SYNCHRONOUS_PAGING_IO),
        pIoParameterBlock->Parameters.Read.Length,
        pIoParameterBlock->Parameters.Read.ByteOffset.QuadPart)
    );
*/

// 其他宏

#define DebugPrintFileObject(_header, _object, _cached) \
    KdPrint(("[Antinvader] %s.\n"       \
        "\t\tName: %ws\n"               \
        "\t\tCached %d\n"               \
        "\t\tFcb: 0x%X\n",              \
        _header,                        \
        _object->FileName.Buffer,       \
        _cached,                        \
        _object->FsContext              \
    ));

#define DebugPrintFileStreamContext(_header, _object) \
    KdPrint(("[Antinvader] %s.\n"       \
        "\t\tName: %ws\n"               \
        "\t\tCached %d\n"               \
        "\t\tCachedFcb: 0x%X\n"         \
        "\t\tNoneCachedFcb: 0x%X\n",    \
        _header,                        \
        _object->usName.Buffer,         \
        _object->bCached,               \
        _object->pfcbCachedFCB,         \
        _object->pfcbNoneCachedFCB      \
    ));

// #else

// #define DebugTrace(_Level,_ProcedureName, _Data)         { NOTHING; }
// #define DebugPrintFileObject(_header,_object,_cached)    { NOTHING; }
// #define DebugPrintFileStreamContext(_header,_object)     { NOTHING; }

// #endif

// Values defined for "Method"
// METHOD_BUFFERED
// METHOD_IN_DIRECT
// METHOD_OUT_DIRECT
// METHOD_NEITHER
//
// Values defined for "Access"
// FILE_ANY_ACCESS
// FILE_READ_ACCESS
// FILE_WRITE_ACCESS

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

const ULONG IOCTL_ANTINVADERDRIVER_OPERATION = CTL_CODE(FILE_DEVICE_ANTINVADERDRIVER, 0x01, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA);

#endif // __ANTINVADERDRIVER_H_VERSION__
