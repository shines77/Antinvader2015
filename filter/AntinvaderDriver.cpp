///////////////////////////////////////////////////////////////////////////////
//
// 版权所有 (c) 2011 - 2012
//
// 原始文件名称     : AntinvaderDriver.cpp
// 工程名称         : AntinvaderDriver
// 创建时间         : 2011-03-20
//
//
// 描述             : Antinvader驱动程序主程序
//
// 更新维护:
//  0000 [2011-03-20] 最初版本.
//
///////////////////////////////////////////////////////////////////////////////

// $Id$

#ifdef __cplusplus
extern "C" {
#endif

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <ntddscsi.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

#ifdef __cplusplus
}   // extern "C"
#endif

#include "AntinvaderDriver.h"
#include "ProcessFunction.h"
#include "ConfidentialProcess.h"
#include "FileFunction.h"
#include "CallbackRoutine.h"
#include "ConfidentialFile.h"
#include "SystemHook.h"
#include "KeLog.h"

////////////////////////////
// 全局变量
///////////////////////////
PDRIVER_OBJECT pdoGlobalDrvObj = NULL;      // 驱动对象
PFLT_FILTER pfltGlobalFilterHandle = NULL;  // 过滤句柄

PFLT_PORT pfpGlobalServerPort;              // 服务器端口, 与Ring3通信
PFLT_PORT pfpGlobalClientPort;              // 客户端端口, 与Ring3通信

// COMMUNICATE_PORT_NAME     管道名称: "\\AntinvaderPort"
PRESET_UNICODE_STRING(usCommunicatePortName, COMMUNICATE_PORT_NAME);

#ifdef __cplusplus
extern "C" {
#endif
/*---------------------------------------------------------
函数名称:   DriverEntry
函数描述:   驱动程序入口,注册微过滤驱动,通信端口
输入参数:
            DriverObject 驱动对象
            RegistryPath 驱动路径
输出参数:
            DriverObject 驱动对象
返回值:
            STATUS_SUCCESSFUL 为成功否则返回失败状态值
其他:
更新维护:   2011.3.20    最初版本
---------------------------------------------------------*/
NTSTATUS DriverEntry(
    __inout PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING   RegistryPath
    )
{
    PAGED_CODE();

    // 初始化 KeLog
    KeLog_Init();

    DebugTraceEx(DEBUG_TRACE_LOAD_UNLOAD, "DriverEntry", "[Antinvader] DriverEntry Enter.");

    // 返回值
    NTSTATUS status = STATUS_SUCCESS;

    // 初始化数据返回值
    BOOLEAN bReturn = 0;

    // 安全性叙述子
    PSECURITY_DESCRIPTOR psdSecurityDescriptor;

    // 对象权限结构
    OBJECT_ATTRIBUTES   oaObjectAttributes;

    BOOLEAN success = InitKeServiceDescirptorTable();

    // 保存全局驱动对象
    pdoGlobalDrvObj = DriverObject;

    //
    // 注册过滤驱动
    //
    if (NT_SUCCESS(status = FltRegisterFilter(DriverObject,               // 驱动对象
                                              &FilterRegistration,        // 驱动注册信息
                                              &pfltGlobalFilterHandle     // 驱动句柄, 保存到全局变量
                                              ))) {
        //
        // 如果成功了 启动过滤
        //
        DebugTraceEx(DEBUG_TRACE_NORMAL_INFO, "DriverEntry", "[Antinvader] Register succeed!");

        if (!NT_SUCCESS(status = FltStartFiltering(pfltGlobalFilterHandle))) {
            //
            // 如果启动失败 卸载驱动
            //
            DebugTraceEx(DEBUG_TRACE_ERROR, "DriverEntry", "[Antinvader] Starting filter failed.");

            FltUnregisterFilter(pfltGlobalFilterHandle);
            return status;
        }

    } else {
        //
        // 如果连注册都没有成功 返回错误码
        //
        DebugTraceEx(DEBUG_TRACE_ERROR, "DriverEntry", "[Antinvader] Register failed.");
        return status;
    }

    //
    // 建立通信端口
    //

    //
    // 初始化安全性叙述子,权限为FLT_PORT_ALL_ACCESS
    //
    status  = FltBuildDefaultSecurityDescriptor(
                            &psdSecurityDescriptor,
                            FLT_PORT_ALL_ACCESS);

    if (!NT_SUCCESS(status)) {
        //
        // 如果初始化失败 ,卸载驱动
        //
        DebugTraceEx(DEBUG_TRACE_ERROR, "DriverEntry", "[Antinvader] Built security descriptor failed.");

        FltUnregisterFilter(pfltGlobalFilterHandle);

        //
        // 返回信息
        //
        return status;
    }

    // 初始化对象权限结构
    InitializeObjectAttributes(&oaObjectAttributes,                        // 结构
                               &usCommunicatePortName,                     // 对象名称
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,   // 内核句柄 大小写不敏感
                               NULL,
                               psdSecurityDescriptor);

    // 创建通信端口
    status = FltCreateCommunicationPort( pfltGlobalFilterHandle,    // 过滤驱动句柄
                                         &pfpGlobalServerPort,      // 服务端端口
                                         &oaObjectAttributes,       // 权限
                                         NULL,
                                         Antinvader_Connect,
                                         Antinvader_Disconnect,
                                         Antinvader_Message,
                                         1                          // 最大连接数
                                        );

    // 释放安全性叙述子
    FltFreeSecurityDescriptor(psdSecurityDescriptor);

    if (!NT_SUCCESS(status)) {
        //
        // 如果最终的操作失败 释放已经申请的资源
        //
        DebugTraceEx(DEBUG_TRACE_ERROR, "DriverEntry", "[Antinvader] Creating communication port failed.");

        if (NULL != pfpGlobalServerPort) {
            FltCloseCommunicationPort(pfpGlobalServerPort);
        }

        if (NULL != pfltGlobalFilterHandle) {
            FltUnregisterFilter(pfltGlobalFilterHandle);
        }
    }

    //
    // 初始化进程名偏移
    //
    InitProcessNameOffset();

//  FctInitializeHashTable();

    //
    // 初始化机密进程表
    //
    PctInitializeHashTable();

    //
    // 初始化旁视链表
    //
    ExInitializeNPagedLookasideList(
        &nliNewFileHeaderLookasideList,
        NULL,
        NULL,
        0,
        CONFIDENTIAL_FILE_HEAD_SIZE,
        MEM_FILE_TAG,
        0);

    /*
    ExInitializeNPagedLookasideList(
        &nliCallbackContextLookasideList,
        NULL,
        NULL,
        0,
        sizeof(_POST_CALLBACK_CONTEXT),
        MEM_CALLBACK_TAG,
        0);

    ExInitializeNPagedLookasideList(
        &nliFileStreamContextLookasideList,
        NULL,
        NULL,
        0,
        FILE_STREAM_CONTEXT_SIZE,
        MEM_TAG_FILE_TABLE,
        0);
    */

    //
    // 结束
    //
    DebugTraceEx(DEBUG_TRACE_LOAD_UNLOAD, "DriverEntry", "[Antinvader] DriverEntry all succeed, leave now.");

    return status;
}

#ifdef __cplusplus
}   // extern "C"
#endif

/*---------------------------------------------------------
函数名称:   Antinvader_Unload
函数描述:   卸载驱动程序,关闭通信端口
输入参数:
            Flags   卸载标志
输出参数:
返回值:
            STATUS_SUCCESS 成功
其他:
更新维护:   2011.3.20    最初版本
---------------------------------------------------------*/
NTSTATUS Antinvader_Unload(__in FLT_FILTER_UNLOAD_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    DebugTraceEx(DEBUG_TRACE_LOAD_UNLOAD, "Unload", "Entered.");

    //
    // 关闭通信端口
    //
    if (pfpGlobalServerPort) {
        DebugTraceEx(DEBUG_TRACE_LOAD_UNLOAD, "Unload", "Closing communication port .... 0x%X", pfpGlobalServerPort);
        FltCloseCommunicationPort( pfpGlobalServerPort);
    }

    //
    // 卸载过滤
    //
    if (pfltGlobalFilterHandle) {
        DebugTraceEx(DEBUG_TRACE_LOAD_UNLOAD, "Unload", "Unregistering filter .... 0x%X", pfltGlobalFilterHandle);
        FltUnregisterFilter( pfltGlobalFilterHandle);
    }

    //
    // 销毁释放所有表格, 资源, 旁视链表等.
    //
    PctFreeTable();

//  ExDeleteNPagedLookasideList(&nliCallbackContextLookasideList);
//  ExDeleteNPagedLookasideList(&nliFileStreamContextLookasideList);

    ExDeleteNPagedLookasideList(&nliNewFileHeaderLookasideList);

    DebugTraceEx(DEBUG_TRACE_LOAD_UNLOAD, "Unload", "All succeed. Leave now.");

    //
    // 卸载 KeLog
    //
    KeLog_Unload();

    return STATUS_SUCCESS;
}
