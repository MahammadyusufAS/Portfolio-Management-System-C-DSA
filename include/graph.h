#ifndef GRAPH_H
#define GRAPH_H

#include <stddef.h>

#define MAX_GRAPH_VERTICES 256

typedef struct GraphEdge {
    size_t dest_vertex;
    double correlation; /* Correlation coefficient [-1.0, +1.0] or sector link weight */
    char relationship[32];
    struct GraphEdge *next;
} GraphEdge;

typedef struct {
    char ticker[24];
    char sector[40];
    GraphEdge *head;
} GraphVertex;

typedef struct {
    GraphVertex vertices[MAX_GRAPH_VERTICES];
    size_t vertex_count;
} StockGraph;

void graph_init(StockGraph *g);
int graph_find_vertex(const StockGraph *g, const char *ticker, size_t *vertex_out);
int graph_add_vertex(StockGraph *g, const char *ticker, const char *sector, size_t *vertex_out);
int graph_add_edge(StockGraph *g, const char *ticker_a, const char *ticker_b,
                   double correlation, const char *relationship);
double graph_get_correlation(const StockGraph *g, const char *ticker_a, const char *ticker_b);
size_t graph_bfs_related(const StockGraph *g, const char *start_ticker, double min_correlation,
                         size_t *vertices_out, size_t max_out);
size_t graph_dfs_sector_cluster(const StockGraph *g, const char *start_ticker,
                                size_t *vertices_out, size_t max_out);
void graph_clear(StockGraph *g);

#endif /* GRAPH_H */
