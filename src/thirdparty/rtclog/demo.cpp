#include "rtclog.h"
#include <vector>
#include <string>

// 模拟一个需要检查的函数
int process_data(const std::vector<int>& data) {
    // 使用宏检查条件，如果失败则打印错误日志并返回 -1
    RTC_CHECK_AND_RET_LOG(!data.empty(), -1, "Data is empty!");
    
    // 正常的逻辑日志
    RTCLOG(RTC_INFO, "Processing %zu items", data.size());
    
    for (size_t i = 0; i < data.size(); ++i) {
        // 详细的调试日志
        RTCLOG(RTC_DEBUG, "Item[%zu] = %d", i, data[i]);
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    // 1. 初始化日志库
    // 传入应用名称或标识
    rtclog_init("demo_app");

    // 2. 配置日志 (C++ 特有)
    // 参数: 
    // - filename_base: 日志文件路径前缀 (例如 "logs/app")
    // - console: 是否同时输出到控制台 (true)
    // - max_log_file_size: 单个日志文件最大大小 (默认 500MB)
    // - max_log_files: 保留的最大日志文件数量
    rtclog_configure("logs/demo.log", true);

    // 3. 设置日志级别
    // 只有级别 <= RTC_DEBUG 的日志才会被记录
    // 级别顺序: FATAL(0) < ERROR(1) < WARN(2) < INFO(3) < DEBUG(4) < TRACE(5)
    rtclog_set_level(RTC_DEBUG);

    RTCLOG(RTC_INFO, "Application started");

    // 4. 基本日志记录用法
    RTCLOG(RTC_WARN, "This is a warning message");
    RTCLOG(RTC_ERROR, "This is an error message with code: %d", 404);

    // 5. 演示函数调用中的日志
    std::vector<int> my_data = {1, 2, 3, 4, 5};
    process_data(my_data);

    // 演示错误情况
    std::vector<int> empty_data;
    if (process_data(empty_data) != 0) {
        RTCLOG(RTC_WARN, "Failed to process empty data");
    }

    // 6. 断言用法
    // 如果条件为假，会打印 FATAL 日志 (注意：根据实现可能会导致程序退出或仅记录)
    int x = 10;
    RTC_ASSERT(x > 0);

    RTCLOG(RTC_INFO, "Application finished");

    return 0;
}
