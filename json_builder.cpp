#include "json_builder.h"

#include <optional>
#include <stdexcept>
#include <vector>

using namespace std::string_literals;

namespace json {
// Builder
Builder::BaseContext Builder::Value(Node::Value value) {
    if (nodes_stack_.empty()) {
        throw std::logic_error("error"s);
    }

    if (nodes_stack_.back()->IsArray()) {
        auto& back_node = nodes_stack_.back()->GetValue();

        if (key_) {
            throw std::logic_error("error"s);
        }
        Array& arr = std::get<Array>(back_node);

        arr.emplace_back();
        arr.back().GetValue() = std::move(value);
        BaseContext ctx(std::move(*this));
        return ctx;
    }

    if (nodes_stack_.back()->IsDict()) {
        auto& back_node = nodes_stack_.back()->GetValue();
        Dict& dict = std::get<Dict>(back_node);

        if (!key_) {
            throw std::logic_error("error"s);
        }

        auto& key = key_.value();

        dict[key].GetValue() = std::move(value);
        key_ = std::nullopt;

        BaseContext ctx(std::move(*this));
        return ctx;
    }

    if (key_) {
        throw std::logic_error("error"s);
    }
    if (nodes_stack_.back()->IsNull()) {
        nodes_stack_.back()->GetValue() = std::move(value);
        nodes_stack_.pop_back();

        BaseContext ctx(std::move(*this));
        return ctx;
    }

    throw std::logic_error("error"s);
}

Builder::DictItemContext Builder::StartDict() {
    if (nodes_stack_.empty()) {
        throw std::logic_error("error"s);
    }

    if (nodes_stack_.back()->IsNull()) {
        nodes_stack_.back()->GetValue() = Dict();
        DictItemContext ctx(std::move(*this));
        return ctx;
    }

    if (nodes_stack_.back()->IsArray()) {
        auto& back_node = nodes_stack_.back()->GetValue();
        Array& arr = std::get<Array>(back_node);

        json::Node node{Dict()};

        arr.emplace_back(std::move(node));
        nodes_stack_.emplace_back(&arr.back());

        DictItemContext ctx(std::move(*this));

        return ctx;
    }

    if (nodes_stack_.back()->IsDict()) {
        auto& back_node = nodes_stack_.back()->GetValue();
        Dict& dict = std::get<Dict>(back_node);

        if (!key_) {
            throw std::logic_error("error"s);
        }

        auto& key = key_.value();
        json::Node node{Dict()};

        dict[key] = std::move(node);

        nodes_stack_.emplace_back(&dict[key]);
        key_ = std::nullopt;

        DictItemContext ctx(std::move(*this));

        return ctx;
    }
    throw std::logic_error("error"s);
}

Builder::ArrayItemContext Builder::StartArray() {
    if (nodes_stack_.empty()) {
        throw std::logic_error("error"s);
    }

    if (nodes_stack_.back()->IsNull()) {
        nodes_stack_.back()->GetValue() = Array();
        ArrayItemContext ctx(std::move(*this));

        return ctx;
    } else {
        if (nodes_stack_.back()->IsDict()) {
            auto& back_node = nodes_stack_.back()->GetValue();
            Dict& dict = std::get<Dict>(back_node);

            if (!key_) {
                throw std::logic_error("error"s);
            }

            auto& key = key_.value();
            json::Node arr{Array()};
            dict[key] = arr;
            nodes_stack_.emplace_back(&dict.at(key));

            key_ = std::nullopt;

            ArrayItemContext ctx(std::move(*this));
            return ctx;
        }
        if (nodes_stack_.back()->IsArray()) {
            auto& back_node = nodes_stack_.back()->GetValue();
            Array& arr = std::get<Array>(back_node);
            json::Array node{Array()};
            arr.emplace_back();
            arr.back().GetValue() = std::move(node);
            nodes_stack_.emplace_back(&arr.back());

            ArrayItemContext ctx(std::move(*this));
            return ctx;
        }
    }

    throw std::logic_error("error"s);
}

Builder::Builder() { nodes_stack_.emplace_back(&root_); }

Builder::DictValueContext Builder::Key(std::string str) {
    if (root_.IsNull() || nodes_stack_.empty()) {
        throw std::logic_error("error"s);
    }

    if (key_ || !nodes_stack_.back()->IsDict()) {
        throw std::logic_error("error"s);
    }
    key_ = std::move(str);

    DictValueContext ctx(std::move(*this));

    return ctx;
}

Document Builder::Build() {
    if (!nodes_stack_.empty()) {
        throw std::logic_error("error"s);
    }
    if (root_.IsNull()) {
        throw std::logic_error("error"s);
    }
    return Document(root_);
}

Builder::BaseContext Builder::EndArray() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray()) {
        throw std::logic_error("error");
    }
    nodes_stack_.pop_back();
    BaseContext ctx(std::move(*this));

    return ctx;
}

Builder::BaseContext Builder::EndDict() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
        throw std::logic_error("error");
    }
    nodes_stack_.pop_back();
    BaseContext ctx(std::move(*this));

    return ctx;
}
// BaseContext

Builder::BaseContext::BaseContext(Builder&& builder) : builder_(builder) {}

Builder::DictItemContext Builder::BaseContext::StartDict() { return builder_.StartDict(); }

Builder::ArrayItemContext Builder::BaseContext::StartArray() { return builder_.StartArray(); }

Builder::BaseContext Builder::BaseContext::Value(Node::Value value) { return builder_.Value(value); }

Builder::BaseContext Builder::BaseContext::EndArray() { return builder_.EndArray(); }

Builder::BaseContext Builder::BaseContext::EndDict() { return builder_.EndDict(); }

Document Builder::BaseContext::Build() { return builder_.Build(); }

Builder::DictValueContext::DictValueContext(Builder&& builder) : builder_(builder) {}

// DictValueContext
Builder::DictItemContext Builder::DictValueContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    DictItemContext ctx(std::move(builder_));

    return ctx;
}

Builder::DictItemContext Builder::DictValueContext::StartDict() { return builder_.StartDict(); }

Builder::ArrayItemContext Builder::DictValueContext::StartArray() { return builder_.StartArray(); }

Builder::DictItemContext::DictItemContext(Builder&& builder) : builder_(builder) {}

// DictItemContext
Builder::DictValueContext Builder::DictItemContext::Key(std::string str) {
    builder_.Key(str);

    DictValueContext ctx(std::move(builder_));

    return ctx;
}

Builder::BaseContext Builder::DictItemContext::EndDict() { return builder_.EndDict(); }

Builder::ArrayItemContext::ArrayItemContext(Builder&& builder) : builder_(builder) {}

// ArrayItemContext
Builder::ArrayItemContext Builder::ArrayItemContext::Value(Node::Value value) {
    builder_.Value(value);
    ArrayItemContext ctx(std::move(builder_));

    return ctx;
}

Builder::ArrayItemContext Builder::ArrayItemContext::StartArray() { return builder_.StartArray(); }

Builder::DictItemContext Builder::ArrayItemContext::StartDict() { return builder_.StartDict(); }

Builder::BaseContext Builder::ArrayItemContext::EndArray() { return builder_.EndArray(); }

}  // namespace json