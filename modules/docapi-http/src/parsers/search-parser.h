/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          search-parser.h
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
#ifndef __SEARCH_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SEARCH_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string_view>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <service.hpp>
//-------------------------------------------------------------------------//
#include "../common/parser.h"
//-------------------------------------------------------------------------//
namespace docapi::parsers {
//-------------------------------------------------------------------------//
  class search_parser : public common::parser<palmira::SearchArgs> {
    //!< Keeps JSON parser, which is not thread-safe.
    mutable simdjson::ondemand::parser parser;

  public:
    /**
     * Destructor.
     */
    virtual ~search_parser() override = default;

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
  };
//-------------------------------------------------------------------------//
} // namespace docapi::parsers
//-------------------------------------------------------------------------//
#endif // __SEARCH_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__

