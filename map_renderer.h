#pragma once
#include <algorithm>
#include <set>
#include <unordered_set>

#include "domain.h"
#include "geo.h"
#include "json.h"
#include "svg.h"

namespace transport_catalogue {

namespace renderer {

using namespace domain;

struct RenderSettings {
    double width = 0;
    double height = 0;

    double padding = 0;

    double line_width = 0;
    double stop_radius = 0;

    int bus_label_font_size = 0;
    svg::Point bus_label_offset;

    int stop_label_font_size = 0;
    svg::Point stop_label_offset;

    svg::Color underlayer_color;
    double underlayer_width = 0;

    std::vector<svg::Color> palette;
};

const svg::Point ParsePoint(const json::Node& json);

const svg::Color ParseColor(const json::Node& json);

const std::vector<svg::Color> ParseColors(const json::Node& json);

const std::map<std::string_view, svg::Point> ComputeStopsCoordsByGrid(const StopsDict& stops_dict,
                                                                      const RenderSettings& render_settings);

const std::unordered_map<BusPtr, svg::Color> SelectBusColors(const BusesDict& buses_dict,
                                                             const RenderSettings& render_settings);

class MapRenderer {
public:
    MapRenderer(const StopsDict& stops_dict, const BusesDict& buses_dict, const json::Dict& render_settings_json);
    MapRenderer(const StopsDict& stops_dict, const BusesDict& buses_dict, const RenderSettings& render_settings);

public:
    const RenderSettings ParseRenderSettings(const json::Dict& json);
    const svg::Document Render() const;

private:
    RenderSettings render_settings_;
    const BusesDict& buses_dict_;
    std::map<std::string_view, svg::Point> stops_coords_;
    std::unordered_map<BusPtr, svg::Color> bus_colors_;

    void RenderBusLines(svg::Document& svg) const;
    void RenderStopPoints(svg::Document& svg) const;
    void RenderBusLabels(svg::Document& svg) const;
    void RenderStopLabels(svg::Document& svg) const;
};

}  // namespace renderer

}  // namespace transport_catalogue