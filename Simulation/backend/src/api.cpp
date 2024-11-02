
#include "api.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

const std::vector<std::string> OverpassDataFetcher::highWayExclude = {
    "footway", "street_lamp", "steps", "pedestrian", "track", "path"
};

cpr::Response OverpassDataFetcher::fetchOverpassData(const BoundingBox& boundingBox) {
    // Construct exclusion string
    std::stringstream exclusionStream;
    for (const auto& exclude : highWayExclude) {
        exclusionStream << "[highway!=\"" << exclude << "\"]";
    }
    std::string exclusion = exclusionStream.str();

    // Construct Overpass query
    std::stringstream queryStream;
    queryStream << "[out:json];("
               << "way[highway]" << exclusion << "[footway!=\"*\"]"
               << "(" 
               << boundingBox.min_lat << "," 
               << boundingBox.min_lon << "," 
               << boundingBox.max_lat << "," 
               << boundingBox.min_lon 
               << ");"
               << "node(w);"
               << ");out skel;";

    std::string query = queryStream.str();

    // Make POST request using CPR library
    cpr::Response response = cpr::Post(
        cpr::Url{"https://overpass-api.de/api/interpreter"},
        cpr::Body{query}
    );

    // Print the response
    std::cout << "Status Code: " << response.status_code << std::endl;
    std::cout << "Response Text: " << response.text << std::endl;

    return response;
}