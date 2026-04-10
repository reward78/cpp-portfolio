#include <iostream>
#include <vector>

struct Edge {
    int from;
    int to;
    int weight;
};

constexpr int kInfinity = 1000000000;

std::vector<int> BellmanFord(const std::vector<Edge>& edges, int vertex_count, int start) {
    std::vector<int> dist(vertex_count, kInfinity);
    dist[start] = 0;

    for (int iteration = 0; iteration < vertex_count - 1; ++iteration) {
        bool changed = false;
        for (const auto& edge : edges) {
            if (dist[edge.from] == kInfinity) {
                continue;
            }
            if (dist[edge.to] > dist[edge.from] + edge.weight) {
                dist[edge.to] = dist[edge.from] + edge.weight;
                changed = true;
            }
        }
        if (!changed) {
            break;
        }
    }

    return dist;
}

int main() {
    int vertex_count = 0;
    int edge_count = 0;
    std::cin >> vertex_count >> edge_count;

    std::vector<Edge> edges(edge_count);
    for (auto& edge : edges) {
        std::cin >> edge.from >> edge.to >> edge.weight;
    }

    int start = 0;
    std::cin >> start;

    const std::vector<int> dist = BellmanFord(edges, vertex_count, start);
    for (int i = 0; i < vertex_count; ++i) {
        if (i > 0) {
            std::cout << ' ';
        }
        if (dist[i] == kInfinity) {
            std::cout << "INF";
        } else {
            std::cout << dist[i];
        }
    }
    std::cout << '\n';

    return 0;
}
