#pragma once

#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

std::string FindCityCode(const nlohmann::json& station_list, const std::string& city);
std::string GetOrFindCityCode(std::unordered_map<std::string, std::string>& city_codes,
                              const nlohmann::json& station_list,
                              const std::string& city);
