#include <chrono>
#include <cpprest/filestream.h>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <thread>
#include <vector>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

// A utility function to find the vertex with minimum distance value, from
// the set of vertices not yet processed.
int minDistance(const std::vector<int> &dist, const std::vector<bool> &sptSet,
                int V) {
  int min = std::numeric_limits<int>::max(), min_index = -1;
  for (int v = 0; v < V; v++)
    if (!sptSet[v] && dist[v] <= min)
      min = dist[v], min_index = v;
  return min_index;
}

void dijkstra(const std::vector<std::vector<int>> &graph, int src,
              std::vector<int> &distances, json::value &iterations) {
  int V = graph.size();
  distances.assign(V, std::numeric_limits<int>::max());
  std::vector<bool> sptSet(V, false); // sptSet[i] will be true if vertex i is
                                      // included in shortest path tree

  distances[src] = 0; // Distance of source vertex from itself is always 0

  for (int count = 0; count < V - 1; count++) {
    // Pick the minimum distance vertex from the set of vertices not yet
    // processed.
    int u = minDistance(distances, sptSet, V);
    sptSet[u] = true;

    // Record the current iteration
    json::value iteration = json::value::object();
    iteration["current_node"] = json::value::number(u);
    json::value neighbors = json::value::array();
    json::value updated_distances = json::value::array();

    // Update dist value of the adjacent vertices of the picked vertex.
    for (int v = 0; v < V; v++) {
      // Update dist[v] only if it's not in sptSet, there is an edge from u to
      // v, and total weight of path from src to v through u is smaller than the
      // current value of dist[v]
      if (!sptSet[v] && graph[u][v] &&
          distances[u] != std::numeric_limits<int>::max() &&
          distances[u] + graph[u][v] < distances[v]) {
        distances[v] = distances[u] + graph[u][v];
        neighbors[v] = json::value::number(1);
      } else {
        neighbors[v] = json::value::number(0);
      }

      // Track the neighbors visited and their updated distances

      updated_distances[v] = json::value::number(distances[v]);
    }

    iteration["neighbors"] = neighbors;
    iteration["updated_distances"] = updated_distances;
    iterations[count] = iteration; // Store the iteration details
  }
}

/* void handle_get(http_request request) {
  std::ifstream file("../../frontend/index.html");
  if (file) {
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    request.reply(status_codes::OK, content, "text/html");
  } else {
    request.reply(status_codes::NotFound, "File not found");
  }
} */

void handle_get(http_request request) {
  std::string path = request.relative_uri().path();
  std::ifstream file;

  if (path == "/") {
    // Serve index.html for the root URL
    file.open("../../frontend/index.html");
  } else if (path == "/script.js") {
    // Serve script.js for the script.js URL
    file.open("../../frontend/script.js");
  }

  if (file) {
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    request.reply(status_codes::OK, content,
                  path == "/" ? "text/html" : "application/javascript");
  } else {
    request.reply(status_codes::NotFound, "File not found");
  }
}

void handle_post(http_request request) {
  request.extract_json()
      .then([](json::value body) {
        // Parse the adjacency matrix from the JSON body
        std::vector<std::vector<int>> graph;
        for (auto &row : body.as_array()) {
          std::vector<int> temp;
          for (auto &elem : row.as_array()) {
            temp.push_back(elem.as_integer());
          }
          graph.push_back(temp);
        }

        // Apply Dijkstra's algorithm
        std::vector<int> distances;
        json::value iterations =
            json::value::array(); // To store all iterations
        dijkstra(graph, 0, distances,
                 iterations); // Assume node 0 as the source

        // Prepare the response JSON
        json::value response = json::value::object();
        json::value final_distances = json::value::array();
        for (size_t i = 0; i < distances.size(); ++i) {
          final_distances[i] = json::value::number(distances[i]);
        }
        response["iterations"] = iterations;
        response["final_distances"] = final_distances;

        return response;
      })
      .then([&request](json::value response) {
        request.reply(status_codes::OK, response);
      });
}

int main() {
  http_listener listener("http://localhost:8080");

  listener.support(methods::GET, handle_get);
  listener.support(methods::POST, handle_post);

  try {
    listener.open().wait();
    std::cout << "Listening on http://localhost:8080" << std::endl;
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }

  return 0;
}
