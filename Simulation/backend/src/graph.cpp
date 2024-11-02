#include "graph.hpp"

// Load map data from Overpass API (mock implementation)
bool DataStorage::loadMapData(const BoundingBox& bbox) {
    cpr::Response ans = OverpassDataFetcher::fetchOverpassData(bbox);
    response = ans.text;
    map = json::parse(response);
    return !response.empty();
}

// Validate node IDs (mock implementation)
bool DataStorage::validateNodeIds(const std::string& start_id, const std::string& end_id) {
    // In a real implementation, this would check if the nodes exist in `response`
    return true;
}
