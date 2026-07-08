#include "insert-parser.h"
//-------------------------------------------------------------------------//
#include <common/module-utils.h>
//-------------------------------------------------------------------------//
#include "../json/simd-json-visit.h"
//-------------------------------------------------------------------------//
namespace docapi::parsers {
//-------------------------------------------------------------------------//
  auto insert_parser::validate(std::string_view body) const -> void {
  }

  auto insert_parser::parse(std::string_view body, const mtc::zmap &opts) const -> std::unique_ptr<value_type> {
    auto args = std::make_unique<value_type>();

    // Making a new unique document id.
    args->objectId = palmira::modules::make_uid();
    args->uVersion = 1;

    // Copying options into metadata.
    args->metadata = mtc::zmap{
      {"_index",    opts.get_charstr("_index", "")},
      {"_started",  opts.get_int64("_started", 0)},
      {"_id",      args->objectId},
      {"_version",    static_cast<std::int64_t>(args->uVersion)},
    };

    // Parsing request body as JSON.
    json::visit_json_cb(body, [&](const json::json_visit_event &event) -> bool {
      std::fprintf(stdout, "%s\n", event.as_str().c_str());

      // Adding a new block.
      if (event.type == docapi::json::json_value_types::string && not event.value.string_value.empty()) {
        args->GetTextAPI().AddMarkupTag({event.name.data(), event.name.size()})->AddBlock(event.value.string_value.data(), event.value.string_value.size());
      } else if (event.type == docapi::json::json_value_types::boolean) {
        args->GetTextAPI().AddMarkupTag({event.name.data(), event.name.size()})->AddBlock(event.value.bool_value ? "true" : "false");
      } else if (event.type == docapi::json::json_value_types::int64) {
        args->GetTextAPI().AddMarkupTag({event.name.data(), event.name.size()})->AddBlock(std::to_string(event.value.int64_value).c_str());
      } else if (event.type == docapi::json::json_value_types::uint64) {
        args->GetTextAPI().AddMarkupTag({event.name.data(), event.name.size()})->AddBlock(std::to_string(event.value.uint64_value).c_str());
      } else if (event.type == docapi::json::json_value_types::double_value) {
        args->GetTextAPI().AddMarkupTag({event.name.data(), event.name.size()})->AddBlock(std::to_string(event.value.double_value).c_str());
      } else if (event.type == docapi::json::json_value_types::null_value) {
        args->GetTextAPI().AddMarkupTag({event.name.data(), event.name.size()})->AddBlock("");
      }

      return false;
    });

    return args;
  }
//-------------------------------------------------------------------------//
} // namespace docapi::parsers
