#include <iostream>
#include <fstream>
#include <vector>
#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <nlohmann/json.hpp>

using namespace Pistache;
using json = nlohmann::json;

// Utility function to find the vertex with minimum distance value
int minDistance(const std::vector<int> &dist, const std::vector<bool> &sptSet, int V) {
    int min = std::numeric_limits<int>::max(), min_index = -1;
    for (int v = 0; v < V; v++) {
        if (!sptSet[v] && dist[v] <= min) {
            min = dist[v];
            min_index = v;
        }
    }
    return min_index;
}

// Dijkstra's algorithm with JSON logging
void dijkstra(const std::vector<std::vector<int>> &graph, int src, std::vector<int> &distances, json &iterations) {
    int V = graph.size();
    distances.assign(V, std::numeric_limits<int>::max());
    std::vector<bool> sptSet(V, false);

    distances[src] = 0;

    for (int count = 0; count < V - 1; count++) {
        int u = minDistance(distances, sptSet, V);
        sptSet[u] = true;

        json iteration;
        iteration["current_node"] = u;
        json neighbors = json::array();
        json updated_distances = json::array();

        for (int v = 0; v < V; v++) {
            if (!sptSet[v] && graph[u][v] && distances[u] != std::numeric_limits<int>::max()
                && distances[u] + graph[u][v] < distances[v]) {
                distances[v] = distances[u] + graph[u][v];
                neighbors.push_back(1);
            } else {
                neighbors.push_back(0);
            }
            updated_distances.push_back(distances[v]);
        }

        iteration["neighbors"] = neighbors;
        iteration["updated_distances"] = updated_distances;
        iterations.push_back(iteration);
    }
}

// CORS Headers setup
void setupCORSHeaders(Http::ResponseWriter& response) {
    response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
    response.headers().add<Http::Header::AccessControlAllowMethods>("GET, POST, OPTIONS");
    response.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type");
}

// Handler for OPTIONS requests (CORS preflight)
void handleOptions(const Rest::Request& request, Http::ResponseWriter response) {
    setupCORSHeaders(response);
    response.send(Http::Code::Ok);
}

// Handler for the API endpoint that runs Dijkstra's algorithm
void handleDijkstra(const Rest::Request& request, Http::ResponseWriter response) {
    setupCORSHeaders(response);
    
    try {
        auto body = json::parse(request.body());
        std::vector<std::vector<int>> graph;

        for (const auto& row : body) {
            std::vector<int> temp;
            for (const auto& elem : row) {
                temp.push_back(elem);
            }
            graph.push_back(temp);
        }

        // Apply Dijkstra's algorithm
        std::vector<int> distances;
        json iterations = json::array();
        dijkstra(graph, 0, distances, iterations);

        // Prepare the response JSON
        json response_json;
        response_json["iterations"] = iterations;
        response_json["final_distances"] = distances;
        std::cout << response_json.dump() << std::endl;
        response.send(Http::Code::Ok, response_json.dump(), MIME(Application, Json));
    } catch (const std::exception &e) {
        response.send(Http::Code::Bad_Request, "Invalid JSON format");
    }
}

// Set up the routes
void setupRoutes(Rest::Router& router) {
    using namespace Rest;

    // Handle CORS preflight requests
    Routes::Options(router, "/api/dijkstra", Routes::bind(&handleOptions));

    // Route for running Dijkstra's algorithm
    Routes::Post(router, "/api/dijkstra", Routes::bind(&handleDijkstra));
}

int main() {
    // Allow binding to all interfaces (important for Docker)
    Http::Endpoint server(Address(Ipv4::any(), Port(9080)));

    // Initialize the server
    auto opts = Http::Endpoint::options()
        .threads(2)
        .flags(Tcp::Options::ReuseAddr);
    server.init(opts);

    // Set up the router
    Rest::Router router;
    setupRoutes(router);

    // Bind the router to the server
    server.setHandler(router.handler());

    std::cout << "Server is listening on http://0.0.0.0:9080" << std::endl;

    // Start the server
    server.serve();

    return 0;
}