#include <initializer_list>
#include <chrono>
#include <deque>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <unordered_map>

#include <cpr/error.h>
#include <cpr/response.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "../GetAPI.h"
#include "../app.h"
#include "../city_code_utils.h"
#include "../Path1.h"
#include "../response_utils.h"

namespace {

class ScopedStdIoRedirect {
public:
    explicit ScopedStdIoRedirect(const std::string& input)
        : input_stream_(input),
          old_cin_(std::cin.rdbuf(input_stream_.rdbuf())),
          old_cout_(std::cout.rdbuf(output_stream_.rdbuf())),
          old_cerr_(std::cerr.rdbuf(error_stream_.rdbuf())) {
    }

    ~ScopedStdIoRedirect() {
        std::cin.rdbuf(old_cin_);
        std::cout.rdbuf(old_cout_);
        std::cerr.rdbuf(old_cerr_);
    }

    [[nodiscard]] std::string StdOut() const {
        return output_stream_.str();
    }

    [[nodiscard]] std::string StdErr() const {
        return error_stream_.str();
    }

private:
    std::istringstream input_stream_;
    std::ostringstream output_stream_;
    std::ostringstream error_stream_;
    std::streambuf* old_cin_;
    std::streambuf* old_cout_;
    std::streambuf* old_cerr_;
};

std::deque<cpr::Response> mock_search_responses;
int mock_path_print_calls = 0;
int mock_search_calls = 0;

std::string MockGetCodeCity(const std::string& city) {
    if (city == "Omsk") {
        return "c66";
    }
    if (city == "Tomsk") {
        return "c67";
    }
    return "unknown";
}

std::string MockGetApi() {
    return "test-api-key";
}

void MockPrintPath(const nlohmann::json&) {
    ++mock_path_print_calls;
}

cpr::Response MockSearchRoutes(const std::string&,
                               const std::string&,
                               const std::string&,
                               const std::string&) {
    ++mock_search_calls;
    if (mock_search_responses.empty()) {
        throw std::runtime_error("MockSearchRoutes called without prepared response");
    }
    cpr::Response response = mock_search_responses.front();
    mock_search_responses.pop_front();
    return response;
}

void SetupInteractiveMocks() {
    mock_search_responses.clear();
    mock_path_print_calls = 0;
    mock_search_calls = 0;
    ClearRouteCacheForTesting();
    SetRunInteractiveDependenciesForTesting(&MockGetCodeCity,
                                            &MockGetApi,
                                            &MockPrintPath,
                                            &MockSearchRoutes);
}

void ResetInteractiveMocks() {
    ResetRunInteractiveDependenciesForTesting();
    ClearRouteCacheForTesting();
    mock_search_responses.clear();
    mock_path_print_calls = 0;
    mock_search_calls = 0;
}

[[nodiscard]] cpr::Response MakeOkSearchResponse(const nlohmann::json& payload) {
    cpr::Response response;
    response.status_code = 200;
    response.error.code = cpr::ErrorCode::OK;
    response.text = payload.dump();
    return response;
}

std::string CapturePathOutput(const nlohmann::json& json) {
    std::ostringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    Path1(json);
    std::cout.rdbuf(old);
    return buffer.str();
}

nlohmann::json MakeDirectSegment() {
    return {
        {"has_transfers", false},
        {"departure", "2026-03-22T00:35:00+03:00"},
        {"arrival", "2026-03-22T07:20:00+06:00"},
        {"duration", 13500.0},
        {"thread",
         {
             {"title", "Saint Petersburg -> Omsk"},
             {"transport_type", "plane"},
             {"carrier", {{"title", "Nordwind"}}},
         }},
    };
}

nlohmann::json MakeOneTransferSegment() {
    return {
        {"has_transfers", true},
        {"departure", "2026-03-22T22:45:00+03:00"},
        {"arrival", "2026-03-23T12:35:00+06:00"},
        {"duration", 49800.0},
        {"departure_from", {{"title", "Pulkovo"}}},
        {"arrival_to", {{"title", "Omsk"}}},
        {"transfers", {{{"title", "Novosibirsk"}}}},
        {"transport_types", {"plane", "plane"}},
    };
}

nlohmann::json MakeTwoTransferSegment() {
    return {
        {"has_transfers", true},
        {"departure", "2026-03-22T08:00:00+03:00"},
        {"arrival", "2026-03-23T20:00:00+06:00"},
        {"duration", 100000.0},
        {"departure_from", {{"title", "Pulkovo"}}},
        {"arrival_to", {{"title", "Omsk"}}},
        {"transfers", {{{"title", "City1"}}, {{"title", "City2"}}}},
        {"transport_types", {"plane", "plane", "train"}},
    };
}

nlohmann::json MakeSegments(std::initializer_list<nlohmann::json> segments) {
    nlohmann::json array = nlohmann::json::array();
    for (const auto& segment : segments) {
        array.push_back(segment);
    }
    return {{"segments", array}};
}

TEST(Path1Tests, PrintsDirectRouteType) {
    const std::string output = CapturePathOutput(MakeSegments({MakeDirectSegment()}));
    EXPECT_NE(output.find("Tip: bez peresadok"), std::string::npos);
}

TEST(Path1Tests, PrintsDirectRouteTitle) {
    const std::string output = CapturePathOutput(MakeSegments({MakeDirectSegment()}));
    EXPECT_NE(output.find("Saint Petersburg -> Omsk"), std::string::npos);
}

TEST(Path1Tests, PrintsDirectDeparture) {
    const std::string output = CapturePathOutput(MakeSegments({MakeDirectSegment()}));
    EXPECT_NE(output.find("2026-03-22T00:35:00+03:00"), std::string::npos);
}

TEST(Path1Tests, PrintsDirectArrival) {
    const std::string output = CapturePathOutput(MakeSegments({MakeDirectSegment()}));
    EXPECT_NE(output.find("2026-03-22T07:20:00+06:00"), std::string::npos);
}

TEST(Path1Tests, PrintsDirectDuration) {
    const std::string output = CapturePathOutput(MakeSegments({MakeDirectSegment()}));
    EXPECT_NE(output.find("13500"), std::string::npos);
}

TEST(Path1Tests, PrintsDirectTransport) {
    const std::string output = CapturePathOutput(MakeSegments({MakeDirectSegment()}));
    EXPECT_NE(output.find("Transport: plane"), std::string::npos);
}

TEST(Path1Tests, PrintsDirectCarrier) {
    const std::string output = CapturePathOutput(MakeSegments({MakeDirectSegment()}));
    EXPECT_NE(output.find("Perevozchik: Nordwind"), std::string::npos);
}

TEST(Path1Tests, PrintsTransferRouteType) {
    const std::string output = CapturePathOutput(MakeSegments({MakeOneTransferSegment()}));
    EXPECT_NE(output.find("Tip: 1 peresadka"), std::string::npos);
}

TEST(Path1Tests, PrintsTransferDirection) {
    const std::string output = CapturePathOutput(MakeSegments({MakeOneTransferSegment()}));
    EXPECT_NE(output.find("Pulkovo -> Omsk"), std::string::npos);
}

TEST(Path1Tests, PrintsTransferDeparture) {
    const std::string output = CapturePathOutput(MakeSegments({MakeOneTransferSegment()}));
    EXPECT_NE(output.find("2026-03-22T22:45:00+03:00"), std::string::npos);
}

TEST(Path1Tests, PrintsTransferArrival) {
    const std::string output = CapturePathOutput(MakeSegments({MakeOneTransferSegment()}));
    EXPECT_NE(output.find("2026-03-23T12:35:00+06:00"), std::string::npos);
}

TEST(Path1Tests, PrintsTransferDuration) {
    const std::string output = CapturePathOutput(MakeSegments({MakeOneTransferSegment()}));
    EXPECT_NE(output.find("49800"), std::string::npos);
}

TEST(Path1Tests, PrintsTransferCity) {
    const std::string output = CapturePathOutput(MakeSegments({MakeOneTransferSegment()}));
    EXPECT_NE(output.find("Peresadka v: Novosibirsk"), std::string::npos);
}

TEST(Path1Tests, PrintsTransferTransportChain) {
    const std::string output = CapturePathOutput(MakeSegments({MakeOneTransferSegment()}));
    EXPECT_NE(output.find("Transport: plane -> plane"), std::string::npos);
}

TEST(Path1Tests, SkipsRoutesWithMoreThanOneTransfer) {
    const std::string output = CapturePathOutput(MakeSegments({MakeTwoTransferSegment()}));
    EXPECT_TRUE(output.empty());
}

TEST(Path1Tests, HandlesEmptySegments) {
    const std::string output = CapturePathOutput(MakeSegments({}));
    EXPECT_TRUE(output.empty());
}

TEST(Path1Tests, HandlesMissingDirectFieldsGracefully) {
    const nlohmann::json payload = {
        {"segments",
         nlohmann::json::array({
             {
                 {"has_transfers", false},
                 {"thread", {{"carrier", {{"title", "Unknown"}}}}},
             },
         })},
    };

    const std::string output = CapturePathOutput(payload);
    EXPECT_NE(output.find("unknown"), std::string::npos);
}

TEST(Path1Tests, KeepsCarrierWhenOtherFieldsMissing) {
    const nlohmann::json payload = {
        {"segments",
         nlohmann::json::array({
             {
                 {"has_transfers", false},
                 {"thread", {{"carrier", {{"title", "Unknown"}}}}},
             },
         })},
    };

    const std::string output = CapturePathOutput(payload);
    EXPECT_NE(output.find("Perevozchik: Unknown"), std::string::npos);
}

TEST(Path1Tests, NumbersRoutesSequentially) {
    const std::string output = CapturePathOutput(
        MakeSegments({MakeDirectSegment(), MakeOneTransferSegment()})
    );
    EXPECT_NE(output.find("Marshrut 1"), std::string::npos);
    EXPECT_NE(output.find("Marshrut 2"), std::string::npos);
}

TEST(Path1Tests, DoesNotPrintSkippedRouteNumber) {
    const std::string output = CapturePathOutput(
        MakeSegments({MakeDirectSegment(), MakeTwoTransferSegment(), MakeOneTransferSegment()})
    );
    EXPECT_EQ(output.find("Marshrut 3"), std::string::npos);
}

TEST(Path1Tests, PrintsTwoValidRoutesWhenMiddleRouteFiltered) {
    const std::string output = CapturePathOutput(
        MakeSegments({MakeDirectSegment(), MakeTwoTransferSegment(), MakeOneTransferSegment()})
    );
    EXPECT_NE(output.find("Marshrut 1"), std::string::npos);
    EXPECT_NE(output.find("Marshrut 2"), std::string::npos);
}

TEST(Path1Tests, MissingTransportTypesFallsBackToUnknown) {
    nlohmann::json transfer = MakeOneTransferSegment();
    transfer.erase("transport_types");
    const std::string output = CapturePathOutput(MakeSegments({transfer}));
    EXPECT_NE(output.find("Transport: unknown"), std::string::npos);
}

TEST(Path1Tests, MissingTransfersArraySkipsTransferRoute) {
    nlohmann::json transfer = MakeOneTransferSegment();
    transfer.erase("transfers");
    const std::string output = CapturePathOutput(MakeSegments({transfer}));
    EXPECT_TRUE(output.empty());
}

TEST(Path1Tests, EmptyTransfersArraySkipsTransferRoute) {
    nlohmann::json transfer = MakeOneTransferSegment();
    transfer["transfers"] = nlohmann::json::array();
    const std::string output = CapturePathOutput(MakeSegments({transfer}));
    EXPECT_TRUE(output.empty());
}

TEST(Path1Tests, MissingHasTransfersTreatsRouteAsDirect) {
    nlohmann::json direct = MakeDirectSegment();
    direct.erase("has_transfers");
    const std::string output = CapturePathOutput(MakeSegments({direct}));
    EXPECT_NE(output.find("Tip: bez peresadok"), std::string::npos);
}

TEST(Path1Tests, MissingThreadFieldsUseUnknownFallback) {
    nlohmann::json direct = MakeDirectSegment();
    direct["thread"] = nlohmann::json::object();
    const std::string output = CapturePathOutput(MakeSegments({direct}));
    EXPECT_NE(output.find("Napravlenie: unknown"), std::string::npos);
    EXPECT_NE(output.find("Transport: unknown"), std::string::npos);
}

TEST(Path1Tests, MissingNumericDurationFallsBackToZero) {
    nlohmann::json direct = MakeDirectSegment();
    direct.erase("duration");
    const std::string output = CapturePathOutput(MakeSegments({direct}));
    EXPECT_NE(output.find("Dlitelnost: 0"), std::string::npos);
}

TEST(Path1Tests, MixedPayloadPrintsOnlyAllowedRoutes) {
    const std::string output = CapturePathOutput(
        MakeSegments({MakeDirectSegment(), MakeOneTransferSegment(), MakeTwoTransferSegment()})
    );
    EXPECT_NE(output.find("Tip: bez peresadok"), std::string::npos);
    EXPECT_NE(output.find("Tip: 1 peresadka"), std::string::npos);
    EXPECT_EQ(output.find("City1"), std::string::npos);
}

TEST(Path1Tests, PrintsBlankLineBetweenRoutes) {
    const std::string output = CapturePathOutput(
        MakeSegments({MakeDirectSegment(), MakeOneTransferSegment()})
    );
    EXPECT_NE(output.find("\n\nMarshrut 2"), std::string::npos);
}

TEST(AppTests, MakeRouteKeyBuildsExpectedKey) {
    EXPECT_EQ(MakeRouteKey("c2", "c66", "2026-03-22"), "c2|c66|2026-03-22");
}

TEST(AppTests, IsCacheFreshReturnsFalseForMissingKey) {
    const std::unordered_map<std::string, CacheEntry> route_cache;
    const auto now = std::chrono::steady_clock::now();
    EXPECT_FALSE(IsCacheFresh(route_cache, "missing", now, std::chrono::minutes(30)));
}

TEST(AppTests, IsCacheFreshReturnsTrueForFreshEntry) {
    const auto now = std::chrono::steady_clock::now();
    std::unordered_map<std::string, CacheEntry> route_cache{
        {"key", {nlohmann::json{{"segments", nlohmann::json::array()}}, now}}
    };
    EXPECT_TRUE(IsCacheFresh(route_cache, "key", now, std::chrono::minutes(30)));
}

TEST(AppTests, IsCacheFreshReturnsFalseForExpiredEntry) {
    const auto now = std::chrono::steady_clock::now();
    std::unordered_map<std::string, CacheEntry> route_cache{
        {"key", {nlohmann::json{{"segments", nlohmann::json::array()}}, now - std::chrono::minutes(31)}}
    };
    EXPECT_FALSE(IsCacheFresh(route_cache, "key", now, std::chrono::minutes(30)));
}

TEST(AppTests, PutRouteIntoCacheInsertsEntry) {
    std::unordered_map<std::string, CacheEntry> route_cache;
    const auto now = std::chrono::steady_clock::now();
    PutRouteIntoCache(route_cache, "key", nlohmann::json{{"value", 1}}, now, 2);
    ASSERT_TRUE(route_cache.contains("key"));
    EXPECT_EQ(route_cache["key"].data_["value"], 1);
}

TEST(AppTests, PutRouteIntoCacheEvictsOldestEntry) {
    const auto now = std::chrono::steady_clock::now();
    std::unordered_map<std::string, CacheEntry> route_cache{
        {"old", {nlohmann::json{{"value", 1}}, now - std::chrono::minutes(2)}},
        {"new", {nlohmann::json{{"value", 2}}, now - std::chrono::minutes(1)}},
    };
    PutRouteIntoCache(route_cache, "latest", nlohmann::json{{"value", 3}}, now, 2);
    EXPECT_FALSE(route_cache.contains("old"));
    EXPECT_TRUE(route_cache.contains("new"));
    EXPECT_TRUE(route_cache.contains("latest"));
}

TEST(AppTests, HasSegmentsReturnsTrueForNonEmptySegments) {
    EXPECT_TRUE(HasSegments(MakeSegments({MakeDirectSegment()})));
}

TEST(AppTests, HasSegmentsReturnsFalseForEmptySegments) {
    EXPECT_FALSE(HasSegments(MakeSegments({})));
}

TEST(AppTests, HasSegmentsReturnsFalseWhenSegmentsMissing) {
    EXPECT_FALSE(HasSegments(nlohmann::json::object()));
}

TEST(AppTests, ThrowIfResponseFailedAllowsSuccessfulResponse) {
    cpr::Response response;
    response.status_code = 200;
    response.error.code = cpr::ErrorCode::OK;
    EXPECT_NO_THROW(ThrowIfResponseFailed(response, "маршрута туда"));
}

TEST(AppTests, ThrowIfResponseFailedThrowsForNetworkError) {
    cpr::Response response;
    response.status_code = 200;
    response.error.code = cpr::ErrorCode::COULDNT_CONNECT;
    response.error.message = "network";
    EXPECT_THROW(ThrowIfResponseFailed(response, "маршрута туда"), std::invalid_argument);
}

TEST(AppTests, ThrowIfResponseFailedThrowsForHttpError) {
    cpr::Response response;
    response.status_code = 500;
    response.error.code = cpr::ErrorCode::OK;
    response.text = "bad";
    EXPECT_THROW(ThrowIfResponseFailed(response, "маршрута туда"), std::invalid_argument);
}

TEST(AppTests, RunInteractiveModeHandlesEmptyInputAndExits) {
    SetupInteractiveMocks();
    const ScopedStdIoRedirect io("\n\n\nexit\n");
    EXPECT_EQ(RunInteractiveMode(), 0);
    EXPECT_NE(io.StdErr().find("Поля отправления, назначения и даты не должны быть пустыми"), std::string::npos);
    EXPECT_EQ(mock_search_calls, 0);
    EXPECT_EQ(mock_path_print_calls, 0);
    ResetInteractiveMocks();
}

TEST(AppTests, RunInteractiveModePrintsNotFoundForEmptySegments) {
    SetupInteractiveMocks();
    mock_search_responses.push_back(MakeOkSearchResponse(MakeSegments({})));
    mock_search_responses.push_back(MakeOkSearchResponse(MakeSegments({})));

    const ScopedStdIoRedirect io("Omsk\nTomsk\n2026-03-22\nexit\n");
    EXPECT_EQ(RunInteractiveMode(), 0);
    EXPECT_NE(io.StdOut().find("Tuda"), std::string::npos);
    EXPECT_NE(io.StdOut().find("Obratno"), std::string::npos);
    EXPECT_NE(io.StdOut().find("Маршруты не найдены"), std::string::npos);
    EXPECT_EQ(mock_search_calls, 2);
    EXPECT_EQ(mock_path_print_calls, 0);
    ResetInteractiveMocks();
}

TEST(AppTests, RunInteractiveModePrintsRoutesWhenSegmentsExist) {
    SetupInteractiveMocks();
    const nlohmann::json payload = MakeSegments({MakeDirectSegment()});
    mock_search_responses.push_back(MakeOkSearchResponse(payload));
    mock_search_responses.push_back(MakeOkSearchResponse(payload));

    const ScopedStdIoRedirect io("Omsk\nTomsk\n2026-03-22\nexit\n");
    EXPECT_EQ(RunInteractiveMode(), 0);
    EXPECT_EQ(mock_search_calls, 2);
    EXPECT_EQ(mock_path_print_calls, 2);
    ResetInteractiveMocks();
}

TEST(AppTests, RunInteractiveModeUsesCacheOnRepeatedRequest) {
    SetupInteractiveMocks();
    const nlohmann::json payload = MakeSegments({MakeDirectSegment()});
    mock_search_responses.push_back(MakeOkSearchResponse(payload));
    mock_search_responses.push_back(MakeOkSearchResponse(payload));

    const ScopedStdIoRedirect io("Omsk\nTomsk\n2026-03-22\n\nOmsk\nTomsk\n2026-03-22\nexit\n");
    EXPECT_EQ(RunInteractiveMode(), 0);
    EXPECT_EQ(mock_search_calls, 2);
    EXPECT_EQ(mock_path_print_calls, 4);
    EXPECT_NE(io.StdOut().find("Туда (cache)"), std::string::npos);
    EXPECT_NE(io.StdOut().find("Обратно (cache)"), std::string::npos);
    ResetInteractiveMocks();
}

TEST(AppTests, RunInteractiveModeReportsNetworkErrors) {
    SetupInteractiveMocks();
    cpr::Response failed;
    failed.status_code = 200;
    failed.error.code = cpr::ErrorCode::COULDNT_CONNECT;
    failed.error.message = "network-down";
    mock_search_responses.push_back(failed);

    const ScopedStdIoRedirect io("Omsk\nTomsk\n2026-03-22\nexit\n");
    EXPECT_EQ(RunInteractiveMode(), 0);
    EXPECT_NE(io.StdErr().find("Ошибка сети: network-down"), std::string::npos);
    EXPECT_EQ(mock_search_calls, 1);
    ResetInteractiveMocks();
}

TEST(ResponseUtilsTests, ParseJsonResponseReturnsJsonForSuccess) {
    cpr::Response response;
    response.status_code = 200;
    response.error.code = cpr::ErrorCode::OK;
    response.text = R"({"value": 1})";
    EXPECT_EQ(ParseJsonResponse(response, "ok")["value"], 1);
}

TEST(ResponseUtilsTests, ParseJsonResponseThrowsForFailedResponse) {
    cpr::Response response;
    response.status_code = 404;
    response.error.code = cpr::ErrorCode::OK;
    EXPECT_THROW(ParseJsonResponse(response, "fail"), std::invalid_argument);
}

TEST(CityCodeUtilsTests, FindCityCodeReturnsCodeForExistingCity) {
    const nlohmann::json station_list = {
        {"countries",
         nlohmann::json::array({
             {
                 {"regions",
                  nlohmann::json::array({
                      {
                          {"settlements",
                           nlohmann::json::array({
                               {{"title", "Омск"}, {"codes", {{"yandex_code", "c66"}}}},
                           })},
                      },
                  })},
             },
         })},
    };
    EXPECT_EQ(FindCityCode(station_list, "Омск"), "c66");
}

TEST(CityCodeUtilsTests, FindCityCodeThrowsForUnknownCity) {
    const nlohmann::json station_list = {{"countries", nlohmann::json::array()}};
    EXPECT_THROW(FindCityCode(station_list, "Неизвестно"), std::invalid_argument);
}

TEST(CityCodeUtilsTests, GetOrFindCityCodeUsesCacheWhenPresent) {
    std::unordered_map<std::string, std::string> city_codes{{"Омск", "cached"}};
    const nlohmann::json station_list = {{"countries", nlohmann::json::array()}};
    EXPECT_EQ(GetOrFindCityCode(city_codes, station_list, "Омск"), "cached");
}

TEST(CityCodeUtilsTests, GetOrFindCityCodeStoresDiscoveredCode) {
    std::unordered_map<std::string, std::string> city_codes;
    const nlohmann::json station_list = {
        {"countries",
         nlohmann::json::array({
             {
                 {"regions",
                  nlohmann::json::array({
                      {
                          {"settlements",
                           nlohmann::json::array({
                               {{"title", "Томск"}, {"codes", {{"yandex_code", "c67"}}}},
                           })},
                      },
                  })},
             },
         })},
    };
    EXPECT_EQ(GetOrFindCityCode(city_codes, station_list, "Томск"), "c67");
    EXPECT_EQ(city_codes["Томск"], "c67");
}

TEST(GetApiTests, GetApiReturnsNonEmptyKey) {
    EXPECT_FALSE(GetApi().empty());
}

}  // namespace
