#include "json_reader.h"

#include <sstream>

#include "request_handler.h"
#include "serialization.h"
#include "transport_catalogue.h"

using namespace transport_catalogue::service;

void transport_catalogue::InputReader::MakeBase(std::istream& input) {
    const auto& input_doc = json::Load(input);
    const auto& input_map = input_doc.GetRoot().AsMap();
    const std::string& file_name = input_map.at("serialization_settings").AsMap().at("file").AsString();

    // At first, we should compute all data and fill database, then serialize db
    transport_catalogue::TransportCatalogue db;

    ManageReadQueries(input_map.at("base_requests").AsArray(), db);

    PrepareDataForMapRenderer(db);

    std::unique_ptr<TransportRouter> router = std::make_unique<TransportRouter>(
        db.GetAllStops(), db.GetAllBuses(), input_map.at("routing_settings").AsDict(), distances_to_other_stops_);

    // serialize database to binary file
    transport_catalogue::Serialize(db, input_map.at("render_settings").AsMap(), stops_dict_for_map_, buses_dict_for_map_,
                            std::move(router), file_name);
}

void transport_catalogue::InputReader::ProcessRequests(std::istream& input, std::ostream& output) {
    const auto& input_doc = json::Load(input);
    const auto& input_map = input_doc.GetRoot().AsMap();
    const std::string& file_name = input_map.at("serialization_settings").AsMap().at("file").AsString();

    TransportCatalogue db;

    renderer::RenderSettings render_settings;

    auto router = transport_catalogue::Deserialize(file_name, db, stops_dict_for_map_, buses_dict_for_map_, render_settings,
                                            distances_to_other_stops_);

    renderer::MapRenderer map_renderer(stops_dict_for_map_, buses_dict_for_map_, render_settings);

    RequestHandler manager(db, map_renderer, std::move(router));

    ManageWriteQueries(input_map.at("stat_requests").AsArray(), manager, output);
}

void transport_catalogue::InputReader::ManageReadQueries(const std::vector<json::Node>& post_requests,
                                                         TransportCatalogue& db) {
    std::string line, query;

    for (const auto& request : post_requests) {
        if (request.AsMap().at("type").AsString() == "Stop") {
            CommandInfo stop_info;

            stop_info.query_type = QueryType::kStop;
            stop_info.name = request.AsMap().at("name").AsString();
            stop_info.point = {request.AsMap().at("latitude").AsDouble(), request.AsMap().at("longitude").AsDouble()};

            if (request.AsMap().count("road_distances") > 0) {
                for (const auto& pair : request.AsMap().at("road_distances").AsMap()) {
                    stop_info.distances_to_other_stops_.emplace(pair.first, pair.second.AsInt());
                }
            }
            data_.push_back(stop_info);
        }

        if (request.AsMap().at("type").AsString() == "Bus") {
            CommandInfo bus_info;

            bus_info.query_type = QueryType::kBus;
            bus_info.name = request.AsMap().at("name").AsString();
            bus_info.json_bus_request = request;

            data_.push_back(bus_info);
        }
    }
    AddDataToDb(db);
}

void transport_catalogue::InputReader::ManageWriteQueries(const std::vector<json::Node>& get_requests,
                                                          const RequestHandler& request_handler,
                                                          std::ostream& output) const {
    using namespace std::string_literals;

    auto builder = json::Builder();

    // format json [{data}]
    builder.StartArray();

    for (const auto& request : get_requests) {
        builder.StartDict();

        builder.Key("request_id").Value(request.AsDict().at("id").AsInt());

        if (request.AsDict().at("type").AsString() == "Bus") {
            const auto& route_response = request_handler.GetRouteInfo(request.AsDict().at("name").AsString());

            if (route_response.first == transport_catalogue::service::ResponseStatus::kSuccess) {
                builder.Key("route_length").Value(route_response.second.route_length);
                builder.Key("curvature").Value(route_response.second.curvature);
                builder.Key("stop_count").Value(route_response.second.total_stops);
                builder.Key("unique_stop_count").Value(route_response.second.unique_stops);

            } else {
                builder.Key("error_message").Value("not found"s);
            }
        }

        if (request.AsDict().at("type").AsString() == "Stop") {
            const auto& stop_response = request_handler.GetBusStopInfo(request.AsDict().at("name").AsString());

            if (stop_response.first == transport_catalogue::service::ResponseStatus::kSuccess) {
                json::Array buses_array;

                for (const auto& bus : stop_response.second.second) {
                    buses_array.emplace_back(bus);
                }

                json::Node arr_node{std::move(buses_array)};
                builder.Key("buses").Value(arr_node.AsArray());

            } else {
                builder.Key("error_message").Value("not found"s);
            }
        }

        if (request.AsDict().at("type").AsString() == "Map") {
            const auto& doc = request_handler.RenderMap();
            std::ostringstream out;

            doc.Render(out);

            builder.Key("map").Value(std::move(out.str()));
        }

        if (request.AsDict().at("type").AsString() == "Route") {
            auto route = request_handler.FindRoute(request_handler.GetStop(request.AsDict().at("from").AsString()),
                                                   request_handler.GetStop(request.AsDict().at("to").AsString()));

            if (route.has_value()) {
                builder.Key("total_time").Value(route->total_time.count());
                json::Array items;
                items.reserve(route->items.size());

                for (const auto& item : route->items) {
                    items.push_back(std::visit(RouteItemResponseBuilder{}, item));
                }

                builder.Key("items").Value(std::move(items));
            } else {
                builder.Key("error_message").Value("not found"s);
            }
        }
        builder.EndDict();
    }
    builder.EndArray();

    json::Print(builder.Build(), output);
}

void transport_catalogue::InputReader::AddDataToDb(TransportCatalogue& db) {
    std::partition(data_.begin(), data_.end(),
                   [](CommandInfo command) { return command.query_type == QueryType::kStop; });

    for (const auto& data : data_) {
        if (data.query_type == QueryType::kStop) {
            Stop stop;
            stop.name = data.name;
            stop.position = data.point;
            distances_to_other_stops_[stop.name] = data.distances_to_other_stops_;

            db.AddStop(stop);
        }

        if (data.query_type == QueryType::kBus) {
            Bus bus = ParseBusDataFromJson(data.json_bus_request, db, data.name);

            db.AddBus(bus);
            db.InsertBusesToStops(bus.stops, db.GetBus(bus.name));
        }
    }

    for (const auto& stop_from : db.GetAllStops()) {
        if (!distances_to_other_stops_.empty() && distances_to_other_stops_.count(stop_from.first) > 0) {
            const auto& second_stops = distances_to_other_stops_.at(stop_from.first);

            for (const auto& second_stop : second_stops) {
                const Stop* stop_to = db.GetStop(second_stop.first);
                db.SetDistanceBetweenStops(stop_from.second, stop_to, second_stop.second);
            }
        }
    }
}

domain::Bus InputReader::ParseBusDataFromJson(const json::Node& bus_request, TransportCatalogue& db,
                                              const std::string& bus_name) {
    Bus bus;
    bus.name = bus_name;
    std::vector<const Stop*> stops;

    for (const auto& stop : bus_request.AsMap().at("stops").AsArray()) {
        const Stop* stop_ptr = db.GetStop(stop.AsString());
        if (stop_ptr != nullptr) {
            stops.emplace_back(stop_ptr);
        }
    }

    bool isRoundTrip = bus_request.AsMap().at("is_roundtrip").AsBool();

    if (!stops.empty()) {
        // Additionaly check first and last stops, because without this checking I can't pass tests in the Trainer
        if (stops.front() == stops.back()) {
            bus.endpoints.emplace_back(stops.front());
        } else {
            bus.endpoints.emplace_back(stops.front());
            bus.endpoints.emplace_back(stops.back());
        }

        if (!isRoundTrip) {
            size_t stops_size = stops.size();

            for (int index = stops_size - 2; index >= 0; --index) {
                stops.emplace_back(stops[index]);
            }
        }
    }
    bus.stops = stops;

    return bus;
}

void transport_catalogue::InputReader::PrepareDataForMapRenderer(TransportCatalogue& db) {
    const std::unordered_map<std::string, BusPtr>& all_buses = db.GetAllBuses();

    for (const auto& [bus_name, bus_ptr] : all_buses) {
        if (!bus_ptr->stops.empty()) {
            buses_dict_for_map_[bus_name] = bus_ptr;

            for (const auto& stop : bus_ptr->stops) {
                stops_dict_for_map_[stop->name] = stop;
            }
        }
    }
}

json::Dict transport_catalogue::RouteItemResponseBuilder::operator()(
    const TransportRouter::RouteInfo::BusItem& bus_item) const {
    return json::Dict{{"type", json::Node("Bus"s)},
                      {"bus", json::Node(bus_item.bus_name)},
                      {"time", json::Node(bus_item.time.count())},
                      {"span_count", json::Node(static_cast<int>(bus_item.span_count))}};
}

json::Dict transport_catalogue::RouteItemResponseBuilder::operator()(
    const TransportRouter::RouteInfo::WaitItem& wait_item) const {
    return json::Dict{
        {"type", json::Node("Wait"s)},
        {"stop_name", json::Node(wait_item.stop_name)},
        {"time", json::Node(wait_item.time.count())},
    };
}
