#pragma once

#include <chrono>
#include <ctime>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

enum class LogLevel { Debug = 0, Info, Warn, Error };

class Logger
{
public:
    static Logger& instance()
    {
        static Logger inst;
        return inst;
    }

    void init(const std::string& filename = "", LogLevel minLevel = LogLevel::Debug)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        minLevel_ = minLevel;
        if (!filename.empty())
        {
            file_.open(filename, std::ios::out | std::ios::app);
        }
    }

    void setMinLevel(LogLevel lvl)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        minLevel_ = lvl;
    }

    void log(LogLevel lvl, const char* file, int line, const char* func, const char* fmt, ...) {
        if (lvl < minLevel_) return;

        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        auto ts = nowString();
        const char* levelStr = levelToString(lvl);

        std::ostringstream oss;
        oss << ts << " [" << levelStr << "] "
            << file << ":" << line << " (" << func << ") - " << buffer << "\n";

        std::string out = oss.str();

        std::lock_guard<std::mutex> lk(mutex_);
        std::fwrite(out.data(), 1, out.size(), stdout);
        fflush(stdout);

        if (file_.is_open()) {
            file_ << out;
            file_.flush();
        }

        // #ifdef _WIN32
        // OutputDebugStringA(out.c_str());
        // #endif
    }

private:
    Logger() : minLevel_(LogLevel::Debug) {}
    ~Logger()
    {
        if (file_.is_open()) file_.close();
    }

    std::string nowString() {
        using namespace std::chrono;
        auto t = system_clock::now();
        auto tt = system_clock::to_time_t(t);
        auto ms = duration_cast<milliseconds>(t.time_since_epoch()) % 1000;

        std::tm tm_buf;
#if defined(_WIN32)
        localtime_s(&tm_buf, &tt);
#else
        localtime_r(&tt, &tm_buf);
#endif
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
            tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, static_cast<int>(ms.count()));
        return std::string(buf);
    }

    const char* levelToString(LogLevel l) const {
        switch (l) {
        case LogLevel::Debug: return "DBG";
        case LogLevel::Info:  return "INF";
        case LogLevel::Warn:  return "WRN";
        case LogLevel::Error: return "ERR";
        default: return "UNK";
        }
    }

    std::mutex mutex_;
    std::ofstream file_;
    LogLevel minLevel_;

#ifdef _DEBUG
#define LOG(level, fmt, ...) \
        do { Logger::instance().log(LogLevel::level, __FILE__, __LINE__, __func__, (fmt), ##__VA_ARGS__); } while(0)
#else
#define LOG(level, fmt, ...) do {} while(0)
#endif
};