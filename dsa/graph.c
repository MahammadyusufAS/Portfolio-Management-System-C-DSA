#include "graph.h"
#include "stock.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 * Adjacency List Graph for Stock Relationships & Sector Correlations
 * Models pairwise correlations and sector linkages between stocks, supporting
 * Breadth-First Search (BFS) and Depth-First Search (DFS) cluster discovery.
 */

void graph_init(StockGraph *g) {
    if (!g) return;
    memset(g, 0, sizeof(*g));
}

int graph_find_vertex(const StockGraph *g, const char *ticker, size_t *vertex_out) {
    if (!g || !ticker || !*ticker) return 0;
    for (size_t i = 0; i < g->vertex_count; ++i) {
        if (stock_stricmp(g->vertices[i].ticker, ticker) == 0) {
            if (vertex_out) *vertex_out = i;
            return 1;
        }
    }
    return 0;
}

int graph_add_vertex(StockGraph *g, const char *ticker, const char *sector, size_t *vertex_out) {
    if (!g || !ticker || !*ticker) return 0;
    size_t existing;
    if (graph_find_vertex(g, ticker, &existing)) {
        if (sector && *sector) {
            stock_copy_str(g->vertices[existing].sector, sizeof(g->vertices[existing].sector), sector);
        }
        if (vertex_out) *vertex_out = existing;
        return 1;
    }
    if (g->vertex_count >= MAX_GRAPH_VERTICES) return 0;
    size_t idx = g->vertex_count++;
    stock_normalize_ticker(ticker, g->vertices[idx].ticker, sizeof(g->vertices[idx].ticker));
    stock_copy_str(g->vertices[idx].sector, sizeof(g->vertices[idx].sector),
                   (sector && *sector) ? sector : "General");
    g->vertices[idx].head = NULL;
    if (vertex_out) *vertex_out = idx;
    return 1;
}

static int upsert_directed_edge(StockGraph *g, size_t src, size_t dst, double corr, const char *rel) {
    GraphEdge *e = g->vertices[src].head;
    while (e) {
        if (e->dest_vertex == dst) {
            e->correlation = corr;
            if (rel) stock_copy_str(e->relationship, sizeof(e->relationship), rel);
            return 1;
        }
        e = e->next;
    }
    GraphEdge *ne = (GraphEdge *)calloc(1, sizeof(GraphEdge));
    if (!ne) return 0;
    ne->dest_vertex = dst;
    ne->correlation = corr;
    stock_copy_str(ne->relationship, sizeof(ne->relationship),
                   (rel && *rel) ? rel : "Sector/Peer");
    ne->next = g->vertices[src].head;
    g->vertices[src].head = ne;
    return 1;
}

int graph_add_edge(StockGraph *g, const char *ticker_a, const char *ticker_b,
                   double correlation, const char *relationship) {
    if (!g || !ticker_a || !ticker_b || stock_stricmp(ticker_a, ticker_b) == 0) return 0;
    size_t u, v;
    if (!graph_find_vertex(g, ticker_a, &u)) {
        if (!graph_add_vertex(g, ticker_a, "General", &u)) return 0;
    }
    if (!graph_find_vertex(g, ticker_b, &v)) {
        if (!graph_add_vertex(g, ticker_b, "General", &v)) return 0;
    }
    if (!upsert_directed_edge(g, u, v, correlation, relationship)) return 0;
    if (!upsert_directed_edge(g, v, u, correlation, relationship)) return 0;
    return 1;
}

double graph_get_correlation(const StockGraph *g, const char *ticker_a, const char *ticker_b) {
    if (!g || !ticker_a || !ticker_b) return 0.0;
    if (stock_stricmp(ticker_a, ticker_b) == 0) return 1.0;
    size_t u, v;
    if (!graph_find_vertex(g, ticker_a, &u) || !graph_find_vertex(g, ticker_b, &v)) return 0.0;
    for (const GraphEdge *e = g->vertices[u].head; e != NULL; e = e->next) {
        if (e->dest_vertex == v) return e->correlation;
    }
    return 0.0;
}

size_t graph_bfs_related(const StockGraph *g, const char *start_ticker, double min_correlation,
                         size_t *vertices_out, size_t max_out) {
    if (!g || !start_ticker || !vertices_out || max_out == 0) return 0;
    size_t start;
    if (!graph_find_vertex(g, start_ticker, &start)) return 0;

    unsigned char visited[MAX_GRAPH_VERTICES] = {0};
    size_t queue[MAX_GRAPH_VERTICES];
    size_t q_front = 0, q_rear = 0;

    visited[start] = 1;
    queue[q_rear++] = start;
    size_t count = 0;

    while (q_front < q_rear && count < max_out) {
        size_t u = queue[q_front++];
        if (u != start) {
            vertices_out[count++] = u;
            if (count >= max_out) break;
        }
        for (const GraphEdge *e = g->vertices[u].head; e != NULL; e = e->next) {
            if (!visited[e->dest_vertex] && fabs(e->correlation) >= min_correlation) {
                visited[e->dest_vertex] = 1;
                queue[q_rear++] = e->dest_vertex;
            }
        }
    }
    return count;
}

static void dfs_rec(const StockGraph *g, size_t u, unsigned char *visited,
                    size_t *out, size_t max_out, size_t *count) {
    visited[u] = 1;
    if (*count < max_out) {
        out[(*count)++] = u;
    }
    for (const GraphEdge *e = g->vertices[u].head; e != NULL; e = e->next) {
        if (!visited[e->dest_vertex] && e->correlation >= 0.4) {
            dfs_rec(g, e->dest_vertex, visited, out, max_out, count);
        }
    }
}

size_t graph_dfs_sector_cluster(const StockGraph *g, const char *start_ticker,
                                size_t *vertices_out, size_t max_out) {
    if (!g || !start_ticker || !vertices_out || max_out == 0) return 0;
    size_t start;
    if (!graph_find_vertex(g, start_ticker, &start)) return 0;
    unsigned char visited[MAX_GRAPH_VERTICES] = {0};
    size_t count = 0;
    dfs_rec(g, start, visited, vertices_out, max_out, &count);
    return count;
}

void graph_clear(StockGraph *g) {
    if (!g) return;
    for (size_t i = 0; i < g->vertex_count; ++i) {
        GraphEdge *e = g->vertices[i].head;
        while (e) {
            GraphEdge *next = e->next;
            free(e);
            e = next;
        }
        g->vertices[i].head = NULL;
    }
    g->vertex_count = 0;
}
