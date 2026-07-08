#include "query-parser.h"
//-------------------------------------------------------------------------//
#include "../common/except.h"
//-------------------------------------------------------------------------//
#include <common/module-utils.h>
//-------------------------------------------------------------------------//
#include "../json/simd-json-visit.h"
//-------------------------------------------------------------------------//
namespace docapi::parsers {
  //-------------------------------------------------------------------------//
  auto query_parser::validate(std::string_view body) const -> void {
  }

  auto query_parser::parse(std::string_view body, const mtc::zmap &opts) const -> std::unique_ptr<value_type> {
    auto args = std::make_unique<value_type>();

    // Parsing request body as JSON.
    json::visit_json_cb(body, [&](const json::json_visit_event &event) -> bool {
      std::fprintf(stdout, "%s\n", event.as_str().c_str());

      // Adding a new block.
      if (event.type == docapi::json::json_value_types::string && not event.value.string_value.empty()) {
        if (event.value.string_value == "query") {

        }
      } else if (event.type == docapi::json::json_value_types::boolean) {
      } else if (event.type == docapi::json::json_value_types::int64) {
      } else if (event.type == docapi::json::json_value_types::uint64) {
      } else if (event.type == docapi::json::json_value_types::double_value) {
      } else if (event.type == docapi::json::json_value_types::null_value) {
      }

      return false;
    });

    return args;
  }
//-------------------------------------------------------------------------//
} // namespace docapi::parsers
