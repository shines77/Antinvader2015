///////////////////////////////////////////////////////////////////////////////
//
// 版权所有 (c) 2011 - 2012
//
// 原始文件名称     : AntinvaderDriver.h
// 工程名称         : AntinvaderDriver
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

#ifndef __ANTINVADERDRIVER_DRIVER_H__
#define __ANTINVADERDRIVER_DRIVER_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "AntinvaderDef.h"

#if 0
///////////////////////////
//  函数声明头文件引用
//////////////////////////

// 基本算法
#include "BasicAlgorithm.h"

// 过滤请求回调
#include "CallbackRoutine.h"

// 进程信息处理
#include "ProcessFunction.h"

// 机密文件数据
#include "ConfidentialFile.h"

// 机密文件数据
#include "FileFunction.h"

// 机密进程数据
#include "ConfidentialProcess.h"

// 微过滤驱动通用函数
#include "MiniFilterFunction.h"

// 挂钩
#include "SystemHook.h"

#endif

// 过滤请求回调
#include "CallbackRoutine.h"

// 机密文件数据
#include "ConfidentialFile.h"

#include <fltKernel.h>

// 驱动卸载
NTSTATUS Antinvader_Unload (__in FLT_FILTER_UNLOAD_FLAGS Flags);

// 包含了需要过滤的IRP请求
const FLT_OPERATION_REGISTRATION Callbacks[] = {
    // 创建
    { IRP_MJ_CREATE, 0, Antinvader_PreCreate, Antinvader_PostCreate },
    // 关闭
    { IRP_MJ_CLOSE, 0, Antinvader_PreClose, Antinvader_PostClose },
    // 清理
    { IRP_MJ_CLEANUP, 0, Antinvader_PreCleanUp, Antinvader_PostCleanUp },
    // 读文件
    { IRP_MJ_READ, 0, Antinvader_PreRead, Antinvader_PostRead },
    // 写文件 跳过缓存
    { IRP_MJ_WRITE, 0, Antinvader_PreWrite, Antinvader_PostWrite },
    // 设置文件信息,用来保护加密头
    { IRP_MJ_SET_INFORMATION, 0, Antinvader_PreSetInformation, Antinvader_PostSetInformation },
    // 目录
    { IRP_MJ_DIRECTORY_CONTROL, 0, Antinvader_PreDirectoryControl, Antinvader_PostDirectoryControl },
    // 读取文件信息
    { IRP_MJ_QUERY_INFORMATION, 0, Antinvader_PreQueryInformation, Antinvader_PostQueryInformation },
    // End of IRP_MJ
    { IRP_MJ_OPERATION_END }
};

CONST FLT_CONTEXT_REGISTRATION Contexts[] = {
    { FLT_VOLUME_CONTEXT, 0, Antinvader_CleanupContext, sizeof(VOLUME_CONTEXT), MEM_CALLBACK_TAG },
    { FLT_STREAM_CONTEXT, 0, Antinvader_CleanupContext, FILE_STREAM_CONTEXT_SIZE, MEM_TAG_FILE_TABLE },
    { FLT_CONTEXT_END }
};

// 驱动注册对象
const FLT_REGISTRATION FilterRegistration = {
    sizeof(FLT_REGISTRATION),           //  大小
    FLT_REGISTRATION_VERSION,           //  版本
    0,                                  //  标志

    Contexts,                           //  上下文
    Callbacks,                          //  操作回调设置 类型FLT_OPERATION_REGISTRATION

    Antinvader_Unload,                  //  卸载调用

    Antinvader_InstanceSetup,           //  启动过滤实例
    NULL,   // Antinvader_InstanceQueryTeardown,    //  销毁查询实例
    NULL,   // Antinvader_InstanceTeardownStart,    //  开始实例销毁回调
    NULL,   // Antinvader_InstanceTeardownComplete, //  完成实例销毁回调

    NULL,                               //  生存文件名称回调 (未使用 NULL)
    NULL,                               //  生成目标文件名称回调(未使用 NULL)
    NULL                                //  标准化过滤器名称缓存(未使用 NULL)
};

////////////////////////////
// 全局变量
///////////////////////////
extern PDRIVER_OBJECT pdoGlobalDrvObj;             // 驱动对象
extern PFLT_FILTER pfltGlobalFilterHandle;         // 过滤句柄

extern PFLT_PORT pfpGlobalServerPort;              // 服务器端口, 与Ring3通信
extern PFLT_PORT pfpGlobalClientPort;              // 客户端端口, 与Ring3通信

#endif // __ANTINVADERDRIVER_DRIVER_H__
