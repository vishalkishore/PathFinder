#include "routes.hpp"
#include "json.hpp"

using json = nlohmann::json;

void setupRoutes(crow::SimpleApp& app, DataStorage& storage) {
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
    ([&storage](const crow::request& req) {
        try {
            auto body = json::parse(req.body);

            if (!body.contains("min_lat") || !body.contains("min_lon") ||
                !body.contains("max_lat") || !body.contains("max_lon")) {
                return crow::response(400, "Missing required coordinates");
            }

            BoundingBox bbox {
                body["min_lat"].get<double>(),
                body["min_lon"].get<double>(),
                body["max_lat"].get<double>(),
                body["max_lon"].get<double>()
            };

            if (bbox.min_lat > bbox.max_lat || bbox.min_lon > bbox.max_lon) {
                return crow::response(400, "Invalid coordinate ranges");
            }

            bool success = storage.loadMapData(bbox);

            if (!success) {
                return crow::response(500, "Failed to load map data");
            }

            json response = {
                {"status", "success"},
                {"message", "Map data loaded successfully"},
                {"bounds", {
                    {"min_lat", bbox.min_lat},
                    {"min_lon", bbox.min_lon},
                    {"max_lat", bbox.max_lat},
                    {"max_lon", bbox.max_lon}
                }},
                {"data", storage.response}
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

    // POST /start-dijkstra endpoint
    CROW_ROUTE(app, "/start-dijkstra")
    .methods(crow::HTTPMethod::POST)
    ([&storage](const crow::request& req) {
        try {
            auto body = json::parse(req.body);

            if (!body.contains("start_node_id") || !body.contains("end_node_id")) {
                return crow::response(400, "Missing start or end node ID");
            }

            std::string start_node_id = body["start_node_id"];
            std::string end_node_id = body["end_node_id"];

            if (!storage.validateNodeIds(start_node_id, end_node_id)) {
                return crow::response(400, "Invalid node ID(s)");
            }

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
