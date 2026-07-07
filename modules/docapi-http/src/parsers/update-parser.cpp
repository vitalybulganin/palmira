#include "update-parser.h"
//-------------------------------------------------------------------------//
namespace docapi::parsers {
  //-------------------------------------------------------------------------//
  auto update_parser::validate(std::string_view body) const -> void {

  }

  auto update_parser::parse(std::string_view body, const mtc::zmap &opts) const -> std::unique_ptr<value_type> {
    return {};
  }
  //-------------------------------------------------------------------------//
} // namespace docapi::parsers
