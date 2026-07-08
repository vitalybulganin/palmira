#include "search-parser.h"
//-------------------------------------------------------------------------//
#include "../common/except.h"
//-------------------------------------------------------------------------//
#include "../json/simd-json-search-parser.h"
//-------------------------------------------------------------------------//
#include "query-parser.h"
//-------------------------------------------------------------------------//
namespace docapi::parsers {
//-------------------------------------------------------------------------//
  search_parser::search_parser() : query_parser(new parsers::query_parser()) {
  }
//-------------------------------------------------------------------------//
  auto search_parser::validate(std::string_view body) const -> void {
  }

  auto search_parser::parse(std::string_view body, const mtc::zmap &opts) const -> std::unique_ptr<value_type> {
    auto args = std::make_unique<value_type>();
    // Parsing a search request.
    const auto req = json::parse_search_request(body, {opts.get_charstr("_index", "")});

    // Setting an order of searching.
    args->order = mtc::zmap{
    {"first",   req.from},
      {"count", req.size}
    };

    if (req.match.has_value()) {
      *args->query.set_charstr(req.match.value().field) = req.match.value().query;
    }

    return args;
  }
//-------------------------------------------------------------------------//
} // namespace docapi::parsers
