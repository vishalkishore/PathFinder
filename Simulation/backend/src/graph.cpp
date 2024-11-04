#include "graph.hpp"
#include <queue>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <iostream>

// Haversine distance calculation
double Graph::haversineDistance(double lat1, double lon1, double lat2, double lon2) const {
    const double R = 6371000; // Earth radius in meters
    const double phi1 = lat1 * M_PI / 180;
    const double phi2 = lat2 * M_PI / 180;
    const double deltaPhi = (lat2 - lat1) * M_PI / 180;
    const double deltaLambda = (lon2 - lon1) * M_PI / 180;

    const double a = std::sin(deltaPhi/2) * std::sin(deltaPhi/2) +
                    std::cos(phi1) * std::cos(phi2) *
                    std::sin(deltaLambda/2) * std::sin(deltaLambda/2);
    const double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));

    return R * c;
}

// A* heuristic function
double Graph::heuristic(int64_t node, int64_t goal) const {
    try {
        const auto& node_coords = nodes.at(node);
        const auto& goal_coords = nodes.at(goal);
        return haversineDistance(
            node_coords.first, node_coords.second,
            goal_coords.first, goal_coords.second
        );
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Invalid node ID in heuristic calculation");
    }
}

void Graph::loadFromJSON(const json& data) {
    try {
        // Clear existing data
        clear();

        // Pre-allocate space
        auto& elements = data["elements"];
        nodes.reserve(elements.size());
        
        // First pass: Load all nodes
        for (auto& element : elements) {
            if (element["type"] == "node") {
                int64_t id = element["id"];
                double lat = element["lat"];
                double lon = element["lon"];
                // Validate coordinates
                if (lat < -90 || lat > 90 || lon < -180 || lon > 180) {
                    throw std::invalid_argument("Invalid coordinates in node " + std::to_string(id));
                }
                
                nodes[id] = {lat, lon};
            }
        }

        // Second pass: Create edges
        size_t edge_count = 0;
        for (const auto& element : elements) {
            if (element["type"] == "way") {
                const auto& nodeRefs = element["nodes"];
                const size_t size = nodeRefs.size();
                
                // Skip ways with less than 2 nodes
                if (size < 2) continue;
                
                // Pre-count edges for this way
                edge_count += size - 1;
                
                // Create edges between consecutive nodes
                for (size_t i = 0; i < size - 1; ++i) {
                    int64_t src = nodeRefs[i];
                    int64_t dst = nodeRefs[i + 1];
                    
                    // Validate node existence
                    if (!nodes.count(src) || !nodes.count(dst)) {
                        continue; // Skip invalid node references
                    }
                    
                    const auto& src_coords = nodes[src];
                    const auto& dst_coords = nodes[dst];
                    
                    // Calculate and cache distance
                    auto distance = haversineDistance(
                        src_coords.first, src_coords.second,
                        dst_coords.first, dst_coords.second
                    );
                    
                    // Create bidirectional edges
                    edges[src].push_back({dst, distance});
                    edges[dst].push_back({src, distance});
                    
                    // Cache distances for both directions
                    distanceCache[{src, dst}] = distance;
                    distanceCache[{dst, src}] = distance;
                }
            }
        }

        // Reserve space for edge vectors based on average connectivity
        if (!nodes.empty()) {
            size_t avg_edges_per_node = (edge_count * 2) / nodes.size();
            for (auto& [_, edge_vec] : edges) {
                edge_vec.reserve(avg_edges_per_node);
            }
        }

    } catch (const json::exception& e) {
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error loading graph: " + std::string(e.what()));
    }
}

std::vector<int64_t> Graph::findPath(int64_t start, int64_t end) {
    // Validate input nodes
    if (!nodes.count(start) || !nodes.count(end)) {
        return {};
    }

    // Custom comparator for the priority queue
    struct CompareNode {
        bool operator()(const std::pair<double, int64_t>& a, 
                       const std::pair<double, int64_t>& b) const {
            return a.first > b.first;
        }
    };

    // Initialize data structures
    std::unordered_map<int64_t, double> gScore;
    std::unordered_map<int64_t, int64_t> prev;
    std::priority_queue<
        std::pair<double, int64_t>,
        std::vector<std::pair<double, int64_t>>,
        CompareNode
    > pq;

    // Initialize scores
    gScore.reserve(nodes.size());
    for (const auto& node : nodes) {
        gScore[node.first] = std::numeric_limits<double>::infinity();
    }
    gScore[start] = 0;

    // Initialize priority queue with start node
    pq.push({heuristic(start, end), start});

    // A* algorithm
    while (!pq.empty()) {
        int64_t current = pq.top().second;
        pq.pop();

        // Found the destination
        if (current == end) break;

        // Look at all neighbors
        auto it = edges.find(current);
        if (it == edges.end()) continue;

        for (const auto& [next, weight] : it->second) {
            double newScore = gScore[current] + weight;
            
            if (newScore < gScore[next]) {
                prev[next] = current;
                gScore[next] = newScore;
                double priority = newScore + heuristic(next, end);
                pq.push({priority, next});
            }
        }
    }

    // Check if path exists
    if (gScore[end] == std::numeric_limits<double>::infinity()) {
        return {};
    }

    // Reconstruct path
    std::vector<int64_t> path;
    path.reserve(nodes.size() / 4);  // Reasonable initial capacity
    
    for (int64_t at = end; at != start; ) {
        auto it = prev.find(at);
        if (it == prev.end()) return {};  // No path exists
        path.push_back(at);
        at = it->second;
    }
    path.push_back(start);
    
    std::reverse(path.begin(), path.end());
    return path;
}

json Graph::getPathState() const {
    json state = {
        {"node_count", nodes.size()},
        {"edge_count", edges.size()},
        {"cache_size", distanceCache.size()}
    };

    // Add some basic statistics
    if (!nodes.empty()) {
        size_t total_edges = 0;
        size_t max_edges = 0;
        for (const auto& [_, edge_list] : edges) {
            total_edges += edge_list.size();
            max_edges = std::max(max_edges, edge_list.size());
        }

        state["average_edges_per_node"] = static_cast<double>(total_edges) / nodes.size();
        state["max_edges_per_node"] = max_edges;
    }

    return state;
}


std::string Graph::printGraph() const {
    std::stringstream ss;
    ss << "Graph Structure:\n";
    ss << "===============\n\n";
    
    // Print nodes
    ss << "Nodes (" << nodes.size() << " total):\n";
    ss << "-----------------\n";
    for (const auto& [id, coords] : nodes) {
        ss << "Node " << id << ": ("
           << std::fixed << std::setprecision(6) 
           << coords.first << ", " << coords.second << ")\n";
    }
    
    // Print edges
    ss << "\nEdges:\n";
    ss << "-----------------\n";
    for (const auto& [src, destinations] : edges) {
        ss << "From Node " << src << ":\n";
        for (const auto& [dst, distance] : destinations) {
            ss << "  → Node " << dst 
               << " (distance: " << std::fixed << std::setprecision(2) 
               << distance << "m)\n";
        }
    }
    
    // Print some statistics
    ss << "\nGraph Statistics:\n";
    ss << "-----------------\n";
    ss << "Total Nodes: " << nodes.size() << "\n";
    ss << "Total Edges: " << edges.size() << "\n";
    ss << "Cached Distances: " << distanceCache.size() << "\n";
    
    // Calculate average connectivity
    if (!nodes.empty()) {
        size_t total_edges = 0;
        size_t max_edges = 0;
        for (const auto& [_, edge_list] : edges) {
            total_edges += edge_list.size();
            max_edges = std::max(max_edges, edge_list.size());
        }
        double avg_edges = static_cast<double>(total_edges) / nodes.size();
        ss << "Average Edges per Node: " << std::fixed << std::setprecision(2) 
           << avg_edges << "\n";
        ss << "Max Edges for a Node: " << max_edges << "\n";
    }
    
    return ss.str();
}

bool Graph::verifyGraph() const {
    // Check for invalid coordinates
    for (const auto& [id, coords] : nodes) {
        if (coords.first < -90 || coords.first > 90 ||
            coords.second < -180 || coords.second > 180) {
            std::cout << "Invalid coordinates for node " << id << std::endl;
            return false;
        }
    }
    
    // Verify edge consistency
    for (const auto& [src, destinations] : edges) {
        // Check if source node exists
        if (!nodes.count(src)) {
            std::cout << "Edge references non-existent source node " << src << std::endl;
            return false;
        }
        
        // Check each destination
        for (const auto& [dst, distance] : destinations) {
            if (!nodes.count(dst)) {
                std::cout << "Edge references non-existent destination node " << dst << std::endl;
                return false;
            }
            
            // Verify distance is positive and reasonable
            if (distance <= 0 || distance > 1000000) { // 1000km seems reasonable max
                std::cout << "Suspicious distance " << distance 
                         << "m between nodes " << src << " and " << dst << std::endl;
                return false;
            }
            
            // Verify bidirectional edge exists
            bool found_reverse = false;
            auto it = edges.find(dst);
            if (it != edges.end()) {
                for (const auto& [rev_dst, rev_dist] : it->second) {
                    if (rev_dst == src) {
                        found_reverse = true;
                        // Check if distances match
                        if (std::abs(rev_dist - distance) > 0.01) {
                            std::cout << "Inconsistent distances for bidirectional edge "
                                     << src << " <-> " << dst << std::endl;
                            return false;
                        }
                        break;
                    }
                }
            }
            if (!found_reverse) {
                std::cout << "Missing reverse edge for " << src << " -> " << dst << std::endl;
                return false;
            }
        }
    }
    
    return true;
}