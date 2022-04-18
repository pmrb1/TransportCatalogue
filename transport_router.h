#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include "domain.h"
#include "graph.h"
#include "json.h"
#include "router.h"

namespace transport_catalogue {

struct StopVertexId {
    graph::VertexId in;
    graph::VertexId out;
};

struct VertexInfo {
    domain::StopPtr stop_ptr;
};

struct BusEdgeInfo {
    domain::BusPtr bus_ptr;
    size_t start_stop_idx = 0;
    size_t finish_stop_idx = 0;
};

struct WaitEdgeInfo {};

using EdgeInfo = std::variant<BusEdgeInfo, WaitEdgeInfo>;

class TransportRouter {
private:
    using BusGraph = graph::DirectedWeightedGraph<double>;
    using Router = graph::Router<double>;

    using DistancesBetweenStops = std::unordered_map<std::string, std::unordered_map<std::string, int>>;
    using StopsDict = std::unordered_map<std::string, domain::StopPtr>;
    using BusesDict = std::unordered_map<std::string, domain::BusPtr>;
    using Minutes = std::chrono::duration<double, std::chrono::minutes::period>;

public:
    struct RouteInfo {
    public:
        struct BusItem {
            std::string bus_name;
            Minutes time = Minutes(0);

            size_t start_stop_idx = 0;
            size_t finish_stop_idx = 0;

            size_t span_count = 0;
        };

        struct WaitItem {
            std::string stop_name;
            Minutes time = Minutes(0);
        };

    public:
        using Item = std::variant<BusItem, WaitItem>;

    public:
        Minutes total_time = Minutes(0);
        std::vector<Item> items;
    };

    struct RoutingSettings {
        int bus_wait_time = 0;
        double bus_velocity = 0;
    };

public:
    TransportRouter(const StopsDict &stops, const BusesDict &buses, const json::Dict &routing_settings,
                    const DistancesBetweenStops &distances);

    explicit TransportRouter(const DistancesBetweenStops &distances);

public:
    [[nodiscard]] std::optional<RouteInfo> FindRoute(domain::StopPtr stop_from, domain::StopPtr stop_to);

    [[nodiscard]] const BusGraph &GetGraph() const;

    [[nodiscard]] graph::Router<double>::RoutesInternalData GetRouterInternalData() const;

    [[nodiscard]] std::pair<int, double> GetRoutingSetting() const;

    [[nodiscard]] const std::unordered_map<domain::StopPtr, StopVertexId> &GetStopVertexIds() const;

    [[nodiscard]] const std::vector<EdgeInfo> &GetEdgesInfo() const;

    [[nodiscard]] const std::vector<VertexInfo> &GetVertiсesInfo() const;

    void SetRoutingSettings(int bus_wait_time, double bus_velocity);

    void SetStopVertexIds(const std::unordered_map<domain::StopPtr, StopVertexId> &stops_vertex_ids);

    void SetEdgesInfo(const std::vector<EdgeInfo> &edges_info);

    void SetVertiсesInfo(const std::vector<VertexInfo> &vertices_info);

    void SetGraph(const BusGraph &graph);

    void SetRouter(std::unique_ptr<Router> router);

private:
    [[nodiscard]] size_t ComputeStopsDistance(domain::StopPtr lhs, domain::StopPtr rhs) const;

    [[nodiscard]] RoutingSettings PrepareRoutingSettings(const json::Dict &settings);

    void FillGraphWithStops(const StopsDict &stops);

    void FillGraphWithBuses(const StopsDict &stops_dict, const BusesDict &buses_dict);

private:
    RoutingSettings routing_settings_;
    BusGraph graph_;

    // used unique_ptr because Router has const reference to Graph (here BusGraph)
    std::unique_ptr<Router> router_;
    std::unordered_map<domain::StopPtr, StopVertexId> stops_vertex_ids_;
    std::vector<EdgeInfo> edges_info_;
    std::vector<VertexInfo> vertices_info_;

    const DistancesBetweenStops &distances_to_other_stops_;
};
}  // namespace transport_catalogue