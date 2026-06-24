/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          errors.h
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
#ifndef __ERRORS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __ERRORS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
namespace docapi::http {
//-------------------------------------------------------------------------//
  /**
   * Gets status as a string.
   * @param status [in] - A status code.
   * @return A status as a string.
   */
  auto status_to_string(int status) -> std::string_view;

  /**
   * Makes a JSON error response.
   * @param type [in] - Error type.
   * @param reason [in] - Error reason.
   * @return Error JSON string.
   */
  auto make_error_json(std::string type, std::string reason) -> std::string;
//-------------------------------------------------------------------------//
} // namespace docapi::http
//-------------------------------------------------------------------------//
#endif // __ERRORS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
