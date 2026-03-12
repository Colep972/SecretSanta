#pragma warning(disable: 4996)
#define NOMINMAX
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "NetworkUtils.h"
#include <stdexcept>

static const char* SERVER_HOST = "santa.colep.fr";
static const int   SERVER_PORT = 443;

nlohmann::json sendRequest(const nlohmann::json& request)
{
    httplib::SSLClient cli(SERVER_HOST, SERVER_PORT);
    cli.enable_server_certificate_verification(false);

    auto result = cli.Post("/api", request.dump(), "application/json");

    if (!result)
        throw std::runtime_error("Connection failed: " + httplib::to_string(result.error()));

    if (result->status != 200)
        throw std::runtime_error("HTTP error: " + std::to_string(result->status));

    return nlohmann::json::parse(result->body);
}