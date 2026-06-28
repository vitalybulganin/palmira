/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          simd-json.h
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
#ifndef __SIMD_JSON_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SIMD_JSON_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
#include <vector>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include "../http/docreq.h"
//-------------------------------------------------------------------------//
namespace docapi::json {
//-------------------------------------------------------------------------//
  /**
   * Converts raw JSON string into string.
   * @param value [in] - Raw JSON string into string.
   * @return A string.
   */
  auto to_string(simdjson::ondemand::raw_json_string value) -> std::string;

  /**
   * Gets a value as boolean.
   * @param value [in] - A value.
   * @return Boolean.
   */
  auto get_bool_value(simdjson::ondemand::value value) -> bool;

  /**
   * Gets a value as a vector of strings.
   * @param value [in] - A value.
   * @return A vector of strings.
   */
  auto get_string_array(simdjson::ondemand::value value) -> std::vector<std::string>;

  /**
   * Gets document options as a source object.
   * @param object [in] - A value.
   * @param options [out] - Options.
   */
  auto get_source_object(simdjson::ondemand::object object, http::document_get_options &options) -> void;

  /**
   * Gets document options as a source value.
   * @param value [in] - A value.
   * @param options [out] - Options.
   * @note
   * Elasticsearch/OpenSearch допускает разные формы _source:
     *
     * "_source": true
     * "_source": false
     * "_source": ["field1", "field2"]
     * "_source": {
     *     "includes": [...],
     *     "excludes": [...]
     * }
   */
  auto get_source_value(simdjson::ondemand::value value, http::document_get_options& options) -> void;

  /**
   * Gets a document item.
   * @param object [in] - An object.
   * @param default_index [in] - Default index.
   * @return A document item.
   */
  auto get_doc_item(simdjson::ondemand::object object, std::string_view default_index) -> http::document_get_item;
//-------------------------------------------------------------------------//
  /**
   * Validates a document on valid.
   * @param parser [in] - A parser.
   * @param body [in] - A document.
   */
  auto validate_json_document(simdjson::ondemand::parser &parser, std::string_view body) -> void;
//-------------------------------------------------------------------------//
} // namespace docapi::json
//-------------------------------------------------------------------------//
#endif // __SIMD_JSON_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
