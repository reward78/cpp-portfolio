#include "app.h"

#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <cpr/api.h>
#include <cpr/cprtypes.h>
#include <cpr/error.h>
#include <cpr/parameters.h>
#include <cpr/response.h>
#include <nlohmann/json.hpp>

#include "GetAPI.h"
#include "Path1.h"
#include "getcodecity.h"

namespace {

std::unordered_map<std::string, CacheEntry> route_cache;

GetCodeCityFn get_code_city_fn = &GetCodeCity;
GetApiFn get_api_fn = &GetApi;
PrintPathFn print_path_fn = &Path1;

cpr::Response SearchRoutes(const std::string& api,
                           const std::string& from,
                           const std::string& to,
                           const std::string& date) {
    return cpr::Get(
        cpr::Url("https://api.rasp.yandex.net/v3.0/search/"),
        cpr::Parameters{{"apikey", api},
                        {"from", from},
                        {"to", to},
                        {"date", date},
                        {"transfers", "true"}});
}

SearchRoutesFn search_routes_fn = &SearchRoutes;

}  // namespace

std::string MakeRouteKey(const std::string& from, const std::string& to, const std::string& date) {
    return from + '|' + to + '|' + date;
}

bool IsCacheFresh(const std::unordered_map<std::string, CacheEntry>& route_cache,
                  const std::string& key,
                  const std::chrono::steady_clock::time_point& now,
                  const std::chrono::minutes& ttl) {
    if (!route_cache.contains(key)) {
        return false;
    }
    return now - route_cache.at(key).time < ttl;
}

void PutRouteIntoCache(std::unordered_map<std::string, CacheEntry>& route_cache,
                       const std::string& key,
                       const nlohmann::json& data,
                       const std::chrono::steady_clock::time_point& now,
                       std::size_t max_cache_size) {
    CacheEntry entry{data, now};
    if (route_cache.size() >= max_cache_size) {
        auto oldest_it = route_cache.begin();
        for (auto it = route_cache.begin(); it != route_cache.end(); ++it) {
            if (it->second.time <= oldest_it->second.time) {
                oldest_it = it;
            }
        }
        route_cache.erase(oldest_it);
    }
    route_cache[key] = entry;
}

bool HasSegments(const nlohmann::json& data) {
    return data.contains("segments") && data["segments"].is_array() && !data["segments"].empty();
}

void ThrowIfResponseFailed(const cpr::Response& response, const std::string& route_name) {
    if (response.error.code != cpr::ErrorCode::OK) {
        throw std::invalid_argument("Ошибка сети: " + response.error.message);
    }
    if (response.status_code != 200) {
        throw std::invalid_argument("Ошибка API для " + route_name + ". HTTP-код: " +
                                    std::to_string(response.status_code) +
                                    ". Ответ: " + response.text);
    }
}

int RunInteractiveMode() {
    while (true) {
        std::string flag;
        std::string from_input;
        std::string to_input;
        std::string data;
        std::cout << "Введите город отправления : ";
        std::getline(std::cin, from_input);
        std::cout << std::endl;
        std::cout << "Введите город назначения : ";
        std::getline(std::cin, to_input);
        std::cout << std::endl;
        std::cout << "Введите дату назначения : ";
        std::getline(std::cin, data);

        try {
            if (from_input.empty() || to_input.empty() || data.empty()) {
                throw std::invalid_argument("Поля отправления, назначения и даты не должны быть пустыми");
            }

            std::string from = get_code_city_fn(from_input);
            std::string to = get_code_city_fn(to_input);
            std::string api = get_api_fn();

            std::string key = MakeRouteKey(from, to, data);
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

            if (IsCacheFresh(route_cache, key, now, std::chrono::minutes(30))) {
                std::cout << "Туда (cache)\n\n";
                print_path_fn(route_cache.at(key).data_);
            } else {
                cpr::Response response = search_routes_fn(api, from, to, data);
                ThrowIfResponseFailed(response, "маршрута туда");

                nlohmann::json j = nlohmann::json::parse(response.text);
                std::cout << "Tuda\n\n";
                PutRouteIntoCache(route_cache, key, j, std::chrono::steady_clock::now(), kMaxCacheSize);
                if (!HasSegments(j)) {
                    std::cout << "Маршруты не найдены\n\n";
                } else {
                    print_path_fn(j);
                }
            }

            std::string key1 = MakeRouteKey(to, from, data);
            if (IsCacheFresh(route_cache, key1, now, std::chrono::minutes(30))) {
                std::cout << "Обратно (cache)\n\n";
                print_path_fn(route_cache.at(key1).data_);
            } else {
                cpr::Response response1 = search_routes_fn(api, to, from, data);
                ThrowIfResponseFailed(response1, "обратного маршрута");

                nlohmann::json j1 = nlohmann::json::parse(response1.text);
                PutRouteIntoCache(route_cache, key1, j1, std::chrono::steady_clock::now(), kMaxCacheSize);
                std::cout << "Obratno\n\n";
                if (!HasSegments(j1)) {
                    std::cout << "Маршруты не найдены\n\n";
                } else {
                    print_path_fn(j1);
                }
            }
        } catch (const std::exception& error) {
            std::cerr << "Ошибка: " << error.what() << "\n\n";
        }

        std::cout << "Введите -exit- если запросы больше не нужны : ";
        std::getline(std::cin, flag);
        if (flag == "exit") {
            break;
        }
    }
    return 0;
}

void SetRunInteractiveDependenciesForTesting(GetCodeCityFn get_code_city,
                                             GetApiFn get_api,
                                             PrintPathFn print_path,
                                             SearchRoutesFn search_routes) {
    get_code_city_fn = get_code_city;
    get_api_fn = get_api;
    print_path_fn = print_path;
    search_routes_fn = search_routes;
}

void ResetRunInteractiveDependenciesForTesting() {
    get_code_city_fn = &GetCodeCity;
    get_api_fn = &GetApi;
    print_path_fn = &Path1;
    search_routes_fn = &SearchRoutes;
}

void ClearRouteCacheForTesting() {
    route_cache.clear();
}
