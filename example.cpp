#include "logger.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <random>

void test_basic_logging() {
    LOG_SCOPE_INFO("�⺻ �α� �׽�Ʈ");

    LOG_DEBUG("����� �޽����Դϴ�");
    LOG_INFO("���� �޽����Դϴ�");
    LOG_WARN("��� �޽����Դϴ�");
    LOG_ERROR("���� �޽����Դϴ�");
    LOG_FATAL("ġ���� ���� �޽����Դϴ�");
}

void test_formatting() {
    LOG_SCOPE_INFO("������ �׽�Ʈ");

    int player_id = 12345;
    float x = 100.5f, y = 200.7f;
    std::string name = "PlayerOne";

    LOG_INFO("�÷��̾� ����: ID={}, �̸�={}", player_id, name);
    LOG_INFO("�÷��̾� {} ��ġ: ({}, {})", player_id, x, y);
    LOG_WARN("ü���� {}% ���ҽ��ϴ�", 25);

    // �پ��� Ÿ�� �׽�Ʈ
    LOG_DEBUG("����: {}, �Ǽ�: {}, ���ڿ�: {}, �Ҹ�: {}",
        42, 3.14, "�׽�Ʈ", true);
}

void test_conditional_logging() {
    LOG_SCOPE_INFO("���Ǻ� �α� �׽�Ʈ");

    int health = 20;
    int max_health = 100;
    bool is_critical = health < (max_health * 0.3);

    LOG_IF(is_critical, ERROR, "�÷��̾� ü���� �����մϴ�! ({}/{})", health, max_health);
    LOG_IF(!is_critical, INFO, "�÷��̾� ü���� ��ȣ�մϴ� ({}/{})", health, max_health);

    for (int i = 0; i < 5; ++i) {
        LOG_IF(i % 2 == 0, DEBUG, "¦�� �ݺ�: {}", i);
    }
}

void worker_thread(int thread_id, int message_count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delay_dist(10, 100);

    for (int i = 0; i < message_count; ++i) {
        LOG_INFO("������ {} - �޽��� {}/{}", thread_id, i + 1, message_count);

        if (i % 10 == 0) {
            LOG_WARN("������ {} - üũ����Ʈ {}", thread_id, i / 10);
        }

        if (i == message_count - 1) {
            LOG_ERROR("������ {} - ������ �޽���", thread_id);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
    }
}

void test_multithreading() {
    LOG_SCOPE_INFO("��Ƽ������ �׽�Ʈ");

    const int thread_count = 5;
    const int messages_per_thread = 10;
    std::vector<std::thread> threads;

    LOG_INFO("{}�� �����忡�� ���� {}�� �޽��� ���� ����", thread_count, messages_per_thread);

    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(worker_thread, i, messages_per_thread);
    }

    for (auto& t : threads) {
        t.join();
    }

    LOG_INFO("��� ������ �۾� �Ϸ�");
}

void performance_test() {
    LOG_SCOPE_INFO("���� �׽�Ʈ");

    const int message_count = 1000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < message_count; ++i) {
        LOG_DEBUG("���� �׽�Ʈ �޽��� {} - ������: {}, {}, {}",
            i, i * 2, i * 3.14, "test_string");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    LOG_INFO("{}�� �α� �޽��� ó�� �ð�: {}ms", message_count, duration.count());
    LOG_INFO("�ʴ� ó����: {} �޽���/��",
        static_cast<double>(message_count) / (duration.count() / 1000.0));
}

void test_log_levels() {
    LOG_SCOPE_INFO("�α� ���� �׽�Ʈ");

    auto& logger = utils::Logger::get_instance();

    LOG_INFO("=== ��� ���� ��� (DEBUG �̻�) ===");
    logger.set_level(utils::LogLevel::DEBUG);
    LOG_DEBUG("DEBUG ���� �޽���");
    LOG_INFO("INFO ���� �޽���");
    LOG_WARN("WARN ���� �޽���");

    LOG_INFO("=== WARN �̻� ��� ===");
    logger.set_level(utils::LogLevel::WARN);
    LOG_DEBUG("�� DEBUG �޽����� ������ �ʽ��ϴ�");
    LOG_INFO("�� INFO �޽����� ������ �ʽ��ϴ�");
    LOG_WARN("�� WARN �޽����� ���Դϴ�");
    LOG_ERROR("�� ERROR �޽����� ���Դϴ�");

    // �ٽ� DEBUG ������ ����
    logger.set_level(utils::LogLevel::DEBUG);
    LOG_INFO("�α� ������ DEBUG�� �����߽��ϴ�");
}

void test_scope_logging() {
    LOG_INFO("=== ������ �α� �׽�Ʈ ���� ===");

    {
        LOG_SCOPE("�����ͺ��̽� ����");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        {
            LOG_SCOPE_DEBUG("���� ����");
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            LOG_INFO("SELECT * FROM users ���� �Ϸ�");
        }

        {
            LOG_SCOPE_INFO("��� ó��");
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            LOG_INFO("100�� ���ڵ� ó�� �Ϸ�");
        }

        LOG_INFO("�����ͺ��̽� �۾� �Ϸ�");
    }

    LOG_INFO("=== ������ �α� �׽�Ʈ �Ϸ� ===");
}

int main() {
    try {
        std::cout << "�ϼ��� �ΰ� �׽�Ʈ ����!\n" << std::endl;

        // �ΰ� �ʱ�ȭ
        auto& logger = utils::Logger::get_instance();

        // �ܼ� ��ũ �߰� (���� ��� Ȱ��ȭ)
        logger.add_sink(std::make_unique<utils::ConsoleSink>(true));

        // ���� ��ũ �߰� (1MB, �ִ� 3�� ����)
        logger.add_sink(std::make_unique<utils::FileSink>(
            "logs/test.log",
            1024 * 1024,  // 1MB
            3             // 3�� ����
        ));

        // �α� ������ DEBUG�� ����
        logger.set_level(utils::LogLevel::DEBUG);

        LOG_INFO(" �ΰ� �ʱ�ȭ �Ϸ� - �ܼ� �� ���� ��� Ȱ��ȭ");
        LOG_INFO(" �α� ����: logs/test.log");

        // ���� �׽�Ʈ ����
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

        // ��Ŀ �����尡 ��� �α׸� ó���� �ð� ����
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        std::cout << "\n��� �׽�Ʈ �Ϸ�!" << std::endl;
        std::cout << "��� Ȯ��:" << std::endl;
        std::cout << "   - �ܼ�: ������ ����� �α� ���" << std::endl;
        std::cout << "   - ����: logs/test.log (�� ���� ����)" << std::endl;
        std::cout << "   - ����: �񵿱� ó���� ���� ó����" << std::endl;
        std::cout << "   - ������: ��Ƽ������ ȯ�濡�� ������ �α�" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "�׽�Ʈ ����: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}