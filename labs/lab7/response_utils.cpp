#include "response_utils.h"

#include <stdexcept>
#include <string>

#include <cpr/error.h>
#include <cpr/response.h>

void ThrowIfApiResponseFailed(const cpr::Response& response, const std::string& message_prefix) {
    if (response.error.code != cpr::ErrorCode::OK) {
        throw std::invalid_argument(message_prefix + ": " + response.error.message);
    }
    if (response.status_code != 200) {
        throw std::invalid_argument(message_prefix + ": HTTP-код " +
                                    std::to_string(response.status_code));
    }
}

nlohmann::json ParseJsonResponse(const cpr::Response& response, const std::string& message_prefix) {
    ThrowIfApiResponseFailed(response, message_prefix);
    return nlohmann::json::parse(response.text);
}
