#include "logger.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <random>

void test_basic_logging() {
    LOG_SCOPE_INFO("기본 로깅 테스트");

    LOG_DEBUG("디버그 메시지입니다");
    LOG_INFO("정보 메시지입니다");
    LOG_WARN("경고 메시지입니다");
    LOG_ERROR("에러 메시지입니다");
    LOG_FATAL("치명적 오류 메시지입니다");
}

void test_formatting() {
    LOG_SCOPE_INFO("포맷팅 테스트");

    int player_id = 12345;
    float x = 100.5f, y = 200.7f;
    std::string name = "PlayerOne";

    LOG_INFO("플레이어 정보: ID={}, 이름={}", player_id, name);
    LOG_INFO("플레이어 {} 위치: ({}, {})", player_id, x, y);
    LOG_WARN("체력이 {}% 남았습니다", 25);

    // 다양한 타입 테스트
    LOG_DEBUG("정수: {}, 실수: {}, 문자열: {}, 불린: {}",
        42, 3.14, "테스트", true);
}

void test_conditional_logging() {
    LOG_SCOPE_INFO("조건부 로깅 테스트");

    int health = 20;
    int max_health = 100;
    bool is_critical = health < (max_health * 0.3);

    LOG_IF(is_critical, ERROR, "플레이어 체력이 위험합니다! ({}/{})", health, max_health);
    LOG_IF(!is_critical, INFO, "플레이어 체력이 양호합니다 ({}/{})", health, max_health);

    for (int i = 0; i < 5; ++i) {
        LOG_IF(i % 2 == 0, DEBUG, "짝수 반복: {}", i);
    }
}

void worker_thread(int thread_id, int message_count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delay_dist(10, 100);

    for (int i = 0; i < message_count; ++i) {
        LOG_INFO("스레드 {} - 메시지 {}/{}", thread_id, i + 1, message_count);

        if (i % 10 == 0) {
            LOG_WARN("스레드 {} - 체크포인트 {}", thread_id, i / 10);
        }

        if (i == message_count - 1) {
            LOG_ERROR("스레드 {} - 마지막 메시지", thread_id);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
    }
}

void test_multithreading() {
    LOG_SCOPE_INFO("멀티스레드 테스트");

    const int thread_count = 5;
    const int messages_per_thread = 10;
    std::vector<std::thread> threads;

    LOG_INFO("{}개 스레드에서 각각 {}개 메시지 전송 시작", thread_count, messages_per_thread);

    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(worker_thread, i, messages_per_thread);
    }

    for (auto& t : threads) {
        t.join();
    }

    LOG_INFO("모든 스레드 작업 완료");
}

void performance_test() {
    LOG_SCOPE_INFO("성능 테스트");

    const int message_count = 1000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < message_count; ++i) {
        LOG_DEBUG("성능 테스트 메시지 {} - 데이터: {}, {}, {}",
            i, i * 2, i * 3.14, "test_string");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    LOG_INFO("{}개 로그 메시지 처리 시간: {}ms", message_count, duration.count());
    LOG_INFO("초당 처리량: {} 메시지/초",
        static_cast<double>(message_count) / (duration.count() / 1000.0));
}

void test_log_levels() {
    LOG_SCOPE_INFO("로그 레벨 테스트");

    auto& logger = utils::Logger::get_instance();

    LOG_INFO("=== 모든 레벨 출력 (DEBUG 이상) ===");
    logger.set_level(utils::LogLevel::DEBUG);
    LOG_DEBUG("DEBUG 레벨 메시지");
    LOG_INFO("INFO 레벨 메시지");
    LOG_WARN("WARN 레벨 메시지");

    LOG_INFO("=== WARN 이상만 출력 ===");
    logger.set_level(utils::LogLevel::WARN);
    LOG_DEBUG("이 DEBUG 메시지는 보이지 않습니다");
    LOG_INFO("이 INFO 메시지도 보이지 않습니다");
    LOG_WARN("이 WARN 메시지는 보입니다");
    LOG_ERROR("이 ERROR 메시지도 보입니다");

    // 다시 DEBUG 레벨로 복원
    logger.set_level(utils::LogLevel::DEBUG);
    LOG_INFO("로그 레벨을 DEBUG로 복원했습니다");
}

void test_scope_logging() {
    LOG_INFO("=== 스코프 로깅 테스트 시작 ===");

    {
        LOG_SCOPE("데이터베이스 연결");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        {
            LOG_SCOPE_DEBUG("쿼리 실행");
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            LOG_INFO("SELECT * FROM users 실행 완료");
        }

        {
            LOG_SCOPE_INFO("결과 처리");
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            LOG_INFO("100개 레코드 처리 완료");
        }

        LOG_INFO("데이터베이스 작업 완료");
    }

    LOG_INFO("=== 스코프 로깅 테스트 완료 ===");
}

int main() {
    try {
        std::cout << "완성된 로거 테스트 시작!\n" << std::endl;

        // 로거 초기화
        auto& logger = utils::Logger::get_instance();

        // 콘솔 싱크 추가 (색상 출력 활성화)
        logger.add_sink(std::make_unique<utils::ConsoleSink>(true));

        // 파일 싱크 추가 (1MB, 최대 3개 파일)
        logger.add_sink(std::make_unique<utils::FileSink>(
            "logs/test.log",
            1024 * 1024,  // 1MB
            3             // 3개 파일
        ));

        // 로그 레벨을 DEBUG로 설정
        logger.set_level(utils::LogLevel::DEBUG);

        LOG_INFO(" 로거 초기화 완료 - 콘솔 및 파일 출력 활성화");
        LOG_INFO(" 로그 파일: logs/test.log");

        // 각종 테스트 실행
        test_basic_logging();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        test_formatting();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        test_conditional_logging();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        test_log_levels();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        test_scope_logging();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        test_multithreading();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        performance_test();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 워커 스레드가 모든 로그를 처리할 시간 제공
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        std::cout << "\n모든 테스트 완료!" << std::endl;
        std::cout << "결과 확인:" << std::endl;
        std::cout << "   - 콘솔: 색상이 적용된 로그 출력" << std::endl;
        std::cout << "   - 파일: logs/test.log (상세 정보 포함)" << std::endl;
        std::cout << "   - 성능: 비동기 처리로 높은 처리량" << std::endl;
        std::cout << "   - 안전성: 멀티스레드 환경에서 안전한 로깅" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "테스트 실패: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}