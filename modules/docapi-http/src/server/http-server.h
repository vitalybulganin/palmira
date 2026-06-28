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
#include "../../../include/server.h"
//-------------------------------------------------------------------------//
#include "../common/thread-pool.h"
//-------------------------------------------------------------------------//
#include "../http/errors.h"
#include "../http/docreq.h"
#include "../http/docres.h"
//-------------------------------------------------------------------------//
namespace docapi {
//-------------------------------------------------------------------------//
  namespace {
//-------------------------------------------------------------------------//
    struct async_response_state final {
      std::atomic_bool aborted{false};
    };
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  class HttpServer : public palmira::IServer {
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
     * @param service [in] - A search service.
     * @param config [in] - A server configuration.
     */
    explicit HttpServer(mtc::api<palmira::IService> service, common::config config);

    /**
     * Destructor.
     */
    virtual ~HttpServer();

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

    template<bool SSL, typename Response, typename Request>
    auto onput(Response *res, Request *req) -> void;

    template <bool SSL, typename Response, typename Request>
    auto onget(Response *res, Request *req) -> void;

    template<typename Response, typename Fn>
    auto onsubmit(Response *res, const std::shared_ptr<async_response_state>& state, Fn &&fn) -> void;

  private:
    auto onloop() -> void;
    auto onlisten(us_listen_socket_t *token) -> void;
  };
//-------------------------------------------------------------------------//
  template <bool SSL>
  auto HttpServer::register_routes(uWS::TemplatedApp<SSL> &app) -> void {
    app.put("/:index/_doc/:id", [this](auto *res, auto *req) {
      this->onput<SSL>(res, req);
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
    const std::string request_id = req->getParameter(1).empty() ? std::string{} : std::string(req->getParameter(1));
    const auto params = http::parse_query(req->getQuery());

    auto body = std::make_shared<std::string>();

    // Reserving resources.
    body->reserve(4096);

    res->onData([this, res, state, index, request_id, params, body](std::string_view chunk, bool last) mutable {
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

      // Forwarding a request to search engine.
      this->onsubmit(res, state, [service = this->service, index, request_id, body = std::move(*body), params, timeout = this->config.request_timeout.count()]() mutable {
        return service->Insert(http::make_index_request(index, request_id, std::move(body), params))->Wait(timeout);
      });
    });
  }

  template<bool SSL, typename Response, typename Request>
  auto HttpServer::onput(Response *res, Request *req) -> void {
    auto state = std::make_shared<async_response_state>();

    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    const std::string index(req->getParameter(0));
    const std::string request_id = req->getParameter(1).empty() ? std::string{} : std::string(req->getParameter(1));
    const auto params = http::parse_query(req->getQuery());

    auto body = std::make_shared<std::string>();

    // Reserving resources.
    body->reserve(4096);

    res->onData([this, res, state, index, request_id, params, body](std::string_view chunk, bool last) mutable {
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

      // Forwarding a request to search engine.
      this->onsubmit(res, state, [service = this->service, index, request_id, body = std::move(*body), params, timeout = this->config.request_timeout.count()]() mutable {
        return service->Update(http::make_index_request(index, request_id, std::move(body), params))->Wait(timeout);
      });
    });
  }

  template<bool SSL, typename Response, typename Request>
  auto HttpServer::onget(Response *res, Request *req) -> void {
    auto ctx = std::make_shared<http::response_context<SSL>>(res);
    const auto params = http::parse_query(req->getQuery());
    const auto routing = http::get_query_param(params, "routing");
    const http::document_get_item doc{
      .index = std::string(req->getParameter(0)),
      .id = std::string(req->getParameter(1)),
      .routing = routing.has_value() ? routing.value() : "",
      .options = http::parse_get_options(params)
    };

    res->onAborted([ctx]() {
      ctx->aborted.store(true, std::memory_order_release);
    });

    /*<!!!>
     * uWebSockets response живёт только в родном event-loop потоке.
     * Тяжёлая работа уходит в ThreadPool, а ответ возвращается через loop->defer().
     */
    this->pool.enqueue([service = this->service, ctx, doc, params, loop = uWS::Loop::get()]() mutable {
      try {
        palmira::SearchArgs search;

        if (service == nullptr) {
          throw (std::invalid_argument("Search engine not created"));
        }
        // Forwarding a search request into search engine.
        service->Search(search, [ctx, loop](const mtc::zmap &resp) {
          http::service_response reply;

          // Converting a search response into reply.

          // Replying a response to client.
          loop->defer([ctx, reply = std::move(reply)]() mutable {
            if (ctx->aborted.load(std::memory_order_acquire)) {
              return;
            }

            // Replying to a response.
            http::send_json_response(ctx.get(), reply);
          });
        });
      } catch (const std::exception &exc) {
        // Replying to error response.
        http::send_error_response(ctx.get(), 500, "internal_server_error", exc.what());
      } catch (...) {
        // Replying to error response.
        http::send_error_response(ctx.get(), 500, "internal_server_error", "unknown");
      }
    });
  }

  template<typename Response, typename Fn>
  auto HttpServer::onsubmit(Response *res, const std::shared_ptr<async_response_state> &state, Fn &&fn) -> void {
    const bool posted = this->pool.enqueue([loop = this->loop, res, state, fn = std::forward<Fn>(fn)]() mutable {
      auto reply = http::service_response();

      try {
        // Making a response from zmap
        reply = http::make_json_response(fn());
      } catch (const std::exception &exc) {
        reply.status = 500;
        reply.body = http::make_error_json("internal_error", exc.what());
      } catch (...) {
        reply.status = 500;
        reply.body = http::make_error_json("internal_error", "unknown error");
      }

      loop->defer([res, state, reply = std::move(reply)]() mutable {
        if (state->aborted.load(std::memory_order_acquire)) {
          return;
        }

        res->writeStatus(http::status_to_string(reply.status))->writeHeader("Content-Type", reply.content_type)->end(reply.body);
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

