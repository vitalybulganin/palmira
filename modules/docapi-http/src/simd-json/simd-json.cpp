#include "simd-json.h"
//-------------------------------------------------------------------------//
#include "../common/except.h"
//-------------------------------------------------------------------------//
namespace docapi::json {
//-------------------------------------------------------------------------//
  auto to_string(simdjson::ondemand::raw_json_string value) -> std::string {
    return {value.raw()};
  }

  auto get_bool_value(simdjson::ondemand::value value) -> bool {
    bool result = false;

    auto error = value.get(result);
    if (error) {
      throw (common::json_parse_error("expected boolean JSON value"));
    }

    return result;
  }

  auto get_string_array(simdjson::ondemand::value value) -> std::vector<std::string> {
    std::vector<std::string> result;
    simdjson::ondemand::array array;

    auto error = value.get_array().get(array);
    if (error) {
      throw (common::json_parse_error("expected JSON array"));
    }

    for (auto element : array) {
      std::string_view string_value;

      error = element.get_string().get(string_value);
      if (error) {
        throw (common::json_parse_error("expected string inside JSON array"));
      }

      result.emplace_back(string_value);
    }

    return result;
  }
//-------------------------------------------------------------------------//
  auto get_source_object(simdjson::ondemand::object object, http::document_get_options &options) -> void {
    for (auto field : object) {
      std::string_view key;
      auto error = field.unescaped_key().get(key);
      if (error) {
        throw (common::json_parse_error("failed to read _source object key"));
      }

      simdjson::ondemand::value value = field.value();

      if (key == "includes" || key == "include") {
        options.source_includes = get_string_array(value);
      } else if (key == "excludes" || key == "exclude") {
        options.source_excludes = get_string_array(value);
      }
    }
  }

  auto get_source_value(simdjson::ondemand::value value, http::document_get_options &options) -> void {
    bool bool_value = false;

    if (not value.get_bool().get(bool_value)) {
      options.source_enabled = bool_value;
      return;
    }

    simdjson::ondemand::array array;
    if (not value.get_array().get(array)) {
      options.source_enabled = true;

      for (auto element : array) {
        std::string_view string_value;
        auto error = element.get_string().get(string_value);
        if (error) {
          throw (common::json_parse_error("expected string in _source array"));
        }

        options.source_includes.emplace_back(string_value);
      }

      return;
    }

    simdjson::ondemand::object object;
    if (not value.get_object().get(object)) {
      options.source_enabled = true;
      get_source_object(object, options);
      return;
    }

    throw (common::json_parse_error("unsupported _source value type"));
  }

  auto get_doc_item(simdjson::ondemand::object object, std::string_view default_index) -> http::document_get_item {
    http::document_get_item item;

    if (not default_index.empty()) {
      item.index = std::string(default_index);
    }

    for (auto field : object) {
      std::string_view key;

      auto error = field.unescaped_key().get(key);
      if (error) {
        throw (common::json_parse_error("failed to read mget doc key"));
      }

      simdjson::ondemand::value value = field.value();
      if (key == "_index") {
        std::string_view string_value;

        error = value.get_string().get(string_value);
        if (error) {
          throw (common::json_parse_error("_index must be string"));
        }

        item.index = string_value;
      } else if (key == "_id") {
        std::string_view string_value;

        error = value.get_string().get(string_value);
        if (error) {
          throw (common::json_parse_error("_id must be string"));
        }

        item.id = string_value;
      } else if (key == "routing" || key == "_routing") {
        std::string_view string_value;

        error = value.get_string().get(string_value);
        if (error) {
          throw (common::json_parse_error("routing must be string"));
        }

        item.routing = string_value;
      } else if (key == "_source") {
        get_source_value(value, item.options);
      }
    }

    if (item.index.empty()) {
      throw (common::json_parse_error("mget doc item has no _index"));
    }

    if (item.id.empty()) {
      throw (common::json_parse_error("mget doc item has no _id"));
    }

    return item;
  }
//-------------------------------------------------------------------------//
  auto validate_json_document(simdjson::ondemand::parser &parser, std::string_view body) -> void {
    simdjson::padded_string padded_body(body);
    simdjson::ondemand::document document;

    auto error = parser.iterate(padded_body).get(document);
    if (error) {
      throw (common::json_parse_error("invalid JSON document body"));
    }

    /*
     * Нам достаточно пройти root value, чтобы убедиться,
     * что JSON синтаксически валиден.
     */
    simdjson::ondemand::value root = document;
    simdjson::ondemand::json_type type;

    error = root.type().get(type);
    if (error) {
      throw common::json_parse_error("failed to detect JSON root type");
    }

    if (type != simdjson::ondemand::json_type::object) {
      throw (common::json_parse_error("index document body must be JSON object"));
    }
  }
//-------------------------------------------------------------------------//
} // namespace docapi::json
