///////////////////////////////////////////////////////////////////////////////
//
// 版权所有 (c) 2011 - 2012
//
// 原始文件名称     : ProcessFunction.cpp
// 工程名称         : AntinvaderDriver
// 创建时间         : 2011-03-20
//
//
// 描述             : 关于进程信息的功能实现
//
// 更新维护:
//  0000 [2011-03-20] 最初版本.
//
///////////////////////////////////////////////////////////////////////////////

#include "ProcessFunction.h"
#include "AntinvaderDriver.h"
#include "ConfidentialProcess.h"

#include <ntddk.h>

/*---------------------------------------------------------
函数名称:   InitProcessNameOffset
函数描述:   初始化EPROCESS结构中进程名的地址偏移量，在DriverEntry驱动被system调用时找到system进程名的偏移量，
			在DriverEntry被进程调用时根据EPROCESS+偏移量直接获取进程名
输入参数:
输出参数:
返回值:
其他:       参考了楚狂人(谭文)的思路
更新维护:   2011.3.20    最初版本
---------------------------------------------------------*/
BOOLEAN InitProcessNameOffset()
{
    PEPROCESS peCurrentProcess = PsGetCurrentProcess();

    //
    // 搜索 EPROCESS 结构,在其中找到 System 字符串地址,
    // 则该地址为 EPROCESS 中进程名称地址.
    //
    for (ULONG i = 0; i < MAX_EPROCESS_SIZE; i++) {
        if (!strncmp("System", (PCHAR)peCurrentProcess + i, strlen("System"))) {
            s_stGlobalProcessNameOffset = i;
            return TRUE;
        }
    }

	return FALSE;
}

/*---------------------------------------------------------
函数名称:   GetCurrentProcessNameA
函数描述:   初始化EPROCESS结构中进程名的地址
输入参数:   ansiCurrentProcessName    保存进程名的缓冲区
输出参数:   pSucceed    是否成功
返回值:     进程名长度
其他:       参考了楚狂人(谭文)的思路
更新维护:   2011.3.20    最初版本
---------------------------------------------------------*/
ULONG FltGetCurrentProcessNameA(
    __in PANSI_STRING ansiCurrentProcessName,
    __out PBOOLEAN pSucceed
    )
{
    PEPROCESS peCurrentProcess;
    ULONG ulLenth;

    //
    // 确保 (IRQL <= APC_LEVEL), 仅在 Debug 模式下发出警告.
    //
    PAGED_CODE();

    if (s_stGlobalProcessNameOffset == 0) {
        if (pSucceed)
            *pSucceed = FALSE;
        return 0;
    }

    //
    // 获得当前进程 EPROCESS, 然后移动一个偏移得到进程名所在位置.
    //
    peCurrentProcess = PsGetCurrentProcess();
    if (peCurrentProcess == NULL) {
        if (pSucceed)
            *pSucceed = FALSE;
        return 0;
    }

    //
    // 直接将这个字符串填到 ansiCurrentProcessName 里面.
    //
    RtlInitAnsiString(ansiCurrentProcessName,
                      ((PCHAR)peCurrentProcess + s_stGlobalProcessNameOffset));

    if (pSucceed)
        *pSucceed = TRUE;

    return ansiCurrentProcessName->Length;
}

/*---------------------------------------------------------
函数名称:   GetCurrentProcessName
函数描述:   初始化EPROCESS结构中进程名的地址
输入参数:   usCurrentProcessName    保存进程名的缓冲区
输出参数:   pSucceed    是否成功
返回值:     进程名长度
其他:       参考了楚狂人(谭文)的思路
更新维护:   2011.3.20    最初版本
---------------------------------------------------------*/
ULONG FltGetCurrentProcessName(
    __in PUNICODE_STRING usCurrentProcessName,
    __out PBOOLEAN pSucceed
    )
{
	if (!pSucceed)
	{
		return 0;
	}

	ANSI_STRING ansiCurrentProcessName = { 0 };

	ULONG ulLenth = FltGetCurrentProcessNameA(&ansiCurrentProcessName, pSucceed);
	if (!(*pSucceed) || ulLenth <= 0)
	{
		return 0;
	}

    ulLenth = RtlAnsiStringToUnicodeSize(&ansiCurrentProcessName);
    if (ulLenth > usCurrentProcessName->MaximumLength) 
	{
        *pSucceed = FALSE;
        return ulLenth;
    }

    //
    // 转换为Unicode
    //
    RtlAnsiStringToUnicodeString(usCurrentProcessName, &ansiCurrentProcessName, FALSE);

    *pSucceed = TRUE;
    return ulLenth;
}

/*---------------------------------------------------------
函数名称:   IsCurrentProcessConfidential
函数描述:   判断当前进程是否是机密进程
输入参数:
输出参数:
返回值:     当前进程是否是机密进程
其他:
更新维护:   2011.4.3     最初版本 仅测试notepad
            2011.7.25    接入机密进程表模块 仅测试
---------------------------------------------------------*/

BOOLEAN IsCurrentProcessConfidential()
{
    WCHAR wProcessNameBuffer[NORMAL_NAME_LENGTH] = { 0 };
	WCHAR wProcessMD5Buffer[NORMAL_MD5_LENGTH] = { 0 };
    CONFIDENTIAL_PROCESS_DATA cpdCurrentProcessData = { 0 };    
    UNICODE_STRING usProcessName = { 0 };
    ULONG ulLength;
    BOOLEAN bSucceed = FALSE;

    RtlInitEmptyUnicodeString(
        &cpdCurrentProcessData.usName,
        wProcessNameBuffer,
		NORMAL_NAME_LENGTH * sizeof(WCHAR));
	RtlInitEmptyUnicodeString(
		&cpdCurrentProcessData.usMd5Digest,
		wProcessMD5Buffer,
		NORMAL_MD5_LENGTH * sizeof(WCHAR));

    ulLength = FltGetCurrentProcessName(&cpdCurrentProcessData.usName, &bSucceed);
    if (!bSucceed) {
        KdDebugPrint("[Antinvader] IsCurrentProcessConfidential(): call GetCurrentProcessName() failed."
            " ulLength = %u\n", ulLength);
        return FALSE;
    }
    KdDebugPrint("[Antinvader] IsCurrentProcessConfidential() ProcessName: %ws, ulLength = %u\n",
        cpdCurrentProcessData.usName.Buffer, ulLength);

#ifdef TEST_DRIVER_NOTEPAD
	// 测试notepad
	UNICODE_STRING usProcessConfidential_notepad = { 0 };
	UNICODE_STRING usProcessConfidential_word = { 0 };
	UNICODE_STRING usProcessConfidential_excel = { 0 };
	UNICODE_STRING usProcessConfidential_ppt = { 0 };

    RtlInitUnicodeString(&usProcessConfidential_notepad, L"notepad.exe");
	RtlInitUnicodeString(&usProcessConfidential_word, L"winword.exe");
	RtlInitUnicodeString(&usProcessConfidential_excel, L"excel.exe");
	RtlInitUnicodeString(&usProcessConfidential_ppt, L"powerpnt.exe");

    RtlInitUnicodeString(&usProcessName, cpdCurrentProcessData.usName.Buffer);

	BOOLEAN ret = FALSE;
	if (TEST_driver_notepad_switch)
	{
		ret = ret || (RtlCompareUnicodeString(&usProcessName, &usProcessConfidential_notepad, TRUE) == 0);
	}
	if (TEST_driver_word_switch)
	{
		ret = ret || (RtlCompareUnicodeString(&usProcessName, &usProcessConfidential_word, TRUE) == 0);
	}
	if (TEST_driver_excel_switch)
	{
		ret = ret || (RtlCompareUnicodeString(&usProcessName, &usProcessConfidential_excel, TRUE) == 0);
	}
	if (TEST_driver_ppt_switch)
	{
		ret = ret || (RtlCompareUnicodeString(&usProcessName, &usProcessConfidential_ppt, TRUE) == 0);
	}
	return ret;
#else
	__try {
		// 在可信进程hash表中查找本进程，如果找到则是可信进程（进程名、md5均一致)，待实现
		bSucceed = ComputeCurrentProcessMD5(&cpdCurrentProcessData.usName,&cpdCurrentProcessData.usMd5Digest);
		if (!bSucceed) {
			KdDebugPrint("[Antinvader] IsCurrentProcessConfidential(): call GetCurrentProcessMD5() failed.");
			return FALSE;
		}
		// TO BE CONTINUE
		return PctIsProcessDataInConfidentialHashTable(&cpdCurrentProcessData, NULL);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return FALSE;
	}
#endif
}

/*---------------------------------------------------------
函数名称:   GetCurrentProcessPath
函数描述:   获取当前进程路径信息
输入参数:   puniFilePath    指向有效内存的字符串指针
输出参数:   puniFilePath    包含镜像所在路径的字符串
返回值:     TRUE, 如果成功找到.
            FALSE, 失败.
其他:       不必初始化传入时的Buffer地址
            原理就是去进程的PEB中把数据刨出来
            一定注意这个函数会申请内存用于存放路径
            记得用完了释放

更新维护:   2011.4.3     最初版本
---------------------------------------------------------*/
//BOOLEAN GetCurrentProcessPath(__inout PUNICODE_STRING puniFilePath)
//{
//    // PEB结构地址
//    ULONG ulPeb;
//
//    // Parameter结构地址
//    ULONG ulParameters;
//
//    // 当前进程
//    PEPROCESS  peCurrentProcess;
//
//    // 找到的地址,等待拷贝
//    PUNICODE_STRING puniFilePathLocated;
//
//    //
//    // 获得当前进程EPROCESS
//    //
//    peCurrentProcess = PsGetCurrentProcess();
//	if (NULL == peCurrentProcess)
//	{
//		return FALSE;
//	}
//
//    //
//    // 对不确定的内存进行访问,经常出错,结构化异常处理套上
//    //
//    __try {
//        //
//        // 获取当前进程PEB地址
//        //
//        ulPeb = *(ULONG*)((ULONG)peCurrentProcess + PEB_STRUCTURE_OFFSET);
//
//        //
//        // 空指针说明是内核进程, 肯定没有PEB结构.
//        //
//        if (!ulPeb) {
//            return FALSE;
//        }
//
//        //
//        // 检测地址是否有效, 无效肯定也不行.
//        //
//        if (!MmIsAddressValid((PVOID)ulPeb)) {
//            return FALSE;
//        }
//
//        //
//        // 计算Parameter地址, 由于不存在指针而
//        // 直接是将结构体本身放在了这里, 故不需
//        // 要再次进行地址有效性检测.
//        //
//        ulParameters = *(PULONG)((ULONG)ulPeb+PARAMETERS_STRUCTURE_OFFSET);
//
//        //
//        // 计算Path地址
//        //
//        puniFilePathLocated = (PUNICODE_STRING)(ulParameters+IMAGE_PATH_STRUCTURE_OFFSET);
//
//        //
//        // 申请内存
//        //
//        puniFilePath->Buffer = (PWCH)ExAllocatePoolWithTag(
//            NonPagedPool,
//            puniFilePathLocated->MaximumLength + 2,
//            MEM_PROCESS_FUNCTION_TAG);
//
//        //
//        // 拷贝数据
//        //
//        RtlCopyUnicodeString(puniFilePath, puniFilePathLocated);
//
//        return TRUE;
//
//    } __except(EXCEPTION_EXECUTE_HANDLER) {
//        KdDebugPrint("[Antinvader] Severe error occured when getting current process path.\r\n");
//#if defined(DBG) && !defined(_WIN64)
//        __asm int 3
//#endif
//    }
//
//    return FALSE;
//}

/*---------------------------------------------------------
函数名称:   GetCurrentProcessMD5
函数描述:   获取当前进程MD5值
输入参数:   进程名、进程完整路径
输出参数:   进程md5
返回值:     是否成功
其他:
更新维护:   2017.1.11
---------------------------------------------------------*/
BOOLEAN ComputeCurrentProcessMD5(__in PUNICODE_STRING punistrCurrentProcessName,
	__inout PUNICODE_STRING punistrCurrentProcessMD5)
{
	// TO BE CONTINUE
	return FALSE;
}

