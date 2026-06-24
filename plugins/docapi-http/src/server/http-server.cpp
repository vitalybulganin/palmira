#include "http-server.h"
//-------------------------------------------------------------------------//
#include <utility>
//-------------------------------------------------------------------------//
#include "../http/docreq.h"
//-------------------------------------------------------------------------//
namespace docapi {
//-------------------------------------------------------------------------//
  HttpServer::HttpServer(common::config cfg, mtc::api<palmira::IService> srv)
    : config(std::move(cfg)), service(std::move(srv)),
      pool(std::max(1U, std::thread::hardware_concurrency() - 1)) {
  }

 HttpServer::~HttpServer() {
    this->HttpServer::Stop();
    this->HttpServer::Wait();
  }
//-------------------------------------------------------------------------//
  void HttpServer::Start() {
    bool expected = false;

    if (not this->started.compare_exchange_strong(expected, true)) {
      return;
    }

    this->thread = std::thread([this]() {
        this->onloop();
    });

    std::unique_lock sync(this->mtx);
    this->cv.wait(sync, [this]() {
        return this->listen_socket != nullptr || this->start_failed;
    });
  }

  void HttpServer::Stop() {
    if (this->stopping.exchange(true)) {
      return;
    }

    this->pool.stop();

    if (this->loop != nullptr) {
      this->loop->defer([this]() {
          if (this->listen_socket != nullptr) {
              us_listen_socket_close(0, listen_socket);
              this->listen_socket = nullptr;
          }
      });
    }
  }

  void HttpServer::Wait() {
    if (this->thread.joinable()) {
      this->thread.join();
    }
  }
//-------------------------------------------------------------------------//
  auto HttpServer::onloop() -> void {
    this->loop = uWS::Loop::get();

    try {
      if (this->config.ssl.enabled) {
        uWS::SSLApp app({
          .key_file_name = this->config.ssl.priv_key_file.c_str(),
          .cert_file_name = this->config.ssl.cert_file.c_str(),
          .dh_params_file_name = this->config.ssl.dh_params_file.empty() ? nullptr : this->config.ssl.dh_params_file.c_str()
        });

        // Registering routes.
        this->register_routes<true>(app);

        // Listening the server.
        app.listen(this->config.listen_host, this->config.port, [this](auto *token) {
          this->onlisten(token);
        });

        // Executing server.
        app.run();
      } else {
        uWS::App app;
        // Registering routes.
        this->register_routes<false>(app);

        // Listening the server.
        app.listen(this->config.listen_host, this->config.port, [this](auto *token) {
          this->onlisten(token);
        });

        // Executing server.
        app.run();
      }
    } catch (...) {
      std::lock_guard sync(this->mtx);

      this->start_failed = true;
      this->cv.notify_all();
    }
  }

  auto HttpServer::onlisten(us_listen_socket_t *token) -> void {
    {
      std::lock_guard sync(this->mtx);
      this->listen_socket = token;
      this->start_failed = token == nullptr;
    }
    this->cv.notify_all();
  }
//-------------------------------------------------------------------------//
} // namespace docapi
