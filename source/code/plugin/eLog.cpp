#include "eLog.h"
#include "../core/AchievementSettings.h"
#include <cstdarg>
#include <cstdio>
#include <Windows.h>

wchar_t eLog::path[260] = {};

// Opt-in via Log= in ManhuntAchievements.ini. Nothing is created or written
// while it is off, including before the INI has been read.
void eLog::Initialise()
{
    const auto file = AchievementSettings::ModuleDirectory() / L"ManhuntAchievements.log";
    wcsncpy_s(path, file.c_str(), _TRUNCATE);
    if (!AchievementSettings::bEnableLog)
        return;
    const auto previous = AchievementSettings::ModuleDirectory() / L"ManhuntAchievements.previous.log";
    CopyFileW(path, previous.c_str(), FALSE);
    if (FILE* log = _wfopen(path, L"w"))
    {
        fprintf(log, "ManhuntAchievements build %s %s | PID=%lu | detail level %d\n",
            __DATE__, __TIME__, GetCurrentProcessId(), AchievementSettings::iLogLevel);
        fclose(log);
    }
}

namespace
{
    void Write(const char* function, const char* format, va_list args)
    {
        FILE* log = _wfopen(eLog::path, L"a");
        if (!log)
            return;
        SYSTEMTIME now{};
        GetLocalTime(&now);
        fprintf(log, "%04u-%02u-%02u %02u:%02u:%02u.%03u [thread %lu] %s | ",
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute,
            now.wSecond, now.wMilliseconds, GetCurrentThreadId(), function);
        vfprintf(log, format, args);
        fputc('\n', log);
        fclose(log);
    }
}

bool eLog::Detailed()
{
    return AchievementSettings::iLogLevel >= 2 && path[0] != 0;
}

void eLog::Verbose(const char* function, const char* format, ...)
{
    if (!eLog::Detailed())
        return;
    va_list args;
    va_start(args, format);
    Write(function, format, args);
    va_end(args);
}

void eLog::Message(const char* function, const char* format, ...)
{
    if (!AchievementSettings::bEnableLog || !path[0])
        return;
    va_list args;
    va_start(args, format);
    Write(function, format, args);
    va_end(args);
}
