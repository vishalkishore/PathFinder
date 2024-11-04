#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <unordered_map>
#include <vector>
#include <utility>
#include <cstdint>
#include "json.hpp"
#define _USE_MATH_DEFINES
#include <cmath>

using json = nlohmann::json;

// Custom hash function for pair of int64_t
struct PairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

class Graph {
private:
    // Node storage: id -> {latitude, longitude}
    std::unordered_map<int64_t, std::pair<double, double>> nodes;
    
    // Edge storage: source_id -> vector of {destination_id, distance}
    std::unordered_map<int64_t, std::vector<std::pair<int64_t, double>>> edges;
    
    // Distance cache: {node1_id, node2_id} -> distance
    std::unordered_map<std::pair<int64_t, int64_t>, double, PairHash> distanceCache;

    // Private helper methods
    double haversineDistance(double lat1, double lon1, double lat2, double lon2) const;
    double heuristic(int64_t node, int64_t goal) const;

public:
    // Constructors
    Graph() = default;
    ~Graph() = default;

    // Disable copying to prevent accidental copies of large graphs
    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;

    // Enable moving
    Graph(Graph&&) = default;
    Graph& operator=(Graph&&) = default;

    // Main interface methods
    void loadFromJSON(const json& data);
    std::vector<int64_t> findPath(int64_t start, int64_t end);
    json getPathState() const;

    // Utility methods
    void clear() {
        nodes.clear();
        edges.clear();
        distanceCache.clear();
    }

    size_t getNodeCount() const { return nodes.size(); }
    size_t getEdgeCount() const { return edges.size(); }
    
    // Optional: Method to get node coordinates if needed
    std::pair<double, double> getNodeCoordinates(int64_t nodeId) const {
        auto it = nodes.find(nodeId);
        if (it != nodes.end()) {
            return it->second;
        }
        throw std::out_of_range("Node ID not found");
    }

    // Optional: Method to check if a path exists between nodes
    bool hasPath(int64_t start, int64_t end) const {
        return nodes.count(start) > 0 && nodes.count(end) > 0;
    }

    std::string printGraph() const;
    
    // Verify graph integrity and consistency
    bool verifyGraph() const;
};

// Optional: Helper struct for geographic coordinates
struct GeoCoord {
    double lat;
    double lon;

    GeoCoord(double latitude, double longitude) 
        : lat(latitude), lon(longitude) {
        if (lat < -90 || lat > 90 || lon < -180 || lon > 180) {
            throw std::invalid_argument("Invalid geographic coordinates");
        }
    }
};

// Optional: Helper struct for bounding box
// struct BoundingBox {
//     double min_lat;
//     double min_lon;
//     double max_lat;
//     double max_lon;

//     BoundingBox(double minLat, double minLon, double maxLat, double maxLon)
//         : min_lat(minLat), min_lon(minLon), max_lat(maxLat), max_lon(maxLon) {
//         if (min_lat > max_lat || min_lon > max_lon) {
//             throw std::invalid_argument("Invalid bounding box coordinates");
//         }
//     }

//     bool contains(const GeoCoord& point) const {
//         return point.lat >= min_lat && point.lat <= max_lat &&
//                point.lon >= min_lon && point.lon <= max_lon;
//     }
// };

#endif // GRAPH_HPP