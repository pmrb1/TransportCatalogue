#pragma once

#include <iomanip>
#include <istream>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "json.h"
#include "transport_catalogue.h"

namespace transport_catalogue {

namespace service {
using namespace transport_catalogue;
using namespace std::string_literals;

enum class ResponseStatus { kSuccess, kError, kNotFound };

class RequestHandler {
public:
    struct RouteParameters {
        std::string route_name;
        int total_stops = 0;
        int unique_stops = 0;
        double route_length = 0;
        double curvature = 1;
    };

public:
    using StopInfo = std::pair<ResponseStatus, std::pair<std::string, std::unordered_map<std::string, int>>>;
    using BusStopInfo = std::pair<ResponseStatus, std::pair<std::string, std::set<std::string>>>;
    using RouteInfo = std::pair<ResponseStatus, RouteParameters>;

public:
    RequestHandler(const TransportCatalogue& db, const renderer::MapRenderer& renderer,
                   std::unique_ptr<TransportRouter> router)
        : db_(db), renderer_(renderer), router_(std::move(router)) {}

    [[nodiscard]] const BusStopInfo GetBusStopInfo(const std::string& route_name) const;

    [[nodiscard]] const RouteInfo GetRouteInfo(const std::string& route_name) const;

    [[nodiscard]] StopPtr GetStop(const std::string& name) const;

    [[nodiscard]] std::optional<TransportRouter::RouteInfo> FindRoute(domain::StopPtr stop_from,
                                                                      domain::StopPtr stop_to) const;
    svg::Document RenderMap() const;

private:
    const TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
    // use unique_ptr because TransportRouter has field router_ which has const reference to BusGraph
    // and in case when TransportRouter has been moved anywhere - reference  to BusGraph become invalid
    std::unique_ptr<TransportRouter> router_;
};

}  // namespace service
}  // namespace transport_catalogue
