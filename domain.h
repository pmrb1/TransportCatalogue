#pragma once
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "geo.h"

namespace domain {

struct Stop {
    std::string name;
    geo::Coordinates position;
};
using StopPtr = const Stop*;

struct Bus {
    std::string name;
    std::vector<StopPtr> stops;
    std::vector<StopPtr> endpoints;
};
using BusPtr = const Bus*;

template <typename Object>
using Dictionary = std::map<std::string, const Object*>;

using StopsDict = Dictionary<Stop>;
using BusesDict = Dictionary<Bus>;

}  // namespace domain