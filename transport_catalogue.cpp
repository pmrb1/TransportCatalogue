#include "transport_catalogue.h"

#include <string>

#include "json_reader.h"
#include "request_handler.h"

namespace transport_catalogue {

void TransportCatalogue::AddBus(const Bus& bus) {
    bus_pool_.emplace_back(bus);
    buses_[bus_pool_.back().name] = &bus_pool_.back();
}

void TransportCatalogue::AddStop(const Stop& stop) {
    stop_pool_.emplace_back(stop);
    stops_[stop_pool_.back().name] = &stop_pool_.back();
}

const std::vector<StopPtr>& TransportCatalogue::FindRouteByName(const std::string& route_name) const {
  
    if (buses_.count(route_name) > 0 && !buses_.at(route_name)->stops.empty()) {
      return buses_.at(route_name)->stops;
    }
    static std::vector<StopPtr> empty_res;
    return empty_res;
}

BusPtr TransportCatalogue::GetBus(const std::string& name) const {
    return buses_.count(name) > 0 ? buses_.at(name) : nullptr;
}

StopPtr TransportCatalogue::GetStop(const std::string& name) const {
    return stops_.count(name) > 0 ? stops_.at(name) : nullptr;
}

double TransportCatalogue::ComputeGeoRouteDistance(const std::vector<StopPtr>& stops) const {
    if (stops.size() == 1) {
        return 0;
    }
    double total_distance = 0;

    for (size_t index = 1; index < stops.size(); ++index) {
        total_distance +=
            geo::ComputeDistance(stops[index - 1]->position,stops[index]->position);
    }

    return total_distance;
}

void TransportCatalogue::SetDistanceBeetbeenStops(const std::string& stop1, const std::string& stop2, int distance) {
    distance_between_stops_[{GetStop(stop1), GetStop(stop2)}] = distance;
}

int TransportCatalogue::GetDistanceBetweenStops(StopPtr from, StopPtr to) const {
    // if we have no result at {from, to} stops, try to find at {to, from} key
    return distance_between_stops_.count({from, to}) > 0   ? distance_between_stops_.at({from, to})
           : distance_between_stops_.count({to, from}) > 0 ? distance_between_stops_.at({to, from})
                                                           : 0;
}

void TransportCatalogue::InsertBusesToStops(const std::vector<StopPtr>& stops, BusPtr bus) {
    for (const auto& stop : stops) {
        bus_by_stop_[stop].emplace(bus);
    }
}

const std::unordered_map<std::string, StopPtr>& TransportCatalogue::GetAllStops() { return stops_; }

const std::unordered_map<std::string, BusPtr>& TransportCatalogue::GetAllBuses() { return buses_; }

void TransportCatalogue::SetDistanceBetweenStops(StopPtr from, StopPtr to, int distance) {
    distance_between_stops_[{from, to}] = distance;
}

const std::unordered_set<BusPtr> TransportCatalogue::GetUniqueBusesAtStop(StopPtr stop) const {
    return bus_by_stop_.count(stop) > 0 ? bus_by_stop_.at(stop) : std::unordered_set<BusPtr>{};
}

}  // namespace transport_catalogue