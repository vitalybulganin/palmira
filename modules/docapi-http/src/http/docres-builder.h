/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          docres-builder.h
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
#ifndef __DOCRES_BUILDER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __DOCRES_BUILDER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string_view>
//-------------------------------------------------------------------------//
#include <common/responsible.h>
//-------------------------------------------------------------------------//
namespace docapi::http {
//-------------------------------------------------------------------------//
  enum class docres_types {none = 0, insert = 1, update, remove, search};
//-------------------------------------------------------------------------//
  class docres_builder {
    friend auto make_docres_builder() -> docres_builder;

    //!< Keeps a type of response.
    docres_types type = docres_types::none;

  public:
    /**
     * Sets a new document response type.
     * @param type [in] - A type of response.
     * @return The reference on the instance.
     */
    auto set(docres_types type) -> docres_builder &;

    /**
     * Sets a new document response type.
     * @param type [in] - A type of response.
     * @return The reference on the instance.
     */
    auto set(std::string_view type) -> docres_builder &;

    /**
     * Builds a new response.
     * @return A new instance of response.
     * @throw std::invalid_argument - Unknown a document response.
     */
    auto build() const -> palmira::modules::response_t;

  protected:
    //!< Constructor.
    docres_builder() = default;
  };
//-------------------------------------------------------------------------//
  //!< Makes a document response builder.
  auto make_docres_builder() -> docres_builder;
//-------------------------------------------------------------------------//
  /**
   * Gets a document response type as a string.
   * @param type [in] - A type of document response.
   * @return A document response type as a string.
   */
  auto to_string(docres_types type) -> std::string_view;

  /**
   * Gets a document response type.
   * @param type [in] - A type of document response as a string.
   * @return A document response type.
   */
  auto docres_type_from(std::string_view type) -> docres_types;
//-------------------------------------------------------------------------//
} // namespace docapi::http
//-------------------------------------------------------------------------//
#endif // __DOCRES_BUILDER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
