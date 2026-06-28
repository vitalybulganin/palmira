/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          remove-parser.h
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
#ifndef __INDEX_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __INDEX_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <service.hpp>
//-------------------------------------------------------------------------//
#include "../common/parser.h"
//-------------------------------------------------------------------------//
namespace docapi::parsers {
//-------------------------------------------------------------------------//
  class remove_parser : public common::parser<palmira::RemoveArgs> {
    //!< Keeps JSON parser, which is not thread-safe.
    simdjson::ondemand::parser parser;

  public:
    /**
     * Destructor.
     */
    virtual ~remove_parser() override = default;

    // Override methods
  public:
    /**
     * Validates a body on valid.
     * @param body [in] - A document body.
     */
    virtual auto validate(std::string_view body) -> void override;

    /**
     * Parses a document.
     * @param body [in] - A document body.
     * @param opts [in] - Options.
     * @return A parsed object.
     */
    virtual auto parse(std::string_view body, const mtc::zmap &opts) -> std::unique_ptr<value_type> override;
  };
//-------------------------------------------------------------------------//
} // namespace docapi::parsers
//-------------------------------------------------------------------------//
#endif // __INDEX_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__

