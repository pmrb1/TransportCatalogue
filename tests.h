#pragma once

#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "json.h"

namespace transport_catalogue {

namespace tests {

template <typename T>
std::ostream& operator<<(std::ostream& stream, const std::vector<T>& vector_items) {
    using namespace std::string_literals;
    stream << "["s;
    for (size_t i = 0; i < vector_items.size(); ++i) {
        if (i != 0) {
            stream << ", "s;
        }
        stream << vector_items[i];
    }
    stream << "]"s;
    return stream;
}

template <typename T>
std::ostream& operator<<(std::ostream& stream, const std::set<T>& set_items) {
    using namespace std::string_literals;
    stream << "{"s;
    for (const auto it = set_items.begin(); it != set_items.end(); ++it) {
        if (it != set_items.begin()) {
            stream << ", "s;
        }
        stream << *it;
    }
    stream << "}"s;
    return stream;
}

template <typename T, typename U>
std::ostream& operator<<(std::ostream& stream, const std::map<T, U>& map_items) {
    using namespace std::string_literals;
    stream << "{"s;
    const auto it = map_items.rbegin();

    for (const auto& [key, value] : map_items) {
        if (key != it->first) {
            stream << key << ": "s << value << ", ";
        } else {
            stream << key << ": "s << value;
        }
    }
    stream << "}"s;
    return stream;
}

template <typename T, typename U>
void AssertEqualImpl(const T& first_element, const U& second_element, const std::string& first_element_string,
                     const std::string& second_element_string, const std::string& file, const std::string& func,
                     unsigned line, const std::string& hint) {
    using namespace std::string_literals;
    if (first_element != second_element) {
        std::cout << std::boolalpha;
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT_EQUAL("s << first_element_string << ", "s << second_element_string << ") failed: "s;
        std::cout << first_element << " != "s << second_element << "."s;

        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

template <typename F>
void RunTestImpl(const F& test, const std::string& test_name) {
    test();
    std::cerr << test_name << " OK" << std::endl;
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

#define RUN_TEST(func) RunTestImpl((func), #func)

std::string Print(const json::Node& node);

json::Document LoadJSON(const std::string& s);

void TestTransportCatalogue();

}  // namespace tests

}  // namespace transport_catalogue