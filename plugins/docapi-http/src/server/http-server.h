/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          http-server.h
* - Created:       06/23/2026
* - Author:        Vitaly Bulganin
* - Description:
* - Comments:
*
-----------------------------------------------------------------------------
*
* - History:
*
===========================================================================*/
#pragma once
//-------------------------------------------------------------------------//
#ifndef __HTTP_SERVER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __HTTP_SERVER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
//-------------------------------------------------------------------------//
#include <App.h>
//-------------------------------------------------------------------------//
#include "server.hpp"
#include "service.hpp"
//-------------------------------------------------------------------------//
#include "../common/thread-pool.h"
#include "../common/config.h"
//-------------------------------------------------------------------------//
#include "../http/errors.h"
#include "../http/docreq.h"
#include "../http/docres.h"
//-------------------------------------------------------------------------//
namespace docapi {
//-------------------------------------------------------------------------//
  namespace {
    struct async_response_state final {
      std::atomic_bool aborted{false};
    };
  } // namespace
//-------------------------------------------------------------------------//
  class HttpServer : public palmira::IServer {
    using service_t = mtc::api<palmira::IService>;

    //!< Keeps a server config.
    const common::config config;

    //!< Keeps a search service.
    service_t service;

    //!< Keeps a pool of workers.
    common::thread_pool pool;

    std::thread thread;
    std::atomic_bool started{false};
    std::atomic_bool stopping{false};

    std::mutex mtx;
    std::condition_variable cv;
    bool start_failed = false;

    uWS::Loop *loop = nullptr;
    us_listen_socket_t *listen_socket = nullptr;

  public:
    /**
     * Constructor.
     * @param config [in] - A server configuration.
     * @param service [in] - A search service.
     */
    explicit HttpServer(common::config config, mtc::api<palmira::IService> service);

    /**
     * Destructor.
     */
    ~HttpServer();

    // Override methods
  public:
    void Start() override;
    void Stop() override;
    void Wait() override;

  protected:
    implement_lifetime_control

    template<bool SSL>
    auto register_routes(uWS::TemplatedApp<SSL> &app) -> void;

    template<bool SSL, typename Response, typename Request>
    auto onindex(Response *res, Request *req) -> void;

    template <bool SSL, typename Response, typename Request>
    auto onget(Response *res, Request *req) -> void;

    template<typename Response, typename Fn>
    auto onsubmit(Response *res, std::shared_ptr<async_response_state> state, Fn &&fn) -> void;

  private:
    auto onloop() -> void;
    auto onlisten(us_listen_socket_t *token) -> void;
  };
//-------------------------------------------------------------------------//
  template <bool SSL>
  auto HttpServer::register_routes(uWS::TemplatedApp<SSL> &app) -> void {
    app.put("/:index/_doc/:id", [this](auto *res, auto *req) {
      this->onindex<SSL>(res, req);
    });

    app.post("/:index/_doc", [this](auto *res, auto *req) {
      this->onindex<SSL>(res, req);
    });

    app.put("/:index/_create/:id", [this](auto *res, auto *req) {
      this->onindex<SSL>(res, req);
    });

    app.post("/:index/_create/:id", [this](auto *res, auto *req) {
      this->onindex<SSL>(res, req);
    });

    app.get("/:index/_doc/:id", [this](auto *res, auto *req) {
      this->onget<SSL>(res, req);
    });

    app.get("/:index/_source/:id", [this](auto *res, auto *req) {
      this->onget<SSL>(res, req);
    });
  }

  template<bool SSL, typename Response, typename Request>
  auto HttpServer::onindex(Response *res, Request *req) -> void {
    auto state = std::make_shared<async_response_state>();
    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    const std::string index(req->getParameter(0));
    const std::string id = req->getParameter(1).empty() ? std::string{} : std::string(req->getParameter(1));
    const auto params = http::parse_query(req->getQuery());

    auto body = std::make_shared<std::string>();

    // Reserving resources.
    body->reserve(4096);

    res->onData([this, res, state, index, id, params, body](std::string_view chunk, bool last) mutable {
      if (state->aborted.load(std::memory_order_acquire)) {
        return;
      }

      if (body->size() + chunk.size() > config.max_body_size) {
        state->aborted.store(true, std::memory_order_release);
        res->writeStatus("413 Payload Too Large")->end(http::make_error_json("payload_too_large", "request body is too large"));
        return;
      }

      body->append(chunk.data(), chunk.size());

      if (not last) {
        return;
      }

      auto request = http::make_index_request(index, id, std::move(*body), params);

/*<TODO> Add impl
      this->onsubmit(res, state, [service = this->service, request = std::move(request), create_only]() mutable {
        return create_only ? service->CreateDocument(std::move(request)) : service->IndexDocument(std::move(request));
      });
*/
    });
  }

  template<bool SSL, typename Response, typename Request>
  auto HttpServer::onget(Response *res, Request *req) -> void {
    auto ctx = std::make_shared<http::response_context<SSL>>(res);

    res->onAborted([ctx]() {
      ctx->aborted.store(true, std::memory_order_release);
    });

    /*<!!!>
     * uWebSockets response живёт только в родном event-loop потоке.
     * Тяжёлая работа уходит в ThreadPool, а ответ возвращается через loop->defer().
     */
    this->pool.enqueue([service = this->service, ctx, req, loop = uWS::Loop::get()]() mutable {
      const auto params = http::parse_query(req->getQuery());
      const auto routing = http::get_query_param(params, "routing");
      const http::document_get_item doc{
        .index = std::string(req->getParameter(0)),
        .id = std::string(req->getParameter(1)),
        .routing = routing.has_value() ? routing.value() : "",
        .options = http::parse_get_options(params)
      };

      try {
        palmira::SearchArgs search;

        // Forwarding a search request into search engine.
        service->Search(search, [ctx, loop](const mtc::zmap &resp) {
          http::service_response reply;

          // Converting a search response into reply.

          // Replying a response to client.
          loop->defer([ctx, reply = std::move(reply)]() mutable {
            if (ctx->aborted.load(std::memory_order_acquire)) {
              return;
            }

            // Replying a response.
            http::send_json_response(ctx.get(), reply);
          });
        });
      } catch (const std::exception &exc) {
        // Replying error response.
        http::send_error_response(ctx.get(), 500, "internal_server_error", exc.what());
      } catch (...) {
        // Replying error response.
        http::send_error_response(ctx.get(), 500, "internal_server_error", "unknown");
      }
    });
  }

  template<typename Response, typename Fn>
  auto HttpServer::onsubmit(Response* res, std::shared_ptr<async_response_state> state, Fn &&fn) -> void {
    auto *loop = this->loop;

    const bool posted = this->pool.enqueue([loop, res, state, fn = std::forward<Fn>(fn)]() mutable {
      http::service_response response;

      try {
        response = fn();
      } catch (const std::exception &exc) {
        response.status = 500;
        response.body = http::make_error_json("internal_error", exc.what());
      } catch (...) {
        response.status = 500;
        response.body = http::make_error_json("internal_error", "unknown error");
      }

      loop->defer([res, state, response = std::move(response)]() mutable {
        if (state->aborted.load(std::memory_order_acquire)) {
          return;
        }

        res->writeStatus(http::status_to_string(response.status))->writeHeader("Content-Type", response.content_type)->end(response.body);
      });
    });

    if (not posted) {
      res->writeStatus("503 Service Unavailable")->writeHeader("Content-Type", "application/json; charset=utf-8")->end(http::make_error_json("service_unavailable", "server is stopping"));
    }
  }
//-------------------------------------------------------------------------//
} // namespace docapi
//-------------------------------------------------------------------------//
#endif // __HTTP_SERVER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__

