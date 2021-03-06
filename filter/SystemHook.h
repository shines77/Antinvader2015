///////////////////////////////////////////////////////////////////////////////
//
// 版权所有 (c) 2011 - 2012
//
// 原始文件名称     : SystemHook.h
// 工程名称         : AntinvaderDriver
// 创建时间         : 2011-04-2
//
//
// 描述             : HOOK挂钩功能的头文件
//
// 更新维护:
//  0000 [2011-04-2]  最初版本.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ntdef.h>
#include <ntifs.h>

////////////////////////////
//      数据结构
////////////////////////////

#pragma pack(push, 1)

// SSDT表结构定义
#if 1
// TODO: 为了兼容 x64 下的SSDT表, 目前这个定义是猜测的, 有待验证.
typedef struct _SERVICE_DESCRIPTOR_ENTRY {
    PVOID * ServiceTableBase;
    PVOID * ServiceCounterTableBase;
    SIZE_T  NumberOfServices;
    UCHAR * ParamTableBase;
} SERVICE_DESCRIPTOR_ENTRY, * PSERVICE_DESCRIPTOR_ENTRY;
#else
typedef struct _SERVICE_DESCRIPTOR_ENTRY {
    unsigned int * ServiceTableBase;
    unsigned int * ServiceCounterTableBase;
    unsigned int   NumberOfServices;
    unsigned char * ParamTableBase;
} SERVICE_DESCRIPTOR_ENTRY, * PSERVICE_DESCRIPTOR_ENTRY;
#endif

#pragma pack(pop)

// ZwCreateProcess函数指针
typedef NTSTATUS (*ZW_CREATE_PROCESS) (
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE InheritFromProcessHandle,
    __in BOOLEAN InheritHandles,
    __in_opt HANDLE SectionHandle ,
    __in_opt HANDLE DebugPort ,
    __in_opt HANDLE ExceptionPort
);

////////////////////////////
//      数据导出
////////////////////////////
#if 0
// 原始的ZwCreateProcess
extern "C"
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE InheritFromProcessHandle,
    __in BOOLEAN InheritHandles,
    __in_opt HANDLE SectionHandle,
    __in_opt HANDLE DebugPort,
    __in_opt HANDLE ExceptionPort
);
#endif

#if defined(_WIN64)

// 64位系统下, 只能自己定义 SSDT 表.
#ifdef __cplusplus
extern "C"
#else
extern
#endif
SERVICE_DESCRIPTOR_ENTRY    KeServiceDescriptorTable;

#else // !_WIN64

// 导出 SSDT 表
#ifdef __cplusplus
extern "C"
#else
extern
#endif
__declspec(dllimport) SERVICE_DESCRIPTOR_ENTRY KeServiceDescriptorTable;

#endif // _WIN64

////////////////////////////
//      宏定义
////////////////////////////

// SSDT地址
#define SSDT_ADDRESS_OF_FUNCTION(_function) \
            KeServiceDescriptorTable.ServiceTableBase[*(SIZE_T *)((PUCHAR)_function + 1)]

// 一般的文件路径长度 用于猜测打开进程的路径长度,如果调整的较为准确
// 可以提高效率,设置的越大时间复杂度越小,空间复杂度越高
#define HOOK_NORMAL_PROCESS_PATH    NORMAL_PATH_LENGTH

// 内存标志
#define MEM_HOOK_TAG    'hook'

////////////////////////////
//      变量定义
////////////////////////////

// 保存原始的ZwCreateOprocess地址
extern ZW_CREATE_PROCESS    ZwCreateProcessOriginal;

////////////////////////////
//      函数申明
////////////////////////////

// 准备挂钩代替的函数
#ifdef __cplusplus
extern "C"
#else
extern
#endif
NTSTATUS
AntinvaderNewCreateProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE InheritFromProcessHandle,
    __in BOOLEAN InheritHandles,
    __in_opt HANDLE SectionHandle,
    __in_opt HANDLE DebugPort,
    __in_opt HANDLE ExceptionPort
);

VOID WriteProtectionOn();

VOID WriteProtectionOff();

BOOLEAN InitKeServiceDescirptorTable();
