/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          config.h
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
#ifndef __CONFIG_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __CONFIG_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
#include <chrono>
#include <cstdint>
//-------------------------------------------------------------------------//
namespace docapi::common {
//-------------------------------------------------------------------------//
  struct ssl {
    //!< Keeps a flag of SSL enabled or not.
    bool enabled = false;

    //!< Keeps a certificate file.
    std::string cert_file;

    //!< Keeps a private key file.
    std::string priv_key_file;

    //!< Keeps parameters file.
    std::string dh_params_file;
  };

  struct config {
    //!< Keeps a host to listen on.
    std::string listen_host = "0.0.0.0";

    //!< Keeps a port to listen on.
    std::uint16_t port = 9200;

    //!< Keeps a number of worker threads (0 - means std::thread::hardware_concurrency() - 1).
    std::uint16_t worker_threads = 0U;

    //!< Keeps a max size of body.
    std::size_t max_body_size = 32U * 1024U * 1024U;

    //!< Keeps a request timeout (30 sec).
    std::chrono::milliseconds request_timeout{30000};

    docapi::common::ssl ssl = {};
  };
//-------------------------------------------------------------------------//
} // namespace docapi::common
//-------------------------------------------------------------------------//
#endif // __CONFIG_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__

