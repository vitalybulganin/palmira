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
//-------------------------------------------------------------------------//
#include <server.h>
//-------------------------------------------------------------------------//
#include "../../../../common/modules-loader.h"
//-------------------------------------------------------------------------//
namespace {
//-------------------------------------------------------------------------//
  std::atomic_bool g_stop_requested = false;
  using create_server_t = server_t(*)(service_t, mtc::zmap config);

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
    std::cerr << "Usage:\n" << "  " << program_name << " <module_path> [port]\n\n" << "Example:\n" << "  " <<
      program_name << " ./libdocapi-http.so 9200\n";
  }

  std::vector<server_t> create_server_modules(std::string_view module_path, const mtc::zmap &config) {
    std::vector<server_t> servers;
    modules::modules_loader loader(module_path.data());
    // Loading a module.
    loader.load<create_server_t>("createServer", [&](create_server_t on_create_server, const std::string &module_name, const char *error) {
      if (error == nullptr) {
        auto server = on_create_server({}, config);
        if (server != nullptr) {
          // Saving a server.
          servers.push_back(server);

          // Starting a server.
          server->Start();
        }
      }
    });

    if (servers.empty()) {
      throw std::runtime_error("createServer returned empty server_t");
    }

    return servers;
  }

  auto destroy_server_modules(const std::vector<server_t> &servers) {
    for (auto &server : servers) {
      // Stopping the server.
      server->Stop();

      // Waiting for finishing it.
      server->Wait();

      // Releasing allocated resources.
      server->Detach();
    }
  }
}

int main(int argc, char** argv) {
  try {
    if (argc < 2 || argc > 3) {
      print_usage(argv[0]);
      return EXIT_FAILURE;
    }

    const std::string module_path = argv[1];
    docapi::common::config config{
      .port = argc >= 3 ? parse_port(argv[2]) : static_cast<std::uint16_t>(9200),
      .worker_threads = 2
    };

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // Creating a list of server modules.
    auto servers = create_server_modules(module_path, config);

    std::cout << "docapi host started. module='" << module_path << "', port=" << config.port << std::endl;
    while (!g_stop_requested.load(std::memory_order_acquire)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "stop requested, stopping server..." << std::endl;
    destroy_server_modules(servers);
  } catch (const std::exception& exception) {
    std::cerr << "fatal error: " << exception.what() << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "fatal error: unknown exception" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "docapi host stopped" << std::endl;
  return EXIT_SUCCESS;
}
