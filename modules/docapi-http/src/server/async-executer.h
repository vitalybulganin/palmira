/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          async-executer.h
* - Created:       07/06/2026
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
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
//-------------------------------------------------------------------------//
#include "../parsers/insert-parser.h"
#include "../parsers/update-parser.h"
#include "../parsers/remove-parser.h"
#include "../parsers/search-parser.h"
//-------------------------------------------------------------------------//
#include "../http/docres-builder.h"
//-------------------------------------------------------------------------//
namespace docapi {
//-------------------------------------------------------------------------//
  enum class executer_types : std::uint8_t {insert = 1, update, remove, search};
//-------------------------------------------------------------------------//
  template<typename executer_context>
  class async_executer final {
    using ontask_t = std::function<void(const executer_context &ctx)>;
    using queue_task_t = std::pair<executer_types, ontask_t>;

    //!< Keeps a mutex.
    std::mutex mtx;

    //!< Keeps a condition.
    std::condition_variable cv;

    //!< Keeps a queue of tasks.
    std::deque<queue_task_t> queue;

    //!< Keeps a list of workers.
    std::vector<std::thread> workers;

    //!< Keeps a flag of stopping.
    bool stopping = false;

  public:
    /**
     * Constructor.
     * @param thread_count
     */
    explicit async_executer(std::size_t thread_count);

    /**
     * Destructor.
     */
    ~async_executer();

    /**
     * Adds a new task into queue.
     * @param type [in] - A type of task.
     * @param task [in] - A callback function.
     * @return
     */
    bool enqueue(executer_types type, ontask_t task);

    //!< Stops proceeding queue.
    void stop() noexcept;

  public:
    async_executer(const async_executer &) = delete;
    async_executer(async_executer &&) = delete;
    async_executer &operator=(const async_executer &) = delete;
    async_executer &&operator=(async_executer &&) = delete;

  private:
      void onrun() noexcept;
};
//-------------------------------------------------------------------------//
  template<typename executer_context>
  async_executer<executer_context>::async_executer(std::size_t thread_count) {
    if (thread_count == 0) {
      thread_count = 1;
    }
    this->workers.reserve(thread_count);

    for (std::size_t i = 0; i < thread_count; ++i) {
      this->workers.emplace_back([this]() {
        this->onrun();
      });
    }
  }

  template<typename executer_context>
  async_executer<executer_context>::~async_executer() {
    this->stop();
  }

  template<typename executer_context>
  bool async_executer<executer_context>::enqueue(executer_types type, ontask_t task) {
    if (!task) {
      return false;
    }
    {
      std::lock_guard sync(this->mtx);
      if (this->stopping) {
        return false;
      }

      this->queue.emplace_back(type, std::move(task));
    }

    return this->cv.notify_one(), true;
  }

  template<typename executer_context>
  void async_executer<executer_context>::stop() noexcept {
    {
      std::lock_guard sync(this->mtx);
      this->stopping = true;
    }

    this->cv.notify_all();

    for (auto &worker: this->workers) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }
//-------------------------------------------------------------------------//
  template<typename executer_context>
  void async_executer<executer_context>::onrun() noexcept {
    const auto insert_parser = std::make_unique<docapi::parsers::insert_parser>();
    const auto insert_responser = http::make_docres_builder().set(http::docres_types::insert).build();
    const auto update_parser = std::make_unique<docapi::parsers::update_parser>();
    const auto update_responser = http::make_docres_builder().set(http::docres_types::update).build();
    const auto remove_parser = std::make_unique<docapi::parsers::remove_parser>();
    const auto remove_responser = http::make_docres_builder().set(http::docres_types::remove).build();
    const auto search_parser = std::make_unique<docapi::parsers::search_parser>();
    const auto search_responser = http::make_docres_builder().set(http::docres_types::search).build();

    for (;;) {
      queue_task_t task;
      {
        std::unique_lock sync(this->mtx);
        this->cv.wait(sync, [this]() {
          return this->stopping || !this->queue.empty();
        });

        if (this->stopping && this->queue.empty()) {
          return;
        }

        task = std::move(this->queue.front());
        this->queue.pop_front();
      }

      if (not task.second) {
        continue;
      }

      try {
        switch (task.first) {
        case executer_types::insert:
          task.second(executer_context(insert_parser.get(), insert_responser.get()));
          break;
        case executer_types::update:
          task.second(executer_context(update_parser.get(), update_responser.get()));
          break;
        case executer_types::remove:
          task.second(executer_context(remove_parser.get(), remove_responser.get()));
          break;
        case executer_types::search:
          task.second(executer_context(search_parser.get(), search_responser.get()));
          break;
        }
      } catch (const std::exception &exc) {//<TODO> Adding logging.
        std::fprintf(stderr, "Proceeding request failed: %s\n", exc.what());
      } catch (...) {//<TODO> Adding logging.
        std::fprintf(stderr, "Proceeding request failed: unknown \n");
      }
    }
  }
//-------------------------------------------------------------------------//
} // namespace docapi

