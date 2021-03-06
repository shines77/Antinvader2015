//
// AntinvaderDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "../KernelControler/KernelControler.h"

#include <stdlib.h>

int __cdecl _tmain(int argc, _TCHAR * argv[])
{
    if (argc < 3) {
        _tprintf(_T("Usage:\n")
            _T("\t-add [process name]\n")
            _T("\t-del [process name]\n"));
        return 0;
    }

    try {
        CFilterDriverObject filterObject;

        //
        // 测试版不启用路径检查和 MD5 校验
        //
        if (!_tcscmp(argv[1], _T("-add"))) {
            if (filterObject.AddConfidentialProcess(argv[2],/* L"Path",*/ L"MD5")) {
                _tprintf(_T("Success.\n"));
            }
        }
        else if (!_tcscmp(argv[1], _T("-del"))) {
            if (filterObject.DeleteConfidentialProcess(argv[2],/* L"Path",*/ L"MD5")) {
                _tprintf(_T("Success.\n"));
            }
        }
        else {
            _tprintf(_T("Invalid input.\n"));
        }
    }
    catch (TCHAR * sz) {
        _tprintf(_T("%ws.\n"), sz);
    }

    ::system("pause");
    return 0;
}
