#include "tests.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <ostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "Profiler.h"
#include "json.h"
#include "json_reader.h"
#include "request_handler.h"
#include "transport_catalogue.h"

using namespace std::string_literals;
using namespace transport_catalogue::tests;
using namespace transport_catalogue::service;

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func,
                unsigned line, const std::string& hint) {
    if (!value) {
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

void TestSpeedRandomDataSvgOutput() {
    std::ifstream input_f, tested;
    std::ofstream output_test;
    std::string json_req;

    json_req += "{ \"base_requests\" : [";

    std::string stop_json;
    std::string bus_json;

    for (int i = 1; i < 100; ++i) {
        std::string name;
        for (int l = 0; l < i; ++l) {
            name += "A"s;
        }

        std::string next_name;
        if (i == 999) {
            next_name += "A"s;

        } else {
            next_name += (name + "A"s);
        }

        double lat = (55.611087 + (static_cast<double>(i) / 100));
        double lng = (37.20829 + (static_cast<double>(i) / 100));

        stop_json += "{ \"type\" : \"Stop\", \"name\" : \"" + name + "\", \"latitude\" : " + std::to_string(lat) +
                     ", \"longitude\" : " + std::to_string(lng) + ", \"road_distances\" : {\"" + next_name +
                     "\" : 3900 } }, ";
    }

    json_req += stop_json;

    for (int i = 1; i < 50; ++i) {
        std::string name = std::to_string(i);

        std::vector<std::string> stop_names;
        for (int l = 0; l < 100; ++l) {
            stop_names.push_back("\"A\""s);
        }

        bus_json += "{\"type\" : \"Bus\", \"name\" : \""s + name + "\",\"stops\" : "s + "[ "s;
        for (int stop_idx = 0; stop_idx < stop_names.size(); ++stop_idx) {
            if (stop_idx == 0) {
                bus_json += stop_names[i];
            } else {
                bus_json += ", "s;
                bus_json += stop_names[i];
            }
        }
        bus_json += "], ";

        bus_json += "\"is_roundtrip\" : false"s;
        if (i == 49) {
            bus_json += "}"s;
        } else {
            bus_json += "},"s;
        }
    }

    json_req += bus_json;
    json_req += "],"s;

    json_req +=
        "\"render_settings\" : {\"width\" : 200, \"height\" : 200, \"padding\" : 30, \"stop_radius\" : 5, "s +
        "\"line_width\" : 14, \"bus_label_font_size\" : 20, \"bus_label_offset\" : [ 7, 15 ], \"stop_label_font_size\" : 20, "s +
        "\"stop_label_offset\" : [ 7, -3 ],\"underlayer_color\" : [ 255, 255, 255, 0.85 ],\"underlayer_width\" : 3, \"color_palette\" : [ \"green\", [ 255, 160, 0 ], \"red\" ] },"s;
    json_req += " \"stat_requests\": [ "s;

    std::string stat_reqest;

    stat_reqest += "{\"id\" : 1, \"type\" : \"Map\"}, "s;
    for (int i = 2; i < 2000; ++i) {
        if (i % 2 == 0) {
            stat_reqest += "{\"id\" : " + std::to_string(i) + ", \"type\" : \"Stop\", \"name\" : \"AAAA\"}, "s;
        } else {
            stat_reqest += "{\"id\" : " + std::to_string(i) + ", \"type\" : \"Bus\", \"name\" : \"10\"}, "s;
        }
    }

    stat_reqest += "{\"id\" : 2000, \"type\" : \"Map\"} ] }"s;
    json_req += stat_reqest;

    std::stringstream test_inp;
    test_inp << json_req;

    output_test.open("Tests\\expected_svg.json");

    TIME_IT("TestSpeedRandomDataSvgOutput");

    transport_catalogue::TransportCatalogue db;
    transport_catalogue::InputReader input_reader;

    input_reader.ManageInputQueries(test_inp, output_test, db);

    std::stringstream output, expected;

    output_test.close();
}

void TestTranportRouter() {
    std::ifstream input_f;
    std::ofstream output_test;

    input_f.open("Tests\\input2.json");
    output_test.open("Tests\\test_svg_write_out.json");

    TIME_IT("TestTranportRouter");

    transport_catalogue::TransportCatalogue db;
    transport_catalogue::InputReader input_reader;

    std::istream& input_q = input_f;

    input_reader.ManageInputQueries(input_q, output_test, db);

    std::stringstream output, expected;

    output_test.close();
}

void TestJson() {
    using namespace json;

    {
        std::stringstream input(
            "  {"
            "\"type\":   \"Bus\","
            "\"stops\" :  ["
            "\"Biryulyovo Zapadnoye\","
            "\"Biryusinka\","
            "\"Universam\","
            "\"Biryulyovo Tovarnaya\","
            "\"Biryulyovo Passazhirskaya\","
            "\"Biryulyovo Zapadnoye\"        "
            "     ], "
            "\"is_roundtrip\": true"
            "\"name\" : \"256\"   "
            "  }\"");

        const Document doc = Load(input);

        Dict root_map = doc.GetRoot().AsDict();

        const std::vector<std::string> expected_stops = {
            "Biryulyovo Zapadnoye",      "Biryusinka",          "Universam", "Biryulyovo Tovarnaya",
            "Biryulyovo Passazhirskaya", "Biryulyovo Zapadnoye"};

        ASSERT_EQUAL(root_map["type"].AsString(), "Bus");
        ASSERT_EQUAL(root_map["name"].AsString(), "256");
        ASSERT(root_map["is_roundtrip"].AsBool());
        for (size_t i = 0; i < 6; ++i) {
            ASSERT_EQUAL(root_map["stops"].AsArray()[i].AsString(), expected_stops[i]);
        }
    }

    {
        std::stringstream input(
            "  {"
            "\"type\":   \"Bus\","
            "\"stops\" :  ["
            "\"Biryulyovo Zapadnoye\","
            "\"Universam\","
            "\"Rossoshanskaya ulitsa\","
            "\"Biryulyovo Zapadnoye\"        "
            "     ], "
            "\"is_roundtrip\": true"
            "\"name\" : \"828\"   "
            "  }\"");

        const Document doc = Load(input);

        Dict root_map = doc.GetRoot().AsDict();

        const std::vector<std::string> expected_stops = {"Biryulyovo Zapadnoye", "Universam", "Rossoshanskaya ulitsa",
                                                         "Biryulyovo Zapadnoye"};

        ASSERT_EQUAL(root_map["type"].AsString(), "Bus");
        ASSERT_EQUAL(root_map["name"].AsString(), "828");
        ASSERT(root_map["is_roundtrip"].AsBool());
        for (size_t i = 0; i < 4; ++i) {
            ASSERT_EQUAL(root_map["stops"].AsArray()[i].AsString(), expected_stops[i]);
        }
    }

    {
        std::stringstream input(
            "  {"
            "\"type\":   \"Bus\","
            "\"stops\" :  ["
            "\"Biryulyovo Zapadnoye\","
            "\"Universam\","
            "\"Rossoshanskaya ulitsa\","
            "\"Biryulyovo Zapadnoye\"        "
            "     ], "
            "\"is_roundtrip\": false"
            "\"name\" : \"828\"   "
            "  }\"");

        const Document doc = Load(input);

        Dict root_map = doc.GetRoot().AsDict();

        const std::vector<std::string> expected_stops = {"Biryulyovo Zapadnoye", "Universam", "Rossoshanskaya ulitsa",
                                                         "Biryulyovo Zapadnoye"};

        ASSERT_EQUAL(root_map["type"].AsString(), "Bus");
        ASSERT_EQUAL(root_map["name"].AsString(), "828");
        ASSERT(!root_map["is_roundtrip"].AsBool());
        for (size_t i = 0; i < 4; ++i) {
            ASSERT_EQUAL(root_map["stops"].AsArray()[i].AsString(), expected_stops[i]);
        }
    }

    {
        std::stringstream input(
            "  {"
            "\"type\":   \"Stop\","
            "\"road_distances\" :  {       },"
            "\"longitude\"  :  37.6517 ,"
            "\"name\" : \"Biryulyovo Zapadnoye\"  , "
            "\"latitude\":55.574371"
            "  }\"");

        const Document doc = Load(input);

        Dict root_map = doc.GetRoot().AsDict();

        ASSERT_EQUAL(root_map["type"].AsString(), "Stop");
        ASSERT(std::abs(root_map["longitude"].AsDouble() - 37.6517) < 0.0000001);
        ASSERT_EQUAL(root_map["name"].AsString(), "Biryulyovo Zapadnoye");
        ASSERT(std::abs(root_map["latitude"].AsDouble() - 55.574371) < 0.0000001);
        ASSERT(root_map["road_distances"].AsDict().empty());
    }

    {
        std::stringstream input(
            "  {"
            "\"type\":   \"Stop\","
            "\"road_distances\" :  {       },"
            "\"longitude\"  :  30 ,"
            "\"name\" : \"Biryulyovo Zapadnoye\"  , "
            "\"latitude\":-55"
            "  }\"");

        const Document doc = Load(input);

        Dict root_map = doc.GetRoot().AsDict();

        ASSERT_EQUAL(root_map["type"].AsString(), "Stop");
        ASSERT(std::abs(root_map["longitude"].AsDouble() - 30) < 0.0000001);
        ASSERT_EQUAL(root_map["name"].AsString(), "Biryulyovo Zapadnoye");
        ASSERT(std::abs(root_map["latitude"].AsDouble() - (-55)) < 0.0000001);
        ASSERT(root_map["road_distances"].AsDict().empty());
    }

    {
        std::stringstream input(
            "  {"
            "\"type\":   \"Stop\",   "
            "  \"road_distances\" :  {       },"
            "    \"longitude\"  :  -37.6517                        ,"
            "\"name\":\"Biryulyovo Zapadnoye\"  , "
            "\"latitude\":-55.574371      "
            "  }         \"");

        const Document doc = Load(input);

        Dict root_map = doc.GetRoot().AsDict();

        ASSERT_EQUAL(root_map["type"].AsString(), "Stop");
        ASSERT(std::abs(root_map["longitude"].AsDouble() - (-37.6517)) < 0.0000001);
        ASSERT_EQUAL(root_map["name"].AsString(), "Biryulyovo Zapadnoye");
        ASSERT(std::abs(root_map["latitude"].AsDouble() - (-55.574371)) < 0.0000001);
        ASSERT(root_map["road_distances"].AsDict().empty());
    }
}

std::string transport_catalogue::tests::Print(const json::Node& node) {
    std::ostringstream out;
    Print(json::Document{node}, out);
    return out.str();
}

json::Document transport_catalogue::tests::LoadJSON(const std::string& s) {
    std::istringstream strm(s);
    return json::Load(strm);
}

void TestJsonWrite() {
    using namespace json;

    {
        Node str_node{"[ 0, 1, 2, 3, 4 ]"s};
        const std::string expected = "[ 0, 1, 2, 3, 4 ]";

        assert(LoadJSON(Print(str_node)).GetRoot() == str_node);
    }
}

void tests::TestTransportCatalogue() {
    //  RUN_TEST(TestJson);
    // RUN_TEST(TestJsonWrite);
    // RUN_TEST(TestSpeedRandomDataSvgOutput);
    RUN_TEST(TestTranportRouter);
}