/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          search-response.h
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
#ifndef __SEARCH_RESPONSE_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SEARCH_RESPONSE_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <common/responsible.h>
//-------------------------------------------------------------------------//
namespace docapi::http {
//-------------------------------------------------------------------------//
  class docres_builder;
//-------------------------------------------------------------------------//
  class search_response : public palmira::modules::responsible {
    friend class docres_builder;

  public:
    /**
     * Destructor.
     */
    virtual ~search_response() override = default;

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
    search_response() = default;
  };
//-------------------------------------------------------------------------//
} // namespace docapi::http
//-------------------------------------------------------------------------//
#endif // __SEARCH_RESPONSE_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
