# include "netServer.hpp"
# include "../service/structo-search.hpp"
# include "../plugins.hpp"
# include "structo/context/x-contents.hpp"
# include "structo/queries.hpp"
# include "structo/indexer/layered-contents.hpp"
# include "structo/storage/posix-fs.hpp"
# include <mtc/sharedLibrary.hpp>
# include <mtc/config.h>
# include <mtc/json.h>
# include <csignal>
# include <zlib.h>
# include <system_error>
# include <functional>
# include <thread>
//-------------------------------------------------------------------------//
#include "../modules/common/modules-loader.h"
#include "server.h"
//-------------------------------------------------------------------------//
using create_server_t = server_t(*)(service_t, mtc::config module_settings);
//-------------------------------------------------------------------------//
std::atomic_bool g_stop_requested = false;
// Keeps a path of modules.
constexpr std::string_view g_modules_path = "lib/libdocapi-http.so";
//-------------------------------------------------------------------------//
namespace {
//-------------------------------------------------------------------------//
  std::function<void(int signo)> g_signal;
//-------------------------------------------------------------------------//
  void blockSignals() {
    sigset_t mask;
    sigemptyset(&mask);

    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGHUP);

    // Блокируем сигналы для всего процесса
    if (pthread_sigmask( SIG_BLOCK, &mask, nullptr ) != 0) {
      throw std::system_error( errno, std::system_category(), "Failed to block signals" );
    }
  }

  void onsignal(int signo) {
    if (g_signal != nullptr) {
      g_signal(signo);
    }
  }

  template<class ... Exceptions, class Action>
  int Protected( Action action );

  template<class Exp, class ... Exceptions, class Action>
  int Protected(Action action) {
    try {
      return Protected<Exceptions...>( action );
    } catch (const Exp &xp) {
      return fprintf( stderr, "%s\n", xp.what() ), EFAULT;
    }
  }

  template<class Action>
  int Protected(Action action) {
    return action();
  }

  template<class ... Exceptions, class Action, class ... Args>
  int Protected( Action action, Args ... args) {
    return Protected<Exceptions...>(action, args...);
  }

  std::vector<server_t> create_server_modules(service_t service, const mtc::config &config) {
    std::vector<server_t> servers;
    uint8_t port_index = 0;
    palmira::modules::modules_loader loader(config);
    // Loading a module.
    loader.load<create_server_t>("createServer", [&](create_server_t on_create_server, const mtc::zmap &module_settings, const char *error) {
      if (error == nullptr) {
        auto server = on_create_server(service, module_settings);
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
//-------------------------------------------------------------------------//
} // namespace
//-------------------------------------------------------------------------//
int main(int argc, char* argv[]) {
  auto search = mtc::api<palmira::IService>();
  auto config = mtc::config();

  if (argc < 2) {
    return fprintf( stdout, "Usage: %s palmira.config\n", argv[0] ), EINVAL;
  }

// open the configuration
  try {
    config = mtc::config::Open(argv[1]);

    // create search
    auto cfg = config.get_section("service");
    if (cfg.empty()) {
      return fprintf(stderr, "Section 'service' not found in configuration file\n");
    }

    if ((search = palmira::CreateStructo(cfg)) == nullptr) {
      throw std::logic_error("unexpected OpenSearch(...) result 'nullptr'");
    }
  } catch (const mtc::config::error &exc) {
    return fprintf( stderr, "Config error: %s\n", exc.what());
  } catch (const mtc::json::parse::error &exc) {
    return fprintf( stderr, "Error parsing config '%s', line %d: %s\n", argv[1], exc.get_json_lineid(), exc.what());
  } catch (const std::exception &exc) {
    return fprintf( stderr, "Error parsing config '%s': %s\n", argv[1], exc.what());
  } catch (...) {
    return fprintf( stderr, "Unknown error opening config\n" );
  }

  // Creating a list of server modules.
  auto servers = create_server_modules(search, config.get_section("modules"));
/*<???>
  try {
    server = CreateInetServer( search, config );
  } catch (const std::invalid_argument & xp) {
    return fprintf(stderr, "Invalid argument: %s\n", xp.what()), EINVAL;
  }
*/

// install signals handler
  struct sigaction sa = {0};

  sa.sa_handler = onsignal;
  sigemptyset(&sa.sa_mask);

  sigaction(SIGINT,  &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);
  sigaction(SIGQUIT, &sa, nullptr);

  g_signal = [&](int signo) {
    std::fprintf(stdout, "Received signal: %d. The process will be stopped...\n", signo);
    // Destroying a list of server modules.
    destroy_server_modules(servers);

    // Setting STOPPED state.
    g_stop_requested.store(true, std::memory_order_release);
  };

  while (not g_stop_requested.load(std::memory_order_acquire)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  // Commiting indexes.
  search->Commit();

  std::fprintf(stdout, "The process stopped\n");
  return 0;
}
