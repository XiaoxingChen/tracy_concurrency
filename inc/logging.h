#if !defined(_LOGGING_H_)
#define _LOGGING_H_

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog/spdlog.h>
#include <vector>

#define LOGT(...) spdlog::trace(__VA_ARGS__)
#define LOGI(...) spdlog::info(__VA_ARGS__);
#define LOGW(...) spdlog::warn(__VA_ARGS__);
#define LOGE(...) spdlog::error(__VA_ARGS__);
#define LOGD(...) spdlog::debug(__VA_ARGS__);

inline void initLogger()
{
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    auto logger = std::make_shared<spdlog::logger>("tracy", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::info);
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e][%n][%^%l%$][%t] %v");
    LOGI("Logger initialized");
}

inline void destroyLogger()
{
    spdlog::drop_all();
}

#endif // _LOGGING_H_
