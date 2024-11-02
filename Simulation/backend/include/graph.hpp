#ifndef GRAPH_HPP
#define GRAPH_HPP

#include "api.hpp"
#include <string>
#include <cpr/cpr.h>
#include "json.hpp"

using json = nlohmann::json;

class DataStorage {
private:
    json map;
public:
    std::string response;

    bool loadMapData(const BoundingBox& bbox);
    bool validateNodeIds(const std::string& start_id, const std::string& end_id);
};

#endif // DATA_STORAGE_HPP
