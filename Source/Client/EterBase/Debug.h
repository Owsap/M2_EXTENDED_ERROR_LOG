/// 1.
// Search
extern void SetLogLevel(UINT uLevel);

// Add above
#if defined(_EXTENDED_ERROR_LOG_)
extern const std::time_t GetLogFileTimeStamp();
#endif
