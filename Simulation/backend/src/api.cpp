
#include "api.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

/**
 * Fetch Overpass data for a given bounding box
 * Response has key elements which containns array of OSMNode objects
 * @typedef {Object} OSMNode
 * @property {String} type
 * @property {Number} id
 * @property {Number} lat
 * @property {Number} lon
 * we are returning as string, further processing is required
 */

const std::vector<std::string> OverpassDataFetcher::highWayExclude = {
    /**
     * Exclude these highway types from the Overpass query
     */
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
               << boundingBox.max_lon 
               << ");"
               << "node(w);"
               << ");out skel;";

    std::string query = queryStream.str();
    std::cout << "Overpass Query: " << query << std::endl;
    // Make POST request using CPR library
    cpr::Response response = cpr::Post(
        cpr::Url{"https://overpass-api.de/api/interpreter"},
        cpr::Body{query}
    );

    // Print the response
    std::cout << "Status Code: " << response.status_code << std::endl;

    return response;
}