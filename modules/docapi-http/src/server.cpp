#include "server.h"
//-------------------------------------------------------------------------//
#include <utility>
//-------------------------------------------------------------------------//
#include "server/http-server.h"
//-------------------------------------------------------------------------//
#include "config.h"
//-------------------------------------------------------------------------//
auto createServer(service_t service, docapi::common::config config) -> server_t {
  return new docapi::HttpServer(std::move(service), std::move(config));
}
