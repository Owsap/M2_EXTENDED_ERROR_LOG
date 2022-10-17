/// 1.
// Add anywhere
#if defined(_EXTENDED_ERROR_LOG_)
#	include "Debug.h"
#endif

/// 2.
// Search @ LONG __stdcall EterExceptionFilter
	fException = fopen("ErrorLog.txt", "wt");
	if (fException)

// Replace with
#if defined(_EXTENDED_ERROR_LOG_)
	// Copy crashed syserr to crash folder.
	{
		auto logfile = std::to_string(GetLogFileTimeStamp());

		std::filesystem::path src_path = std::filesystem::current_path();
		src_path /= "syserr\\" + logfile + ".txt";

		std::filesystem::path copy_path = std::filesystem::current_path();
		copy_path /= "syserr\\crash\\" + logfile + ".txt";

		bool copy = std::filesystem::copy_file(src_path, copy_path);
	}

	std::filesystem::path error_log_file = std::filesystem::current_path();
	error_log_file /= "syserr\\crash\\error_log.txt";

	fException = fopen(error_log_file.string().c_str(), "wt");
#else
	fException = fopen("ErrorLog.txt", "wt");
#endif

/// 3.
// Search
	return EXCEPTION_EXECUTE_HANDLER;

// Add above
#if defined(_EXTENDED_ERROR_LOG_)
	// Dump DxDiag Information
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	PROCESS_INFORMATION pi;

	std::filesystem::path crash_path = std::filesystem::current_path();
	crash_path /= "syserr\\crash";

	if (CreateProcess("C:\\Windows\\System32\\dxdiag.exe", "/dontskip /whql:off /t dxdiag_log.txt", NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, crash_path.string().c_str(), &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
#endif
