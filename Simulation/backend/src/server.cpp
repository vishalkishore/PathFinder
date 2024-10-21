// server.cpp
#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <nlohmann/json.hpp>

using namespace Pistache;
using namespace Pistache::Rest;
using json = nlohmann::json;

void handleHello(const Rest::Request&, Http::ResponseWriter response) {
    response.send(Http::Code::Ok, "Hello from Pistache!\n");
}

void handleJson(const Rest::Request&, Http::ResponseWriter response) {
    json responseObject = {
        {"message", "Hello from Pistache in JSON in compose!"}
    };
    response.send(Http::Code::Ok, responseObject.dump(), MIME(Application, Json));
}

void setupRoutes(Router& router) {
    Routes::Get(router, "/hello", Routes::bind(&handleHello));
    Routes::Get(router, "/json", Routes::bind(&handleJson));
}

int main() {
    Http::Endpoint server(Address(Ipv4::any(), Port(9080)));
    
    auto opts = Http::Endpoint::options().threads(1);
    server.init(opts);

    Router router;
    setupRoutes(router);

    server.setHandler(router.handler());
    server.serve();

    return 0;
}