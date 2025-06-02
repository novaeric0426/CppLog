# CppLog

C++ 헤더 전용 로깅 라이브러리입니다. 비동기 로깅, 다중 출력 대상, 유연한 포맷팅을 지원합니다.

## ✨ 주요 기능

- **헤더 전용**: 단일 헤더 파일만 포함하면 끝
- **스레드 안전**: 워커 스레드를 사용한 비동기 로깅
- **크로스 플랫폼**: Windows, Linux, macOS 지원
- **다중 출력**: 콘솔 및 파일 출력, 쉬운 확장 가능
- **포맷 문자열**: Python 스타일 `{}` 플레이스홀더 포맷팅
- **로그 레벨**: DEBUG, INFO, WARN, ERROR, FATAL
- **스코프 로깅**: RAII 기반 함수/스코프 실행 시간 측정

## 🚀 빠른 시작

### 설치

이 라이브러리는 헤더 전용이므로 `logger.hpp` 파일을 프로젝트에 복사하고 포함하기만 하면 됩니다:

```cpp
#include "logger.hpp"
```

### 기본 사용법

```cpp
#include "logger.hpp"

int main() {
    auto& logger = utils::Logger::get_instance();
    
    // 콘솔 출력 추가
    logger.add_sink(std::make_unique<utils::ConsoleSink>());
    
    // 기본 로깅
    logger.info("안녕하세요, 세계!");
    logger.error("오류가 발생했습니다: {}", 42);
    
    return 0;
}
```

## 📖 사용 가이드

### 출력 대상(Sink) 설정

#### 콘솔 출력
```cpp
auto& logger = utils::Logger::get_instance();

// 컬러 콘솔 출력 (기본값)
logger.add_sink(std::make_unique<utils::ConsoleSink>(true));

// 컬러 없는 콘솔 출력
logger.add_sink(std::make_unique<utils::ConsoleSink>(false));
```

#### 파일 출력 및 로테이션
```cpp
// 기본 파일 로깅
logger.add_sink(std::make_unique<utils::FileSink>("app.log"));

// 사용자 정의 로테이션 설정
logger.add_sink(std::make_unique<utils::FileSink>(
    "app.log",           // 파일명
    5 * 1024 * 1024,    // 최대 크기: 5MB
    10                   // 보관할 파일 수: 10개
));
```

### 로그 레벨

```cpp
auto& logger = utils::Logger::get_instance();

// 최소 로그 레벨 설정
logger.set_level(utils::LogLevel::INFO);

// 다양한 로그 레벨
logger.debug("디버그 메시지: {}", value);
logger.info("애플리케이션이 시작되었습니다");
logger.warn("경고입니다: {}", warning_msg);
logger.error("오류가 발생했습니다: {}", error_code);
logger.fatal("치명적 오류: {}", fatal_msg);
```

### 포맷팅된 로깅

```cpp
logger.info("사용자 {}가 IP {}에서 로그인했습니다", username, ip_address);
logger.error("{} 항목 처리에 실패했습니다. {} 개 성공", total, success_count);
logger.debug("항목 처리 중 {}/{}: {}", current, total, item_name);
```

### 조건부 로깅

```cpp
// 조건이 참일 때만 로깅
logger.log_if(debug_enabled, utils::LogLevel::DEBUG, "디버그 정보: {}", data);

// 매크로 사용
LOG_IF(connection_failed, ERROR, "{} 번 시도 후 연결에 실패했습니다", retry_count);
```

### 스코프 로깅

함수 진입/종료를 실행 시간과 함께 자동으로 로깅합니다:

```cpp
void process_data() {
    LOG_SCOPE("process_data");  // 진입과 종료를 실행 시간과 함께 로깅
    
    // 함수 로직
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 소멸자가 자동으로 종료 시간을 로깅
}
```

출력 결과:
```
[2024-06-02 10:30:45.123] DEBUG [thread_id] → process_data 시작
[2024-06-02 10:30:45.223] DEBUG [thread_id] ← process_data 완료 (100ms)
```

### 매크로 사용

편의를 위해 제공되는 매크로를 사용할 수 있습니다:

```cpp
LOG_DEBUG("디버그 메시지: {}", value);
LOG_INFO("정보 메시지");
LOG_WARN("경고: {}", warning);
LOG_ERROR("오류: {}", error);
LOG_FATAL("치명적 오류: {}", fatal);

// 조건부 로깅
LOG_IF(condition, INFO, "조건부 메시지: {}", data);

// 다양한 레벨의 스코프 로깅
LOG_SCOPE_INFO("중요한_함수");
LOG_SCOPE_DEBUG("디버그_함수");
```

## ⚙️ 설정

### 큐 크기

```cpp
// 최대 큐 크기 설정 (기본값: 10000)
logger.set_max_queue_size(50000);
```

### 다중 출력 대상

```cpp
auto& logger = utils::Logger::get_instance();

// 여러 출력 대상 추가
logger.add_sink(std::make_unique<utils::ConsoleSink>());
logger.add_sink(std::make_unique<utils::FileSink>("app.log"));
logger.add_sink(std::make_unique<utils::FileSink>("error.log"));

// 모든 출력 대상 제거
logger.clear_sinks();
```

## 🔧 고급 사용법

### 사용자 정의 출력 대상

`LogSink`를 상속받아 자신만의 출력 대상을 만들 수 있습니다:

```cpp
class CustomSink : public utils::LogSink {
public:
    void write(const utils::LogEntry& entry) override {
        // 사용자 정의 로깅 로직
        custom_output << "[" << utils::format_timestamp(entry.timestamp) << "] "
                     << utils::level_to_string(entry.level) << ": "
                     << entry.message << std::endl;
    }
    
    void flush() override {
        custom_output.flush();
    }
};

// 사용자 정의 출력 대상 사용
logger.add_sink(std::make_unique<CustomSink>());
```

### 스레드 안전성

로거는 완전히 스레드 안전하며 여러 스레드에서 동시에 사용할 수 있습니다:

```cpp
void worker_thread(int thread_id) {
    for (int i = 0; i < 100; ++i) {
        LOG_INFO("스레드 {}가 항목 {} 처리 중", thread_id, i);
    }
}

int main() {
    auto& logger = utils::Logger::get_instance();
    logger.add_sink(std::make_unique<utils::ConsoleSink>());
    
    // 여러 스레드 시작
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker_thread, i);
    }
    
    // 모든 스레드 대기
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
```

## 📝 출력 형식

### 콘솔 출력
```
[2024-06-02 10:30:45.123] INFO  [140234] 애플리케이션이 시작되었습니다
[2024-06-02 10:30:45.124] ERROR [140234] 연결 실패: 타임아웃
```

### 파일 출력
```
[2024-06-02 10:30:45.123] [INFO ] [12345] 애플리케이션이 시작되었습니다
[2024-06-02 10:30:45.124] [ERROR] [12345] 연결 실패: 타임아웃
```

## 🖥️ 플랫폼 지원

- 모든 플랫폼에서 사용 가능합니다.
- **C++17**: 최소 요구 표준

## 📚 예제

더 자세한 사용 예제는 example.cpp 파일을 확인해주세요.

```
