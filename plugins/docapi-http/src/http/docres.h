/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          docres.h
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
#ifndef __DOCRES_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __DOCRES_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <atomic>
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include <mtc/zmap.h>
//-------------------------------------------------------------------------//
namespace uWS {
//-------------------------------------------------------------------------//
  template<bool SSL>
  class HttpResponse;
//-------------------------------------------------------------------------//
}// namespace uWS
//-------------------------------------------------------------------------//
namespace docapi::http {
//-------------------------------------------------------------------------//
  template<bool SSL>
  struct response_context {
    //<!!!> The owner of the pointer is uWebSockets. It must be used ONLY in the uWS loop thread before aborting.
    uWS::HttpResponse<SSL> *response = nullptr;

    //!< Keeps a status of aborted or not.
    std::atomic_bool aborted = false;
  };

  struct service_response {
    //!< Keeps a response status.
    int status = 200;

    //!< Keeps a response content type.
    std::string content_type = "application/json; charset=utf-8";

    //!< Keeps a response body.
    std::string body;
  };
//-------------------------------------------------------------------------//
  auto make_json_response(const mtc::zmap &zmap) -> service_response;
//-------------------------------------------------------------------------//
  template<bool SSL>
  void send_json_response(response_context<SSL> *ctx, const service_response &response);

  template<bool SSL>
  void send_error_response(response_context<SSL> *ctx, int status, std::string_view error_type, std::string_view reason);
//-------------------------------------------------------------------------//
}// namespace docapi::http
//-------------------------------------------------------------------------//
#endif // __DOCRES_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
