#include "insert-parser.h"
//-------------------------------------------------------------------------//
#include "../simd-json/simd-json-visit.h"
//-------------------------------------------------------------------------//
namespace docapi::parsers {
//-------------------------------------------------------------------------//
  auto insert_parser::validate(std::string_view body) -> void {
  }

  auto insert_parser::parse(std::string_view body, const mtc::zmap &opts) -> std::unique_ptr<value_type> {
    auto args = std::make_unique<value_type>();

    // Reading values from options
    for (const auto &mdata : opts) {
    }

    // Parsing request body as JSON.
    json::VisitJsonFast(body, [&](const json::json_visit_event &event) {
      std::fprintf(stdout, "%s\n", event.as_str().c_str());

      if (event.type == docapi::json::json_value_types::string && not event.string_value.empty()) {
        // Adding a new block.
        args->GetTextAPI().AddBlock(event.string_value.data(), event.string_value.size());
      }
    });

    return args;
  }
//-------------------------------------------------------------------------//
} // namespace docapi::parsers
