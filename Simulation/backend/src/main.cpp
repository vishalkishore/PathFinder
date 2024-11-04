#include "crow.h"
#include "graph.hpp"
#include "routes.hpp"
#include "crow/middlewares/cors.h"

int main() {
    crow::App<crow::CORSHandler> app;
    
    // CORS configuration
    auto& cors = app.get_middleware<crow::CORSHandler>();
    
    // Configure CORS
    cors
        .global()
        .headers("Content-Type", "Authorization", "Accept")
        .methods("POST"_method, "GET"_method, "PUT"_method, "DELETE"_method, "OPTIONS"_method)
        .prefix("/")
        .origin("http://localhost:5173")
        .allow_credentials();
    
    // Initialize graph
    Graph graph;

    // Set up routes
    setupRoutes(app, graph);

    // Configure and run the application
    app.port(8080)
       .multithreaded()
       .run();

    return 0;
}