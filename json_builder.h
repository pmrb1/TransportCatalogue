#pragma once
#include <optional>
#include <string>

#include "json.h"

namespace json {

class Builder {
public:
    class DictValueContext;
    class DictItemContext;
    class ArrayItemContext;

public:
    class BaseContext {
    public:
        BaseContext(Builder&& builder);

    public:
        DictItemContext StartDict();
        ArrayItemContext StartArray();

        BaseContext Value(Node::Value value);
        BaseContext EndArray();
        BaseContext EndDict();

        Document Build();

    private:
        Builder& builder_;
    };

    class DictValueContext {
    public:
        DictValueContext(Builder&& builder);

    public:
        DictItemContext Value(Node::Value value);
        DictItemContext StartDict();
        ArrayItemContext StartArray();

    private:
        Builder& builder_;
    };

    class DictItemContext {
    public:
        DictItemContext(Builder&& builder);

    public:
        DictValueContext Key(std::string str);
        BaseContext EndDict();

    private:
        Builder& builder_;
    };

    class ArrayItemContext {
    public:
        ArrayItemContext(Builder&& builder);

    public:
        ArrayItemContext Value(Node::Value value);
        ArrayItemContext StartArray();
        DictItemContext StartDict();
        BaseContext EndArray();

    private:
        Builder& builder_;
    };

public:
    Builder();

public:
    DictValueContext Key(std::string str);
    BaseContext Value(Node::Value value);

    DictItemContext StartDict();
    ArrayItemContext StartArray();

    BaseContext EndArray();
    BaseContext EndDict();

    Document Build();

private:
    Node root_;
    std::optional<std::string> key_;
    std::vector<Node*> nodes_stack_;
};

}  // namespace json