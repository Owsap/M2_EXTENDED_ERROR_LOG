/// 1.
// Search
static UINT gs_uLevel = 0;

// Add above
#if defined(_EXTENDED_ERROR_LOG_)
static std::time_t gs_timestamp;

const std::time_t GetLogFileTimeStamp()
{
	return gs_timestamp;
}
#endif

/// 2.
// Search
void OpenLogFile(bool bUseLogFile)
{
	[ . . . ]
}

// Replace with
void OpenLogFile(bool bUseLogFile)
{
#if !defined(_DISTRIBUTE)
#	if defined(_EXTENDED_ERROR_LOG_)
	{
		// Create syserr & crash path
		std::filesystem::create_directories("syserr\\crash");

		// Create log file
		std::filesystem::path cur_path = std::filesystem::current_path();

		// Generate file based on time stamp
		const auto clock = std::chrono::system_clock::now();
		gs_timestamp = std::chrono::duration_cast<std::chrono::seconds>(clock.time_since_epoch()).count();

		cur_path /= "syserr\\" + std::to_string(gs_timestamp) + ".txt";

		auto file = freopen(cur_path.string().c_str(), "w", stderr);
		if (!file) throw std::runtime_error("Failed to create system error file.");
	}
#	else
	freopen("syserr.txt", "w", stderr);
#	endif
	if (bUseLogFile)
	{
		isLogFile = true;
		CLogFile::Instance().Initialize();
	}
#endif
}
