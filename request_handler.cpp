#include "request_handler.h"

#include "json.h"
#include "json_builder.h"
#include "json_reader.h"
#include "map_renderer.h"

using namespace transport_catalogue::service;

const RequestHandler::BusStopInfo RequestHandler::GetBusStopInfo(const std::string& bus_stop_name) const {
    const Stop* stop = db_.GetStop(bus_stop_name);
    std::set<std::string> result;

    if (stop != nullptr) {
        const auto& buses = db_.GetUniqueBusesAtStop(stop);

        for (const auto& bus : buses) {
            result.insert(bus->name);
        }

        return RequestHandler::BusStopInfo{ResponseStatus::kSuccess, {bus_stop_name, result}};
    }

    return RequestHandler::BusStopInfo{ResponseStatus::kNotFound, {bus_stop_name, result}};
}

const RequestHandler::RouteInfo RequestHandler::GetRouteInfo(const std::string& route_name) const {
    RouteParameters result;

    result.route_name = route_name;

    const auto& stops_ptrs = db_.FindRouteByName(route_name);

    if (stops_ptrs.empty()) {
        return {ResponseStatus::kNotFound, result};
    }
    std::set<std::string> unique_stops;

    for (const auto& stop : stops_ptrs) {
        unique_stops.insert(stop->name);
    }
    int real_length = 0;

    for (size_t i = 0; i < stops_ptrs.size() - 1; ++i) {
        real_length += db_.GetDistanceBetweenStops(stops_ptrs[i], stops_ptrs[i + 1]);
    }

    double direct_length = db_.ComputeGeoRouteDistance(stops_ptrs);
    double curvature = real_length / direct_length;

    result.route_length = real_length;
    result.curvature = curvature;
    result.total_stops = stops_ptrs.size();
    result.unique_stops = unique_stops.size();

    return {ResponseStatus::kSuccess, result};
}

StopPtr transport_catalogue::service::RequestHandler::GetStop(const std::string& name) const {
    return db_.GetStop(name);
}

std::optional<TransportRouter::RouteInfo> transport_catalogue::service::RequestHandler::FindRoute(
    domain::StopPtr stop_from, domain::StopPtr stop_to) const {
    return router_->FindRoute(stop_from, stop_to);
}

svg::Document transport_catalogue::service::RequestHandler::RenderMap() const { return renderer_.Render(); }
