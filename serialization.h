#pragma once

#include <transport_catalogue.pb.h>

#include <string>
#include <variant>

#include "request_handler.h"
#include "transport_catalogue.h"

namespace transport_catalogue {
using DistancesBetweenStops = std::unordered_map<std::string, std::unordered_map<std::string, int>>;

void Serialize(TransportCatalogue &db, const json::Dict &render_settings_json, const StopsDict &stops_dict_for_map,
               const BusesDict &buses_dict_for_map, std::unique_ptr<TransportRouter> router,
               const std::string &file_name);

std::unique_ptr<TransportRouter> Deserialize(const std::string &file_name, TransportCatalogue &db,
                                             StopsDict &stops_dict_for_map, BusesDict &buses_dict_for_map,
                                             renderer::RenderSettings &render_settings,
                                             const DistancesBetweenStops &distances);

}  // namespace transport_catalogue