#pragma once
#include "graph.h"
#include "storage.h"

class GraphRepository {
private:
    Storage& storage;

public:
    explicit GraphRepository(Storage& storage_ref);

    void create_tables();
    void save_graph(const Graph& g);
    Graph load_graph();
};
