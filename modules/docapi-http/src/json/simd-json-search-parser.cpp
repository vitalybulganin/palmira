#include "simd-json-search-parser.h"

//-------------------------------------------------------------------------//
namespace docapi::json {
//-------------------------------------------------------------------------//
  namespace {
//-------------------------------------------------------------------------//
    auto copy_string(std::string_view value) -> std::string {
      return {value.data(), value.size()};
    }

    auto read_string_view(simdjson::ondemand::value value, std::string_view context) -> std::string_view {
      std::string_view result;
      throw_if_error(value.get_string().get(result), context);
      return result;
    }

    auto read_scalar_as_string(simdjson::ondemand::value value, std::string_view context) -> std::string {
      std::string_view string_value;

      if (not value.get_string().get(string_value)) {
        return copy_string(string_value);
      }

      simdjson::ondemand::number number;

      if (not value.get_number().get(number)) {
        switch (number.get_number_type()) {
          case simdjson::ondemand::number_type::signed_integer:
            return std::to_string(number.get_int64());
          case simdjson::ondemand::number_type::unsigned_integer:
            return std::to_string(number.get_uint64());
          case simdjson::ondemand::number_type::floating_point_number:
            return std::to_string(number.get_double());
        }
      }

      bool bool_value = false;
      if (not value.get_bool().get(bool_value)) {
        return bool_value ? "true" : "false";
      }

      throw (parse_error(std::string(context) + ": expected scalar value"));
    }

    auto read_key(simdjson::ondemand::field& field) -> std::string_view {
      std::string_view key;
      throw_if_error(field.unescaped_key(false).get(key), "failed to read object key");

      return key;
    }

    auto read_double(simdjson::ondemand::value value, std::string_view context) -> double {
      double result = 0.0;
      throw_if_error(value.get_double().get(result), context);

      return result;
    }

    std::string_view read_string(simdjson::ondemand::value value) {
      std::string_view result = {};
      throw_if_error(value.get_string().get(result), "failed to read string");

      return result;
    }

    auto read_double_value(simdjson::ondemand::value value, std::string_view context) -> double {
      double result = 0.0;
      throw_if_error(value.get_double().get(result), context);
      return result;
    }

    auto read_string_value(simdjson::ondemand::value value, std::string_view context) -> std::string {
      std::string_view result;
      throw_if_error(value.get_string().get(result), context);

      return copy_string(result);
    }

    std::uint64_t read_non_negative_integer(simdjson::ondemand::value value, std::string_view field_name) {
      simdjson::ondemand::number number;
      throw_if_error(value.get_number().get(number), std::string("failed to read number field: ") + std::string(field_name));

      switch (number.get_number_type()) {
        case simdjson::ondemand::number_type::signed_integer: {
          const auto v = number.get_int64();

          if (v < 0) {
            throw (parse_error(std::string(field_name) + " must be non-negative"));
          }
          return static_cast<std::uint64_t>(v);
        }
        case simdjson::ondemand::number_type::unsigned_integer: {
          return number.get_uint64();
        }
        case simdjson::ondemand::number_type::floating_point_number: {
          throw (parse_error(std::string(field_name) + " must be integer"));
        }
      }

      throw (parse_error("unknown number type"));
    }

    bool read_bool(simdjson::ondemand::value value, std::string_view field_name) {
      bool result = false;
      throw_if_error(value.get_bool().get(result), std::string("failed to read bool field: ") + std::string(field_name));

      return result;
    }

    auto read_bool_value(simdjson::ondemand::value value, std::string_view context) -> bool {
      bool result = false;
      docapi::json::throw_if_error(value.get_bool().get(result), context);
      return result;
    }

    void parse_string_array(simdjson::ondemand::array array, std::vector<std::string> &output) {
      for (auto item_result : array) {
        simdjson::ondemand::value item;
        throw_if_error(std::move(item_result).get(item), "failed to read array item");

        output.emplace_back(copy_string(read_string(item)));
      }
    }

    /*
     * Поддерживаем две Elastic-compatible формы:
     *
     * 1. Простая:
     *    { "match": { "name": "brave" } }
     *
     * 2. Полная:
     *    {
     *      "match": {
     *        "name": {
     *          "query": "brave",
     *          "operator": "and",
     *          "analyzer": "standard",
     *          "fuzziness": "AUTO",
     *          "minimum_should_match": "75%",
     *          "zero_terms_query": "none",
     *          "lenient": false,
     *          "auto_generate_synonyms_phrase_query": true,
     *          "boost": 1.5
     *        }
     *      }
     *    }
     */
    auto parse_match_query(simdjson::ondemand::object match_object) -> match_query {
      json::match_query result;

      for (auto field_result : match_object) {
        simdjson::ondemand::field field;
        docapi::json::throw_if_error(std::move(field_result).get(field), "failed to read match field");

        result.field = copy_string(read_key(field));
        simdjson::ondemand::value value = field.value();

        auto object_result = value.get_object();
        if (not object_result.error()) {
          simdjson::ondemand::object params = object_result.value();

          for (auto param_result : params) {
            simdjson::ondemand::field param;
            throw_if_error(std::move(param_result).get(param), "failed to read match parameter");

            const std::string_view key = read_key(param);
            simdjson::ondemand::value param_value = param.value();

            if (key == "query") {
              result.query = read_scalar_as_string(param_value, "match.query");
            } else if (key == "operator") {
              result.operator_value = read_string_value(param_value, "match.operator must be string");
            } else if (key == "analyzer") {
              result.analyzer = read_string_value(param_value, "match.analyzer must be string");
            } else if (key == "fuzziness") {
              result.fuzziness = read_scalar_as_string(param_value, "match.fuzziness");
            } else if (key == "minimum_should_match") {
              result.minimum_should_match = read_scalar_as_string(param_value, "match.minimum_should_match");
            } else if (key == "zero_terms_query") {
              result.zero_terms_query = read_string_value(param_value, "match.zero_terms_query must be string");
            } else if (key == "lenient") {
              result.lenient = read_bool_value(param_value, "match.lenient must be boolean");
            } else if (key == "auto_generate_synonyms_phrase_query") {
              result.auto_generate_synonyms_phrase_query = read_bool_value(param_value, "match.auto_generate_synonyms_phrase_query must be boolean");
            } else if (key == "boost") {
              result.boost = read_double_value(param_value, "match.boost must be double");
            } else {
              /*
               * Elastic обычно допускает расширение параметров.
               * На первом этапе неизвестные параметры игнорируем.
               * Позже можно включить strict-режим.
               */
            }
          }
        } else {
          result.query = read_scalar_as_string(value, "match field value");
        }

        if (result.field.empty()) {
          throw (parse_error("match query field is empty"));
        }

        if (result.query.empty()) {
          throw (parse_error("match query requires query value"));
        }

        if (not result.operator_value.empty() && result.operator_value != "or" && result.operator_value != "and") {
          throw (parse_error("match.operator must be \"or\" or \"and\""));
        }

        if (not result.zero_terms_query.empty() && result.zero_terms_query != "none" && result.zero_terms_query != "all") {
          throw (parse_error("match.zero_terms_query must be \"none\" or \"all\""));
        }
        return result;
      }

      throw (parse_error("match query must contain field"));
    }

    auto parse_term_query(simdjson::ondemand::object term_object) -> docapi::json::term_query {
      term_query result;

      for (auto field_result : term_object) {
        simdjson::ondemand::field field;
        throw_if_error(std::move(field_result).get(field), "failed to read term field");

        result.field = copy_string(read_key(field));
        simdjson::ondemand::value value = field.value();

        auto object_result = value.get_object();
        if (not object_result.error()) {
          simdjson::ondemand::object params = object_result.value();

          for (auto param_result : params) {
            simdjson::ondemand::field param;
            throw_if_error(std::move(param_result).get(param), "failed to read term parameter");

            const auto key = read_key(param);
            simdjson::ondemand::value param_value = param.value();

            if (key == "value") {
              result.value = read_scalar_as_string(param_value, "term.value");
            } else if (key == "boost") {
              result.boost = read_double_value(param_value, "term.boost must be double");
            }
          }
        } else {
          result.value = read_scalar_as_string(value, "term field value");
        }

        if (result.field.empty()) {
          throw (parse_error("term query field is empty"));
        }

        if (result.value.empty()) {
          throw (parse_error("term query requires value"));
        }

        return result;
      }

      throw (parse_error("term query must contain field"));
    }

    auto parse_terms_query(simdjson::ondemand::object terms_object) -> docapi::json::terms_query {
      terms_query result;
      for (auto field_result : terms_object) {
        simdjson::ondemand::field field;
        throw_if_error(std::move(field_result).get(field), "failed to read terms field");

        result.field = copy_string(read_key(field));

        simdjson::ondemand::array values;
        throw_if_error(field.value().get_array().get(values), "terms query value must be array");

        for (auto value_result : values) {
          simdjson::ondemand::value value;
          throw_if_error(std::move(value_result).get(value), "failed to read terms value");

          result.values.emplace_back(read_scalar_as_string(value, "terms value"));
        }

        if (result.field.empty()) {
          throw (parse_error("terms query field is empty"));
        }

        if (result.values.empty()) {
          throw (parse_error("terms query requires non-empty values array"));
        }

        return result;
      }

      throw (parse_error("terms query must contain field"));
    }

    void parse_source_object(simdjson::ondemand::object object, source_filter &source) {
      for (auto field_result : object) {
        simdjson::ondemand::field field;
        throw_if_error(std::move(field_result).get(field), "failed to read _source field");

        const std::string_view key = read_key(field);
        simdjson::ondemand::value value = field.value();

        if (key == "includes" || key == "include") {
          simdjson::ondemand::array array;
          throw_if_error(value.get_array().get(array), "_source.includes must be array");

          parse_string_array(array, source.includes);
        } else if (key == "excludes" || key == "exclude") {
          simdjson::ondemand::array array;
          throw_if_error(value.get_array().get(array), "_source.excludes must be array");

          parse_string_array(array, source.excludes);
        }
      }
    }

    void parse_source_value(simdjson::ondemand::value value, source_filter &source) {
      /*
       * Важно:
       * Здесь НЕ вызываем value.type().
       * Работаем по допустимым формам _source.
       */

      auto bool_result = value.get_bool();

      if (not bool_result.error()) {
        source.enabled = bool_result.value();
        return;
      }

      auto array_result = value.get_array();
      if (not array_result.error()) {
        source.enabled = true;
        simdjson::ondemand::array array = array_result.value();
        parse_string_array(array, source.includes);
        return;
      }

      auto object_result = value.get_object();
      if (not object_result.error()) {
        source.enabled = true;
        simdjson::ondemand::object object = object_result.value();
        parse_source_object(object, source);
        return;
      }

      throw parse_error("_source must be boolean, array or object");
    }

    auto parse_string_array(simdjson::ondemand::value value, std::vector<std::string> &output, std::string_view context) -> void {
      simdjson::ondemand::array array;
      throw_if_error(value.get_array().get(array), context);

      for (auto item_result : array) {
        simdjson::ondemand::value item;
        docapi::json::throw_if_error(std::move(item_result).get(item), context);

        output.emplace_back(read_scalar_as_string(item, context));
      }
    }

    auto parse_track_total_hits(simdjson::ondemand::value value, search_request& request) -> void {
      bool bool_value = false;

      if (not value.get_bool().get(bool_value)) {
        request.track_total_hits_enabled = bool_value;
        request.track_total_hits_limit.reset();
        return;
      }

      simdjson::ondemand::number number;
      throw_if_error(value.get_number().get(number), "track_total_hits must be boolean or non-negative integer");

      request.track_total_hits_enabled = true;

      if (number.get_number_type() == simdjson::ondemand::number_type::signed_integer) {
        const auto signed_value = number.get_int64();

        if (signed_value < 0) {
          throw (parse_error("track_total_hits must be non-negative"));
        }

        request.track_total_hits_limit = static_cast<std::uint64_t>(signed_value);
        return;
      }

      if (number.get_number_type() == simdjson::ondemand::number_type::unsigned_integer) {
        request.track_total_hits_limit = number.get_uint64();
        return;
      }

      throw (parse_error("track_total_hits must be boolean or integer"));
    }

    auto parse_sort(simdjson::ondemand::value value, search_request& request) -> void {
      simdjson::ondemand::array array;
      throw_if_error(value.get_array().get(array), "sort must be array");

      for (auto item_result : array) {
        simdjson::ondemand::value item;
        throw_if_error(std::move(item_result).get(item), "failed to read sort item");

        std::string_view sort_string;

        if (not item.get_string().get(sort_string)) {
          request.sort.push_back(sort_clause{
            .field = copy_string(sort_string),
            .order = "asc"
          });
          continue;
        }

        simdjson::ondemand::object object;
        throw_if_error(item.get_object().get(object), "sort item must be string or object");

        for (auto field_result : object) {
          simdjson::ondemand::field field;
          throw_if_error(std::move(field_result).get(field), "failed to read sort field");

          sort_clause clause;
          clause.field = copy_string(read_key(field));

          simdjson::ondemand::value sort_value = field.value();

          std::string_view order_string;
          if (not sort_value.get_string().get(order_string)) {
            clause.order = copy_string(order_string);
          } else {
            simdjson::ondemand::object params;
            throw_if_error(sort_value.get_object().get(params), "sort field value must be string or object");

            for (auto param_result : params) {
              simdjson::ondemand::field param;
              throw_if_error(std::move(param_result).get(param), "failed to read sort param");

              const auto key = read_key(param);
              if (key == "order") {
                clause.order = read_string_value(param.value(), "sort.order must be string");
              }
            }
          }

          if (clause.order != "asc" && clause.order != "desc") {
            throw (parse_error("sort.order must be asc or desc"));
          }

          request.sort.emplace_back(std::move(clause));
          break;
        }
      }
    }

    auto parse_docvalue_fields(simdjson::ondemand::value value, search_request& request) -> void {
      simdjson::ondemand::array array;
      throw_if_error(value.get_array().get(array), "docvalue_fields must be array");

      for (auto item_result : array) {
        simdjson::ondemand::value item;
        throw_if_error(std::move(item_result).get(item), "failed to read docvalue_fields item");

        std::string_view string_item;
        if (not item.get_string().get(string_item)) {
          request.docvalue_fields.push_back(field_request{
            .field = copy_string(string_item),
            .format = ""
          });
          continue;
        }

        simdjson::ondemand::object object;
        throw_if_error(item.get_object().get(object), "docvalue_fields item must be string or object");

        field_request field;
        for (auto field_result : object) {
          simdjson::ondemand::field json_field;
          throw_if_error(std::move(field_result).get(json_field), "failed to read docvalue field object");

          const auto key = read_key(json_field);
          if (key == "field") {
            field.field = read_string_value(json_field.value(), "docvalue_fields.field must be string");
          } else if (key == "format") {
            field.format = read_string_value(json_field.value(), "docvalue_fields.format must be string");
          }
        }

        if (field.field.empty()) {
          throw (parse_error("docvalue_fields object requires field"));
        }

        request.docvalue_fields.emplace_back(std::move(field));
      }
    }

    query_kinds detect_query_kind(simdjson::ondemand::object query_object) {
      /*
       * Query DSL на первом этапе не типизируем полностью.
       * Только определяем верхнеуровневый тип.
       */
      for (auto field_result: query_object) {
        simdjson::ondemand::field field;
        throw_if_error(std::move(field_result).get(field), "failed to read query field");

        const std::string_view key = read_key(field);
        if (key == "match_all") { return query_kinds::match_all; }
        if (key == "match_none") { return query_kinds::match_none; }
        if (key == "term") { return query_kinds::term; }
        if (key == "terms") { return query_kinds::terms; }
        if (key == "range") { return query_kinds::range; }
        if (key == "bool") { return query_kinds::bool_query; }
        if (key == "as_tree") { return query_kinds::as_tree; }

        return query_kinds::raw;
      }
      return query_kinds::none;
    }

    mget_doc parse_mget_doc_object(simdjson::ondemand::object object, std::string_view default_index) {
      mget_doc doc;

      if (not default_index.empty()) {
        doc.index = copy_string(default_index);
      }

      for (auto field_result : object) {
        simdjson::ondemand::field field;
        throw_if_error(std::move(field_result).get(field), "failed to read mget doc field");

        const std::string_view key = read_key(field);
        simdjson::ondemand::value value = field.value();

        if (key == "_index") {
          doc.index = copy_string(read_string(value));
        } else if (key == "_id") {
          doc.id = copy_string(read_string(value));
        } else if (key == "routing" || key == "_routing") {
          doc.routing = copy_string(read_string(value));
        } else if (key == "_source") {
          parse_source_value(value, doc.source);
        }
      }

      if (doc.index.empty()) {
        throw (parse_error("mget document has no _index"));
      }

      if (doc.id.empty()) {
        throw (parse_error("mget document has no _id"));
      }

      return doc;
    }
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  void validate_json_object(std::string_view body) {
    if (body.empty()) {
      return;
    }

    simdjson::padded_string padded(body);
    simdjson::ondemand::document document;
    auto &context = get_thread_parser_context();
    throw_if_error(context.parser.iterate(padded).get(document), "failed to parse JSON");

    simdjson::ondemand::object root;
    throw_if_error(document.get_object().get(root), "JSON body must be object");
  }

  search_request parse_search_request(std::string_view body, std::vector<std::string> indices) {
    auto &context = get_thread_parser_context();
    simdjson::padded_string padded(body);
    simdjson::ondemand::document document;
    throw_if_error(context.parser.iterate(padded).get(document), "failed to parse search body");

    simdjson::ondemand::object root;
    throw_if_error(document.get_object().get(root), "search body must be object");

    search_request request;
    request.indices = std::move(indices);
    request.body = copy_string(body);

    for (auto field_result : root) {
      simdjson::ondemand::field field;
      throw_if_error(std::move(field_result).get(field), "failed to read search root field");

      const std::string_view key = read_key(field);
      simdjson::ondemand::value value = field.value();

      if (key == "from") {
        request.from = read_non_negative_integer(value, "from");
      } else if (key == "size") {
        request.size = read_non_negative_integer(value, "size");
      } else if (key == "_source") {
        parse_source_value(value, request.source);
      } else if (key == "timeout") {
        request.timeout = copy_string(read_string(value));
      } else if (key == "explain") {
        request.explain = read_bool(value, "explain");
      } else if (key == "profile") {
        request.profile = read_bool(value, "profile");
      } else if (key == "track_scores") {
        request.track_scores = read_bool(value, "track_scores");
      } else if (key == "sort") {
        parse_sort(value, request);
      } else if (key == "stored_fields") {
        parse_string_array(value, request.stored_fields, "stored_fields must be array");
      } else if (key == "fields") {
        parse_string_array(value, request.fields, "fields must be array");
      } else if (key == "docvalue_fields") {
        parse_docvalue_fields(value, request);
      } else if (key == "search_after") {
        parse_string_array(value, request.search_after, "search_after must be array");
      } else if (key == "track_total_hits") {
        parse_track_total_hits(value, request);
      } else if (key == "track_scores") {
        request.track_scores = read_bool_value(value, "track_scores must be boolean");
      } else if (key == "version") {
        request.version = read_bool_value(value, "version must be boolean");
      } else if (key == "seq_no_primary_term") {
        request.seq_no_primary_term = read_bool_value(value, "seq_no_primary_term must be boolean");
      } else if (key == "terminate_after") {
        request.terminate_after = read_non_negative_integer(value, "terminate_after");
      } else if (key == "min_score") {
        request.min_score = read_double_value(value, "min_score must be double");
        request.has_min_score = true;
      } else if (key == "query") {
        simdjson::ondemand::object query_object;
        throw_if_error(value.get_object().get(query_object), "query must be object");

        for (auto query_field_result : query_object) {
          simdjson::ondemand::field query_field;
          throw_if_error(std::move(query_field_result).get(query_field), "failed to read query field");

          const auto query_key = read_key(query_field);
          simdjson::ondemand::value query_value = query_field.value();

          if (query_key == "match") {
            simdjson::ondemand::object match_object;
            throw_if_error(query_value.get_object().get(match_object), "match query must be object");

            request.query_type = query_kinds::match;
            request.match = parse_match_query(match_object);
          } else if (query_key == "term") {
            simdjson::ondemand::object term_object;
            throw_if_error(query_value.get_object().get(term_object), "term query must be object");

            request.query_type = query_kinds::term;
            request.term = parse_term_query(term_object);
          } else if (query_key == "terms") {
            simdjson::ondemand::object terms_object;
            throw_if_error(query_value.get_object().get(terms_object), "terms query must be object");

            request.query_type = query_kinds::terms;
            request.terms = parse_terms_query(terms_object);
          } else if (query_key == "match_all") {
            request.query_type = query_kinds::match_all;
          } else if (query_key == "match_none") {
            request.query_type = query_kinds::match_none;
          } else if (query_key == "as_tree") {
            request.query_type = query_kinds::as_tree;
          } else {
            request.query_type = query_kinds::raw;
          }
          break;
        }
      } else {
        /*
         * Elastic-compatible режим:
         * неизвестные поля не ломаем на первом этапе.
         * Позже здесь можно включить strict validation.
         */
      }
    }

    return request;
  }

  mget_request parse_mget_request(std::string_view body, std::string_view default_index) {
    simdjson::padded_string padded(body);
    auto &context = get_thread_parser_context();
    simdjson::ondemand::document document;
    throw_if_error(context.parser.iterate(padded).get(document), "failed to parse mget body");

    simdjson::ondemand::object root;
    throw_if_error(document.get_object().get(root), "mget body must be object");

    mget_request request;
    for (auto field_result : root) {
      simdjson::ondemand::field field;
      throw_if_error(std::move(field_result).get(field), "failed to read mget root field");

      const std::string_view key = read_key(field);
      simdjson::ondemand::value value = field.value();

      if (key == "docs") {
        simdjson::ondemand::array docs;
        throw_if_error(value.get_array().get(docs), "mget docs must be array");

        for (auto doc_result : docs) {
          simdjson::ondemand::value doc_value;
          throw_if_error(std::move(doc_result).get(doc_value), "failed to read mget docs item");

          simdjson::ondemand::object doc_object;
          throw_if_error(doc_value.get_object().get(doc_object), "mget docs item must be object");

          request.docs.emplace_back(parse_mget_doc_object(doc_object, default_index));
        }
      } else if (key == "ids") {
        if (default_index.empty()) {
          throw (parse_error("mget ids form requires index in URL"));
        }

        simdjson::ondemand::array ids;
        throw_if_error(value.get_array().get(ids), "mget ids must be array");

        for (auto id_result : ids) {
          simdjson::ondemand::value id_value;

          throw_if_error(std::move(id_result).get(id_value), "failed to read mget id");

          mget_doc doc;
          doc.index = copy_string(default_index);
          doc.id = copy_string(read_string(id_value));

          request.docs.emplace_back(std::move(doc));
        }
      }
    }

    if (request.docs.empty()) {
      throw (parse_error("mget request contains no documents"));
    }

    return request;
  }
//-------------------------------------------------------------------------//
} // namespace docapi::json
