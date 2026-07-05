/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          simd-json.h
* - Created:       07/04/2026
* - Author:        Vitaly Bulganin
* - Description:   Модуль предназначен для последовательного обхода JSON-документов:
*                   - обычных JSON body;
*                   - NDJSON body для будущего _bulk API;
*                   - поисковых запросов;
*                   - create index body;
*                   - document body.
* - Comments:
*
-----------------------------------------------------------------------------
*
* - History:
*
===========================================================================*/
#pragma once
//-------------------------------------------------------------------------//
#ifndef __SIMD_JSON_VISIT_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SIMD_JSON_VISIT_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
namespace docapi::json {
//-------------------------------------------------------------------------//
  enum class json_value_types {
    object_begin,
    object_end,
    array_begin,
    array_end,
    string,
    int64,
    uint64,
    double_value,
    boolean,
    null_value
  };

  struct json_visit_event {
    /*
     * Полный путь до текущего узла.
     *
     * Примеры:
     *   /query/as_tree/operation
     *   /docs[0]/_id
     *   /index/_id
     */
    std::string_view path;

    /*
     * Имя поля объекта.
     * Для элементов массива пустое.
     */
    std::string_view name;

    /*
     * Значение для json_value_type::string.
     * Для остальных типов пустое.
     *
     * ВАЖНО:
     * string_value валиден только во время callback.
     */
    std::string_view string_value;

    /*
     * Типизированные значения.
     * Использовать нужно только поле, соответствующее type.
     */
    std::int64_t int64_value = 0;
    std::uint64_t uint64_value = 0;
    double double_value = 0.0;
    bool bool_value = false;

    json_value_types type = json_value_types::null_value;

    /*
     * Для обычного JSON всегда 0.
     * Для NDJSON — номер непустой JSON-строки.
     */
    std::uint64_t document_index = 0;

    auto as_str() const noexcept -> std::string;
  };
//-------------------------------------------------------------------------//
  using json_visit_callback_t = std::function<void(const json_visit_event &event)>;
//-------------------------------------------------------------------------//
  class json_visit_error final : public std::runtime_error {
  public:
    explicit json_visit_error(const std::string &message)
      : std::runtime_error(message) {
    }
  };

  namespace detail {
    inline void ThrowIfError(simdjson::error_code error, std::string_view context) {
      if (error) {
        throw json_visit_error(std::string(context) + ": " + simdjson::error_message(error));
      }
    }

    inline std::string AppendObjectPath(std::string_view parent, std::string_view key) {
      std::string result;

      if (parent.empty()) {
        result.reserve(key.size() + 1);
        result += "/";
        result += key;
      } else {
        result.reserve(parent.size() + key.size() + 1);
        result += parent;
        result += "/";
        result += key;
      }

      return result;
    }

    inline std::string AppendArrayPath(std::string_view parent, std::size_t index) {
      std::string result(parent);

      result += "[";
      result += std::to_string(index);
      result += "]";

      return result;
    }

    template<typename Callback>
    inline void EmitBase(Callback &callback,
                         std::string_view path,
                         std::string_view name,
                         json_value_types type,
                         std::uint64_t document_index) {
      json_visit_event event;
      event.path = path;
      event.name = name;
      event.type = type;
      event.document_index = document_index;

      callback(event);
    }

    template<typename Callback>
    inline void EmitString(Callback &callback,
                           std::string_view path,
                           std::string_view name,
                           std::string_view value,
                           std::uint64_t document_index) {
      json_visit_event event;
      event.path = path;
      event.name = name;
      event.string_value = value;
      event.type = json_value_types::string;
      event.document_index = document_index;

      callback(event);
    }

    template<typename Callback>
    inline void EmitInt64(Callback &callback,
                          std::string_view path,
                          std::string_view name,
                          std::int64_t value,
                          std::uint64_t document_index) {
      json_visit_event event;
      event.path = path;
      event.name = name;
      event.int64_value = value;
      event.type = json_value_types::int64;
      event.document_index = document_index;

      callback(event);
    }

    template<typename Callback>
    inline void EmitUInt64(Callback &callback,
                           std::string_view path,
                           std::string_view name,
                           std::uint64_t value,
                           std::uint64_t document_index) {
      json_visit_event event;
      event.path = path;
      event.name = name;
      event.uint64_value = value;
      event.type = json_value_types::uint64;
      event.document_index = document_index;

      callback(event);
    }

    template<typename Callback>
    inline void EmitDouble(Callback &callback,
                           std::string_view path,
                           std::string_view name,
                           double value,
                           std::uint64_t document_index) {
      json_visit_event event;
      event.path = path;
      event.name = name;
      event.double_value = value;
      event.type = json_value_types::double_value;
      event.document_index = document_index;

      callback(event);
    }

    template<typename Callback>
    inline void EmitBool(Callback &callback,
                         std::string_view path,
                         std::string_view name,
                         bool value,
                         std::uint64_t document_index) {
      json_visit_event event;
      event.path = path;
      event.name = name;
      event.bool_value = value;
      event.type = json_value_types::boolean;
      event.document_index = document_index;

      callback(event);
    }

    template<typename Callback>
    void WalkValue(simdjson::ondemand::value value,
                   std::string_view path,
                   std::string_view name,
                   Callback &callback,
                   std::uint64_t document_index);

    template<typename Callback>
    void WalkObject(simdjson::ondemand::object object,
                    std::string_view path,
                    std::string_view name,
                    Callback &callback,
                    std::uint64_t document_index) {
      EmitBase(callback, path, name, json_value_types::object_begin, document_index);

      for (auto field_result : object) {
        simdjson::ondemand::field field;

        ThrowIfError(std::move(field_result).get(field), "failed to read object field");

        std::string_view key;
        ThrowIfError(field.unescaped_key(false).get(key), "failed to read key");

        auto child_value = field.value();
        auto child_path = AppendObjectPath(path, key);
        WalkValue(child_value, child_path, key, callback, document_index);
      }

      EmitBase(callback, path, name, json_value_types::object_end, document_index);
    }

    template<typename Callback>
    void WalkArray(simdjson::ondemand::array array,
                   std::string_view path,
                   std::string_view name,
                   Callback &callback,
                   std::uint64_t document_index) {
      EmitBase(callback, path, name, json_value_types::array_begin, document_index);

      std::size_t index = 0;

      for (auto element_result : array) {
        simdjson::ondemand::value element;
        ThrowIfError(element_result.get(element), "failed to read array element");

        std::string child_path = AppendArrayPath(path, index);
        WalkValue(element, child_path, {}, callback, document_index);

        ++index;
      }

      EmitBase(callback, path, name, json_value_types::array_end, document_index);
    }

    template<typename Callback>
    void WalkNumber(simdjson::ondemand::value value, std::string_view path, std::string_view name, Callback &callback, std::uint64_t document_index) {
      simdjson::ondemand::number number;
      ThrowIfError(value.get_number().get(number), "failed to read JSON number");

      switch (number.get_number_type()) {
        case simdjson::ondemand::number_type::signed_integer: {
          EmitInt64(callback, path, name, number.get_int64(), document_index);
          break;
        }
        case simdjson::ondemand::number_type::unsigned_integer: {
          EmitUInt64(callback, path, name, number.get_uint64(), document_index);
          break;
        }
        case simdjson::ondemand::number_type::floating_point_number: {
          EmitDouble(callback, path, name, number.get_double(), document_index);
          break;
        }
      }
    }

    template<typename Callback>
    void WalkValue(simdjson::ondemand::value value,
                   std::string_view path,
                   std::string_view name,
                   Callback &callback,
                   std::uint64_t document_index) {
      simdjson::ondemand::json_type type;
      ThrowIfError(value.type().get(type), "failed to detect JSON value type");

      switch (type) {
        case simdjson::ondemand::json_type::object: {
          simdjson::ondemand::object object;

          ThrowIfError(value.get_object().get(object), "failed to get JSON object");
          WalkObject(object, path, name, callback, document_index);
          break;
        }

        case simdjson::ondemand::json_type::array: {
          simdjson::ondemand::array array;

          ThrowIfError(value.get_array().get(array), "failed to get JSON array");
          WalkArray(array, path, name, callback, document_index);
          break;
        }
        case simdjson::ondemand::json_type::string: {
          std::string_view result;

          ThrowIfError(value.get_string().get(result), "failed to get JSON string");
          EmitString(callback, path, name, result, document_index);
          break;
        }
        case simdjson::ondemand::json_type::number: {
          WalkNumber(value, path, name, callback, document_index);
          break;
        }
        case simdjson::ondemand::json_type::boolean: {
          bool result = false;

          ThrowIfError(value.get_bool().get(result), "failed to get JSON boolean");
          EmitBool(callback, path, name, result, document_index);
          break;
        }
        case simdjson::ondemand::json_type::null: {
          EmitBase(callback, path, name, json_value_types::null_value, document_index);
          break;
        }
      }
    }

    template<typename Callback>
    void VisitSingleJsonDocumentFast(std::string_view json, Callback &callback, std::uint64_t document_index) {
      /*
       * simdjson требует padding после входного буфера.
       * padded_string делает безопасную копию.
       *
       * Для максимальной производительности HTTP body collector позже можно
       * сделать padded-буфер сразу при накоплении тела запроса.
       */
      simdjson::padded_string padded_json(json);

      /*
       * Один parser на worker-thread.
       * parser нельзя использовать одновременно из разных потоков.
       */
      thread_local simdjson::ondemand::parser parser;
      simdjson::ondemand::value root;
      simdjson::ondemand::document document;
      ThrowIfError(parser.iterate(padded_json).get(document), "failed to parse JSON document");

      ThrowIfError(document.get_value().get(root), "failed to read root JSON value");
      WalkValue(root, {}, {}, callback, document_index);
    }

    inline bool IsWhitespaceOnly(std::string_view line) {
      for (const char c: line) {
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
          return false;
        }
      }

      return true;
    }
  }
//-------------------------------------------------------------------------//
  /*
   * Hot path API.
   *
   * Эти функции не используют std::function.
   * Callback может быть lambda/functor и будет оптимизирован компилятором.
   */
  template<typename Callback>
  void VisitJsonFast(std::string_view json, Callback &&callback) {
    auto callback_holder = std::forward<Callback>(callback);

    detail::VisitSingleJsonDocumentFast(json, callback_holder, 0);
  }

  template<typename Callback>
  void VisitNdjsonFast(std::string_view ndjson, Callback &&callback) {
    auto callback_holder = std::forward<Callback>(callback);
    std::uint64_t document_index = 0;
    std::size_t offset = 0;

    while (offset < ndjson.size()) {
      const std::size_t line_end = ndjson.find('\n', offset);

      std::string_view line;

      if (line_end == std::string_view::npos) {
        line = ndjson.substr(offset);
        offset = ndjson.size();
      } else {
        line = ndjson.substr(offset, line_end - offset);
        offset = line_end + 1;
      }

      if (not line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
      }

      if (line.empty() || detail::IsWhitespaceOnly(line)) {
        continue;
      }

      detail::VisitSingleJsonDocumentFast(line, callback_holder, document_index);

      ++document_index;
    }
  }
//-------------------------------------------------------------------------//
  auto to_string_view(json_value_types type) -> std::string_view;
//-------------------------------------------------------------------------//
  /*
   * Convenience API.
   *
   * Удобно для тестов и не-hot-path логики.
   * Для _bulk использовать VisitNdjsonFast().
   */
  void VisitJson(std::string_view json, const json_visit_callback_t &callback);
  void VisitNdjson(std::string_view ndjson, const json_visit_callback_t &callback);
//-------------------------------------------------------------------------//
} // namespace docapi::json
//-------------------------------------------------------------------------//
#endif // __SIMD_JSON_VISIT_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
