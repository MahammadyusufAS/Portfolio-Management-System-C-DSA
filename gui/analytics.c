#include "gui.h"
#ifdef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HWND make_ctrl(GuiContext *ctx, AppView view, const char *cls, const char *txt,
                      DWORD style, int x, int y, int w, int h, int id) {
    HWND c = CreateWindowExA(0, cls, txt, WS_CHILD | style,
                             x, y, w, h, ctx->hwnd_main, (HMENU)(INT_PTR)id,
                             GetModuleHandle(NULL), NULL);
    if (ctx->font_body) SendMessage(c, WM_SETFONT, (WPARAM)ctx->font_body, TRUE);
    return gui_register_control(ctx, view, c);
}

static void lv_add_col(HWND lv, int idx, const char *title, int width) {
    LVCOLUMNA col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    col.pszText = (LPSTR)title;
    col.cx = width;
    col.iSubItem = idx;
    ListView_InsertColumn(lv, idx, &col);
}

static void lv_set_cell(HWND lv, int row, int col, const char *text) {
    LVITEMA item = {0};
    item.mask = LVIF_TEXT;
    item.iItem = row;
    item.iSubItem = col;
    item.pszText = (LPSTR)text;
    if (col == 0) {
        ListView_InsertItem(lv, &item);
    } else {
        ListView_SetItem(lv, &item);
    }
}

void gui_analytics_create(GuiContext *ctx) {
    int cx = 210, cy = 20;
    make_ctrl(ctx, VIEW_ANALYTICS, "STATIC", "PORTFOLIO ANALYTICS, GRAPH CORRELATIONS & DSA BENCHMARKS", SS_LEFT, cx, cy, 560, 26, 0);

    make_ctrl(ctx, VIEW_ANALYTICS, "STATIC", "Benchmark Search Ticker:", SS_LEFT, cx, cy + 122, 175, 22, 0);
    ctx->ana_bench_ticker = make_ctrl(ctx, VIEW_ANALYTICS, "EDIT", "INFY", WS_BORDER | ES_AUTOHSCROLL, cx + 180, cy + 119, 110, 26, ID_ANA_BENCH_TICKER);
    make_ctrl(ctx, VIEW_ANALYTICS, "BUTTON", "Compare Search & Sort DSA Complexities", BS_PUSHBUTTON, cx + 305, cy + 118, 280, 28, ID_ANA_BTN_BENCH);

    /* Sector Diversification & Graph Cluster Table */
    ctx->ana_sector_list = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
        WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
        cx, cy + 158, 1020, 195, ctx->hwnd_main, (HMENU)(INT_PTR)ID_ANA_SECTOR_LIST,
        GetModuleHandle(NULL), NULL);
    ListView_SetExtendedListViewStyle(ctx->ana_sector_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    gui_register_control(ctx, VIEW_ANALYTICS, ctx->ana_sector_list);

    lv_add_col(ctx->ana_sector_list, 0, "Sector / Category", 160);
    lv_add_col(ctx->ana_sector_list, 1, "Holdings", 90);
    lv_add_col(ctx->ana_sector_list, 2, "Invested Value", 140);
    lv_add_col(ctx->ana_sector_list, 3, "Market Value", 140);
    lv_add_col(ctx->ana_sector_list, 4, "Portfolio Weight %", 140);
    lv_add_col(ctx->ana_sector_list, 5, "Unrealized P/L", 140);
    lv_add_col(ctx->ana_sector_list, 6, "Return %", 120);

    /* Search & Sort Algorithm Complexity Comparison Table */
    ctx->ana_bench_list = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
        WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
        cx, cy + 368, 1020, 215, ctx->hwnd_main, (HMENU)(INT_PTR)ID_ANA_BENCH_LIST,
        GetModuleHandle(NULL), NULL);
    ListView_SetExtendedListViewStyle(ctx->ana_bench_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    gui_register_control(ctx, VIEW_ANALYTICS, ctx->ana_bench_list);

    lv_add_col(ctx->ana_bench_list, 0, "Algorithm / Data Structure", 230);
    lv_add_col(ctx->ana_bench_list, 1, "Category", 120);
    lv_add_col(ctx->ana_bench_list, 2, "Average Complexity", 140);
    lv_add_col(ctx->ana_bench_list, 3, "Worst-Case Complexity", 150);
    lv_add_col(ctx->ana_bench_list, 4, "Measured Steps / Comps", 165);
    lv_add_col(ctx->ana_bench_list, 5, "Measured Time", 165);
}

void gui_analytics_refresh(GuiContext *ctx) {
    if (!ctx || !ctx->ana_sector_list || !ctx->ana_bench_list || !ctx->portfolio) return;
    Portfolio *p = ctx->portfolio;
    PortfolioAnalytics ana;
    portfolio_compute_analytics(p, &ana);

    ListView_DeleteAllItems(ctx->ana_sector_list);
    char buf[128];
    for (size_t i = 0; i < ana.sector_count; ++i) {
        const SectorAllocation *sa = &ana.sectors[i];
        lv_set_cell(ctx->ana_sector_list, (int)i, 0, sa->sector);
        snprintf(buf, sizeof(buf), "%zu", sa->stock_count);
        lv_set_cell(ctx->ana_sector_list, (int)i, 1, buf);
        snprintf(buf, sizeof(buf), "%.2f", sa->invested_value);
        lv_set_cell(ctx->ana_sector_list, (int)i, 2, buf);
        snprintf(buf, sizeof(buf), "%.2f", sa->market_value);
        lv_set_cell(ctx->ana_sector_list, (int)i, 3, buf);
        snprintf(buf, sizeof(buf), "%.2f%%", sa->weight_percent);
        lv_set_cell(ctx->ana_sector_list, (int)i, 4, buf);
        snprintf(buf, sizeof(buf), "%+.2f", sa->unrealized_pl);
        lv_set_cell(ctx->ana_sector_list, (int)i, 5, buf);
        double r = sa->invested_value > 0.0 ? (sa->unrealized_pl / sa->invested_value) * 100.0 : 0.0;
        snprintf(buf, sizeof(buf), "%+.2f%%", r);
        lv_set_cell(ctx->ana_sector_list, (int)i, 6, buf);
    }

    char query[32] = "INFY";
    if (ctx->ana_bench_ticker) GetWindowTextA(ctx->ana_bench_ticker, query, sizeof(query));
    search_compare_all(&p->stocks, &p->ticker_index, &p->avl_index, query, &ctx->last_search_report);

    ListView_DeleteAllItems(ctx->ana_bench_list);
    const SearchComparisonReport *rep = &ctx->last_search_report;

    /* 4 Searching Algorithms */
    lv_set_cell(ctx->ana_bench_list, 0, 0, "Hash Table Search (Separate Chaining)");
    lv_set_cell(ctx->ana_bench_list, 0, 1, "Search");
    lv_set_cell(ctx->ana_bench_list, 0, 2, "O(1)");
    lv_set_cell(ctx->ana_bench_list, 0, 3, "O(n)");
    snprintf(buf, sizeof(buf), "%zu step(s)", rep->hash_metric.steps);
    lv_set_cell(ctx->ana_bench_list, 0, 4, buf);
    snprintf(buf, sizeof(buf), "%.1f ns", rep->hash_metric.elapsed_nanoseconds);
    lv_set_cell(ctx->ana_bench_list, 0, 5, buf);

    lv_set_cell(ctx->ana_bench_list, 1, 0, "AVL Tree Search (Balanced BST)");
    lv_set_cell(ctx->ana_bench_list, 1, 1, "Search");
    lv_set_cell(ctx->ana_bench_list, 1, 2, "O(log n)");
    lv_set_cell(ctx->ana_bench_list, 1, 3, "O(log n)");
    snprintf(buf, sizeof(buf), "%zu step(s) (H=%d)", rep->avl_metric.steps, avl_height(&p->avl_index));
    lv_set_cell(ctx->ana_bench_list, 1, 4, buf);
    snprintf(buf, sizeof(buf), "%.1f ns", rep->avl_metric.elapsed_nanoseconds);
    lv_set_cell(ctx->ana_bench_list, 1, 5, buf);

    lv_set_cell(ctx->ana_bench_list, 2, 0, "Binary Search (Sorted Index Array)");
    lv_set_cell(ctx->ana_bench_list, 2, 1, "Search");
    lv_set_cell(ctx->ana_bench_list, 2, 2, "O(log n)");
    lv_set_cell(ctx->ana_bench_list, 2, 3, "O(log n)");
    snprintf(buf, sizeof(buf), "%zu step(s)", rep->binary_metric.steps);
    lv_set_cell(ctx->ana_bench_list, 2, 4, buf);
    snprintf(buf, sizeof(buf), "%.1f ns", rep->binary_metric.elapsed_nanoseconds);
    lv_set_cell(ctx->ana_bench_list, 2, 5, buf);

    lv_set_cell(ctx->ana_bench_list, 3, 0, "Linear Search (Sequential Scan)");
    lv_set_cell(ctx->ana_bench_list, 3, 1, "Search");
    lv_set_cell(ctx->ana_bench_list, 3, 2, "O(n)");
    lv_set_cell(ctx->ana_bench_list, 3, 3, "O(n)");
    snprintf(buf, sizeof(buf), "%zu step(s)", rep->linear_metric.steps);
    lv_set_cell(ctx->ana_bench_list, 3, 4, buf);
    snprintf(buf, sizeof(buf), "%.1f ns", rep->linear_metric.elapsed_nanoseconds);
    lv_set_cell(ctx->ana_bench_list, 3, 5, buf);

    /* 3 Sorting Algorithms */
    size_t cap = p->stocks.count > 0 ? p->stocks.count : 1;
    SortItem *items = (SortItem *)calloc(cap, sizeof(SortItem));
    if (items) {
        size_t cnt = 0;
        SortStats st_m, st_q, st_i;
        sort_build_items(&p->stocks, SORT_BY_MARKET_VALUE, items, cap, &cnt);
        merge_sort_items(items, cnt, SORT_BY_MARKET_VALUE, SORT_DESC, &st_m);
        sort_build_items(&p->stocks, SORT_BY_MARKET_VALUE, items, cap, &cnt);
        quick_sort_items(items, cnt, SORT_BY_MARKET_VALUE, SORT_DESC, &st_q);
        sort_build_items(&p->stocks, SORT_BY_MARKET_VALUE, items, cap, &cnt);
        insertion_sort_items(items, cnt, SORT_BY_MARKET_VALUE, SORT_DESC, &st_i);

        lv_set_cell(ctx->ana_bench_list, 4, 0, "Merge Sort (Stable Divide & Conquer)");
        lv_set_cell(ctx->ana_bench_list, 4, 1, "Sorting");
        lv_set_cell(ctx->ana_bench_list, 4, 2, "O(n log n)");
        lv_set_cell(ctx->ana_bench_list, 4, 3, "O(n log n)");
        snprintf(buf, sizeof(buf), "%zu comps / %zu moves", st_m.comparisons, st_m.moves);
        lv_set_cell(ctx->ana_bench_list, 4, 4, buf);
        snprintf(buf, sizeof(buf), "%.2f us", st_m.elapsed_microseconds);
        lv_set_cell(ctx->ana_bench_list, 4, 5, buf);

        lv_set_cell(ctx->ana_bench_list, 5, 0, "Quick Sort (Median-of-Three Pivot)");
        lv_set_cell(ctx->ana_bench_list, 5, 1, "Sorting");
        lv_set_cell(ctx->ana_bench_list, 5, 2, "O(n log n)");
        lv_set_cell(ctx->ana_bench_list, 5, 3, "O(n^2)");
        snprintf(buf, sizeof(buf), "%zu comps / %zu moves", st_q.comparisons, st_q.moves);
        lv_set_cell(ctx->ana_bench_list, 5, 4, buf);
        snprintf(buf, sizeof(buf), "%.2f us", st_q.elapsed_microseconds);
        lv_set_cell(ctx->ana_bench_list, 5, 5, buf);

        lv_set_cell(ctx->ana_bench_list, 6, 0, "Insertion Sort (Adaptive In-Place)");
        lv_set_cell(ctx->ana_bench_list, 6, 1, "Sorting");
        lv_set_cell(ctx->ana_bench_list, 6, 2, "O(n^2)");
        lv_set_cell(ctx->ana_bench_list, 6, 3, "O(n^2)");
        snprintf(buf, sizeof(buf), "%zu comps / %zu moves", st_i.comparisons, st_i.moves);
        lv_set_cell(ctx->ana_bench_list, 6, 4, buf);
        snprintf(buf, sizeof(buf), "%.2f us", st_i.elapsed_microseconds);
        lv_set_cell(ctx->ana_bench_list, 6, 5, buf);
        free(items);
    }

    InvalidateRect(ctx->hwnd_main, NULL, FALSE);
}

void gui_analytics_paint(GuiContext *ctx, HDC hdc, RECT client_rc) {
    if (!ctx || !ctx->portfolio) return;
    Portfolio *p = ctx->portfolio;
    PortfolioAnalytics ana;
    portfolio_compute_analytics(p, &ana);

    int cx = 210, cy = 54;
    int cw = client_rc.right - cx - 25;
    if (cw < 700) cw = 700;

    RECT banner = { cx, cy, cx + cw, cy + 76 };
    HBRUSH wb = CreateSolidBrush(RGB(255, 255, 255));
    HPEN bp = CreatePen(PS_SOLID, 1, RGB(218, 224, 235));
    SelectObject(hdc, wb);
    SelectObject(hdc, bp);
    RoundRect(hdc, banner.left, banner.top, banner.right, banner.bottom, 10, 10);

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, ctx->font_bold);
    SetTextColor(hdc, RGB(30, 41, 59));

    const char *best_tk = (ana.best_stock_index >= 0) ? p->stocks.items[ana.best_stock_index].ticker : "N/A";
    const char *worst_tk = (ana.worst_stock_index >= 0) ? p->stocks.items[ana.worst_stock_index].ticker : "N/A";
    char line1[256], line2[256];
    snprintf(line1, sizeof(line1),
             "Net Total P/L: %+.2f   |   Overall Return: %+.2f%%   |   Diversification Score: %.1f / 100.0   |   Graph Avg Correlation: %.2f",
             ana.net_total_pl, ana.overall_return_pct, ana.diversification_score, ana.avg_pair_correlation);
    snprintf(line2, sizeof(line2),
             "Best Performing Holding: %s   |   Lagging Holding: %s   |   Active Sectors: %zu   |   Graph Vertices: %zu",
             best_tk, worst_tk, ana.sector_count, p->correlation_graph.vertex_count);

    TextOutA(hdc, cx + 16, cy + 14, line1, (int)strlen(line1));
    SelectObject(hdc, ctx->font_body);
    SetTextColor(hdc, RGB(71, 85, 105));
    TextOutA(hdc, cx + 16, cy + 42, line2, (int)strlen(line2));

    DeleteObject(wb);
    DeleteObject(bp);
}

void gui_analytics_layout(GuiContext *ctx, int content_x, int content_y, int content_w, int content_h) {
    if (!ctx || !ctx->ana_sector_list || !ctx->ana_bench_list) return;
    MoveWindow(ctx->ana_sector_list, content_x, content_y + 158, content_w, 195, TRUE);
    MoveWindow(ctx->ana_bench_list, content_x, content_y + 368, content_w, content_h - 380, TRUE);
}

int gui_analytics_command(GuiContext *ctx, int id, int notify_code) {
    (void)notify_code;
    if (id == ID_ANA_BTN_BENCH) {
        gui_analytics_refresh(ctx);
        gui_set_status(ctx, "Completed live complexity comparison across Linear, Binary, Hash, and AVL searches.");
        return 1;
    }
    return 0;
}
#endif
