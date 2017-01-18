///////////////////////////////////////////////////////////////////////////////
//
// 版权所有 (c) 2011 - 2012
//
// 原始文件名称     : SystemHook.cpp
// 工程名称         : AntinvaderDriver
// 创建时间         : 2011-04-2
//
//
// 描述             : HOOK挂钩功能的实现文件
//
// 更新维护:
//  0000 [2011-04-2]  最初版本.
//
///////////////////////////////////////////////////////////////////////////////

#include "SystemHook.h"

#include <ntdef.h>
#include <intrin.h>   // For __readmsr
#include <fltKernel.h>

#pragma intrinsic(__readmsr)

#if defined(_WIN64)

// 64位系统下，只能自己定义 SSDT 表
SERVICE_DESCRIPTOR_ENTRY    KeServiceDescriptorTable;

#endif

ZW_CREATE_PROCESS   ZwCreateProcessOriginal;

//////////////////////////////////////////////////////////////////////////
//
// See: http://czy0538.lofter.com/post/1ccbdcd5_4079090
//
// Modified by Guozi(wokss@163.com)
//
//////////////////////////////////////////////////////////////////////////
UINT64 GetKeServiceDescirptorTableShadow64()
{
    UINT64 KeServiceDescirptorTable = 0;                                    // 接收 KeServiceDescirptorTable 地址
    PUCHAR startAddressSearch = (PUCHAR)__readmsr((ULONG)(0xC0000082));     // 读取 KiSystemCall64 地址
    PUCHAR endAddressSearch = startAddressSearch + 0x500;                   // 搜索的结束地址
    ULONG tmpAddress = 0;                                                   // 用于保存临时地址

    // 先检查前面两个字节是否是有效的地址空间?
    if (!MmIsAddressValid(startAddressSearch) || !MmIsAddressValid(startAddressSearch + 1))
        return 0;

    // 从 KiSystemCall64 开始搜索其函数体内关于 KeServiceDescriptorTable 结构的信息
    for (PUCHAR now = startAddressSearch; now < endAddressSearch; ++now) {
        // 只检查第 3 个字节即可，因为前面两个字节已经检查过了
        if (MmIsAddressValid(now + 2)) {
            // 特征码 0x4c, 0x8d, 0x15
            if (now[0] == 0x4c && now[1] == 0x8d && now[2] == 0x15) {
                // 保存后 4 个字节
                RtlCopyMemory(&tmpAddress, now + 3, 4);
                // 得到 KeServiceDescirptorTable 表的真实地址
                KeServiceDescirptorTable = (UINT64)tmpAddress + (UINT64)now + 7;
            }
        }
        else {
            // 跳过 3 个字节
            now += 3;
            while (now < endAddressSearch) {
                // 往前搜索, 找到三个连续有效的地址空间的起始位置
                if (MmIsAddressValid(now) && MmIsAddressValid(now + 1) && MmIsAddressValid(now + 2)) {
                    // 因为跳到下一个循环, now 会加1, 所以这里先减去 1.
                    now--;
                    break;
                }
                // 同样, 跳过 3 个字节
                now += 3;
            }
        }
    }
    return KeServiceDescirptorTable;
}

//
// See: http://czy0538.lofter.com/post/1ccbdcd5_4079090
// See: http://bbs.csdn.net/topics/391491205 (The another implement routine)
//
UINT64 GetKeServiceDescirptorTableShadow64_Original()
{
    UINT64 KeServiceDescirptorTable = 0;                                    // 接收 KeServiceDescirptorTable 地址
    PUCHAR startAddressSearch = (PUCHAR)__readmsr((ULONG)(0xC0000082));     // 读取 KiSystemCall64 地址
    PUCHAR endAddressSearch = startAddressSearch + 0x500;                   // 搜索的结束地址
    ULONG tmpAddress = 0;                                           // 用于保存临时地址
    int j = 0;                                                      // 用于进行索引

    // 从 KiSystemCall64 开始搜索其函数体内关于 KeServiceDescriptorTable 结构的信息
    for (PUCHAR i = startAddressSearch; i < endAddressSearch; i++, j++) {
        if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2)) {
            // 特征码 0x4c 0x8d 0x15
            if (startAddressSearch[j] == 0x4c &&
                startAddressSearch[j + 1] == 0x8d &&
                startAddressSearch[j + 2] == 0x15) {
                // 保存后4个机器码
                RtlCopyMemory(&tmpAddress, i + 3, 4);
                // 得到KeServiceDescirptorTable表真实地址
                KeServiceDescirptorTable = (UINT64)tmpAddress + (UINT64)i + 7;
            }
        }
    }
    return KeServiceDescirptorTable;
}

BOOLEAN InitKeServiceDescirptorTable()
{
#if defined(_WIN64)
    SERVICE_DESCRIPTOR_ENTRY * lpKeServiceDescirptorTable = (SERVICE_DESCRIPTOR_ENTRY *)GetKeServiceDescirptorTableShadow64();
    if (lpKeServiceDescirptorTable != NULL) {
        RtlCopyMemory((void *)&KeServiceDescriptorTable, lpKeServiceDescirptorTable, sizeof(SERVICE_DESCRIPTOR_ENTRY));
        return TRUE;
    }
#endif
    return FALSE;
}

//
// See: http://czy0538.lofter.com/post/1ccbdcd5_4079090
//
UINT64 GetSSDTFunctionAddress64(INT32 index)
{
    INT64 address = 0;
    SERVICE_DESCRIPTOR_ENTRY * lpServiceDescriptorTable = (SERVICE_DESCRIPTOR_ENTRY *)GetKeServiceDescirptorTableShadow64();
    PULONG ssdt = (PULONG)lpServiceDescriptorTable->ServiceTableBase;
    ULONG dwOffset = ssdt[index];
    dwOffset >>= 4;                     // get real offset
    address = (UINT64)ssdt + dwOffset;  // get real address of function in ssdt
    KdPrint(("GetSSDTFunctionAddress(%d): 0x%llX\n", index, address));
    return address;
}

/*---------------------------------------------------------
函数名称:   WriteProtectionOn
函数描述:   开启写保护和中断
输入参数:
输出参数:
返回值:
其他:
更新维护:   2011.4.3     最初版本
---------------------------------------------------------*/
inline
VOID WriteProtectionOn()
{
#if !defined(_WIN64)
    __asm {
        MOV     EAX, CR0
        OR      EAX, 10000H     // 关闭写保护
        MOV     CR0, EAX
        STI                     // 允许中断
    }
#endif
}

/*---------------------------------------------------------
函数名称:   WriteProtectionOff
函数描述:   关闭写保护和中断
输入参数:
输出参数:
返回值:
其他:
更新维护:   2011.4.3     最初版本
---------------------------------------------------------*/
inline
VOID WriteProtectionOff()
{
#if !defined(_WIN64)
    __asm {
        CLI                         // 不允许中断
        MOV     EAX, CR0
        AND     EAX, NOT 10000H     // 取消写保护
        MOV     CR0, EAX
    }
#endif
}

#if !defined(_WIN64)
#ifdef __cplusplus
extern "C"
#endif
_declspec(naked)
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
    __in_opt HANDLE ExceptionPort)
{
   __asm {
       MOV      EAX, 1Fh
       LEA      EDX, [ESP + 04]
       INT      2Eh
       RET      20h
   }
}
#elif 1
#ifdef __cplusplus
extern "C"
#endif
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
    __in_opt HANDLE ExceptionPort)
{
    return 0;
}
#else
_declspec(naked)
NTSTATUS
NTAPI
ZwCreateProcess(
         PHANDLE phProcess,
         ACCESS_MASK DesiredAccess,
         POBJECT_ATTRIBUTES ObjectAttributes,
         HANDLE hParentProcess,
         BOOLEAN bInheritParentHandles,
         HANDLE hSection OPTIONAL,
         HANDLE hDebugPort OPTIONAL,
         HANDLE hExceptionPort OPTIONAL)
{
   __asm {
       MOV      EAX, 1Fh
       LEA      EDX, [ESP + 04]
       INT      2Eh
       RET      20h
   }
}
#endif

VOID HookInitializeFunctionAddress()
{
    // 保存原始地址
    ZwCreateProcessOriginal = (ZW_CREATE_PROCESS)SSDT_ADDRESS_OF_FUNCTION(ZwCreateProcess);
}

VOID HookOnSSDT()
{
    // 关闭写保护 不允许中断
    WriteProtectionOff();

    // 修改新地址
    SSDT_ADDRESS_OF_FUNCTION(ZwCreateProcess) = (unsigned int)(void *)&AntinvaderNewCreateProcess;

    // 关闭写保护 不允许中断
    WriteProtectionOn();
}

VOID HookOffSSDT()
{
    // 原始地址一定为空
    //FLT_ASSERT(ZwCreateProcessOriginal == NULL);

    // 关闭写保护 不允许中断
    WriteProtectionOff();

    // 修改回原来的地址
    SSDT_ADDRESS_OF_FUNCTION(ZwCreateProcess) = (unsigned int)(void *)&ZwCreateProcessOriginal;

    // 关闭写保护 不允许中断
    WriteProtectionOn();
}

/*---------------------------------------------------------
函数名称:   AntinvaderNewCreateProcess
函数描述:   用于替换ZwCreateProcess的挂钩函数
输入参数:   同ZwCreateProcess
输出参数:   同ZwCreateProcess
返回值:     同ZwCreateProcess
其他:
更新维护:   2011.4.5     最初版本
---------------------------------------------------------*/

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
    )
{
/*
    // 返回值
    NTSTATUS status;

    // 将要打开进程的目录的文件对象
    PFILE_OBJECT pFileRootObject;

    // 将由打开进程的文件对象
    PFILE_OBJECT pFileObject;

    // 保存文件路径的空间 先猜一个大小.
    WCHAR  wPathString[HOOK_NORMAL_PROCESS_PATH];

    // 保存文件路径空间的地址
    PWSTR pPathString = wPathString;

    // 文件路径
    PUNICODE_STRING usFilePath = {0};

    // 新申请的文件路径 临时使用
    PUNICODE_STRING usFilePathNewAllocated = {0};

    // 文件路径长度
    ULONG ulPathLength;
    USHORT ustPathLength;

    //
    // 先调用原版的ZwCreateProcess创建进程
    //
    status = ZwCreateProcessOriginal(
            ProcessHandle,
            DesiredAccess,
            ObjectAttributes,
            InheritFromProcessHandle,
            InheritHandles,
            SectionHandle ,
            DebugPort ,
            ExceptionPort);

    //
    // 如果创建失败就不需要判断了
    //
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // 先判断进程名称是否匹配 不匹配就返回
    //
    if (IsProcessConfidential(ObjectAttributes->ObjectName, NULL, NULL)) {
        return STATUS_SUCCESS;
    }

    //
    // 初始化字符串
    //
    RtlInitEmptyUnicodeString(
        usFilePath,
        pPathString,
        HOOK_NORMAL_PROCESS_PATH);

    //
    // 获取进程所在路径的文件对象,失败就直接返回
    //
    status = ObReferenceObjectByHandle(
        ObjectAttributes->RootDirectory,// 路径句柄
        GENERIC_READ,// 只读
        *IoFileObjectType,// 文件对象
        KernelMode,// 内核模式
        (PVOID *)&pFileRootObject,// 保存对象地址的空间
        NULL// 驱动程序使用NULL
        );

    if (!NT_SUCCESS(status)) {
        return STATUS_SUCCESS;
    }

    //
    // 使用文件对象获取路径
    //
    ulPathLength = FctGetFilePath(
        pFileRootObject,
        usFilePath,
        CONFIDENTIAL_FILE_NAME_FILE_OBJECT);

    //
    // 判断猜测的内存是否过小,如果查询失败则ulPathLength = 0,不会执行下列语句
    //
    if (ulPathLength > HOOK_NORMAL_PROCESS_PATH) {
        //
        // 申请内存,连着进程名称也申请了
        //
        pPathString = (PWSTR)ExAllocatePoolWithTag(
                        NonPagedPool,
                        ulPathLength + ObjectAttributes->ObjectName->Length + 1,
                        MEM_HOOK_TAG
              );

        //
        // 重新初始化字符串
        //
        RtlInitEmptyUnicodeString(
                usFilePath,
                pPathString,
                (USHORT)(ulPathLength + ObjectAttributes->ObjectName->Length +1 )
                // 路径长度 + 一个斜杠长度 + 文件名长度
                );

        //
        // 重新查询
        //
        ulPathLength = FctGetFilePath(
                pFileRootObject,
                usFilePath,
                CONFIDENTIAL_FILE_NAME_FILE_OBJECT);
    }

    if (!ulPathLength) {
        //
        // 如果获取失败使用QUERY_NAME_STRING查询
        //
        ulPathLength = FctGetFilePath(
            pFileRootObject,
            usFilePath,
            CONFIDENTIAL_FILE_NAME_QUERY_NAME_STRING);

        if (ulPathLength > HOOK_NORMAL_PROCESS_PATH) {
            //
            // 申请内存,连着进程名称也申请了
            //
            pPathString = (PWSTR)ExAllocatePoolWithTag(
                            NonPagedPool,
                            ulPathLength + ObjectAttributes->ObjectName->Length + 1 ,
                            MEM_HOOK_TAG);

            //
            // 重新初始化字符串
            //
            RtlInitEmptyUnicodeString(
                    usFilePath,
                    pPathString,
                    (USHORT)(ulPathLength + ObjectAttributes->ObjectName->Length +1 )
                    // 路径长度 + 一个斜杠长度 + 文件名长度
                    );

            //
            // 重新查询
            //
            ulPathLength = FctGetFilePath(
                    pFileRootObject,
                    usFilePath,
                    CONFIDENTIAL_FILE_NAME_QUERY_NAME_STRING);
        }

        if (!ulPathLength) {
            //
            // 还是失败,没招了,直接返回
            //
            return STATUS_SUCCESS;
        }
    }

    if (ulPathLength + 1 > usFilePath->MaximumLength) {
        //
        // 申请内存,连着进程名称也申请了
        //
        pPathString = (PWSTR)ExAllocatePoolWithTag(
                        NonPagedPool,
                        ulPathLength + ObjectAttributes->ObjectName->Length + 1,
                        MEM_HOOK_TAG);

        //
        // 重新初始化字符串
        //
        RtlInitEmptyUnicodeString(
                usFilePathNewAllocated,
                pPathString,
                (USHORT)(ulPathLength + ObjectAttributes->ObjectName->Length + 1)
                // 路径长度 + 一个斜杠长度 + 文件名长度
                );

        //
        // 拷贝字符串
        //
        RtlCopyUnicodeString(usFilePathNewAllocated , usFilePath);

        usFilePath = usFilePathNewAllocated;
    }

    //
    // 判断路径后面是否是"\",如果不是就加上
    //

    if (usFilePath->Buffer[ulPathLength - 1] != L'\\') {
        RtlAppendUnicodeToString(usFilePath , L"\\");
    }

    //
    // 如果加上进程名称大小又不够了
    //
    if (ulPathLength + 1 + ObjectAttributes->ObjectName->Length > usFilePath->MaximumLength) {
        //
        // 申请内存
        //
        pPathString = (PWSTR)ExAllocatePoolWithTag(
                        NonPagedPool,
                        ulPathLength + ObjectAttributes->ObjectName->Length + 1 ,
                        MEM_HOOK_TAG);

        //
        // 重新初始化字符串
        //
        RtlInitEmptyUnicodeString(
                usFilePathNewAllocated,
                pPathString,
                (USHORT)(ulPathLength + ObjectAttributes->ObjectName->Length +1 )
                // 路径长度 + 一个斜杠长度 + 文件名长度
                );

        //
        // 拷贝字符串
        //
        RtlCopyUnicodeString( usFilePathNewAllocated, usFilePath);

        usFilePath = usFilePathNewAllocated;
    }

    //
    // 把文件名增加上去
    //
    RtlAppendUnicodeStringToString(usFilePath, ObjectAttributes->ObjectName);

    //
    // 现在判断进程路径是否匹配 不匹配就返回
    //
    if (IsProcessConfidential(NULL ,usFilePath , NULL)) {
        return STATUS_SUCCESS;
    }
*/
    return STATUS_SUCCESS;
}
