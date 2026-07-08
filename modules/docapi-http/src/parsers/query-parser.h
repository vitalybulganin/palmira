/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          query-parser.h
* - Created:       07/07/2026
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
#ifndef __QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include "../common/parser.h"
//-------------------------------------------------------------------------//
namespace docapi::parsers {
//-------------------------------------------------------------------------//
  class search_parser;
//-------------------------------------------------------------------------//
  class query_parser : public common::parser<mtc::zmap> {
    friend class search_parser;

    //!< Keeps JSON parser, which is not thread-safe.
    simdjson::ondemand::parser parser;

  public:
    query_parser(const query_parser &) = delete;
    query_parser(query_parser &&) = delete;
    query_parser &operator=(const query_parser &) = delete;
    query_parser &operator=(query_parser &&) = delete;

    /**
     * Destructor.
     */
    virtual ~query_parser() override = default;

    // Override methods
  public:
    /**
     * Validates a body on valid.
     * @param body [in] - A document body.
     */
    virtual auto validate(std::string_view body) const -> void override;

    /**
     * Parses a document.
     * @param body [in] - A document body.
     * @param opts [in] - Options.
     * @return A parsed object.
     */
    virtual auto parse(std::string_view body, const mtc::zmap &opts) const -> std::unique_ptr<value_type> override;

  protected:
    //!< Constructor.
    query_parser() = default;
  };
  //-------------------------------------------------------------------------//
} // namespace docapi::parsers
//-------------------------------------------------------------------------//
#endif // __QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
