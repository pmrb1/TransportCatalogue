#include "transport_router.h"

namespace transport_catalogue {
TransportRouter::TransportRouter(const StopsDict& stops, const BusesDict& buses, const json::Dict& routing_settings,
                                 const DistancesBetweenStops& distances)
    : routing_settings_(PrepareRoutingSettings(routing_settings)), distances_to_other_stops_(distances) {
    const size_t vertex_count = stops.size() * 2;

    vertices_info_.resize(vertex_count);
    graph_ = BusGraph(vertex_count);

    FillGraphWithStops(stops);
    FillGraphWithBuses(stops, buses);

    router_ = std::make_unique<Router>(graph_);
}

std::optional<TransportRouter::RouteInfo> TransportRouter::FindRoute(domain::StopPtr stop_from,
                                                                     domain::StopPtr stop_to) {
    const graph::VertexId vertex_from = stops_vertex_ids_.at(stop_from).out;
    const graph::VertexId vertex_to = stops_vertex_ids_.at(stop_to).out;

    const auto route = router_->BuildRoute(vertex_from, vertex_to);

    if (!route) {
        return std::nullopt;
    }

    RouteInfo route_info;

    Minutes minutes(route->weight);
    route_info.total_time = minutes;

    route_info.items.reserve(route->edges.size());

    for (size_t edge_idx = 0; edge_idx < route->edges.size(); ++edge_idx) {
        const graph::EdgeId edge_id = route->edges[edge_idx];

        const auto& edge = graph_.GetEdge(edge_id);
        const auto& edge_info = edges_info_[edge_id];

        if (std::holds_alternative<BusEdgeInfo>(edge_info)) {
            const auto& bus_edge_info = std::get<BusEdgeInfo>(edge_info);

            route_info.items.emplace_back(RouteInfo::BusItem{
                bus_edge_info.bus_ptr->name,
                Minutes(edge.weight),
                bus_edge_info.start_stop_idx,
                bus_edge_info.finish_stop_idx,
                bus_edge_info.finish_stop_idx - bus_edge_info.start_stop_idx,
            });
        } else {
            const graph::VertexId vertex_id = edge.from;

            route_info.items.emplace_back(RouteInfo::WaitItem{
                vertices_info_[vertex_id].stop_ptr->name,
                Minutes(edge.weight),
            });
        }
    }

    return route_info;
}

size_t TransportRouter::ComputeStopsDistance(domain::StopPtr lhs, domain::StopPtr rhs) const {
    auto lhs_it = distances_to_other_stops_.find(lhs->name);

    if (lhs_it != distances_to_other_stops_.end()) {
        if (auto it = lhs_it->second.find(rhs->name); it != lhs_it->second.end()) {
            return it->second;
        }
        return ComputeStopsDistance(rhs, lhs);
    }

    return ComputeStopsDistance(rhs, lhs);
}

TransportRouter::RoutingSettings TransportRouter::PrepareRoutingSettings(const json::Dict& settings) {
    return {settings.at("bus_wait_time").AsInt(), settings.at("bus_velocity").AsDouble()};
}

void TransportRouter::FillGraphWithStops(const StopsDict& stops) {
    graph::VertexId vertex_id = 0;

    for (const auto& [stop_name, stop_ptr] : stops) {
        auto& vertex_ids = stops_vertex_ids_[stop_ptr];

        vertex_ids.in = vertex_id++;
        vertex_ids.out = vertex_id++;
        vertices_info_[vertex_ids.in] = {stop_ptr};
        vertices_info_[vertex_ids.out] = {stop_ptr};

        edges_info_.emplace_back(WaitEdgeInfo{});

        const graph::EdgeId edge_id =
            graph_.AddEdge({vertex_ids.out, vertex_ids.in, static_cast<double>(routing_settings_.bus_wait_time)});
    }
}

void TransportRouter::FillGraphWithBuses(const StopsDict& stops_dict, const BusesDict& buses_dict) {
    for (const auto& [_, bus_ptr] : buses_dict) {
        const auto& bus = *bus_ptr;
        const size_t stop_count = bus.stops.size();

        if (stop_count == 0) {
            continue;
        }
        for (size_t start_stop_idx = 0; start_stop_idx < stop_count; ++start_stop_idx) {
            const graph::VertexId start_vertex = stops_vertex_ids_[bus.stops[start_stop_idx]].in;

            int total_distance = 0;

            for (size_t finish_stop_idx = start_stop_idx + 1; finish_stop_idx < stop_count; ++finish_stop_idx) {
                total_distance += ComputeStopsDistance(stops_dict.at(bus.stops[finish_stop_idx - 1]->name),
                                                       stops_dict.at(bus.stops[finish_stop_idx]->name));

                edges_info_.emplace_back(BusEdgeInfo{
                    bus_ptr,
                    start_stop_idx,
                    finish_stop_idx,
                });

                const graph::EdgeId edge_id =
                    graph_.AddEdge({start_vertex, stops_vertex_ids_[bus.stops[finish_stop_idx]].out,
                                    total_distance * 1.0 / (routing_settings_.bus_velocity * 1000.0 / 60)});

                assert(edge_id == edges_info_.size() - 1);
            }
        }
    }
}

TransportRouter::TransportRouter(const DistancesBetweenStops& distances) : distances_to_other_stops_(distances) {}

const TransportRouter::BusGraph& TransportRouter::GetGraph() const { return graph_; }

std::pair<int, double> TransportRouter::GetRoutingSetting() const {
    return {routing_settings_.bus_wait_time, routing_settings_.bus_velocity};
}
graph::Router<double>::RoutesInternalData TransportRouter::GetRouterInternalData() const {
    return router_->GetRoutesInternalData();
}

const std::vector<EdgeInfo>& TransportRouter::GetEdgesInfo() const { return edges_info_; }

const std::vector<VertexInfo>& TransportRouter::GetVertiсesInfo() const { return vertices_info_; }

const std::unordered_map<domain::StopPtr, StopVertexId>& TransportRouter::GetStopVertexIds() const {
    return stops_vertex_ids_;
}

void TransportRouter::SetRoutingSettings(int bus_wait_time, double bus_velocity) {
    routing_settings_.bus_wait_time = bus_wait_time;
    routing_settings_.bus_velocity = bus_velocity;
}

void TransportRouter::SetGraph(const TransportRouter::BusGraph& graph) { graph_ = graph; }

void TransportRouter::SetRouter(std::unique_ptr<Router> router) { router_ = std::move(router); }

void TransportRouter::SetStopVertexIds(const std::unordered_map<domain::StopPtr, StopVertexId>& stops_vertex_ids) {
    stops_vertex_ids_ = stops_vertex_ids;
}

void TransportRouter::SetEdgesInfo(const std::vector<EdgeInfo>& edges_info) { edges_info_ = edges_info; }

void TransportRouter::SetVertiсesInfo(const std::vector<VertexInfo>& vertices_info) { vertices_info_ = vertices_info; }

}  // namespace transport_catalogue