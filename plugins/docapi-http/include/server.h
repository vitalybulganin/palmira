/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          server.h
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
#ifndef __SERVER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SERVER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include "server.hpp"
//-------------------------------------------------------------------------//
using server_t = mtc::api<palmira::IServer>;
//-------------------------------------------------------------------------//
/**
 * Creates a new server.
 * @param listening_port [in] - A listening port.
 * @return A server instance.
 */
extern "C" auto createServer(std::uint16_t listening_port) -> server_t;
//-------------------------------------------------------------------------//
#endif // __SERVER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
