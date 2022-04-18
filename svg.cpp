#include "svg.h"

#include <ostream>
namespace svg {

using namespace std::literals;

std::string HtmlEncodeString(std::string_view data) {
    std::string result = ""s;
    for (char it : data) {
        std::string letter;
        it == '\"'   ? letter = "&quot;"
        : it == '\'' ? letter = "&apos;"
        : it == '<'  ? letter = "&lt;"
        : it == '>'  ? letter = "&gt;"
        : it == '&'  ? letter = "&amp;"
                     : letter = it;

        result += letter;
    }
    return result;
}

RenderContext::RenderContext(std::ostream& out) : out(out) {}

RenderContext::RenderContext(std::ostream& out, int indent_step, int indent)
    : out(out), indent_step(indent_step), indent(indent) {}

RenderContext RenderContext::Indented() const { return {out, indent_step, indent + indent_step}; }

void RenderContext::RenderIndent() const {
    for (int i = 0; i < indent; ++i) {
        out.put(' ');
    }
}

ColorPrinter::ColorPrinter(std::ostream& out) : out_(out) {}

void ColorPrinter::operator()(const std::monostate&) {
    using namespace std::literals;

    out_ << "none"sv;
}

void ColorPrinter::operator()(const Rgb& color) {
    using namespace std::literals;

    out_ << "rgb("sv << static_cast<unsigned>(color.red) << ","sv << static_cast<unsigned>(color.green) << ","sv
         << static_cast<unsigned>(color.blue) << ")"sv;
}

void ColorPrinter::operator()(const Rgba& color) {
    using namespace std::literals;

    out_ << "rgba("sv << static_cast<unsigned>(color.red) << ","sv << static_cast<unsigned>(color.green) << ","sv
         << static_cast<unsigned>(color.blue) << ","sv << color.opacity << ")"sv;
}

void ColorPrinter::operator()(const std::string& color_string) { out_ << color_string; }

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    RenderObject(context);
}

Circle& Circle::SetCenter(Point center) {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}

// Polyline--------------------------------------------------------------------------

const Polyline& Polyline::AddPoint(Point point) {
    points_.emplace_back(std::move(point));
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline "sv;
    out << "points=\""sv;
    if (points_.size() > 0) {
        for (size_t ind = 0; ind < points_.size(); ++ind) {
            if (ind != 0) {
                out << " "sv;
                points_[ind].Render(out);
            } else {
                points_[ind].Render(out);
            }
        }
    }
    out << "\""sv;
    RenderAttrs(context.out);

    out << "/>"sv;
}

// Text--------------------------------------------------------------------------------
Text& Text::SetPosition(Point pos) {
    coords_ = std::move(pos);
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = std::move(offset);
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
}

Text& Text::SetData(std::string data) {
    std::string result = ""s;
    for (char it : data) {
        std::string letter;
        it == '\"'   ? letter = "&quot;"
        : it == '\'' ? letter = "&apos;"
        : it == '<'  ? letter = "&lt;"
        : it == '>'  ? letter = "&gt;"
        : it == '&'  ? letter = "&amp;"
                     : letter = it;

        result += letter;
    }
    data_ = result;
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text"sv;
    RenderAttrs(context.out);
    out << " x=\""sv << coords_.x << "\" y=\""sv << coords_.y << "\" "sv;

    out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;

    out << "font-size=\""sv << font_size_ << "\""sv;

    if (font_family_) out << " font-family=\""sv << *font_family_ << "\""sv;
    if (font_weight_) out << " font-weight=\""sv << *font_weight_ << "\""sv;

    out << ">"sv;

    out << HtmlEncodeString(data_) << "</text>"sv;
}

// Document--------------------------------------------------------------------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) { objects_.push_back(std::move(obj)); }

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\\n"sv;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv;
    if (!objects_.empty()) {
        for (const auto& object : objects_) {
            out << "\\n  "sv;
            object->Render(out);
        }
    }
    out << "\\n</svg>"sv;
}

}  // namespace svg