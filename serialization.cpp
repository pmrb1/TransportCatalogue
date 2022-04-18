#include "serialization.h"

#include <fstream>

#include "request_handler.h"

using namespace transport_catalogue;
//----------------------------------Serialization----------------------------------------//

void SerializeStop(transport_catalogue_proto::TransportCatalogue& db_proto, const StopPtr stop) {
    transport_catalogue_proto::Stop& stop_proto = *db_proto.add_stops();
    stop_proto.set_name(stop->name);
    stop_proto.set_lng(stop->position.lng);
    stop_proto.set_lat(stop->position.lat);
}

void SerializeBus(transport_catalogue_proto::TransportCatalogue& db_proto, const BusPtr bus) {
    transport_catalogue_proto::Bus& bus_proto = *db_proto.add_buses();
    bus_proto.set_name(bus->name);

    for (const auto& stop : bus->stops) {
        bus_proto.add_stops(stop->name);
    }

    for (const auto& endpoint : bus->endpoints) {
        bus_proto.add_endpoints(endpoint->name);
    }
}

void SerializeDistanceBetweenStops(transport_catalogue_proto::TransportCatalogue& db_proto,
                                   const std::pair<const Stop*, const Stop*> stops_pair, int distance) {
    transport_catalogue_proto::DistanceBetweenStops& distance_between_stops = *db_proto.add_distances_between_stops();

    distance_between_stops.set_stop_from(stops_pair.first->name);
    distance_between_stops.set_stop_to(stops_pair.second->name);
    distance_between_stops.set_distance(distance);
}

void SerializeDataForMapRenderer(transport_catalogue_proto::TransportCatalogue& db_proto,
                                 const StopsDict& stops_dict_for_map, const BusesDict& buses_dict_for_map) {
    for (const auto& [name, _] : buses_dict_for_map) {
        db_proto.add_buses_for_map(name);
    }

    for (const auto& [name, _] : stops_dict_for_map) {
        db_proto.add_stops_for_map(name);
    }
}

void SerializePoint(svg::Point point, transport_catalogue_proto::Point& point_proto) {
    point_proto.set_x(point.x);
    point_proto.set_y(point.y);
}

void SerializeColor(const svg::Color& color, transport_catalogue_proto::Color& color_proto) {
    if (std::holds_alternative<std::monostate>(color)) {
        color_proto.set_is_none(true);

    } else if (std::holds_alternative<std::string>(color)) {
        const auto& name = std::get<std::string>(color);
        color_proto.set_name(name);

    } else {
        if (std::holds_alternative<svg::Rgba>(color)) {
            auto& rgba_proto = *color_proto.mutable_rgba();
            const auto& rgba = std::get<svg::Rgba>(color);

            rgba_proto.set_red(rgba.red);
            rgba_proto.set_green(rgba.green);
            rgba_proto.set_blue(rgba.blue);
            rgba_proto.set_has_opacity(true);
            rgba_proto.set_opacity(rgba.opacity);
        } else {
            auto& rgba_proto = *color_proto.mutable_rgba();
            const auto& rgb = std::get<svg::Rgb>(color);

            rgba_proto.set_red(rgb.red);
            rgba_proto.set_green(rgb.green);
            rgba_proto.set_blue(rgb.blue);
        }
    }
}

void SerializeRenderSettings(transport_catalogue_proto::RenderSettings& render_settings_proto,
                             const json::Dict& settings_json) {
    render_settings_proto.set_width(settings_json.at("width").AsDouble());
    render_settings_proto.set_height(settings_json.at("height").AsDouble());
    render_settings_proto.set_padding(settings_json.at("padding").AsDouble());

    render_settings_proto.set_line_width(settings_json.at("line_width").AsDouble());
    render_settings_proto.set_underlayer_width(settings_json.at("underlayer_width").AsDouble());
    render_settings_proto.set_stop_radius(settings_json.at("stop_radius").AsDouble());

    render_settings_proto.set_stop_label_font_size(settings_json.at("stop_label_font_size").AsInt());
    render_settings_proto.set_bus_label_font_size(settings_json.at("bus_label_font_size").AsInt());

    const auto& palette = renderer::ParseColors(settings_json.at("color_palette"));
    const auto& underlayer_color = renderer::ParseColor(settings_json.at("underlayer_color"));
    const auto& stop_label_offset = renderer::ParsePoint(settings_json.at("stop_label_offset"));
    const auto& bus_label_offset = renderer::ParsePoint(settings_json.at("bus_label_offset"));

    for (const svg::Color& color : palette) {
        SerializeColor(color, *render_settings_proto.add_palette());
    }
    SerializeColor(underlayer_color, *render_settings_proto.mutable_underlayer_color());
    SerializePoint(stop_label_offset, *render_settings_proto.mutable_stop_label_offset());
    SerializePoint(bus_label_offset, *render_settings_proto.mutable_bus_label_offset());
}

void SerializeRouter(
    transport_catalogue_proto::Router& proto,
    const std::vector<std::vector<std::optional<graph::RouteInternalData<double>>>>& routes_internal_data) {
    for (const auto& source_data : routes_internal_data) {
        auto& source_data_proto = *proto.add_sources_data();

        for (const auto& route_data : source_data) {
            auto& route_data_proto = *source_data_proto.add_targets_data();

            if (route_data) {
                route_data_proto.set_exists(true);
                route_data_proto.set_weight(route_data->weight);

                if (route_data->prev_edge) {
                    route_data_proto.set_has_prev_edge(true);
                    route_data_proto.set_prev_edge(*route_data->prev_edge);
                }
            }
        }
    }
}

void SerializeGraph(transport_catalogue_proto::DirectedWeightedGraph& direct_weighted_graph_proto,
                    graph::DirectedWeightedGraph<double> graph) {
    for (const auto& edge : graph.GetEdges()) {
        auto& edge_proto = *direct_weighted_graph_proto.add_edges();

        edge_proto.set_from(edge.from);
        edge_proto.set_to(edge.to);
        edge_proto.set_weight(edge.weight);
    }

    for (const auto& incidence_list : graph.GetIncidenceList()) {
        auto& incidence_list_proto = *direct_weighted_graph_proto.add_incidence_lists();

        for (const auto edge_id : incidence_list) {
            incidence_list_proto.add_edge_ids(edge_id);
        }
    }
}

void SerializeRoutingSettings(transport_catalogue_proto::RoutingSettings& proto_routing_settings,
                              std::pair<int, double> routing_settings) {
    proto_routing_settings.set_bus_wait_time(routing_settings.first);
    proto_routing_settings.set_bus_velocity(routing_settings.second);
}

void SerializeTransportRouter(transport_catalogue_proto::TransportRouter& transport_router_proto,
                              std::unique_ptr<TransportRouter> router) {
    auto& routing_settings_proto = *transport_router_proto.mutable_routing_settings();
    SerializeRoutingSettings(routing_settings_proto, router->GetRoutingSetting());

    SerializeGraph(*transport_router_proto.mutable_graph(), router->GetGraph());
    SerializeRouter(*transport_router_proto.mutable_router(), router->GetRouterInternalData());

    for (const auto& [stop_ptr, vertex_ids] : router->GetStopVertexIds()) {
        auto& vertex_ids_proto = *transport_router_proto.add_stops_vertex_ids();

        vertex_ids_proto.set_name(stop_ptr->name);
        vertex_ids_proto.set_in(vertex_ids.in);
        vertex_ids_proto.set_out(vertex_ids.out);
    }

    for (const auto& [stop_ptr] : router->GetVertiсesInfo()) {
        transport_router_proto.add_vertices_info()->set_stop_name(stop_ptr->name);
    }

    for (const auto& edge_info : router->GetEdgesInfo()) {
        auto& edge_info_proto = *transport_router_proto.add_edges_info();

        if (std::holds_alternative<BusEdgeInfo>(edge_info)) {
            const auto& bus_edge_info = std::get<BusEdgeInfo>(edge_info);
            auto& bus_edge_info_proto = *edge_info_proto.mutable_bus_data();

            bus_edge_info_proto.set_bus_name(bus_edge_info.bus_ptr->name);
            bus_edge_info_proto.set_start_stop_idx(bus_edge_info.start_stop_idx);
            bus_edge_info_proto.set_finish_stop_idx(bus_edge_info.finish_stop_idx);
        } else {
            edge_info_proto.mutable_wait_data();
        }
    }
}

void transport_catalogue::Serialize(TransportCatalogue& db, const json::Dict& render_settings_json,
                                    const StopsDict& stops_dict_for_map, const BusesDict& buses_dict_for_map,
                                    std::unique_ptr<TransportRouter> router, const std::string& file_name) {
    // we should copy all data from db to binary file
    // pointers to objects will be saved by objects names,
    // when we deserialize binary file - we'll get new pointers
    transport_catalogue_proto::TransportCatalogue db_proto;

    for (const auto& [_, stop] : db.GetAllStops()) {
        SerializeStop(db_proto, stop);
    }

    for (const auto& [_, bus] : db.GetAllBuses()) {
        SerializeBus(db_proto, bus);
    }

    for (const auto& [stops_pair, distance] : db.GetDistances()) {
        SerializeDistanceBetweenStops(db_proto, stops_pair, distance);
    }
    SerializeDataForMapRenderer(db_proto, stops_dict_for_map, buses_dict_for_map);

    SerializeRenderSettings(*db_proto.mutable_render_settings(), render_settings_json);

    SerializeTransportRouter(*db_proto.mutable_router(), std::move(router));

    std::ofstream out_file(file_name, std::ios::binary);
    db_proto.SerializeToOstream(&out_file);
}

//---------------------------------------Deserialization---------------------------------------------//

using RouterProto = transport_catalogue_proto::TransportRouter;
using StopsPtrDict = std::unordered_map<std::string, StopPtr>;
using BusesPtrDict = std::unordered_map<std::string, BusPtr>;

std::string ReadFileData(const std::string& file_name) {
    std::ifstream file(file_name, std::ios::binary | std::ios::ate);
    const std::ifstream::pos_type end_pos = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string data(end_pos, '\0');
    file.read(&data[0], end_pos);
    return data;
}

domain::Stop DeserializeStop(const transport_catalogue_proto::Stop& stop_proto) {
    domain::Stop stop;

    stop.name = stop_proto.name();
    stop.position.lng = stop_proto.lng();
    stop.position.lat = stop_proto.lat();

    return stop;
}

svg::Point DeserializePoint(const transport_catalogue_proto::Point& point_proto) {
    return {point_proto.x(), point_proto.y()};
}

svg::Color DeserializeColor(const transport_catalogue_proto::Color& color_proto) {
    if (color_proto.is_none()) {
        return std::monostate{};
    }

    if (!color_proto.has_rgba()) {
        return color_proto.name();
    }

    const auto& rgba_proto = color_proto.rgba();
    const auto red = static_cast<uint8_t>(rgba_proto.red());
    const auto green = static_cast<uint8_t>(rgba_proto.green());
    const auto blue = static_cast<uint8_t>(rgba_proto.blue());
    const svg::Rgb rgb{red, green, blue};

    if (rgba_proto.has_opacity()) {
        return svg::Rgba{rgb.red, rgb.green, rgb.blue, rgba_proto.opacity()};
    }

    return rgb;
}

renderer::RenderSettings DeserializeRenderSettings(
    const transport_catalogue_proto::RenderSettings& render_settings_proto) {
    renderer::RenderSettings result;

    result.width = render_settings_proto.width();
    result.height = render_settings_proto.height();
    result.padding = render_settings_proto.padding();
    result.palette.reserve(render_settings_proto.palette_size());

    for (const auto& color : render_settings_proto.palette()) {
        result.palette.push_back(DeserializeColor(color));
    }

    result.line_width = render_settings_proto.line_width();
    result.underlayer_color = DeserializeColor(render_settings_proto.underlayer_color());
    result.underlayer_width = render_settings_proto.underlayer_width();

    result.stop_radius = render_settings_proto.stop_radius();
    result.stop_label_offset = DeserializePoint(render_settings_proto.stop_label_offset());
    result.stop_label_font_size = render_settings_proto.stop_label_font_size();
    result.bus_label_font_size = render_settings_proto.bus_label_font_size();
    result.bus_label_offset = DeserializePoint(render_settings_proto.bus_label_offset());

    return result;
}

graph::DirectedWeightedGraph<double> DeserializeGraph(const transport_catalogue_proto::DirectedWeightedGraph& proto) {
    graph::DirectedWeightedGraph<double> graph;

    graph.GetEdges().reserve(proto.edges_size());

    for (const auto& edge_proto : proto.edges()) {
        auto& edge = graph.GetEdges().emplace_back();
        edge.from = edge_proto.from();
        edge.to = edge_proto.to();
        edge.weight = edge_proto.weight();
    }
    graph.GetIncidenceList().reserve(proto.incidence_lists_size());

    for (const auto& incidence_list_proto : proto.incidence_lists()) {
        auto& incidence_list = graph.GetIncidenceList().emplace_back();
        incidence_list.reserve(incidence_list_proto.edge_ids_size());

        for (const auto edge_id : incidence_list_proto.edge_ids()) {
            incidence_list.push_back(edge_id);
        }
    }

    return graph;
}

std::unique_ptr<graph::Router<double>> DeserializeRouter(const transport_catalogue_proto::Router& router_proto,
                                                         const graph::DirectedWeightedGraph<double>& graph) {
    std::vector<std::vector<std::optional<graph::RouteInternalData<double>>>> routes_internal_data;

    routes_internal_data.reserve(router_proto.sources_data_size());

    for (const auto& source_data_proto : router_proto.sources_data()) {
        auto& source_data = routes_internal_data.emplace_back();
        source_data.reserve(source_data_proto.targets_data_size());

        for (const auto& route_data_proto : source_data_proto.targets_data()) {
            auto& route_data = source_data.emplace_back();

            if (route_data_proto.exists()) {
                route_data = graph::RouteInternalData<double>{route_data_proto.weight(), std::nullopt};

                if (route_data_proto.has_prev_edge()) {
                    route_data->prev_edge = route_data_proto.prev_edge();
                }
            }
        }
    }

    return std::make_unique<graph::Router<double>>(graph, routes_internal_data);
}

std::unordered_map<const Stop*, StopVertexId> DeserializeStopVertexIds(const RouterProto& transport_router_proto,
                                                                       const StopsPtrDict& stops) {
    std::unordered_map<const Stop*, StopVertexId> stops_vertex_ids;

    for (const auto& stop_vertex_ids_proto : transport_router_proto.stops_vertex_ids()) {
        stops_vertex_ids[stops.at(stop_vertex_ids_proto.name())] = {
            stop_vertex_ids_proto.in(),
            stop_vertex_ids_proto.out(),
        };
    }
    return stops_vertex_ids;
}

std::vector<VertexInfo> DeserializeVerticesInfo(const RouterProto& transport_router_proto, const StopsPtrDict& stops) {
    std::vector<VertexInfo> vertices_info;

    vertices_info.reserve(transport_router_proto.vertices_info_size());

    for (const auto& vertex_info_proto : transport_router_proto.vertices_info()) {
        vertices_info.emplace_back().stop_ptr = stops.at(vertex_info_proto.stop_name());
    }

    return vertices_info;
}

std::vector<EdgeInfo> DeserializeEdgesInfo(const RouterProto& transport_router_proto, const BusesPtrDict& buses) {
    std::vector<EdgeInfo> edges_info;

    edges_info.reserve(transport_router_proto.edges_info_size());

    for (const auto& edge_info_proto : transport_router_proto.edges_info()) {
        auto& edge_info = edges_info.emplace_back();

        if (edge_info_proto.has_bus_data()) {
            const auto& bus_info_proto = edge_info_proto.bus_data();

            edge_info = BusEdgeInfo{
                buses.at(bus_info_proto.bus_name()),
                bus_info_proto.start_stop_idx(),
                bus_info_proto.finish_stop_idx(),
            };
        } else {
            edge_info = WaitEdgeInfo{};
        }
    }

    return edges_info;
}

std::unique_ptr<TransportRouter> DeserializeTransportRouter(const RouterProto& transport_router_proto,
                                                            const StopsPtrDict& stops, const BusesPtrDict& buses,
                                                            const DistancesBetweenStops& distances) {
    std::unique_ptr<TransportRouter> router = std::make_unique<TransportRouter>(distances);

    router->SetRoutingSettings(transport_router_proto.routing_settings().bus_wait_time(),
                               transport_router_proto.routing_settings().bus_velocity());

    router->SetGraph(DeserializeGraph(transport_router_proto.graph()));

    router->SetRouter(std::move(DeserializeRouter(transport_router_proto.router(), router->GetGraph())));

    router->SetStopVertexIds(DeserializeStopVertexIds(transport_router_proto, stops));

    router->SetVertiсesInfo(DeserializeVerticesInfo(transport_router_proto, stops));

    router->SetEdgesInfo(DeserializeEdgesInfo(transport_router_proto, buses));

    return std::move(router);
}

std::unique_ptr<TransportRouter> transport_catalogue::Deserialize(const std::string& file_name, TransportCatalogue& db,
                                                                  StopsDict& stops_dict_for_map,
                                                                  BusesDict& buses_dict_for_map,
                                                                  renderer::RenderSettings& render_settings,
                                                                  const DistancesBetweenStops& distances) {
    transport_catalogue_proto::TransportCatalogue transport_catalogue_proto;

    std::string data = ReadFileData(file_name);
    // check ParseFromString is correct
    assert(transport_catalogue_proto.ParseFromString(data));

    for (const auto& stop : transport_catalogue_proto.stops()) {
        db.AddStop(DeserializeStop(stop));
    }

    for (const auto& bus : transport_catalogue_proto.buses()) {
        domain::Bus bus_to_add;
        bus_to_add.name = bus.name();

        for (const auto& bus_stop : bus.stops()) {
            bus_to_add.stops.emplace_back(db.GetStop(bus_stop));
        }

        for (const auto& endpoint : bus.endpoints()) {
            bus_to_add.endpoints.emplace_back(db.GetStop(endpoint));
        }
        db.AddBus(bus_to_add);
        db.InsertBusesToStops(bus_to_add.stops, db.GetBus(bus_to_add.name));
    }

    for (const auto& stop : transport_catalogue_proto.stops_for_map()) {
        stops_dict_for_map[stop] = db.GetStop(stop);
    }

    for (const auto& bus : transport_catalogue_proto.buses_for_map()) {
        buses_dict_for_map[bus] = db.GetBus(bus);
    }

    for (const auto& distances : transport_catalogue_proto.distances_between_stops()) {
        db.SetDistanceBetweenStops(db.GetStop(distances.stop_from()), db.GetStop(distances.stop_to()),
                                   distances.distance());
    }

    render_settings = DeserializeRenderSettings(transport_catalogue_proto.render_settings());

    std::unique_ptr<TransportRouter> router = std::move(
        DeserializeTransportRouter(transport_catalogue_proto.router(), db.GetAllStops(), db.GetAllBuses(), distances));

    return std::move(router);
}
