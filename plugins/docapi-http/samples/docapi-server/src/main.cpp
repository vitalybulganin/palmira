#include <dlfcn.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <csignal>

#include <server.h>

/*
 * История изменений:
 * 2026-06-26:
 * - Добавлен минимальный host-service для загрузки HTTP-модуля через dlopen/dlsym.
 * - Используется существующая точка входа createServer(std::uint16_t).
 * - Добавлена RAII-обёртка для dlopen/dlclose.
 * - Добавлена безопасная обработка SIGINT/SIGTERM.
 * - Добавлено гарантированное выполнение Stop() перед завершением процесса.
 *
 * Назначение файла:
 * Данный файл реализует минимальный исполняемый сервис, который динамически
 * загружает модуль REST Document API, получает из него функцию createServer(),
 * создаёт экземпляр сервера, запускает его и держит процесс живым до получения
 * сигнала завершения.
 *
 * Важно:
 * Сервис не линкуется с HTTP-модулем напрямую. Модуль загружается во время
 * выполнения через dlopen(). Это позволяет обновлять/подменять реализацию
 * HTTP-модуля без пересборки host-service, если ABI интерфейса не меняется.
 */

namespace {
std::atomic_bool g_stop_requested = false;

void SignalHandler(int signal_number) {
    /*
     * В обработчике сигнала нельзя выполнять сложную логику:
     * нельзя логировать через iostream, нельзя брать mutex, нельзя вызывать
     * произвольные методы C++ объектов.
     *
     * Поэтому только выставляем атомарный флаг.
     */
    (void)signal_number;
    g_stop_requested.store(true, std::memory_order_release);
}

class shared_library final {
public:
    explicit shared_library(std::string path) : path_(std::move(path)) {
        handle_ = dlopen(path_.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (handle_ == nullptr) {
            const char* error = dlerror();

            throw std::runtime_error(std::string("dlopen failed for '") + path_ + "': " + (error != nullptr ? error : "unknown error"));
        }
    }

    ~shared_library() {
        if (handle_ != nullptr) {
            dlclose(handle_);
            handle_ = nullptr;
        }
    }

    shared_library(const shared_library&) = delete;
    shared_library& operator=(const shared_library&) = delete;

    shared_library(shared_library&&) = delete;
    shared_library& operator=(shared_library&&) = delete;

    template <typename Symbol>
    Symbol load_symbol(const char* symbol_name) const {
        /*
         * dlerror() хранит ошибку потока. Перед dlsym() очищаем старое значение,
         * чтобы корректно отличить реальную ошибку от валидного nullptr-символа.
         */
        dlerror();

        void* symbol = dlsym(handle_, symbol_name);
        const char* error = dlerror();

        if (error != nullptr) {
            throw std::runtime_error(std::string("dlsym failed for symbol '") + symbol_name + "': " + error);
        }
        return reinterpret_cast<Symbol>(symbol);
    }

private:
    std::string path_;
    void* handle_ = nullptr;
};

std::uint16_t parse_port(std::string_view value) {
    std::size_t processed = 0;

    const int port = std::stoi(std::string(value), &processed, 10);
    if (processed != value.size()) {
        throw std::runtime_error("port contains non-numeric characters");
    }

    if (port <= 0 || port > 65535) {
        throw std::runtime_error("port must be in range 1..65535");
    }

    return static_cast<std::uint16_t>(port);
}

void print_usage(const char* program_name) {
    std::cerr<< "Usage:\n" << "  " << program_name << " <module_path> [port]\n\n" << "Example:\n" << "  " << program_name << " ./libdocapi-http.so 9200\n";
}

}

/*
 * Ниже предполагается, что server_t известен host-service на этапе компиляции.
 *
 * То есть main.cpp должен включать ваш публичный заголовок модуля, например:
 *
 *   #include \"docapi/server.h\"
 *
 * где объявлены:
 *
 *   using server_t = ...;
 *   class/server interface with Start(), Stop(), Wait()
 *
 * Здесь я оставляю место для include, потому что точное имя вашего заголовка
 * зависит от текущей структуры проекта.
 */

int main(int argc, char** argv) {
    using create_server_fn = server_t(*)(std::uint16_t listening_port);

  try {
        if (argc < 2 || argc > 3) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        const std::string module_path = argv[1];
        const std::uint16_t port = argc >= 3 ? parse_port(argv[2]) : static_cast<std::uint16_t>(9200);

        std::signal(SIGINT, SignalHandler);
        std::signal(SIGTERM, SignalHandler);

        shared_library module(module_path);


        auto create_server = module.load_symbol<create_server_fn>("createServer");
        auto server = create_server(port);
        if (server == nullptr) {
            throw std::runtime_error("createServer returned empty server_t");
        }

        server->Start();

        std::cout << "docapi host started. module='" << module_path << "', port=" << port << std::endl;

        while (!g_stop_requested.load(std::memory_order_acquire)) {
            /*
             * Если ваш server->Wait() блокирует до Stop(), можно заменить
             * этот цикл на отдельную схему с signal thread.
             *
             * Здесь оставлен polling-вариант, потому что он минимален и
             * безопасен для случая, когда Wait() тоже блокирующий.
             */
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        std::cout << "stop requested, stopping server..." << std::endl;

        server->Stop();

        /*
         * Важно:
         * server должен быть разрушен ДО dlclose().
         * Иначе vtable/код деструктора могут уже находиться в выгруженной .so.
         */
        server->Detach();

        std::cout << "docapi host stopped" << std::endl;
    } catch (const std::exception& exception) {
        std::cerr << "fatal error: " << exception.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "fatal error: unknown exception" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
