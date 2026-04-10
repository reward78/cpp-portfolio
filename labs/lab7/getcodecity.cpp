#include "getstationlist.h"
#include "getcodecity.h"
#include "city_code_utils.h"

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>

static std::unordered_map<std::string, std::string> city_codes;
std::string GetCodeCity(const std::string& city) {
    nlohmann::json station_list = GetStationList();
    return GetOrFindCityCode(city_codes, station_list, city);
}
