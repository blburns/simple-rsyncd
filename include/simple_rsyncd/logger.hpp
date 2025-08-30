#pragma once

#include <string>

namespace simple_rsyncd {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

class Logger {
public:
    Logger();
    ~Logger();
    
    void setLevel(LogLevel level);
    LogLevel getLevel() const;
    
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);

private:
    void log(const std::string& level, const std::string& message);
    
    LogLevel level_;
};

} // namespace simple_rsyncd
