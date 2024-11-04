#include "routes.hpp"
#include "api.hpp"
#include "graph.hpp"
#include "json.hpp"
#include <algorithm>  
#include <vector>

using json = nlohmann::json;

void setupRoutes(crow::App<crow::CORSHandler>& app, Graph& graph) {
    // Root endpoint
    CROW_ROUTE(app, "/")([]() {
        json response = {
            {"message", "Hello from Crow!"},
            {"status", "success"}
        };
        return crow::response(response.dump());
    });

    // POST /bounding-box endpoint
    CROW_ROUTE(app, "/bounding-box")
    .methods(crow::HTTPMethod::POST)
    ([&graph](const crow::request& req) {
        try {
            auto body = json::parse(req.body);
            std::cout << "Received request body: " << body.dump(2) << "\n";

            // Validate request body is an array with exactly 2 elements
            if (!body.is_array() || body.size() != 2) {
                return crow::response(400, "Request body must be an array with exactly 2 coordinate objects");
            }

            // Validate coordinates
            for (const auto& item : body) {
                if (!item.contains("latitude") || !item.contains("longitude")) {
                    return crow::response(400, "Each coordinate object must contain 'latitude' and 'longitude'");
                }
                if (!item["latitude"].is_number() || !item["longitude"].is_number()) {
                    return crow::response(400, "Latitude and longitude must be numeric values");
                }
                
                // Validate coordinate ranges
                double lat = item["latitude"].get<double>();
                double lon = item["longitude"].get<double>();
                if (lat < -90 || lat > 90 || lon < -180 || lon > 180) {
                    return crow::response(400, "Coordinates out of valid range");
                }
            }

            // Create bounding box
            BoundingBox bbox {
                std::min(body[0]["latitude"].get<double>(), body[1]["latitude"].get<double>()),
                std::min(body[0]["longitude"].get<double>(), body[1]["longitude"].get<double>()),
                std::max(body[0]["latitude"].get<double>(), body[1]["latitude"].get<double>()),
                std::max(body[0]["longitude"].get<double>(), body[1]["longitude"].get<double>())
            };

            // Validate bounding box size
            // const double MAX_BBOX_SIZE = 0.1; // Maximum size in degrees
            // if (bbox.max_lat - bbox.min_lat > MAX_BBOX_SIZE || 
            //     bbox.max_lon - bbox.min_lon > MAX_BBOX_SIZE) {
            //     return crow::response(400, "Bounding box too large. Maximum size is 0.1 degrees");
            // }

            // Fetch OSM data
            std::cout << "Fetching OSM data..." << "\n";
            cpr::Response ans = OverpassDataFetcher::fetchOverpassData(bbox);
            
            if (ans.status_code != 200) {
                std::cout << "Overpass API error: " << ans.status_code << " - " << ans.text << "\n";
                return crow::response(500, "Failed to fetch OSM data: " + ans.text);
            }

            try {
                json osmData = json::parse(ans.text);
                std::cout << "Successfully parsed OSM data" << "\n";
                
                // // Clear existing graph data before loading new data
                // graph.clear(); // Assuming you have a clear method
                
                std::cout << "Loading graph data..." << std::endl;
                graph.loadFromJSON(osmData);
                std::cout << "Graph data loaded successfully" << std::endl;


                // Get graph state (could be sent to client)
                json state = graph.getPathState();
                std::cout << "Graph state: " << state.dump(2) << "\n";

                // Return success response without pathfinding for now
                json response = {
                    {"status", "success"},
                    {"message", "Map data loaded successfully"},
                    {"bounds", {
                        {"min_lat", bbox.min_lat},
                        {"min_lon", bbox.min_lon},
                        {"max_lat", bbox.max_lat},
                        {"max_lon", bbox.max_lon}
                    }},
                    // {"data", osmData},
                    // {"path", path},
                    {"state", state}
                };

                return crow::response(200, response.dump());
            } catch (const json::exception& e) {
                std::cout << "JSON parsing error: " << e.what() << "\n";
                return crow::response(500, "Failed to parse OSM data: " + std::string(e.what()));
            }
        }
        catch (const json::exception& e) {
            std::cout << "Request parsing error: " << e.what() << "\n";
            return crow::response(400, "Invalid JSON format: " + std::string(e.what()));
        }
        catch (const std::exception& e) {
            std::cout << "Unexpected error: " << e.what() << "\n";
            return crow::response(500, "Internal server error: " + std::string(e.what()));
        }
    });

    CROW_ROUTE(app, "/direct-path")
    .methods(crow::HTTPMethod::POST)
    ([&graph](const crow::request& req) {
        try {
            json body = json::parse(req.body);
            std::cout << "Received request body: " << body.dump(2) << std::endl;
            json startNode = body["start-node"];
            json endNode = body["end-node"];
            auto boundingBoxReq = body["bounding-box"];

            // Validate request body is an array with exactly 2 elements
            if (!boundingBoxReq.is_array() || boundingBoxReq.size() != 2) {
                return crow::response(400, "Request body must be an array with exactly 2 coordinate objects");
            }

            // Validate coordinates
            for (const auto& item : boundingBoxReq) {
                if (!item.contains("latitude") || !item.contains("longitude")) {
                    return crow::response(400, "Each coordinate object must contain 'latitude' and 'longitude'");
                }
                if (!item["latitude"].is_number() || !item["longitude"].is_number()) {
                    return crow::response(400, "Latitude and longitude must be numeric values");
                }
                
                // Validate coordinate ranges
                double lat = item["latitude"].get<double>();
                double lon = item["longitude"].get<double>();
                if (lat < -90 || lat > 90 || lon < -180 || lon > 180) {
                    return crow::response(400, "Coordinates out of valid range");
                }
            }

            // Create bounding box
            BoundingBox bbox {
                std::min(boundingBoxReq[0]["latitude"].get<double>(), boundingBoxReq[1]["latitude"].get<double>()),
                std::min(boundingBoxReq[0]["longitude"].get<double>(), boundingBoxReq[1]["longitude"].get<double>()),
                std::max(boundingBoxReq[0]["latitude"].get<double>(), boundingBoxReq[1]["latitude"].get<double>()),
                std::max(boundingBoxReq[0]["longitude"].get<double>(), boundingBoxReq[1]["longitude"].get<double>())
            };

            // Fetch OSM data
            std::cout << "Fetching OSM data..." << "\n";
            cpr::Response ans = OverpassDataFetcher::fetchOverpassData(bbox);
            
            if (ans.status_code != 200) {
                std::cout << "Overpass API error: " << ans.status_code << " - " << ans.text << "\n";
                return crow::response(500, "Failed to fetch OSM data: " + ans.text);
            }

            try {
                json osmData = json::parse(ans.text);
                std::cout << "Successfully parsed OSM data" << "\n";
                
                // // Clear existing graph data before loading new data
                // graph.clear(); // Assuming you have a clear method
                
                std::cout << "Loading graph data..." << std::endl;
                graph.loadFromJSON(osmData);
                graph.verifyGraph();
                std::vector<json> path = graph.findPath(startNode, endNode);

                // Return success response without pathfinding for now
                json response = {
                    {"status", "success"},
                    {"message", "Map data loaded successfully"},
                    {"bounds", {
                        {"min_lat", bbox.min_lat},
                        {"min_lon", bbox.min_lon},
                        {"max_lat", bbox.max_lat},
                        {"max_lon", bbox.max_lon}
                    }},
                    // {"data", osmData},
                    {"path", path},
                    // {"state", state}
                };

                return crow::response(200, response.dump());
            } catch (const json::exception& e) {
                std::cout << "JSON parsing error: " << e.what() << "\n";
                return crow::response(500, "Failed to parse OSM data: " + std::string(e.what()));
            }
        }
        catch (const json::exception& e) {
            std::cout << "Request parsing error: " << e.what() << "\n";
            return crow::response(400, "Invalid JSON format: " + std::string(e.what()));
        }
        catch (const std::exception& e) {
            std::cout << "Unexpected error: " << e.what() << "\n";
            return crow::response(500, "Internal server error: " + std::string(e.what()));
        }
    });

    // POST /start-dijkstra endpoint
    CROW_ROUTE(app, "/start-dijkstra")
    .methods(crow::HTTPMethod::POST)
    ([&graph](const crow::request& req) {
        try {
            auto body = json::parse(req.body);

            if (!body.contains("start_node_id") || !body.contains("end_node_id")) {
                return crow::response(400, "Missing start or end node ID");
            }

            std::string start_node_id = body["start_node_id"];
            std::string end_node_id = body["end_node_id"];

            // if (!storage.validateNodeIds(start_node_id, end_node_id)) {
            //     return crow::response(400, "Invalid node ID(s)");
            // }

            json response = {
                {"status", "success"},
                {"message", "Pathfinding initiated"},
                {"websocket_url", "ws://localhost:8080/exploration"},
                {"request_id", "unique-request-id"},
                {"start_node", start_node_id},
                {"end_node", end_node_id}
            };

            return crow::response(200, response.dump());
        }
        catch (const json::exception& e) {
            return crow::response(400, "Invalid JSON format");
        }
        catch (const std::exception& e) {
            return crow::response(500, e.what());
        }
    });
}
