/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          simd-json-parser.h
* - Created:       06/24/2026
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
#ifndef __SIMD_JSON_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SIMD_JSON_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
//-------------------------------------------------------------------------//
namespace docapi::json {
//-------------------------------------------------------------------------//
  struct parse_error final : public std::runtime_error {
    /**
     * Constructor.
     * @param message [in] - Error message.
     */
    explicit parse_error(const std::string &message) : std::runtime_error(message) {
    }
  };
//-------------------------------------------------------------------------//
  inline void throw_if_error(simdjson::error_code error, std::string_view context)  {
    if (error) {
      throw (parse_error(std::string(context) + ": " + simdjson::error_message(error)));
    }
  }
//-------------------------------------------------------------------------//
  struct parser_context {
    simdjson::ondemand::parser parser;

    void reset() noexcept {
      /*
       * simdjson::ondemand::parser переиспользуется.
       * Важно: не использовать один parser параллельно из разных потоков.
       */
    }
  };

  struct term_query {
    //!< Keeps a name of field.
    std::string field;

    //!< Keeps a value of field.
    std::string value;

    double boost = 1.0;
  };

  struct terms_query {
    //!< Keeps a name of field.
    std::string field;

    //!< Keeps a list of field values.
    std::vector<std::string> values;
  };

  struct sort_clause {
    std::string field;
    std::string order = "asc";
  };

  struct field_request {
    std::string field;
    std::string format;
  };

  struct source_filter {
    //!< Keeps a flag of using filter.
    bool enabled = true;

    //!< Keeps a list of includes.
    std::vector<std::string> includes;

    //!< Keeps a list of excludes.
    std::vector<std::string> excludes;
  };

  struct mget_doc {
    //!< Keeps a name of index.
    std::string index;
    //!< Keeps a document id.
    std::string id;
    //!< Keeps a routing.
    std::string routing;
    //!< Keeps a source filter.
    source_filter source;
  };

  struct mget_request {
    //!< Keeps a list of documents.
    std::vector<mget_doc> docs;
  };

  //!< Supported query kinds.
  enum class query_kinds {
    none,
    raw,
    match_all,
    match_none,
    match,
    term,
    terms,
    range,
    bool_query,
    as_tree
  };

  struct match_query {
    //!< Keeps a name of field, which executes a match.
    std::string field;

    //!< Keeps a query.
    std::string query;

    //!< operator: "or" / "and".
    std::string operator_value;

    //!< fuzziness: например "AUTO".
    std::string fuzziness;

    //!< analyzer.
    std::string analyzer;

    //!< Keeps a minimum_should_match.
    std::string minimum_should_match;

    //!< Keeps zero terms query.
    std::string zero_terms_query; // "none" / "all"

    bool lenient = false;
    bool auto_generate_synonyms_phrase_query = true;

    //!< Keeps a boost.
    double boost = 1.0;
  };

  //!< Keeps a search request.
  struct search_request {
    //!< Keeps a list of indices.
    std::vector<std::string> indices;

    std::uint64_t from = 0; //<!!!> Elasticsearch uses index from 0.
    std::uint64_t size = 10;

    source_filter source;

    std::vector<sort_clause> sort;
    std::vector<std::string> stored_fields;
    std::vector<field_request> docvalue_fields;
    std::vector<std::string> fields;
    std::vector<std::string> search_after;

    std::optional<std::string> timeout;

    bool explain = false;
    bool profile = false;
    bool track_scores = false;
    bool version = false;
    bool seq_no_primary_term = false;

    bool track_total_hits_enabled = true;
    std::optional<std::uint64_t> track_total_hits_limit;

    std::uint64_t terminate_after = 0;
    double min_score = 0.0;
    bool has_min_score = false;

    query_kinds query_type = query_kinds::none;

    //!< Keeps a query match.
    std::optional<match_query> match;
    //!< Keeps a term.
    std::optional<term_query> term;
    //!< Keeps a list of terms.
    std::optional<terms_query> terms;

    //!< Keeps original body,
    std::string body;
  };
//-------------------------------------------------------------------------//
  inline auto get_thread_parser_context() -> parser_context & {
    thread_local parser_context context;
    return context;
  }
//-------------------------------------------------------------------------//
  /**
   * Validates JSON object.
   * @param body [in] - JSON object.
   */
  auto validate_json_object(std::string_view body) -> void;

  /**
   * Parses mget request.
   * @param body
   * @param default_index
   * @return
   */
  auto parse_mget_request(std::string_view body, std::string_view default_index) -> mget_request;

  /**
   * Parses a search request.
   * @param body
   * @param indices
   * @return
   */
  auto parse_search_request(std::string_view body, std::vector<std::string> indices) -> search_request;
//-------------------------------------------------------------------------//
} // namespace docapi::json
//-------------------------------------------------------------------------//
#endif // __SIMD_JSON_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
