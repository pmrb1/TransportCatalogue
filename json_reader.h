#pragma once

#include <iostream>
#include <istream>
#include <string>
#include <string_view>
#include <vector>

#include "geo.h"
#include "json.h"
#include "json_builder.h"
#include "request_handler.h"
#include "transport_catalogue.h"

namespace transport_catalogue {

using namespace transport_catalogue::service;

enum class QueryType { kBus, kStop };

struct CommandInfo {
    QueryType query_type;
    std::string name;
    geo::Coordinates point;
    json::Node json_bus_request;
    std::unordered_map<std::string, int> distances_to_other_stops_;
};

class InputReader {
public:
    void MakeBase(std::istream& input);

    void ProcessRequests(std::istream& input, std::ostream& output);

private:
    void ManageReadQueries(const std::vector<json::Node>& post_requests, TransportCatalogue& db);

    void ManageWriteQueries(const std::vector<json::Node>& get_requests, const RequestHandler& request_handler,
                            std::ostream& output) const;

    void AddDataToDb(TransportCatalogue& db);

    domain::Bus ParseBusDataFromJson(const json::Node& bus_request, TransportCatalogue& db, const std::string& bus_name);

    void PrepareDataForMapRenderer(TransportCatalogue& db);

private:
    std::vector<CommandInfo> data_;
    StopsDict stops_dict_for_map_;
    BusesDict buses_dict_for_map_;

    std::unordered_map<std::string, std::unordered_map<std::string, int>> distances_to_other_stops_;
};

struct RouteItemResponseBuilder {
    json::Dict operator()(const TransportRouter::RouteInfo::BusItem& bus_item) const;
    json::Dict operator()(const TransportRouter::RouteInfo::WaitItem& wait_item) const;
};

}  // namespace transport_catalogue
