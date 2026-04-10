#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <unordered_map>

#include <cpr/response.h>
#include <nlohmann/json.hpp>

inline constexpr std::size_t kMaxCacheSize = 100;

struct CacheEntry {
    nlohmann::json data_;
    std::chrono::steady_clock::time_point time;
};

std::string MakeRouteKey(const std::string& from, const std::string& to, const std::string& date);
bool IsCacheFresh(const std::unordered_map<std::string, CacheEntry>& route_cache,
                  const std::string& key,
                  const std::chrono::steady_clock::time_point& now,
                  const std::chrono::minutes& ttl);
void PutRouteIntoCache(std::unordered_map<std::string, CacheEntry>& route_cache,
                       const std::string& key,
                       const nlohmann::json& data,
                       const std::chrono::steady_clock::time_point& now,
                       std::size_t max_cache_size);
bool HasSegments(const nlohmann::json& data);
void ThrowIfResponseFailed(const cpr::Response& response, const std::string& route_name);
int RunInteractiveMode();

using GetCodeCityFn = std::string (*)(const std::string& city);
using GetApiFn = std::string (*)();
using PrintPathFn = void (*)(const nlohmann::json& data);
using SearchRoutesFn = cpr::Response (*)(const std::string& api,
                                         const std::string& from,
                                         const std::string& to,
                                         const std::string& date);

void SetRunInteractiveDependenciesForTesting(GetCodeCityFn get_code_city,
                                             GetApiFn get_api,
                                             PrintPathFn print_path,
                                             SearchRoutesFn search_routes);
void ResetRunInteractiveDependenciesForTesting();
void ClearRouteCacheForTesting();
