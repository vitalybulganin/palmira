#include "search-parser.h"
//-------------------------------------------------------------------------//
#include "../common/except.h"
//-------------------------------------------------------------------------//
#include "../simd-json/simd-json.h"
#include <string_view>
//-------------------------------------------------------------------------//
namespace docapi::parsers {
  //-------------------------------------------------------------------------//
  auto search_parser::validate(std::string_view body) const -> void {
  }

  auto search_parser::parse(std::string_view body, const mtc::zmap &opts) const -> std::unique_ptr<value_type> {
    simdjson::padded_string padded_body(body);
    simdjson::ondemand::document document;
    simdjson::ondemand::object root;

    auto search = std::make_unique<value_type>();

    const std::string_view default_index = opts.get_charstr("default_index", "");
    auto error = this->parser.iterate(padded_body).get(document);
    if (error) {
        throw (common::json_parse_error("invalid mget JSON body"));
    }

    error = document.get_object().get(root);
    if (error) {
        throw common::json_parse_error("mget body must be JSON object");
    }

/*
    auto  get_id = req.GetUri().parameters().get( "id", "undefined" );
    auto  sz_req = jsn.get_charstr( "string" );
    auto  ws_req = jsn.get_widestr( "string" );
    auto  zm_req = jsn.get_zmap   ( "query");


    search->order["first"] = jsn.get_int32( "first", 1 );
    search->order["count"] = jsn.get_int32( "count", 10 );

    if ( sz_req != nullptr )  search.query = structo::queries::ParseQuery( *sz_req );
    else
      if ( ws_req != nullptr )  search.query = structo::queries::ParseQuery( *ws_req );
      else
        if ( zm_req != nullptr )  search.query = *zm_req;
        else
          throw std::invalid_argument( "request contains neither 'query' nor 'string'" );
*/

    for (auto field : root) {
      std::string_view key;

      error = field.unescaped_key().get(key);
      if (error) {
        throw (common::json_parse_error("failed to read mget root key"));
      }

      simdjson::ondemand::value value = field.value();
      if (key == "docs") {
        simdjson::ondemand::array docs;

        error = value.get_array().get(docs);
        if (error) {
          throw (common::json_parse_error("mget docs must be array"));
        }

        for (auto doc_value : docs) {
          simdjson::ondemand::object doc_object;

          error = doc_value.get_object().get(doc_object);
          if (error) {
            throw (common::json_parse_error("mget docs item must be object"));
          }
          const auto doc = json::get_doc_item(doc_object, default_index);

          search->query.get_array_zmap()->push_back(mtc::zmap{
            {"_index",   doc.index},
            {"_id",      doc.id},
            {"_routing", doc.routing},
            {"_options", doc.options.as_zval()},
          });
        }
      } else if (key == "ids") {
        simdjson::ondemand::array ids;

        error = value.get_array().get(ids);
        if (error) {
          throw common::json_parse_error("mget ids must be array");
        }

        if (default_index.empty()) {
          throw (common::json_parse_error("mget ids form requires default index"));
        }

        for (auto id_value : ids) {
          std::string_view id;

          error = id_value.get_string().get(id);
          if (error) {
              throw (common::json_parse_error("mget ids item must be string"));
          }

          search->query.get_array_zmap()->push_back(mtc::zmap {
            {"index", std::string(default_index)},
            {"id", id}
          });
        }
      }
    }

    if (search->query.empty()) {
        throw (common::json_parse_error("mget request contains no documents"));
    }

    return search;
  }
//-------------------------------------------------------------------------//
} // namespace docapi::parsers
