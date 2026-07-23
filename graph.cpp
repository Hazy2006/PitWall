#include "graph.h"
#include <stdexcept>
#include <vector>
#include <queue>
#include <unordered_set>
#include <stack>
#include <algorithm>
#include <limits>

Graph::Graph() : next_id(0) {}

int Graph::count_vertices() const {
    return static_cast<int>(node_registry.size());
}

int Graph::count_edges() const {
    return static_cast<int>(edges.size());
}

int Graph::add_node(std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Cannot add a null node pointer.");
    }
    int id = next_id++;
    node_registry[id] = node;
    successors[id] = std::vector<int>();
    predecessors[id] = std::vector<int>();
    return id;
}

int Graph::add_node_with_id(int id, std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Cannot add a null node pointer.");
    }
    if (node_registry.count(id) != 0) {
        throw std::invalid_argument("Node ID is already taken.");
    }
    node_registry[id] = node;
    successors[id] = std::vector<int>();
    predecessors[id] = std::vector<int>();
    next_id = std::max(next_id, id + 1);
    return id;
}

std::shared_ptr<Node> Graph::get_node(int id) const {
    if (node_registry.count(id) == 0) {
        throw std::invalid_argument("Node ID does not exist.");
    }
    return node_registry.at(id);
}

void Graph::remove_node(int id) {
    if (node_registry.count(id) == 0) {
        throw std::invalid_argument("Node ID does not exist.");
    }

    for (int succ : successors[id]) {
        edges.erase({ id, succ });
    }
    for (int pred : predecessors[id]) {
        edges.erase({ pred, id });
    }

    for (auto& entry : successors) {
        auto& neighbors = entry.second;
        neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), id), neighbors.end());
    }
    for (auto& entry : predecessors) {
        auto& neighbors = entry.second;
        neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), id), neighbors.end());
    }

    successors.erase(id);
    predecessors.erase(id);
    node_registry.erase(id);
}

bool Graph::is_edge(int x, int y) const {
    if (node_registry.count(x) == 0 || node_registry.count(y) == 0) {
        throw std::invalid_argument("One or both vertices do not exist!");
    }
    return edges.count({ x, y }) > 0;
}

void Graph::add_edge(int x, int y, const Edge& edge_data) {
    if (node_registry.count(x) == 0 || node_registry.count(y) == 0) {
        throw std::invalid_argument("One or both vertices do not exist!");
    }
    if (is_edge(x, y)) {
        return;
    }

    successors[x].push_back(y);
    predecessors[y].push_back(x);
    edges[{x, y}] = edge_data;
}

Edge Graph::get_edge(int x, int y) const {
    if (!is_edge(x, y)) {
        throw std::invalid_argument("Edge does not exist!");
    }
    return edges.at({ x, y });
}

std::vector<int> Graph::bfs(int start_node_id) const {
    if (node_registry.count(start_node_id) == 0) {
        throw std::invalid_argument("Start node does not exist!");
    }

    std::vector<int> result;
    std::queue<int> q;
    std::unordered_set<int> seen;

    q.push(start_node_id);
    seen.insert(start_node_id);

    while (!q.empty()) {
        int cur = q.front();
        q.pop();
        result.push_back(cur);

        auto it = successors.find(cur);
        if (it != successors.end()) {
            for (int nb : it->second) {
                if (seen.insert(nb).second) {
                    q.push(nb);
                }
            }
        }
    }

    return result;
}

std::vector<int> Graph::dfs(int start_node_id) const {
    if (node_registry.count(start_node_id) == 0) {
        throw std::invalid_argument("Start node does not exist!");
    }

    std::vector<int> result;
    std::stack<int> stk;
    std::unordered_set<int> seen;

    stk.push(start_node_id);

    while (!stk.empty()) {
        int cur = stk.top();
        stk.pop();

        if (!seen.insert(cur).second) continue;
        result.push_back(cur);

        auto it = successors.find(cur);
        if (it != successors.end()) {
            const auto& nbrs = it->second;
            for (auto rit = nbrs.rbegin(); rit != nbrs.rend(); ++rit) {
                if (seen.count(*rit) == 0) {
                    stk.push(*rit);
                }
            }
        }
    }

    return result;
}

std::vector<int> Graph::get_all_node_ids() const {
    std::vector<int> ids;
    ids.reserve(node_registry.size());
    for (const auto& entry : node_registry) {
        ids.push_back(entry.first);
    }
    return ids;
}

std::vector<std::pair<int, int>> Graph::get_all_edge_keys() const {
    std::vector<std::pair<int, int>> keys;
    keys.reserve(edges.size());
    for (const auto& entry : edges) {
        keys.push_back(entry.first);
    }
    return keys;
}

std::vector<int> Graph::dijkstra(int start, int end) const {
    if (node_registry.count(start) == 0 || node_registry.count(end) == 0) {
        throw std::invalid_argument("One or both vertices do not exist!");
    }

    std::unordered_map<int, double> dist;
    std::unordered_map<int, int> prev;
    std::priority_queue<std::pair<double, int>,
        std::vector<std::pair<double, int>>,
        std::greater<std::pair<double, int>>> pq;

    dist[start] = 0.0;
    pq.push({ 0.0, start });

    while (!pq.empty()) {
        auto [d, cur] = pq.top();
        pq.pop();

        if (dist.count(cur) && d > dist[cur]) {
            continue;
        }

        if (cur == end) {
            break;
        }

        auto it = successors.find(cur);
        if (it != successors.end()) {
            for (int nb : it->second) {
                double weight = edges.at({ cur, nb }).affinityScore;
                double new_dist = d + weight;
                if (dist.count(nb) == 0 || new_dist < dist[nb]) {
                    dist[nb] = new_dist;
                    prev[nb] = cur;
                    pq.push({ new_dist, nb });
                }
            }
        }
    }

    if (dist.count(end) == 0) {
        return {};
    }

    std::vector<int> path;
    for (int at = end; ; at = prev[at]) {
        path.push_back(at);
        if (at == start) break;
    }
    std::reverse(path.begin(), path.end());
    return path;
}