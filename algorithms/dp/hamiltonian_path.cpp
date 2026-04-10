#include <algorithm>
#include <iostream>
#include <vector>

int main() {
    int vertex_count = 0;
    int edge_count = 0;
    std::cin >> vertex_count >> edge_count;

    std::vector<std::vector<int>> graph(vertex_count);
    for (int i = 0; i < edge_count; ++i) {
        int from = 0;
        int to = 0;
        std::cin >> from >> to;
        graph[from].push_back(to);
    }

    const int full_mask_size = 1 << vertex_count;
    std::vector<std::vector<bool>> dp(full_mask_size, std::vector<bool>(vertex_count, false));
    std::vector<std::vector<int>> parent(full_mask_size, std::vector<int>(vertex_count, -1));

    for (int vertex = 0; vertex < vertex_count; ++vertex) {
        dp[1 << vertex][vertex] = true;
    }

    for (int mask = 0; mask < full_mask_size; ++mask) {
        for (int vertex = 0; vertex < vertex_count; ++vertex) {
            if (!dp[mask][vertex]) {
                continue;
            }
            for (int to : graph[vertex]) {
                if ((mask & (1 << to)) != 0) {
                    continue;
                }

                const int new_mask = mask | (1 << to);
                if (!dp[new_mask][to]) {
                    dp[new_mask][to] = true;
                    parent[new_mask][to] = vertex;
                }
            }
        }
    }

    const int full_mask = full_mask_size - 1;
    int end_vertex = -1;
    for (int vertex = 0; vertex < vertex_count; ++vertex) {
        if (dp[full_mask][vertex]) {
            end_vertex = vertex;
            break;
        }
    }

    if (end_vertex == -1) {
        std::cout << "Hamiltonian path does not exist\n";
        return 0;
    }

    std::vector<int> path;
    int mask = full_mask;
    int vertex = end_vertex;
    while (vertex != -1) {
        path.push_back(vertex);
        const int previous_vertex = parent[mask][vertex];
        mask ^= (1 << vertex);
        vertex = previous_vertex;
    }

    std::reverse(path.begin(), path.end());

    std::cout << "Hamiltonian path:\n";
    for (int i = 0; i < static_cast<int>(path.size()); ++i) {
        if (i > 0) {
            std::cout << ' ';
        }
        std::cout << path[i];
    }
    std::cout << '\n';

    return 0;
}
