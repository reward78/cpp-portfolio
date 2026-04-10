#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "Path1.h"

namespace {

nlohmann::json GetObject(const nlohmann::json& object, const char* key) {
    if (!object.contains(key) || !object[key].is_object()) {
        return nlohmann::json::object();
    }
    return object[key];
}

std::string GetString(const nlohmann::json& object, const char* key, const std::string& fallback = "unknown") {
    if (!object.contains(key) || !object[key].is_string()) {
        return fallback;
    }
    return object[key].get<std::string>();
}

double GetNumber(const nlohmann::json& object, const char* key, double fallback = 0.0) {
    if (!object.contains(key) || !object[key].is_number()) {
        return fallback;
    }
    return object[key].get<double>();
}

std::string GetTransportTypes(const nlohmann::json& segment) {
    if (!segment.contains("transport_types") || !segment["transport_types"].is_array()) {
        return "unknown";
    }

    std::string result;
    bool first = true;
    for (const auto& transport : segment["transport_types"]) {
        if (!first) {
            result += " -> ";
        }
        first = false;
        result += transport.get<std::string>();
    }
    return result;
}

void PrintDirectRoute(const nlohmann::json& segment, int route_number) {
    const nlohmann::json thread = GetObject(segment, "thread");
    const nlohmann::json carrier = GetObject(thread, "carrier");

    std::cout << "Marshrut " << route_number << '\n';
    std::cout << "Tip: bez peresadok\n";
    std::cout << "Napravlenie: " << GetString(thread, "title") << '\n';
    std::cout << "Otpravlenie: " << GetString(segment, "departure") << '\n';
    std::cout << "Pribytie: " << GetString(segment, "arrival") << '\n';
    std::cout << "Dlitelnost: " << GetNumber(segment, "duration") << '\n';
    std::cout << "Transport: " << GetString(thread, "transport_type") << '\n';
    std::cout << "Perevozchik: " << GetString(carrier, "title") << "\n\n";
}

void PrintTransferRoute(const nlohmann::json& segment, int route_number) {
    const nlohmann::json departure_from = GetObject(segment, "departure_from");
    const nlohmann::json arrival_to = GetObject(segment, "arrival_to");

    std::cout << "Marshrut " << route_number << '\n';
    std::cout << "Tip: 1 peresadka\n";
    std::cout << "Napravlenie: "
              << GetString(departure_from, "title") << " -> "
              << GetString(arrival_to, "title") << '\n';
    std::cout << "Otpravlenie: " << GetString(segment, "departure") << '\n';
    std::cout << "Pribytie: " << GetString(segment, "arrival") << '\n';
    std::cout << "Dlitelnost: " << GetNumber(segment, "duration") << '\n';
    std::cout << "Peresadka v: " << GetString(segment["transfers"][0], "title") << '\n';
    std::cout << "Transport: " << GetTransportTypes(segment) << "\n\n";
}

}  // namespace

void Path1(const nlohmann::json& j1) {
    int route_number = 1;
    for (const auto& segment : j1["segments"]) {
        if (!segment.value("has_transfers", false)) {
            PrintDirectRoute(segment, route_number++);
        } else if (segment.contains("transfers") &&
                   segment["transfers"].is_array() &&
                   segment["transfers"].size() == 1) {
            PrintTransferRoute(segment, route_number++);
        }
    }
}
