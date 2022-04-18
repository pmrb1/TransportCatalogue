#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace svg {
using namespace std::literals;

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

struct Rgb {
    Rgb() = default;

    Rgb(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

struct Rgba {
    Rgba() = default;

    Rgba(uint8_t r, uint8_t g, uint8_t b, double o) : red(r), green(g), blue(b), opacity(o) {}

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;

    double opacity = 1.0;
};

using Color = std::variant<std::monostate, svg::Rgb, svg::Rgba, std::string>;

struct ColorPrinter {
    ColorPrinter(std::ostream& out);

    void operator()(const std::monostate&);
    void operator()(const Rgb& color);
    void operator()(const Rgba& color);
    void operator()(const std::string& color_string);

    std::ostream& out_;
};

inline std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& stroke_line) {
    switch (stroke_line) {
        case StrokeLineJoin::ARCS:
            out << "arcs"sv;
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel"sv;
            break;
        case StrokeLineJoin::MITER:
            out << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            out << "round"sv;
            break;
    }
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const StrokeLineCap& stroke_line_cap) {
    switch (stroke_line_cap) {
        case StrokeLineCap::BUTT:
            out << "butt";
            break;
        case StrokeLineCap::ROUND:
            out << "round";
            break;
        case StrokeLineCap::SQUARE:
            out << "square";
            break;
    }

    return out;
}

inline const Color NoneColor{std::monostate()};

inline std::ostream& operator<<(std::ostream& out, const Color& color) {
    std::visit(ColorPrinter(out), color);
    return out;
}

template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }

    Owner& SetStrokeWidth(double width) {
        width_ = width;
        return AsOwner();
    }

    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
        line_cap_ = line_cap;
        return AsOwner();
    }

    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
        line_join_ = line_join;
        return AsOwner();
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

        if (fill_color_) {
            out << " fill=\""sv << *fill_color_ << "\""sv;
        }
        if (stroke_color_) {
            out << " stroke=\""sv << *stroke_color_ << "\""sv;
        }
        if (width_) {
            out << " stroke-width=\""sv << *width_ << "\""sv;
        }
        if (line_cap_) {
            out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
        }

        if (line_join_) {
            out << " stroke-linejoin=\""sv << *line_join_ << "\""sv;
        }
    }

private:
    Owner& AsOwner() { return static_cast<Owner&>(*this); }

    std::optional<double> width_;
    std::optional<StrokeLineCap> line_cap_;
    std::optional<StrokeLineJoin> line_join_;
    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
};

struct Point {
    Point() = default;
    Point(double x, double y) : x(x), y(y) {}

    void Render(std::ostream& out) const { out << x << ","sv << y; }

    double x = 0;
    double y = 0;
};

struct RenderContext {
public:
    RenderContext(std::ostream& out);
    RenderContext(std::ostream& out, int indent_step, int indent = 0);

public:
    RenderContext Indented() const;
    void RenderIndent() const;

public:
    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

class Object {
public:
    Object() = default;

public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

class ObjectContainer {
public:
    template <typename Obj>
    void Add(Obj obj) {
        AddPtr(std::make_unique<Obj>(std::move(obj)));
    }

    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

    virtual ~ObjectContainer() = default;
};

class Drawable {
public:
    virtual void Draw(ObjectContainer& container) const = 0;

    virtual ~Drawable() = default;
};

class Circle final : public Object, public PathProps<Circle> {
public:
    Circle() = default;

    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

class Polyline final : public Object, public PathProps<Polyline> {
public:
    const Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;
    std::vector<Point> points_;
};

class Text final : public Object, public PathProps<Text> {
public:
    Text() = default;

public:
    Text& SetPosition(Point pos);
    Text& SetOffset(Point offset);

    Text& SetFontSize(uint32_t size);
    Text& SetFontFamily(std::string font_family);
    Text& SetFontWeight(std::string font_weight);

    Text& SetData(std::string data);

private:
    void RenderObject(const RenderContext& context) const override;

private:
    Point coords_ = Point();
    Point offset_ = Point();
    uint32_t font_size_ = 1;

    std::optional<std::string> font_family_;
    std::optional<std::string> font_weight_;
    std::string data_ = ""s;
};

class Document : public ObjectContainer {
public:
    void AddPtr(std::unique_ptr<Object>&& obj);

    void Render(std::ostream& out) const;

private:
    std::vector<std::unique_ptr<Object>> objects_;
};

}  // namespace svg