/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          insert-response.h
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
#ifndef __INSERT_RESPONSE_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __INSERT_RESPONSE_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <common/responsible.h>
//-------------------------------------------------------------------------//
namespace docapi::http {
//-------------------------------------------------------------------------//
  class docres_builder;
//-------------------------------------------------------------------------//
  class insert_response : public palmira::modules::responsible {
    friend class docres_builder;

  public:
    /**
     * Destructor.
     */
    virtual ~insert_response() override = default;

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
    insert_response() = default;
  };
//-------------------------------------------------------------------------//
} // namespace docapi::http
//-------------------------------------------------------------------------//
#endif // __INSERT_RESPONSE_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
