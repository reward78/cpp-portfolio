#pragma once

#include <string>

#include <cpr/response.h>
#include <nlohmann/json.hpp>

void ThrowIfApiResponseFailed(const cpr::Response& response, const std::string& message_prefix);
nlohmann::json ParseJsonResponse(const cpr::Response& response, const std::string& message_prefix);
