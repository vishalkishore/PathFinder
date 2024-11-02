// main.cpp
#include "crow.h"
#include "graph.hpp"
#include "routes.hpp"


int main() {
    crow::SimpleApp app;
    DataStorage storage;

    // Set up routes
    setupRoutes(app, storage);

    // Configure and run the application
    app.port(8080)
       .multithreaded()
       .run();

    return 0;
}
