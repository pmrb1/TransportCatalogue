#pragma once

#include <algorithm>
#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "domain.h"
#include "geo.h"
#include "map_renderer.h"
#include "svg.h"
#include "transport_router.h"

namespace transport_catalogue {
using namespace domain;

class TransportCatalogue {
public:
    using StopsPair = std::pair<const Stop*, const Stop*>;

public:
    struct PairStopHasher;
    void AddBus(const Bus& bus);

    void AddStop(const Stop& stop);

    [[nodiscard]] const std::vector<StopPtr>& FindRouteByName(const std::string& route_name) const;

    [[nodiscard]] BusPtr GetBus(const std::string& name) const;

    [[nodiscard]] StopPtr GetStop(const std::string& name) const;

    [[nodiscard]] double ComputeGeoRouteDistance(const std::vector<StopPtr>& stops) const;

    void SetDistanceBeetbeenStops(const std::string& stop1, const std::string& stop2, int distance);

    [[nodiscard]] int GetDistanceBetweenStops(StopPtr from, StopPtr to) const;

    void InsertBusesToStops(const std::vector<StopPtr>& stops, BusPtr bus);

    [[nodiscard]] const std::unordered_map<std::string, StopPtr>& GetAllStops();

    [[nodiscard]] const std::unordered_map<std::string, BusPtr>& GetAllBuses();

    void SetDistanceBetweenStops(StopPtr from, StopPtr to, int distance);

    [[nodiscard]] const std::unordered_set<BusPtr> GetUniqueBusesAtStop(StopPtr stop) const;

    std::unordered_map<StopsPair, int, PairStopHasher> GetDistances(){
        return distance_between_stops_;
    }

public:
    struct PairStopHasher {
        size_t operator()(const StopsPair& pair) const {
            size_t h_1 = h1(pair.first);
            size_t h_2 = h1(pair.second);

            return h_2 * 37 + h_1 * 37 * 37;
        }

    private:
        std::hash<const void*> h1;
    };

private:
    std::deque<Bus> bus_pool_;
    std::deque<Stop> stop_pool_;

    std::unordered_map<std::string, StopPtr> stops_;
    std::unordered_map<std::string, BusPtr> buses_;
    std::unordered_map<StopPtr, std::unordered_set<BusPtr>> bus_by_stop_;
    std::unordered_map<StopsPair, int, PairStopHasher> distance_between_stops_;
};

}  // namespace transport_catalogue