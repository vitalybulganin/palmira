#include "../include/server.h"
//-------------------------------------------------------------------------//
#include "server/http-server.h"
//-------------------------------------------------------------------------//
#include "common/config.h"
//-------------------------------------------------------------------------//
auto createServer(std::uint16_t listening_port) -> server_t {
  docapi::common::config config{
    .port = listening_port
  };

  return new docapi::HttpServer(std::move(config), {});
}
