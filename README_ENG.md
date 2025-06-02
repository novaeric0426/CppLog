# CppLog

A header-only C++ logging library with support for asynchronous logging, multiple output targets, and flexible formatting.

## ✨ Key Features

- **Header-only**: Just include a single header file
- **Thread-safe**: Asynchronous logging with worker thread
- **Cross-platform**: Windows, Linux, macOS support
- **Multiple outputs**: Console and file output with easy extensibility
- **Format strings**: Python-style `{}` placeholder formatting
- **Log levels**: DEBUG, INFO, WARN, ERROR, FATAL
- **Scope logging**: RAII-based function/scope execution timing

## 🚀 Quick Start

### Installation

Since this is a header-only library, simply copy the `logger.hpp` file to your project and include it:

```cpp
#include "logger.hpp"
```

### Basic Usage

```cpp
#include "logger.hpp"

int main() {
    auto& logger = utils::Logger::get_instance();
    
    // Add console output
    logger.add_sink(std::make_unique<utils::ConsoleSink>());
    
    // Basic logging
    logger.info("Hello, World!");
    logger.error("An error occurred: {}", 42);
    
    return 0;
}
```

## 📖 Usage Guide

### Setting Up Output Targets (Sinks)

#### Console Output
```cpp
auto& logger = utils::Logger::get_instance();

// Colored console output (default)
logger.add_sink(std::make_unique<utils::ConsoleSink>(true));

// Console output without colors
logger.add_sink(std::make_unique<utils::ConsoleSink>(false));
```

#### File Output with Rotation
```cpp
// Basic file logging
logger.add_sink(std::make_unique<utils::FileSink>("app.log"));

// Custom rotation settings
logger.add_sink(std::make_unique<utils::FileSink>(
    "app.log",           // filename
    5 * 1024 * 1024,    // max size: 5MB
    10                   // number of files to keep: 10
));
```

### Log Levels

```cpp
auto& logger = utils::Logger::get_instance();

// Set minimum log level
logger.set_level(utils::LogLevel::INFO);

// Various log levels
logger.debug("Debug message: {}", value);
logger.info("Application started");
logger.warn("This is a warning: {}", warning_msg);
logger.error("An error occurred: {}", error_code);
logger.fatal("Fatal error: {}", fatal_msg);
```

### Formatted Logging

```cpp
logger.info("User {} logged in from IP {}", username, ip_address);
logger.error("Failed to process {} items, {} succeeded", total, success_count);
logger.debug("Processing item {}/{}: {}", current, total, item_name);
```

### Conditional Logging

```cpp
// Log only when condition is true
logger.log_if(debug_enabled, utils::LogLevel::DEBUG, "Debug info: {}", data);

// Using macro
LOG_IF(connection_failed, ERROR, "Connection failed after {} attempts", retry_count);
```

### Scope Logging

Automatically log function entry/exit with execution timing:

```cpp
void process_data() {
    LOG_SCOPE("process_data");  // Logs entry and exit with execution time
    
    // Function logic
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Destructor automatically logs exit time
}
```

Output:
```
[2024-06-02 10:30:45.123] DEBUG [thread_id] → process_data 시작
[2024-06-02 10:30:45.223] DEBUG [thread_id] ← process_data 완료 (100ms)
```

### Using Macros

You can use the provided convenience macros:

```cpp
LOG_DEBUG("Debug message: {}", value);
LOG_INFO("Info message");
LOG_WARN("Warning: {}", warning);
LOG_ERROR("Error: {}", error);
LOG_FATAL("Fatal error: {}", fatal);

// Conditional logging
LOG_IF(condition, INFO, "Conditional message: {}", data);

// Scope logging with different levels
LOG_SCOPE_INFO("important_function");
LOG_SCOPE_DEBUG("debug_function");
```

## ⚙️ Configuration

### Queue Size

```cpp
// Set maximum queue size (default: 10000)
logger.set_max_queue_size(50000);
```

### Multiple Output Targets

```cpp
auto& logger = utils::Logger::get_instance();

// Add multiple output destinations
logger.add_sink(std::make_unique<utils::ConsoleSink>());
logger.add_sink(std::make_unique<utils::FileSink>("app.log"));
logger.add_sink(std::make_unique<utils::FileSink>("error.log"));

// Clear all output targets
logger.clear_sinks();
```

## 🔧 Advanced Usage

### Custom Output Target

Create your own custom sink by inheriting from `LogSink`:

```cpp
class CustomSink : public utils::LogSink {
public:
    void write(const utils::LogEntry& entry) override {
        // Your custom logging logic
        custom_output << "[" << utils::format_timestamp(entry.timestamp) << "] "
                     << utils::level_to_string(entry.level) << ": "
                     << entry.message << std::endl;
    }
    
    void flush() override {
        custom_output.flush();
    }
};

// Use your custom sink
logger.add_sink(std::make_unique<CustomSink>());
```

### Thread Safety

The logger is fully thread-safe and can be used from multiple threads simultaneously:

```cpp
void worker_thread(int thread_id) {
    for (int i = 0; i < 100; ++i) {
        LOG_INFO("Thread {} processing item {}", thread_id, i);
    }
}

int main() {
    auto& logger = utils::Logger::get_instance();
    logger.add_sink(std::make_unique<utils::ConsoleSink>());
    
    // Start multiple threads
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker_thread, i);
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
```

## 📝 Output Format

### Console Output
```
[2024-06-02 10:30:45.123] INFO  [140234] Application started
[2024-06-02 10:30:45.124] ERROR [140234] Connection failed: timeout
```

### File Output
```
[2024-06-02 10:30:45.123] [INFO ] [12345] Application started
[2024-06-02 10:30:45.124] [ERROR] [12345] Connection failed: timeout
```

## 🖥️ Platform Support

- Available on all platforms
- **C++17**: Minimum required standard

## 📚 Examples

For more detailed usage examples, please check the example.cpp file.