#include "json.h"

#include <cmath>
#include <set>
using namespace std;

namespace json {

const Array& Node::AsArray() const {
    if (std::get_if<Array>(this) == nullptr) {
        using namespace std::string_literals;
        throw std::logic_error("Неверный тип данных"s);
    }
    return *std::get_if<Array>(this);
}

const Dict& Node::AsMap() const {
    if (std::get_if<Dict>(this) == nullptr) {
        using namespace std::string_literals;
        throw std::logic_error("Неверный тип данных"s);
    }
    return *std::get_if<Dict>(this);
}

const Dict& Node::AsDict() const {
    if (std::get_if<Dict>(this) == nullptr) {
        using namespace std::string_literals;
        throw std::logic_error("Неверный тип данных"s);
    }
    return *std::get_if<Dict>(this);
}

int Node::AsInt() const {
    if (std::get_if<int>(this) == nullptr) {
        using namespace std::string_literals;
        throw std::logic_error("Неверный тип данных"s);
    }
    return *std::get_if<int>(this);
}

const string& Node::AsString() const {
    if (std::get_if<std::string>(this) == nullptr) {
        using namespace std::string_literals;
        throw std::logic_error("Неверный тип данных"s);
    }
    return *std::get_if<std::string>(this);
}

bool Node::AsBool() const {
    if (std::get_if<bool>(this) == nullptr) {
        using namespace std::string_literals;
        throw std::logic_error("Неверный тип данных"s);
    }
    return *std::get_if<bool>(this);
}

double Node::AsDouble() const {
    if (std::get_if<double>(this) != nullptr) {
        return *std::get_if<double>(this);
    } else if (std::get_if<int>(this) != nullptr) {
        return static_cast<double>(*std::get_if<int>(this));
    } else {
        using namespace std::string_literals;

        throw std::logic_error("Неверный тип данных"s);
    }
}

bool Node::IsNull() const { return (std::get_if<std::nullptr_t>(this) != nullptr); }

bool Node::IsArray() const { return (std::get_if<Array>(this) != nullptr); }

bool Node::IsMap() const { return (std::get_if<Dict>(this) != nullptr); }

bool Node::IsDict() const { return (std::get_if<Dict>(this) != nullptr); }

bool Node::IsBool() const { return (std::get_if<bool>(this) != nullptr); }

bool Node::IsInt() const { return (std::get_if<int>(this) != nullptr); }

bool Node::IsDouble() const { return (std::get_if<double>(this) != nullptr || std::get_if<int>(this) != nullptr); }

bool Node::IsPureDouble() const {
    if (std::get_if<double>(this) == nullptr) {
        return false;
    }
    return true;
}

bool Node::IsString() const { return (std::get_if<std::string>(this) != nullptr); }

bool Node::operator==(const Node& other) const { return this->GetValue() == other.GetValue(); }

bool Node::operator!=(const Node& other) const { return !(this->GetValue() == other.GetValue()); }

void OutNode::operator()(std::nullptr_t) const { out << "null"; }

void OutNode::operator()(Array array) const {
    using namespace std::string_literals;
    out << "["s;
    for (size_t i = 0; i < array.size(); ++i) {
        std::visit(OutNode{out}, array[i].GetValue());
        if (i + 1 != array.size()) {
            out << ", "s;
        }
    }
    out << "]"s;
}

void OutNode::operator()(Dict map) const {
    using namespace std::string_literals;
    int flag = 0;
    out << "{ "s;
    for (const auto& [key, node] : map) {
        if (flag == 1) {
            out << ", "s;
        }
        out << " \""s << key << "\": "s;
        std::visit(OutNode{out}, node.GetValue());
        out << ""s;
        flag = 1;
    }
    out << " }"s;
}

void OutNode::operator()(bool value) const { out << std::boolalpha << value; }

void OutNode::operator()(int value) const { out << value; }

void OutNode::operator()(double value) const { out << value; }

void OutNode::operator()(const std::string& str) const {
    using namespace std::string_literals;
    std::string str_;
    str_ += "\"";
    for (const char& c : str) {
        if (c == '"') {
            str_ += "\\\""s;
        } else {
            str_ += c;
        }
    }
    str_ += "\"";
    out << str_;
}

Document::Document(Node root) : root_(move(root)) {}

const Node& Document::GetRoot() const { return root_; }

bool Document::operator==(const Document& other) const { return root_ == other.root_; }

bool Document::operator!=(const Document& other) const { return root_ != other.root_; }

Node LoadNode(istream& input) {
    char c;

    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 't' || c == 'f') {
        // bool value
        input.putback(c);
        return LoadBool(input);
    } else if (c == 'n') {
        // null value
        input.putback(c);
        return LoadNull(input);
    } else {
        // digit value
        input.putback(c);
        return NodeNumber(LoadNumber(input));
    }
}

Node LoadArray(istream& input) {
    Array result;

    char c;

    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    if (c != ']') {
        using namespace std::string_literals;
        throw ParsingError("error"s);
    }

    return Node(move(result));
}

Node LoadBool(istream& input) {
    std::string value;
    for (char c; input >> c;) {
        value += c;
        if (value == "true"s) {
            return Node(true);
        }
        if (value == "false"s) {
            return Node(false);
        }
        if (value.size() == 5) {
            break;
        }
    }
    using namespace std::string_literals;
    throw ParsingError("Неверный булевый тип"s);
    return Node();
}

Node LoadNull(istream& input) {
    std::string value;
    for (char c; input >> c;) {
        value += c;
        if (value == "null"s) {
            return Node(nullptr);
        }
        if (value.size() == 4) {
            break;
        }
    }
    throw ParsingError("Неверный тип на входе"s);
    return Node();
}

Number LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return std::stoi(parsed_num);
            } catch (...) {
                // В случае неудачи, например, при переполнении
                // код ниже попробует преобразовать строку в double
            }
        }
        return std::stod(parsed_num);
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

Node NodeNumber(const Number& number) {
    if (std::get_if<int>(&number) != nullptr) {
        return Node(*std::get_if<int>(&number));
    } else {
        return Node(*std::get_if<double>(&number));
    }
}

Node LoadString(istream& input) {
    input >> std::noskipws;
    string line;

    char prev = ' ';
    char c;
    bool current_escape = false;
    while (input >> c) {
        if (c == '"' && prev != '\\') {
            break;
        }
        current_escape = false;
        if (prev == '\\') {
            switch (c) {
                case 'n':
                    line += "\n";
                    current_escape = true;
                    break;
                case 'r':
                    line += "\r";
                    current_escape = true;
                    break;
                case 't':
                    line += "\t";
                    current_escape = true;
                    break;

                case '\\':
                    line += "\\";
                    current_escape = true;
                    break;
            }
        }

        if (c != '\\' && !current_escape) {
            line += c;
        }

        prev = c;
    }

    input >> std::skipws;
    if (line.size() == 0 || c != '"') {
        throw ParsingError("Пустая строка на входе"s);
    }
    return Node(move(line));
}

Node LoadDict(istream& input) {
    std::map<std::string, Node> result;
    char c;
    for (; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }

        if (c == '"') {
            std::string key = LoadString(input).AsString();
            input >> c;
            result.emplace(std::move(key), LoadNode(input));
        }
    }
    if (c != '}') {
        using namespace std::string_literals;
        throw ParsingError("error"s);
    }

    return Node(std::move(result));
}

Document Load(istream& input) { return Document{LoadNode(input)}; }

void Print(const Document& doc, std::ostream& output) { std::visit(OutNode{output}, doc.GetRoot().GetValue()); }
}  // namespace json