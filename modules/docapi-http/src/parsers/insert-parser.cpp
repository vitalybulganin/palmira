#include "insert-parser.h"
//-------------------------------------------------------------------------//
namespace docapi::parsers {
  //-------------------------------------------------------------------------//
  auto insert_parser::validate(std::string_view body) -> void {

  }

  auto insert_parser::parse(std::string_view body, const mtc::zmap &opts) -> std::unique_ptr<value_type> {
    return {};
  }
//-------------------------------------------------------------------------//
} // namespace docapi::parsers
