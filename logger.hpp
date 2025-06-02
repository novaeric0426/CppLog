#pragma once

#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <filesystem>

namespace utils {

	enum class LogLevel {
		DEBUG = 0,
		INFO = 1,
		WARN = 2,
		ERROR = 3,
		FATAL = 4
	};

	struct LogEntry {
		std::chrono::system_clock::time_point timestamp;
		LogLevel level;
		std::thread::id thread_id;
		std::string message;
	};

	inline std::string level_to_string(LogLevel level) {
		switch (level) {
		case LogLevel::DEBUG: return "DEBUG";
		case LogLevel::INFO:  return "INFO ";
		case LogLevel::WARN:  return "WARN ";
		case LogLevel::ERROR: return "ERROR";
		case LogLevel::FATAL: return "FATAL";
		default: return "UNKNOWN";
		}
	}

	inline std::string level_to_short_string(LogLevel level) {
		switch (level) {
		case LogLevel::DEBUG: return "D";
		case LogLevel::INFO:  return "I";
		case LogLevel::WARN:  return "W";
		case LogLevel::ERROR: return "E";
		case LogLevel::FATAL: return "F";
		default: return "?";
		}
	}

	inline std::string format_timestamp(const std::chrono::system_clock::time_point& tp) {
		auto time_t = std::chrono::system_clock::to_time_t(tp);
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			tp.time_since_epoch()) % 1000;

		std::tm time_info;
#ifdef _WIN32
		if (localtime_s(&time_info, &time_t) != 0) {
#else
		if (localtime_r(&time_t, &time_info) == nullptr) {
#endif
			throw std::runtime_error("Failed to convert time");
		}

		std::ostringstream oss;
		oss << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S");
		oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
		return oss.str();
	}

	class LogSink {
	public:
		virtual ~LogSink() = default;
		virtual void write(const LogEntry& entry) = 0;
		virtual void flush() = 0;
	};

	class ConsoleSink : public LogSink {
	private:
		bool use_colors_;

	public:
		explicit ConsoleSink(bool use_colors = true) : use_colors_(use_colors) {}

		void write(const LogEntry& entry) override {
			std::string color_code = use_colors_ ? get_color_code(entry.level) : "";
			std::string reset_code = use_colors_ ? get_reset_code() : "";

			std::cout << color_code
				<< "[" << format_timestamp(entry.timestamp) << "] "
				<< level_to_string(entry.level) << " "
				<< "[" << entry.thread_id << "] "
				<< entry.message
				<< reset_code << std::endl;
		}

		void flush() override {
			std::cout.flush();
		}

	private:
		inline std::string get_color_code(LogLevel level) const {
			switch (level) {
			case LogLevel::DEBUG: return "\033[36m";    // Cyan
			case LogLevel::INFO:  return "\033[32m";    // Green
			case LogLevel::WARN:  return "\033[33m";    // Yellow
			case LogLevel::ERROR: return "\033[31m";    // Red
			case LogLevel::FATAL: return "\033[35m";    // Magenta
			default: return "";
			}
		}

		inline std::string get_reset_code() const {
			return "\033[0m";
		}
	};

	class FileSink : public LogSink {
	private:
		std::string filename_;
		std::ofstream file_;
		size_t max_file_size_;
		int max_files_;
		size_t current_size_;

	public:
		FileSink(const std::string& filename,
			size_t max_file_size = 10 * 1024 * 1024,
			int max_files = 5)
			: filename_(filename), max_file_size_(max_file_size), max_files_(max_files), current_size_(0) {

			// 디렉터리 생성
			std::filesystem::path file_path(filename);
			if (file_path.has_parent_path()) {
				std::filesystem::create_directories(file_path.parent_path());
			}

			file_.open(filename_, std::ios::app);
			if (file_.is_open()) {
				// 기존 파일 크기 확인
				file_.seekp(0, std::ios::end);
				current_size_ = file_.tellp();
			}
		}

		~FileSink() {
			if (file_.is_open()) {
				file_.close();
			}
		}

		void write(const LogEntry& entry) override {
			if (!file_.is_open()) return;

			std::string log_line = "[" + format_timestamp(entry.timestamp) + "] "
				+ "[" + level_to_string(entry.level) + "] "
				+ "[" + std::to_string(std::hash<std::thread::id>{}(entry.thread_id)) + "] ";

			log_line += entry.message + "\n";

			file_ << log_line;
			current_size_ += log_line.length();

			check_and_rotate();
		}

		void flush() override {
			if (file_.is_open()) {
				file_.flush();
			}
		}

	private:
		inline void check_and_rotate() {
			if (current_size_ >= max_file_size_) {
				rotate_file();
			}
		}

		inline void rotate_file() {
			if (!file_.is_open()) return;

			file_.close();

			// 기존 로그 파일들 이름 변경
			for (int i = max_files_ - 1; i > 0; --i) {
				std::string old_name = filename_ + "." + std::to_string(i);
				std::string new_name = filename_ + "." + std::to_string(i + 1);

				if (std::filesystem::exists(old_name)) {
					if (i == max_files_ - 1) {
						std::filesystem::remove(new_name);  // 오래된 파일 삭제
					}
					std::filesystem::rename(old_name, new_name);
				}
			}

			// 현재 파일을 .1로 이름 변경
			if (std::filesystem::exists(filename_)) {
				std::filesystem::rename(filename_, filename_ + ".1");
			}

			// 새 파일 생성
			file_.open(filename_, std::ios::out);
			current_size_ = 0;
		}
	};

	class Logger {
	private:
		std::vector<std::unique_ptr<LogSink>> sinks_;
		LogLevel min_level_;
		std::queue<LogEntry> log_queue_;
		std::mutex queue_mutex_;
		std::condition_variable queue_condition_;
		std::thread worker_thread_;
		std::atomic<bool> running_;
		std::atomic<bool> initialized_;
		std::once_flag init_flag_;
		size_t max_queue_size_;

		template<typename T>
		void safe_append(std::ostringstream& oss, T&& value) {
			try {
				oss << std::forward<T>(value);
			}
			catch (...) {
				oss << "[FORMAT_ERROR]";
			}
		}

		// 템플릿 헬퍼 함수들 (Exception Safe)
		void format_recursive(std::ostringstream& oss, const std::string& format) {
			oss << format;
		}

		template<typename T, typename... Args>
		void format_recursive(std::ostringstream& oss, const std::string& format,
			T&& value, Args&&... args) {
			size_t pos = format.find("{}");
			if (pos != std::string::npos) {
				oss << format.substr(0, pos);
				safe_append(oss, std::forward<T>(value));
				format_recursive(oss, format.substr(pos + 2), std::forward<Args>(args)...);
			}
			else {
				oss << format;
				// 남은 인자들은 무시 (포맷 문자열에 {} 부족)
			}
		}

		// Lazy Initialization
		void ensure_initialized() {
			if (!initialized_.load(std::memory_order_acquire)) {
				std::call_once(init_flag_, [this] {
					running_ = true;
					worker_thread_ = std::thread(&Logger::worker_loop, this);
					initialized_.store(true, std::memory_order_release);
					});
			}
		}

		void log_entry(LogEntry&& entry) {
			ensure_initialized();

			{
				std::lock_guard<std::mutex> lock(queue_mutex_);

				// 큐가 가득 찬 경우 오래된 로그 드롭
				if (log_queue_.size() >= max_queue_size_) {
					log_queue_.pop();
				}

				log_queue_.emplace(std::move(entry));
			}
			queue_condition_.notify_one();
		}

		void worker_loop() {
			while (running_.load()) {
				std::unique_lock<std::mutex> lock(queue_mutex_);

				queue_condition_.wait(lock, [this] {
					return !log_queue_.empty() || !running_.load();
					});

				std::vector<LogEntry> batch;
				while (!log_queue_.empty() && batch.size() < 100) {
					batch.emplace_back(std::move(log_queue_.front()));
					log_queue_.pop();
				}

				lock.unlock();

				// 배치 처리된 로그들을 모든 싱크에 출력
				for (const auto& entry : batch) {
					for (auto& sink : sinks_) {
						try {
							sink->write(entry);
						}
						catch (const std::exception& e) {
							// 싱크 오류는 무시하고 계속 진행
							std::cerr << "Logger sink error: " << e.what() << std::endl;
						}
					}
				}

				// 모든 싱크 플러시
				for (auto& sink : sinks_) {
					try {
						sink->flush();
					}
					catch (const std::exception& e) {
						std::cerr << "Logger sink flush error: " << e.what() << std::endl;
					}
				}
			}

			// 종료 시 남은 로그들 처리
			std::lock_guard<std::mutex> lock(queue_mutex_);
			while (!log_queue_.empty()) {
				const auto& entry = log_queue_.front();
				for (auto& sink : sinks_) {
					try {
						sink->write(entry);
						sink->flush();
					}
					catch (...) {
						// 종료 시에는 에러 무시
					}
				}
				log_queue_.pop();
			}
		}

	public:
		static Logger& get_instance() {
			static Logger instance;
			return instance;
		}

		Logger() : min_level_(LogLevel::DEBUG),
			running_(false),
			initialized_(false),
			max_queue_size_(10000) {
		}

		~Logger() {
			if (initialized_.load()) {
				running_.store(false);
				queue_condition_.notify_all();

				if (worker_thread_.joinable()) {
					worker_thread_.join();
				}
			}
		}

		// 복사/이동 방지 (싱글톤)
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;
		Logger(Logger&&) = delete;
		Logger& operator=(Logger&&) = delete;

		void add_sink(std::unique_ptr<LogSink> sink) {
			ensure_initialized();
			sinks_.push_back(std::move(sink));
		}

		void clear_sinks() {
			ensure_initialized();
			sinks_.clear();
		}

		void set_level(LogLevel level) {
			min_level_ = level;
		}

		void set_max_queue_size(size_t size) {
			max_queue_size_ = size;
		}

		// 모든 템플릿 함수들
		template<typename... Args>
		void debug(const std::string& format, Args&&... args) {
			log(LogLevel::DEBUG, format, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void info(const std::string& format, Args&&... args) {
			log(LogLevel::INFO, format, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void warn(const std::string& format, Args&&... args) {
			log(LogLevel::WARN, format, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void error(const std::string& format, Args&&... args) {
			log(LogLevel::ERROR, format, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void fatal(const std::string& format, Args&&... args) {
			log(LogLevel::FATAL, format, std::forward<Args>(args)...);
		}

		// 조건부 로깅
		template<typename... Args>
		void log_if(bool condition, LogLevel level, const std::string& format, Args&&... args) {
			if (condition) {
				log(level, format, std::forward<Args>(args)...);
			}
		}

	private:
		template<typename... Args>
		void log(LogLevel level, const std::string& format, Args&&... args) {
			if (level < min_level_) return;

			std::ostringstream oss;
			try {
				if constexpr (sizeof...(args) > 0) {
					format_recursive(oss, format, std::forward<Args>(args)...);
				}
				else {
					oss << format;
				}
			}
			catch (...) {
				oss.str("");  // 스트림 초기화
				oss << "[LOG_ERROR] " << format;
			}

			LogEntry entry{
				std::chrono::system_clock::now(),
				level,
				std::this_thread::get_id(),
				oss.str()
			};

			log_entry(std::move(entry));
		}
	};

	// RAII 스코프 로깅
	class ScopeLogger {
	private:
		std::string scope_name_;
		std::chrono::steady_clock::time_point start_time_;
		LogLevel level_;

	public:
		ScopeLogger(const std::string& scope_name, LogLevel level = LogLevel::DEBUG)
			: scope_name_(scope_name), start_time_(std::chrono::steady_clock::now()), level_(level) {

			switch (level_) {
			case LogLevel::DEBUG: Logger::get_instance().debug("→ {} 시작", scope_name_); break;
			case LogLevel::INFO:  Logger::get_instance().info("→ {} 시작", scope_name_); break;
			case LogLevel::WARN:  Logger::get_instance().warn("→ {} 시작", scope_name_); break;
			case LogLevel::ERROR: Logger::get_instance().error("→ {} 시작", scope_name_); break;
			case LogLevel::FATAL: Logger::get_instance().fatal("→ {} 시작", scope_name_); break;
			}
		}

		~ScopeLogger() {
			auto end_time = std::chrono::steady_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);

			switch (level_) {
			case LogLevel::DEBUG: Logger::get_instance().debug("← {} 완료 ({}ms)", scope_name_, duration.count()); break;
			case LogLevel::INFO:  Logger::get_instance().info("← {} 완료 ({}ms)", scope_name_, duration.count()); break;
			case LogLevel::WARN:  Logger::get_instance().warn("← {} 완료 ({}ms)", scope_name_, duration.count()); break;
			case LogLevel::ERROR: Logger::get_instance().error("← {} 완료 ({}ms)", scope_name_, duration.count()); break;
			case LogLevel::FATAL: Logger::get_instance().fatal("← {} 완료 ({}ms)", scope_name_, duration.count()); break;
			}
		}
	};

	// 편의 매크로들
#define LOG_DEBUG(...) utils::Logger::get_instance().debug(__VA_ARGS__)
#define LOG_INFO(...)  utils::Logger::get_instance().info(__VA_ARGS__)
#define LOG_WARN(...)  utils::Logger::get_instance().warn(__VA_ARGS__)
#define LOG_ERROR(...) utils::Logger::get_instance().error(__VA_ARGS__)
#define LOG_FATAL(...) utils::Logger::get_instance().fatal(__VA_ARGS__)

// 조건부 로깅 매크로
#define LOG_IF(condition, level, ...) \
    utils::Logger::get_instance().log_if(condition, utils::LogLevel::level, __VA_ARGS__)

// 스코프 로깅 매크로
#define LOG_SCOPE(name) utils::ScopeLogger _scope_logger_(name)
#define LOG_SCOPE_DEBUG(name) utils::ScopeLogger _scope_logger_(name, utils::LogLevel::DEBUG)
#define LOG_SCOPE_INFO(name) utils::ScopeLogger _scope_logger_(name, utils::LogLevel::INFO)

} // namespace utils