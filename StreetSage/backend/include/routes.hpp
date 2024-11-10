// routes.hpp
#ifndef ROUTES_HPP
#define ROUTES_HPP

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "graph.hpp"

// Define route handlers
void setupRoutes(crow::App<crow::CORSHandler>& app, Graph& graph);

#endif // ROUTES_HPP
