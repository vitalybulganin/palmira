#include "../docres-builder.h"
//-------------------------------------------------------------------------//
#include <stdexcept>
//-------------------------------------------------------------------------//
#include "../responses/insert-response.h"
#include "../responses/update-response.h"
#include "../responses/remove-response.h"
#include "../responses/search-response.h"
//-------------------------------------------------------------------------//
namespace docapi::http {
//-------------------------------------------------------------------------//
  auto docres_builder::set(docres_types docres_type) -> docres_builder & {
    return this->type = docres_type, *this;
  }

  auto docres_builder::set(std::string_view type) -> docres_builder & {
    return this->type = docres_type_from(type), *this;
  }

  auto docres_builder::build() const -> palmira::modules::response_t {
    if (this->type == docres_types::none) {
      throw (std::invalid_argument("Document response type not set"));
    }

    if (this->type == docres_types::insert) {
      return palmira::modules::response_t(new insert_response());
    } else if (this->type == docres_types::update) {
      return palmira::modules::response_t(new update_response());
    } else if (this->type == docres_types::remove) {
      return palmira::modules::response_t(new remove_response());
    } else if (this->type == docres_types::search) {
      return palmira::modules::response_t(new search_response());
    }
    return {};
  }
//-------------------------------------------------------------------------//
  auto make_docres_builder() -> docres_builder {
    return {};
  }
//-------------------------------------------------------------------------//
  auto to_string(docres_types type) -> std::string_view {
    switch (type) {
      case docres_types::none: return "none";
      case docres_types::insert: return "insert";
      case docres_types::update: return "update";
      case docres_types::remove: return "remove";
      case docres_types::search: return "search";
    }
    return "none";
  }

  auto docres_type_from(std::string_view type) -> docres_types {
    if (type == "insert") {
      return docres_types::insert;
    } else if (type == "update") {
      return docres_types::update;
    } else if (type == "remove") {
      return docres_types::remove;
    } else if (type == "search") {
      return docres_types::search;
    }
    return docres_types::none;
  }
//-------------------------------------------------------------------------//
} // namespace docapi::http
