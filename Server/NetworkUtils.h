#pragma once
#include <string>
#include <nlohmann/json.hpp>

// Sends a JSON request to the SecretSanta HTTPS API and returns the parsed response.
// Throws std::runtime_error on connection failure.
nlohmann::json sendRequest(const nlohmann::json& request);