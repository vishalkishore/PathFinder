// routes.hpp
#ifndef ROUTES_HPP
#define ROUTES_HPP

#include "crow.h"
#include "graph.hpp"

// Define route handlers
void setupRoutes(crow::SimpleApp& app, DataStorage& storage);

#endif // ROUTES_HPP
