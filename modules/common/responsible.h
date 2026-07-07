/*!==========================================================================
* \file
* - Program:       modules-common
* - File:          responsible.h
* - Created:       07/04/2026
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
#ifndef __RESPONSABLE_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __RESPONSABLE_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
#include <memory>
//-------------------------------------------------------------------------//
#include <mtc/zmap.h>
//-------------------------------------------------------------------------//
namespace palmira::modules {
//-------------------------------------------------------------------------//
  struct responsible {
    /**
     * Makes a response.
     * @param resp [in] - A response.
     * @return A response as a string.
     */
    virtual auto make_reply(const mtc::zmap &resp) const -> std::string = 0;

    /**
     * Destructor.
     */
    virtual ~responsible() = default;
  };
//-------------------------------------------------------------------------//
  using response_t = std::shared_ptr<responsible>;
//-------------------------------------------------------------------------//
} // namespace palmira::modules
//-------------------------------------------------------------------------//
#endif // __RESPONSABLE_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
