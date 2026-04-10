#include <iostream>
#include <set>
#include <utility>
#include <vector>

constexpr int kInfinity = 1000000000;

std::vector<int> Dijkstra(const std::vector<std::vector<std::pair<int, int>>>& graph, int start) {
    std::vector<int> dist(graph.size(), kInfinity);
    std::set<std::pair<int, int>> queue;

    dist[start] = 0;
    queue.insert({0, start});

    while (!queue.empty()) {
        const int vertex = queue.begin()->second;
        queue.erase(queue.begin());

        for (const auto& [to, weight] : graph[vertex]) {
            if (dist[to] > dist[vertex] + weight) {
                queue.erase({dist[to], to});
                dist[to] = dist[vertex] + weight;
                queue.insert({dist[to], to});
            }
        }
    }

    return dist;
}

int main() {
    int vertex_count = 0;
    int edge_count = 0;
    std::cin >> vertex_count >> edge_count;

    std::vector<std::vector<std::pair<int, int>>> graph(vertex_count);
    for (int i = 0; i < edge_count; ++i) {
        int from = 0;
        int to = 0;
        int weight = 0;
        std::cin >> from >> to >> weight;
        graph[from].push_back({to, weight});
    }

    int start = 0;
    std::cin >> start;

    const std::vector<int> dist = Dijkstra(graph, start);
    for (int i = 0; i < vertex_count; ++i) {
        if (i > 0) {
            std::cout << ' ';
        }
        std::cout << dist[i];
    }
    std::cout << '\n';

    return 0;
}
