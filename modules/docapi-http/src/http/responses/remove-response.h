/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          remove-response.h
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
#ifndef __REMOVE_RESPONSE_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __REMOVE_RESPONSE_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <common/responsible.h>
//-------------------------------------------------------------------------//
namespace docapi::http {
//-------------------------------------------------------------------------//
  class docres_builder;
//-------------------------------------------------------------------------//
  class remove_response : public palmira::modules::responsible {
    friend class docres_builder;

  public:
    /**
     * Destructor.
     */
    virtual ~remove_response() override = default;

    // Override methods
  public:
    /**
     * Makes a response.
     * @param resp [in] - A response.
     * @return A response as a string.
     */
    virtual auto make_reply(const mtc::zmap &resp) const -> std::string override;

  protected:
    //!< Constructor.
    remove_response() = default;
  };
//-------------------------------------------------------------------------//
} // namespace docapi::http
//-------------------------------------------------------------------------//
#endif // __REMOVE_RESPONSE_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
