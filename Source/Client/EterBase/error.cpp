#include "StdAfx.h"
#if defined(_EXTENDED_ERROR_LOG_)
#	include "Debug.h"
#endif

FILE* fException;

#if defined(_EXTENDED_ERROR_LOG_)
static constexpr bool s_bShowSymbolName = true;
static constexpr bool s_bShowLineNumber = true;
#endif

BOOL CALLBACK EnumerateLoadedModulesProc(PCSTR ModuleName, ULONG ModuleBase, ULONG ModuleSize, PVOID UserContext)
{
	DWORD offset = *((DWORD*)UserContext);

	if (offset >= ModuleBase && offset <= ModuleBase + ModuleSize)
	{
		fprintf(fException, "%s", ModuleName);
		return FALSE;
	}
	else
		return TRUE;
}

LONG __stdcall EterExceptionFilter(_EXCEPTION_POINTERS* pExceptionInfo)
{
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();

#if defined(_EXTENDED_ERROR_LOG_)
	std::filesystem::path cur_path = std::filesystem::current_path();

	// Copy crashed syserr to crash folder.
	{
		auto logfile = std::to_string(GetLogFileTimeStamp());

		auto src_path = cur_path.string() + "\\syserr\\" + logfile + ".txt";
		auto copy_path = cur_path.string() + "\\syserr\\crash\\" + logfile + ".txt";

		std::filesystem::copy_file(src_path, copy_path);
	}

	auto error_log_file = cur_path.string() + "\\syserr\\crash\\error_log.txt";
	fException = fopen(error_log_file.c_str(), "wt");
#else
	fException = fopen("ErrorLog.txt", "wt");
#endif
	if (fException)
	{
#if defined(_EXTENDED_ERROR_LOG_)
		SymInitialize(hProcess, NULL, TRUE);
#endif

		HMODULE hModule = GetModuleHandle(NULL);
		TCHAR szModuleName[256];
		GetModuleFileName(hModule, szModuleName, sizeof(szModuleName));
		std::time_t timestamp = static_cast<std::time_t>(GetTimestampForLoadedLibrary(hModule));

		fprintf(fException, "Module Name: %s\n", szModuleName);
		fprintf(fException, "Time Stamp: %s - %s\n", std::to_string(timestamp).c_str(), std::ctime(&timestamp));
		fprintf(fException, "Exception Type: 0x%08x\n", pExceptionInfo->ExceptionRecord->ExceptionCode);
		fprintf(fException, "\n");

		CONTEXT& rContext = *pExceptionInfo->ContextRecord;

		fprintf(fException, "eax: 0x%08x\tebx: 0x%08x\n", rContext.Eax, rContext.Ebx);
		fprintf(fException, "ecx: 0x%08x\tedx: 0x%08x\n", rContext.Ecx, rContext.Edx);
		fprintf(fException, "esi: 0x%08x\tedi: 0x%08x\n", rContext.Esi, rContext.Edi);
		fprintf(fException, "ebp: 0x%08x\tesp: 0x%08x\n", rContext.Ebp, rContext.Esp);
		fprintf(fException, "\n");

		// Initialize stack walking.
		STACKFRAME rStackFrame = { 0 };
		rStackFrame.AddrPC.Offset = rContext.Eip;
		rStackFrame.AddrStack.Offset = rContext.Esp;
		rStackFrame.AddrFrame.Offset = rContext.Ebp;
		rStackFrame.AddrPC.Mode = AddrModeFlat; // EIP
		rStackFrame.AddrStack.Mode = AddrModeFlat; // EBP
		rStackFrame.AddrFrame.Mode = AddrModeFlat; // ESP

#if defined(_EXTENDED_ERROR_LOG_)
		DWORD dwDisplament = 0;
		while (StackWalk(IMAGE_FILE_MACHINE_I386, hProcess, hThread, &rStackFrame, &rContext, NULL, &SymFunctionTableAccess, &SymGetModuleBase, NULL))
		{
			DWORD dwAddress = rStackFrame.AddrPC.Offset;
			if (dwAddress == 0)
				break;

			// Get symbol name
			TCHAR szBuffer[MAX_PATH + sizeof(IMAGEHLP_SYMBOL)] = { 0 };
			PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL)szBuffer;
			pSymbol->SizeOfStruct = sizeof(szBuffer);
			pSymbol->MaxNameLength = sizeof(szBuffer) - sizeof(IMAGEHLP_SYMBOL);

			// Get module name
			IMAGEHLP_MODULE moduleInfo = { 0 };
			moduleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
			TCHAR* szModule = "";
			if (SymGetModuleInfo(hProcess, dwAddress, &moduleInfo))
			{
				szModule = moduleInfo.ModuleName;
			}

			if (SymGetSymFromAddr(hProcess, rStackFrame.AddrPC.Offset, &dwDisplament, pSymbol))
			{
				// Get file name and line count.
				IMAGEHLP_LINE ImageLine = { 0 };
				ImageLine.SizeOfStruct = sizeof(IMAGEHLP_LINE);

				if (SymGetLineFromAddr(hProcess, rStackFrame.AddrPC.Offset, &dwDisplament, &ImageLine))
				{
					//fprintf(fException, "%08x %s!%s [%s @ %lu]\n", pSymbol->Address, szModule, pSymbol->Name, ImageLine.FileName, ImageLine.LineNumber);
					fprintf(fException, "%08x %s!%s @ %lu\n",
						pSymbol->Address,
						szModule,
						s_bShowSymbolName ? pSymbol->Name : "",
						s_bShowLineNumber ? ImageLine.LineNumber : 0
					);
				}
				else
					fprintf(fException, "%08x %s!%s\n", pSymbol->Address, szModule, s_bShowSymbolName ? pSymbol->Name : "");
			}
		}
		SymCleanup(hProcess);
#else
		for (int i = 0; i < 512 && rStackFrame.AddrPC.Offset; ++i)
		{
			if (StackWalk(IMAGE_FILE_MACHINE_I386, hProcess, hThread, &rStackFrame, &rContext, NULL, NULL, NULL, NULL) != FALSE)
			{
				fprintf(fException, "0x%08x\t", rStackFrame.AddrPC.Offset);
				EnumerateLoadedModules(hProcess, (PENUMLOADED_MODULES_CALLBACK)EnumerateLoadedModulesProc, &rStackFrame.AddrPC.Offset);
				fprintf(fException, "\n");
			}
			else
			{
				break;
			}
		}
#endif

		fprintf(fException, "\n");
		fflush(fException);

		fclose(fException);
		fException = NULL;
	}

#if defined(_EXTENDED_ERROR_LOG_)
	// Dump DxDiag Information
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	PROCESS_INFORMATION pi;

	auto crash_path = cur_path.string() + "\\syserr\\crash";
	if (CreateProcess("C:\\Windows\\System32\\dxdiag.exe", "/dontskip /whql:off /t dxdiag_log.txt", NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, crash_path.c_str(), &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
#endif

	return EXCEPTION_EXECUTE_HANDLER;
}

void SetEterExceptionHandler()
{
	SetUnhandledExceptionFilter(EterExceptionFilter);
}
