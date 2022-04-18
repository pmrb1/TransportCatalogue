#include "map_renderer.h"

#include <cassert>

inline const double EPSILON = 1e-6;
bool IsZero(double value) { return std::abs(value) < EPSILON; }

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width, double max_height,
                    double padding)
        : padding_(padding) {
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it] =
            std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it] =
            std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        }
    }

    svg::Point operator()(geo::Coordinates coords) const {
        return {(coords.lng - min_lon_) * zoom_coeff_ + padding_, (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

transport_catalogue::renderer::MapRenderer::MapRenderer(const StopsDict& stops_dict, const BusesDict& buses_dict,
                                                        const json::Dict& render_settings_json)
    : render_settings_(ParseRenderSettings(render_settings_json)),
      buses_dict_(buses_dict),
      stops_coords_(ComputeStopsCoordsByGrid(stops_dict, render_settings_)),
      bus_colors_(SelectBusColors(buses_dict, render_settings_)) {}

transport_catalogue::renderer::MapRenderer::MapRenderer(const StopsDict& stops_dict, const BusesDict& buses_dict,
                                                        const RenderSettings& render_settings)
    : render_settings_(render_settings),
      buses_dict_(buses_dict),
      stops_coords_(ComputeStopsCoordsByGrid(stops_dict, render_settings_)),
      bus_colors_(SelectBusColors(buses_dict, render_settings_)) {}

const transport_catalogue::renderer::RenderSettings transport_catalogue::renderer::MapRenderer::ParseRenderSettings(
    const json::Dict& settings_json) {
    RenderSettings result;

    result.width = settings_json.at("width").AsDouble();
    result.height = settings_json.at("height").AsDouble();
    result.padding = settings_json.at("padding").AsDouble();
    result.palette = ParseColors(settings_json.at("color_palette"));
    result.line_width = settings_json.at("line_width").AsDouble();
    result.underlayer_color = ParseColor(settings_json.at("underlayer_color"));
    result.underlayer_width = settings_json.at("underlayer_width").AsDouble();
    result.stop_radius = settings_json.at("stop_radius").AsDouble();
    result.stop_label_offset = ParsePoint(settings_json.at("stop_label_offset"));
    result.stop_label_font_size = settings_json.at("stop_label_font_size").AsInt();

    result.bus_label_font_size = settings_json.at("bus_label_font_size").AsInt();
    result.bus_label_offset = ParsePoint(settings_json.at("bus_label_offset"));

    return result;
}

const svg::Document transport_catalogue::renderer::MapRenderer::Render() const {
    svg::Document svg;

    RenderBusLines(svg);

    RenderBusLabels(svg);

    RenderStopPoints(svg);

    RenderStopLabels(svg);

    return svg;
}

void transport_catalogue::renderer::MapRenderer::RenderBusLines(svg::Document& svg) const {
    for (const auto& [bus_name, bus_ptr] : buses_dict_) {
        const auto& stops_ptrs = bus_ptr->stops;

        if (!stops_ptrs.empty()) {
            svg::Polyline line;

            line.SetFillColor("none")
                .SetStrokeColor(bus_colors_.at(bus_ptr))
                .SetStrokeWidth(render_settings_.line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            for (const auto& stop : stops_ptrs) {
                line.AddPoint(stops_coords_.at(stop->name));
            }
            svg.Add(std::move(line));
        }
    }
}

void transport_catalogue::renderer::MapRenderer::RenderStopPoints(svg::Document& svg) const {
    for (const auto& [_, point] : stops_coords_) {
        svg.Add(svg::Circle{}.SetCenter(point).SetRadius(render_settings_.stop_radius).SetFillColor({"white"}));
    }
}

void transport_catalogue::renderer::MapRenderer::RenderBusLabels(svg::Document& svg) const {
    for (const auto& [bus_name, bus_ptr] : buses_dict_) {
        const auto& stops = bus_ptr->stops;
        if (!stops.empty()) {
            const auto& color = bus_colors_.at(bus_ptr);
            for (const auto& endpoint : bus_ptr->endpoints) {
                auto base_text = svg::Text{}
                                     .SetPosition(stops_coords_.at(endpoint->name))
                                     .SetOffset(render_settings_.bus_label_offset)
                                     .SetFontSize(render_settings_.bus_label_font_size)
                                     .SetFontFamily("Verdana")
                                     .SetFontWeight("bold")
                                     .SetData(bus_name);

                svg.Add(svg::Text{base_text}
                            .SetFillColor(render_settings_.underlayer_color)
                            .SetStrokeColor(render_settings_.underlayer_color)
                            .SetStrokeWidth(render_settings_.underlayer_width)
                            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND));

                svg.Add(svg::Text{base_text}.SetFillColor(color));
            }
        }
    }
}

void transport_catalogue::renderer::MapRenderer::RenderStopLabels(svg::Document& svg) const {
    for (const auto& [name, point] : stops_coords_) {
        auto base_text = svg::Text{}
                             .SetPosition(point)
                             .SetOffset(render_settings_.stop_label_offset)
                             .SetFontSize(render_settings_.stop_label_font_size)
                             .SetFontFamily("Verdana")
                             .SetData({name.begin(), name.end()});
        svg.Add(svg::Text(base_text)
                    .SetFillColor(render_settings_.underlayer_color)
                    .SetStrokeColor(render_settings_.underlayer_color)
                    .SetStrokeWidth(render_settings_.underlayer_width)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND));

        svg.Add(base_text.SetFillColor({"black"}));
    }
}

const svg::Point transport_catalogue::renderer::ParsePoint(const json::Node& json) {
    const auto& array = json.AsArray();

    return {array[0].AsDouble(), array[1].AsDouble()};
}

const svg::Color transport_catalogue::renderer::ParseColor(const json::Node& json) {
    if (json.IsString()) {
        return json.AsString();
    }
    const auto& array = json.AsArray();

    assert(array.size() == 3 || array.size() == 4);

    svg::Rgb rgb{static_cast<uint8_t>(array[0].AsInt()), static_cast<uint8_t>(array[1].AsInt()),
                 static_cast<uint8_t>(array[2].AsInt())};
    if (array.size() == 3) {
        return rgb;
    } else {
        return svg::Rgba{rgb.red, rgb.green, rgb.blue, array[3].AsDouble()};
    }
}

const std::vector<svg::Color> transport_catalogue::renderer::ParseColors(const json::Node& json) {
    const auto& array = json.AsArray();
    std::vector<svg::Color> colors;

    colors.reserve(array.size());

    std::transform(begin(array), end(array), back_inserter(colors), ParseColor);

    return colors;
}

const std::map<std::string_view, svg::Point> transport_catalogue::renderer::ComputeStopsCoordsByGrid(
    const StopsDict& stops_dict, const RenderSettings& render_settings) {
    std::vector<geo::Coordinates> points;

    points.reserve(stops_dict.size());

    for (const auto& [_, stop_ptr] : stops_dict) {
        points.emplace_back(stop_ptr->position);
    }
    const double max_width = render_settings.width;
    const double max_height = render_settings.height;
    const double padding = render_settings.padding;

    const SphereProjector projector(std::begin(points), std::end(points), max_width, max_height, padding);

    std::map<std::string_view, svg::Point> stop_coords;

    for (const auto& [stop_name, stop_ptr] : stops_dict) {
        stop_coords[stop_name] = projector(stop_ptr->position);
    }

    return stop_coords;
}

const std::unordered_map<domain::BusPtr, svg::Color> transport_catalogue::renderer::SelectBusColors(
    const BusesDict& buses_dict, const RenderSettings& render_settings) {
    const auto& palette = render_settings.palette;
    std::unordered_map<domain::BusPtr, svg::Color> bus_colors;
    int idx = 0;

    for (const auto& [bus_name, bus_ptr] : buses_dict) {
        bus_colors[bus_ptr] = palette[idx++ % palette.size()];
    }

    return bus_colors;
}
