#include "city_code_utils.h"

#include <stdexcept>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

std::string FindCityCode(const nlohmann::json& station_list, const std::string& city) {
    for (const auto& country : station_list["countries"]) {
        for (const auto& region : country["regions"]) {
            for (const auto& settlement : region["settlements"]) {
                if (settlement["title"] == city) {
                    return settlement["codes"]["yandex_code"];
                }
            }
        }
    }
    throw std::invalid_argument("Город не найден: " + city);
}

std::string GetOrFindCityCode(std::unordered_map<std::string, std::string>& city_codes,
                              const nlohmann::json& station_list,
                              const std::string& city) {
    if (city_codes.contains(city)) {
        return city_codes[city];
    }
    city_codes[city] = FindCityCode(station_list, city);
    return city_codes[city];
}
