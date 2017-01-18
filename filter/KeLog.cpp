///////////////////////////////////////////////////////////////////////////////
//
// 版权所有 (c) 2016 - 2017
//
// 原始文件名称     : KeLog.cpp
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


#include <stdio.h>
#include <stdarg.h>
#include <ntifs.h>
#include <ntstrsafe.h>

#include "KeLog.h"
#include "AntinvaderDriver.h"
#include "ProcessFunction.h"
#include "FileFunction.h"
#include "ConfidentialFile.h"

#pragma comment(lib, "ntsafestr.lib")

#ifndef KELOG_TAG
#define KELOG_TAG   'KeLg'
#endif

//
// See: http://blog.csdn.net/iamrainliang/article/details/2065534
// See: http://bbs.pediy.com/showthread.php?p=1435172
//

//
// Enable log event: for synchronization
//
static KEVENT   s_eventKeLogComplete;
static KEVENT   s_eventFltKeLogComplete;

//
// "\\??\\C:\\a.txt" Or "\\Device\\HarddiskVolume1\\a.txt"
//
static WCHAR    s_szLogFile[] = L"\\??\\C:\\KeLog.log";
static WCHAR    s_szFltLogFile[] = L"\\??\\C:\\FltKeLog.log";

//----------------------------------------------------------------------
//
// initialization interface
//
//----------------------------------------------------------------------
//
// initialize the global data structures, when the driver is loading.
// (Call in DriverEntry())
//
NTSTATUS
KeLog_Init()
{
    //
    // 确保IRQL <= APC_LEVEL
    //
    PAGED_CODE();

    // Initialize the event
    KeInitializeEvent(&s_eventKeLogComplete, SynchronizationEvent, TRUE);
    KeInitializeEvent(&s_eventFltKeLogComplete, SynchronizationEvent, FALSE);
    return STATUS_SUCCESS;
}

NTSTATUS
KeLog_Unload()
{
    //
    // 确保IRQL <= APC_LEVEL
    //
    PAGED_CODE();

    // Reset the FltKeLogComplete event.
    KeClearEvent(&s_eventFltKeLogComplete);

    // Wait for the event done
    KeWaitForSingleObject(&s_eventKeLogComplete, Executive, KernelMode, TRUE, 0);
    return STATUS_SUCCESS;
}

static void KeLog_AcquireLock()
{
    //
    // 确保IRQL <= APC_LEVEL
    //
    PAGED_CODE();

    // Wait for enable log event
    KeWaitForSingleObject(&s_eventKeLogComplete, Executive, KernelMode, TRUE, 0);
    KeClearEvent(&s_eventKeLogComplete);
}

static void KeLog_ReleaseLock()
{
    //
    // 确保IRQL <= APC_LEVEL
    //
    PAGED_CODE();

    // Set enable log event
    KeSetEvent(&s_eventKeLogComplete, 0, FALSE);
}

void KeLog_GetCurrentTime(PTIME_FIELDS timeFileds)
{
    LARGE_INTEGER sysTime, localTime;

    //
    // 确保IRQL <= APC_LEVEL
    //
    PAGED_CODE();

    // 获取当前系统时间
    KeQuerySystemTime(&sysTime);

    // 转换为当地时间
    ExSystemTimeToLocalTime(&sysTime, &localTime);

    // 转换为人们可以理解的时间格式
    RtlTimeToTimeFields(&localTime, timeFileds);
}

void KeLog_GetCurrentTimeString(ANSI_STRING * time)
{
    //
    // 确保IRQL <= APC_LEVEL
    //
    PAGED_CODE();

    //FLT_ASSERT(time != NULL);

    TIME_FIELDS sysTime;
    KeLog_GetCurrentTime(&sysTime);

    RtlStringCbPrintfA(time->Buffer, time->MaximumLength, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
        (INT32)sysTime.Year, (INT32)sysTime.Month, (INT32)sysTime.Day,
        (INT32)sysTime.Hour, (INT32)sysTime.Minute, (INT32)sysTime.Second, (INT32)sysTime.Milliseconds);
}

void KeLog_GetCurrentTimeString2(CHAR * time)
{
    //
    // 确保IRQL <= APC_LEVEL
    //
    PAGED_CODE();

    TIME_FIELDS sysTime;
    KeLog_GetCurrentTime(&sysTime);

    sprintf(time, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
        (INT32)sysTime.Year, (INT32)sysTime.Month, (INT32)sysTime.Day,
        (INT32)sysTime.Hour, (INT32)sysTime.Minute, (INT32)sysTime.Second, (INT32)sysTime.Milliseconds);
}

static
VOID
KeLog_FileCompleteCallback(
    __in PFLT_CALLBACK_DATA CallbackData,
    __in PFLT_CONTEXT Context
    )
{
    //
    // 确保IRQL <= APC_LEVEL
    //
    PAGED_CODE();

    //
    // 设置完成标志
    //
    KeSetEvent((PRKEVENT)Context, 0, FALSE);
}

//----------------------------------------------------------------------
//
// KeLog_FltLogPrint
//
// Trace to file.
//
//----------------------------------------------------------------------
BOOLEAN
KeLog_FltLogPrint(PFLT_INSTANCE pfiInstance, LPCSTR lpszFormat, ...)
{
    //
    // 确保IRQL <= APC_LEVEL
    //
    PAGED_CODE();

#if 0
    static volatile int count = 0;
    count++;
    if (count > 1) {
        return FALSE;
    }
#endif

    if (pfltGlobalFilterHandle == NULL) {
        KeLog_TracePrint(("KeLog(): KeLog_FltLogPrint() pfltGlobalFilterHandle is nullptr !!\n"));
        return FALSE;
    }

    if (pfiInstance == NULL) {
        KeLog_TracePrint(("KeLog(): KeLog_FltLogPrint() pfiInstance is nullptr !!\n"));
        return FALSE;
    }

    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        KeLog_TracePrint(("KeLog: IRQL too hight... (IRQL Level = %u\n", (UINT32)KeGetCurrentIrql()));
        return FALSE;
    }

    static const unsigned int kBufSize = 2048;
    BOOLEAN success = FALSE;
    CHAR szTime[64];
    ANSI_STRING ansiTime;
    ANSI_STRING ansiBuffer;
    PCHAR pszBuffer;
    ULONG ulBufSize;
    int nSize;
    va_list argList;
    ULONG ulLength;
    BOOLEAN bSucceed = FALSE;
    ANSI_STRING ansiProcessName = { 0 };
    TIME_FIELDS sysTime;
    LPCWSTR lpszLogFile = s_szFltLogFile;
    NTSTATUS status;
    PVOID pszFileBuffer = NULL;

    //KeLog_AcquireLock();

    ulLength = FltGetCurrentProcessNameA(&ansiProcessName, &bSucceed);
    if (!bSucceed || ulLength <= 0) {
        // Can not get the process name
        RtlInitAnsiString(&ansiProcessName, "Uknown Name");
    }

    RtlInitEmptyAnsiString(&ansiTime, szTime, sizeof(CHAR) * 64);
    KeLog_GetCurrentTimeString(&ansiTime);

    pszFileBuffer = FltAllocatePoolAlignedWithTag(pfiInstance, PagedPool, sizeof(CHAR) * kBufSize, KELOG_TAG);
    if (pszFileBuffer == NULL) {
        KeLog_TracePrint(("KeLog: KeLog_FltLogPrint(), FltAllocatePoolAlignedWithTag() Failed ...\n"));
        return FALSE;
    }

    RtlZeroMemory(pszFileBuffer, sizeof(CHAR) * kBufSize);

    ansiBuffer.Buffer = (PCHAR)pszFileBuffer;
    ansiBuffer.Length = 0;
    ansiBuffer.MaximumLength = sizeof(CHAR) * kBufSize;

    // Add process name and time string, "Time [pid] ProcessName : XXXXXX" .
    status = RtlStringCbPrintfA(ansiBuffer.Buffer, kBufSize, "%s [%4u] %s : ",
        ansiTime.Buffer, (ULONG)PsGetCurrentProcessId(), ansiProcessName.Buffer);
    if (!NT_SUCCESS(status)) {
        KeLog_TracePrint(("KeLog: KeLog_FltLogPrint(), RtlStringCbPrintfA() Failed ...\n"));
        return FALSE;
    }

    ulBufSize = strlen(ansiBuffer.Buffer);
    ansiBuffer.Length = (USHORT)(ulBufSize * sizeof(CHAR));
    pszBuffer = ansiBuffer.Buffer + ulBufSize;

    va_start(argList, lpszFormat);
    // The last argument to wvsprintf points to the arguments  
    status = RtlStringCbVPrintfA(pszBuffer, kBufSize - ulBufSize, lpszFormat, argList);
    // The va_end macro just zeroes out pArgList for no good reason  
    va_end(argList);

    if (!NT_SUCCESS(status)) {
        KeLog_TracePrint(("KeLog: KeLog_FltLogPrint(), RtlStringCbVPrintfA() Failed ...\n"));
        return FALSE;
    }

    ulBufSize = strlen(ansiBuffer.Buffer);
    ansiBuffer.Length = (USHORT)(ulBufSize * sizeof(CHAR));

    if (ulBufSize > 0)
        ansiBuffer.Buffer[ulBufSize] = '\0';
    else
        ansiBuffer.Buffer[0] = '\0';

    // Get the Unicode log filename.
    UNICODE_STRING fileName;

    // Get a handle to the log file object
    fileName.Buffer = NULL;
    fileName.Length = 0;
    fileName.MaximumLength = (wcslen(lpszLogFile) + 1) * sizeof(WCHAR);
    fileName.Buffer = (PWCH)FltAllocatePoolAlignedWithTag(pfiInstance, PagedPool, fileName.MaximumLength, 0);
    if (fileName.Buffer == NULL) {
        KeLog_TracePrint(("KeLog: KeLog_FltLogPrint(), ExAllocatePool Failed ...\n"));
        return FALSE;
    }
    RtlZeroMemory(fileName.Buffer, fileName.MaximumLength);
    status = RtlAppendUnicodeToString(&fileName, (PWSTR)lpszLogFile);
    if (!NT_SUCCESS(status)) {
        KeLog_TracePrint(("KeLog: KeLog_FltLogPrint(), RtlAppendUnicodeToString() Failed ...\n"));
        return FALSE;
    }

    KeLog_AcquireLock();

    __try {
        IO_STATUS_BLOCK IoStatus = { 0 };
        OBJECT_ATTRIBUTES objectAttributes;
        HANDLE FileHandle = NULL;
        PFILE_OBJECT pfoFileObject = NULL;
        LARGE_INTEGER ByteOffset;

        InitializeObjectAttributes(&objectAttributes,
            (PUNICODE_STRING)&fileName,
            OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

        //
        // FltCreateFile function
        // See: https://msdn.microsoft.com/zh-cn/library/windows/hardware/ff541935(v=vs.85).aspx
        //

        //
        // FltCreateFileEx routine
        // See: https://msdn.microsoft.com/zh-cn/library/windows/hardware/ff541937(v=vs.85).aspx
        //

        //
        // FltCreateFileEx2 routine
        // See: https://msdn.microsoft.com/library/windows/hardware/ff541939
        //
#if 0
        status = FltCreateFileEx(
            pfltGlobalFilterHandle,
            pfiInstance,
            &FileHandle,
            &pfoFileObject,
            FILE_APPEND_DATA,
            &objectAttributes,
            &IoStatus,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_WRITE,
            FILE_OPEN_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0,
            IO_IGNORE_SHARE_ACCESS_CHECK);
#elif 0
        status = FltCreateFileEx(
            pfltGlobalFilterHandle,
            pfiInstance,
            &FileHandle,
            &pfoFileObject,
            FILE_APPEND_DATA | GENERIC_WRITE,
            &objectAttributes,
            &IoStatus,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            /* FILE_SHARE_READ | */ FILE_SHARE_WRITE,
            FILE_OPEN_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_ALERT,
            NULL,
            0,
            IO_IGNORE_SHARE_ACCESS_CHECK);
#else
        //
        // 为了兼容 Windows XP, 使用 FltCreateFile(), 兼容性更好一些.
        //
        // SYNCHRONIZE
        //
        // The caller can synchronize the completion of an I/O operation by waiting for the returned FileHandle to be set to the Signaled state.
        // This flag must be set if the CreateOptions FILE_SYNCHRONOUS_IO_ALERT or FILE_SYNCHRONOUS_IO_NONALERT flag is set.
        //
        status = FltCreateFile(
            pfltGlobalFilterHandle,
            pfiInstance,
            &FileHandle,
            /* GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | */ FILE_APPEND_DATA,
            &objectAttributes,
            &IoStatus,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN_IF,
            /* FILE_WRITE_THROUGH | */ FILE_NON_DIRECTORY_FILE /*| FILE_SYNCHRONOUS_IO_NONALERT */,
            NULL,
            0,
            IO_IGNORE_SHARE_ACCESS_CHECK);

        DebugTraceEx(DEBUG_TRACE_TEMPORARY, "KeLog_FltLogPrint",
            "FltCreateFile(), status = %d, FileHandle = 0x%p, IoStatus.Infomation = %u.",
            status, FileHandle, (ULONG)IoStatus.Information);

        if (NT_SUCCESS(status)) {
            status = ObReferenceObjectByHandle(FileHandle,
                /*GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | */ FILE_APPEND_DATA,
                *IoFileObjectType,
                //NULL,
                KernelMode, (PVOID *)&pfoFileObject, NULL);
		    if (!NT_SUCCESS(status)) {
			    FltClose(FileHandle);
			    pfoFileObject = NULL;
		    }
        }
#endif

        if (NT_SUCCESS(status) && (pfoFileObject != NULL)) {
#if 1
            LARGE_INTEGER nAllocSize, nFileSize;
            BOOLEAN bIsDirectory = FALSE;

            // Reset the FltKeLogComplete event.
            //KeClearEvent(&s_eventFltKeLogComplete);

            status = FileGetStandardInformation(pfiInstance, pfoFileObject, &nAllocSize, &nFileSize, &bIsDirectory);
            if (NT_SUCCESS(status)) {
                // Set offset to the end of file.
                ByteOffset.QuadPart = nFileSize.QuadPart;

                //
                // See: https://msdn.microsoft.com/zh-cn/library/windows/hardware/ff544610(v=vs.85).aspx
                //
                status = FltWriteFile(
                    pfiInstance,
                    pfoFileObject,
                    &ByteOffset,
                    ulBufSize,
                    pszFileBuffer,
                    0, // FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
                    NULL,
                    //KeLog_FileCompleteCallback,
                    //&s_eventFltKeLogComplete
                    NULL,
                    NULL
                    );

                /*
                if (NT_SUCCESS(status)) {
                    //
                    // 等待 FltWriteFile 完成.
                    //
                    //KeWaitForSingleObject(&s_eventFltKeLogComplete, Executive, KernelMode, TRUE, 0);

                    ByteOffset.LowPart += ulBufSize;

                    // Set the new end of file.
                    status = FileSetSize(pfiInstance, pfoFileObject, &ByteOffset);

                    // Set the new callback data.
                    // FltSetCallbackDataDirty(cbData);
                }
                //*/
            }
#endif
            // Close 文件
            FltClose(FileHandle);
        }

        KeLog_ReleaseLock();
        success = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        KeLog_ReleaseLock();
        KeLog_TracePrint(("KeLog(): KeLog_FltLogPrint() exception code: %0xd !!\n", GetExceptionCode()));
        success = FALSE;
    }

    if (fileName.Buffer)
        FltFreePoolAlignedWithTag(pfiInstance, fileName.Buffer, 0);

    if (pszFileBuffer)
        FltFreePoolAlignedWithTag(pfiInstance, pszFileBuffer, KELOG_TAG);

    return success;
}

//----------------------------------------------------------------------
//
// KeLog_LogPrint
//
// Trace to file.
//
//----------------------------------------------------------------------
BOOLEAN
KeLog_LogPrint(LPCSTR lpszLog, ...)
{
    //
    // 确保IRQL <= APC_LEVEL
    //
    PAGED_CODE();

    BOOLEAN success = FALSE;
#if 0
    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        KeLog_TracePrint(("KeLog: IRQL too hight... (IRQL Level = %u\n", (UINT32)KeGetCurrentIrql()));
        return FALSE;
    }

    CHAR szBuffer[1024];
    CHAR szTime[32];
    PCHAR pszBuffer = szBuffer;
    ULONG ulBufSize;
    int nSize;
    va_list pArglist;
    ULONG ulLength;
    BOOLEAN bSucceed = FALSE;
    ANSI_STRING ansiProcessName = { 0 };
    TIME_FIELDS sysTime;
    LPCWSTR lpszLogFile = s_szLogFile;
    NTSTATUS status;

    ulLength = FltGetCurrentProcessNameA(&ansiProcessName, &bSucceed);
    if (!bSucceed || ulLength <= 0) {
        // Can not get the process name
        RtlInitAnsiString(&ansiProcessName, "Uknown Name");
    }

    KeLog_GetCurrentTimeString(szTime);

    // Add process name and time string
    sprintf(szBuffer, "[%s][%16s:%d] ", szTime,
        ansiProcessName.Buffer, (ULONG)PsGetCurrentProcessId());
    pszBuffer = szBuffer + strlen(szBuffer);

    va_start(pArglist, lpszLog);
    // The last argument to wvsprintf points to the arguments  
    nSize = _vsnprintf(pszBuffer, 1024 - 32, lpszLog, pArglist);
    // The va_end macro just zeroes out pArgList for no good reason  
    va_end(pArglist);
    if (nSize > 0)
        pszBuffer[nSize] = 0;
    else
        pszBuffer[0] = 0;

    ulBufSize = strlen(szBuffer);

    // Get the Unicode log filename.
    UNICODE_STRING fileName;

    // Get a handle to the log file object
    fileName.Buffer = NULL;
    fileName.Length = 0;
    fileName.MaximumLength = (wcslen(lpszLogFile) + 1) * sizeof(WCHAR);
    fileName.Buffer = (PWCH)ExAllocatePool(PagedPool, fileName.MaximumLength);
    if (fileName.Buffer == NULL) {
        KeLog_TracePrint(("KeLog: ExAllocatePool Failed ...\n"));
        return FALSE;
    }
    RtlZeroMemory(fileName.Buffer, fileName.MaximumLength);
    status = RtlAppendUnicodeToString(&fileName, (PWSTR)lpszLogFile);

    KeLog_AcquireLock();

    __try {
        IO_STATUS_BLOCK IoStatus;
        OBJECT_ATTRIBUTES objectAttributes;
        HANDLE FileHandle;

        InitializeObjectAttributes(&objectAttributes,
            (PUNICODE_STRING)&fileName,
            OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

        status = ZwCreateFile(&FileHandle,
            FILE_APPEND_DATA,
            &objectAttributes,
            &IoStatus,
            0,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_WRITE,
            FILE_OPEN_IF,
            FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0);

        if (NT_SUCCESS(status)) {
            ZwWriteFile(FileHandle,
                NULL,
                NULL,
                NULL,
                &IoStatus,
                szBuffer,
                ulBufSize,
                NULL,
                NULL);
            ZwClose(FileHandle);
        }

        KeLog_ReleaseLock();
        success = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        KeLog_ReleaseLock();
        KeLog_TracePrint(("KeTracePrint: KeLog() except: %0xd !!\n", GetExceptionCode()));
        success = FALSE;
    }

    if (fileName.Buffer)
        ExFreePool(fileName.Buffer);
#endif
    return success;
}
