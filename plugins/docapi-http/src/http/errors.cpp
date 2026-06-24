#include "errors.h"
//-------------------------------------------------------------------------//
namespace docapi::http {
//-------------------------------------------------------------------------//
  auto status_to_string(int status) -> std::string_view {
    switch (status) {
    case 200: return "200 OK";
    case 201: return "201 Created";
    case 202: return "202 Accepted";
    case 204: return "204 No Content";

    case 400: return "400 Bad Request";
    case 401: return "401 Unauthorized";
    case 403: return "403 Forbidden";
    case 404: return "404 Not Found";
    case 405: return "405 Method Not Allowed";
    case 409: return "409 Conflict";
    case 413: return "413 Payload Too Large";
    case 415: return "415 Unsupported Media Type";
    case 429: return "429 Too Many Requests";

    case 500: return "500 Internal Server Error";
    case 501: return "501 Not Implemented";
    case 503: return "503 Service Unavailable";

    default: return "500 Internal Server Error";
    }
  }

  auto make_error_json(std::string type, std::string reason) -> std::string {
    return R"({"error":{"type":")" + type + R"(","reason":")" + reason + R"("},"status":500})";
  }
//-------------------------------------------------------------------------//
} // namespace docapi::http
