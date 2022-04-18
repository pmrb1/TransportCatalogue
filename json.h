#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;
using Number = std::variant<int, double>;

class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node final : private std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> {
public:
    using variant::variant;
    using Value = variant;

    Node() = default;

    const Array& AsArray() const;
    const Dict& AsMap() const;
    const Dict& AsDict() const;
    int AsInt() const;
    const std::string& AsString() const;
    bool AsBool() const;
    double AsDouble() const;

    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;
    bool IsDict() const;
    bool IsBool() const;
    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsString() const;

    const Value& GetValue() const { return *this; }
    Value& GetValue() { return *this; }

    bool operator==(const Node&) const;
    bool operator!=(const Node&) const;
};

class OutNode {
public:
    OutNode(std::ostream& output) : out(output) {}

    void operator()(std::nullptr_t) const;
    void operator()(Array array) const;
    void operator()(Dict map) const;
    void operator()(bool value) const;
    void operator()(int value) const;
    void operator()(double value) const;
    void operator()(const std::string& str) const;

private:
    std::ostream& out;
};

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

public:
    bool operator==(const Document& other) const;
    bool operator!=(const Document& other) const;

private:
    Node root_;
};

Node LoadNode(std::istream& input);
Node LoadArray(std::istream& input);
Node LoadBool(std::istream& input);
Node LoadNull(std::istream& input);
Number LoadNumber(std::istream& input);
Node NodeNumber(const Number& number);
Node LoadString(std::istream& input);
Node LoadDict(std::istream& input);

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json