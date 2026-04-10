#include <algorithm>
#include <iostream>
#include <vector>

constexpr int kInfinity = 1000000000;

std::vector<std::vector<int>> FloydWarshall(std::vector<std::vector<int>> dist) {
    const int vertex_count = static_cast<int>(dist.size());

    for (int through = 0; through < vertex_count; ++through) {
        for (int from = 0; from < vertex_count; ++from) {
            for (int to = 0; to < vertex_count; ++to) {
                if (dist[from][through] == kInfinity || dist[through][to] == kInfinity) {
                    continue;
                }
                dist[from][to] = std::min(dist[from][to], dist[from][through] + dist[through][to]);
            }
        }
    }

    return dist;
}

int main() {
    int vertex_count = 0;
    int edge_count = 0;
    std::cin >> vertex_count >> edge_count;

    std::vector<std::vector<int>> dist(vertex_count, std::vector<int>(vertex_count, kInfinity));
    for (int i = 0; i < vertex_count; ++i) {
        dist[i][i] = 0;
    }

    for (int i = 0; i < edge_count; ++i) {
        int from = 0;
        int to = 0;
        int weight = 0;
        std::cin >> from >> to >> weight;
        dist[from][to] = std::min(dist[from][to], weight);
    }

    const std::vector<std::vector<int>> answer = FloydWarshall(dist);
    for (int i = 0; i < vertex_count; ++i) {
        for (int j = 0; j < vertex_count; ++j) {
            if (j > 0) {
                std::cout << ' ';
            }
            if (answer[i][j] == kInfinity) {
                std::cout << "INF";
            } else {
                std::cout << answer[i][j];
            }
        }
        std::cout << '\n';
    }

    return 0;
}
