#include <string>

#include <cpr/api.h>
#include <cpr/cprtypes.h>
#include <cpr/parameters.h>
#include <cpr/response.h>

#include "GetAPI.h"
#include "getstationlist.h"
#include "response_utils.h"

nlohmann::json GetStationList() {
    std::string api = GetApi();
    cpr::Response response = cpr::Get(
    cpr::Url("https://api.rasp.yandex-net.ru/v3.0/stations_list/"),
    cpr::Parameters{
        {"apikey", api}
    }
    );
    return ParseJsonResponse(response, "Ошибка списка станций");
}
