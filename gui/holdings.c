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

void gui_holdings_create(GuiContext *ctx) {
    int cx = 210, cy = 20;

    make_ctrl(ctx, VIEW_HOLDINGS, "STATIC", "STOCK CATALOG & HOLDINGS MANAGEMENT", SS_LEFT, cx, cy, 420, 26, 0);

    /* Add / Update / Remove Stock Controls */
    make_ctrl(ctx, VIEW_HOLDINGS, "STATIC", "Ticker:", SS_LEFT, cx, cy + 34, 55, 22, 0);
    ctx->hold_ticker = make_ctrl(ctx, VIEW_HOLDINGS, "EDIT", "", WS_BORDER | ES_AUTOHSCROLL, cx + 55, cy + 32, 90, 26, ID_HOLD_TICKER);

    make_ctrl(ctx, VIEW_HOLDINGS, "STATIC", "Company:", SS_LEFT, cx + 155, cy + 34, 70, 22, 0);
    ctx->hold_name = make_ctrl(ctx, VIEW_HOLDINGS, "EDIT", "", WS_BORDER | ES_AUTOHSCROLL, cx + 225, cy + 32, 160, 26, ID_HOLD_NAME);

    make_ctrl(ctx, VIEW_HOLDINGS, "STATIC", "Sector:", SS_LEFT, cx + 395, cy + 34, 50, 22, 0);
    ctx->hold_sector = make_ctrl(ctx, VIEW_HOLDINGS, "EDIT", "Technology", WS_BORDER | ES_AUTOHSCROLL, cx + 450, cy + 32, 110, 26, ID_HOLD_SECTOR);

    make_ctrl(ctx, VIEW_HOLDINGS, "STATIC", "Price:", SS_LEFT, cx + 570, cy + 34, 45, 22, 0);
    ctx->hold_price = make_ctrl(ctx, VIEW_HOLDINGS, "EDIT", "", WS_BORDER | ES_AUTOHSCROLL, cx + 615, cy + 32, 85, 26, ID_HOLD_PRICE);

    make_ctrl(ctx, VIEW_HOLDINGS, "BUTTON", "Add / Update", BS_PUSHBUTTON, cx + 710, cy + 31, 105, 28, ID_HOLD_BTN_ADD);
    make_ctrl(ctx, VIEW_HOLDINGS, "BUTTON", "Update Price", BS_PUSHBUTTON, cx + 820, cy + 31, 100, 28, ID_HOLD_BTN_UPDATE);
    make_ctrl(ctx, VIEW_HOLDINGS, "BUTTON", "Remove Stock", BS_PUSHBUTTON, cx + 925, cy + 31, 105, 28, ID_HOLD_BTN_REMOVE);

    /* Search & DSA Sorting Controls */
    make_ctrl(ctx, VIEW_HOLDINGS, "STATIC", "Search (Ticker/Name/Sector):", SS_LEFT, cx, cy + 72, 185, 22, 0);
    ctx->hold_search = make_ctrl(ctx, VIEW_HOLDINGS, "EDIT", "", WS_BORDER | ES_AUTOHSCROLL, cx + 190, cy + 69, 160, 26, ID_HOLD_SEARCH);

    make_ctrl(ctx, VIEW_HOLDINGS, "STATIC", "Sort By:", SS_LEFT, cx + 365, cy + 72, 55, 22, 0);
    ctx->hold_sort_field = make_ctrl(ctx, VIEW_HOLDINGS, "COMBOBOX", "", CBS_DROPDOWNLIST | WS_VSCROLL, cx + 420, cy + 68, 135, 200, ID_HOLD_SORT_FIELD);
    SendMessageA(ctx->hold_sort_field, CB_ADDSTRING, 0, (LPARAM)"Market Value");
    SendMessageA(ctx->hold_sort_field, CB_ADDSTRING, 0, (LPARAM)"Current Price");
    SendMessageA(ctx->hold_sort_field, CB_ADDSTRING, 0, (LPARAM)"Quantity Owned");
    SendMessageA(ctx->hold_sort_field, CB_ADDSTRING, 0, (LPARAM)"Profit / Loss");
    SendMessageA(ctx->hold_sort_field, CB_ADDSTRING, 0, (LPARAM)"Total Invested");
    SendMessageA(ctx->hold_sort_field, CB_ADDSTRING, 0, (LPARAM)"Ticker Symbol");
    SendMessageA(ctx->hold_sort_field, CB_SETCURSEL, 0, 0);

    make_ctrl(ctx, VIEW_HOLDINGS, "STATIC", "Algorithm:", SS_LEFT, cx + 565, cy + 72, 68, 22, 0);
    ctx->hold_sort_alg = make_ctrl(ctx, VIEW_HOLDINGS, "COMBOBOX", "", CBS_DROPDOWNLIST | WS_VSCROLL, cx + 635, cy + 68, 145, 200, ID_HOLD_SORT_ALG);
    SendMessageA(ctx->hold_sort_alg, CB_ADDSTRING, 0, (LPARAM)"Merge Sort O(n log n)");
    SendMessageA(ctx->hold_sort_alg, CB_ADDSTRING, 0, (LPARAM)"Quick Sort O(n log n)");
    SendMessageA(ctx->hold_sort_alg, CB_ADDSTRING, 0, (LPARAM)"Insertion Sort O(n^2)");
    SendMessageA(ctx->hold_sort_alg, CB_SETCURSEL, 0, 0);

    ctx->hold_sort_order = make_ctrl(ctx, VIEW_HOLDINGS, "COMBOBOX", "", CBS_DROPDOWNLIST | WS_VSCROLL, cx + 790, cy + 68, 110, 200, ID_HOLD_SORT_ORDER);
    SendMessageA(ctx->hold_sort_order, CB_ADDSTRING, 0, (LPARAM)"Descending");
    SendMessageA(ctx->hold_sort_order, CB_ADDSTRING, 0, (LPARAM)"Ascending");
    SendMessageA(ctx->hold_sort_order, CB_SETCURSEL, 0, 0);

    make_ctrl(ctx, VIEW_HOLDINGS, "BUTTON", "Sort Now", BS_PUSHBUTTON, cx + 910, cy + 68, 100, 28, ID_HOLD_BTN_SORT);

    ctx->hold_list = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
        WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        cx, cy + 108, 1020, 480, ctx->hwnd_main, (HMENU)(INT_PTR)ID_HOLD_LIST,
        GetModuleHandle(NULL), NULL);
    ListView_SetExtendedListViewStyle(ctx->hold_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    gui_register_control(ctx, VIEW_HOLDINGS, ctx->hold_list);

    lv_add_col(ctx->hold_list, 0, "ID", 48);
    lv_add_col(ctx->hold_list, 1, "Ticker", 95);
    lv_add_col(ctx->hold_list, 2, "Company Name", 185);
    lv_add_col(ctx->hold_list, 3, "Sector", 115);
    lv_add_col(ctx->hold_list, 4, "Current Price", 100);
    lv_add_col(ctx->hold_list, 5, "Qty Owned", 85);
    lv_add_col(ctx->hold_list, 6, "Avg Purchase", 100);
    lv_add_col(ctx->hold_list, 7, "Total Invested", 110);
    lv_add_col(ctx->hold_list, 8, "Market Value", 110);
    lv_add_col(ctx->hold_list, 9, "Unrealized P/L", 115);
}

void gui_holdings_refresh(GuiContext *ctx) {
    if (!ctx || !ctx->hold_list || !ctx->portfolio) return;
    Portfolio *p = ctx->portfolio;
    ListView_DeleteAllItems(ctx->hold_list);

    char query[64] = "";
    if (ctx->hold_search) GetWindowTextA(ctx->hold_search, query, sizeof(query));

    ctx->sort_field = (SortField)SendMessageA(ctx->hold_sort_field, CB_GETCURSEL, 0, 0);
    ctx->sort_alg   = (SortAlgorithm)SendMessageA(ctx->hold_sort_alg, CB_GETCURSEL, 0, 0);
    ctx->sort_order = (SortOrder)SendMessageA(ctx->hold_sort_order, CB_GETCURSEL, 0, 0);

    size_t cap = p->stocks.count > 0 ? p->stocks.count : 1;
    SortItem *sorted = (SortItem *)calloc(cap, sizeof(SortItem));
    if (!sorted) return;

    size_t count = 0;
    sort_build_items(&p->stocks, ctx->sort_field, sorted, cap, &count);
    sort_items(sorted, count, ctx->sort_field, ctx->sort_order, ctx->sort_alg, &ctx->last_sort_stats);

    int row = 0;
    char buf[128];
    for (size_t k = 0; k < count; ++k) {
        const Stock *s = &p->stocks.items[sorted[k].index];
        if (query[0] && !stock_stristr(s->ticker, query) &&
            !stock_stristr(s->name, query) && !stock_stristr(s->sector, query)) {
            continue;
        }
        snprintf(buf, sizeof(buf), "%d", s->id);
        lv_set_cell(ctx->hold_list, row, 0, buf);
        lv_set_cell(ctx->hold_list, row, 1, s->ticker);
        lv_set_cell(ctx->hold_list, row, 2, s->name);
        lv_set_cell(ctx->hold_list, row, 3, s->sector);
        snprintf(buf, sizeof(buf), "%.2f", s->current_price);
        lv_set_cell(ctx->hold_list, row, 4, buf);
        snprintf(buf, sizeof(buf), "%d", s->quantity);
        lv_set_cell(ctx->hold_list, row, 5, buf);
        snprintf(buf, sizeof(buf), "%.2f", s->average_cost);
        lv_set_cell(ctx->hold_list, row, 6, buf);
        snprintf(buf, sizeof(buf), "%.2f", s->total_investment);
        lv_set_cell(ctx->hold_list, row, 7, buf);
        snprintf(buf, sizeof(buf), "%.2f", s->market_value);
        lv_set_cell(ctx->hold_list, row, 8, buf);
        snprintf(buf, sizeof(buf), "%+.2f", s->unrealized_pl);
        lv_set_cell(ctx->hold_list, row, 9, buf);
        row++;
    }
    free(sorted);
}

void gui_holdings_layout(GuiContext *ctx, int content_x, int content_y, int content_w, int content_h) {
    if (!ctx || !ctx->hold_list) return;
    MoveWindow(ctx->hold_list, content_x, content_y + 108, content_w, content_h - 120, TRUE);
}

int gui_holdings_command(GuiContext *ctx, int id, int notify_code) {
    Portfolio *p = ctx->portfolio;
    char ticker[32] = "", name[80] = "", sector[40] = "", price_str[32] = "";

    if (id == ID_HOLD_SEARCH && notify_code == EN_CHANGE) {
        gui_holdings_refresh(ctx);
        return 1;
    }
    if (id == ID_HOLD_BTN_SORT ||
        ((id == ID_HOLD_SORT_FIELD || id == ID_HOLD_SORT_ALG || id == ID_HOLD_SORT_ORDER) && notify_code == CBN_SELCHANGE)) {
        gui_holdings_refresh(ctx);
        gui_set_status(ctx, "Sorted %zu stocks (%zu comparisons, %zu moves, %.2f us).",
                       ctx->last_sort_stats.element_count,
                       ctx->last_sort_stats.comparisons,
                       ctx->last_sort_stats.moves,
                       ctx->last_sort_stats.elapsed_microseconds);
        return 1;
    }
    if (id == ID_HOLD_BTN_ADD) {
        GetWindowTextA(ctx->hold_ticker, ticker, sizeof(ticker));
        GetWindowTextA(ctx->hold_name, name, sizeof(name));
        GetWindowTextA(ctx->hold_sector, sector, sizeof(sector));
        GetWindowTextA(ctx->hold_price, price_str, sizeof(price_str));
        double price = strtod(price_str, NULL);
        if (!stock_validate_ticker(ticker) || !name[0] || price <= 0.0) {
            MessageBoxA(ctx->hwnd_main, "Please enter a valid ticker, company name, and positive price.",
                        "Validation Error", MB_ICONWARNING);
            return 1;
        }
        int res = portfolio_add_stock(p, ticker, name, sector[0] ? sector : "General", price);
        if (!res) {
            MessageBoxA(ctx->hwnd_main, "Failed to add or update stock.", "Error", MB_ICONERROR);
            return 1;
        }
        db_save_portfolio(p, p->db_path);
        gui_holdings_refresh(ctx);
        gui_set_status(ctx, res == 2 ? "Updated existing stock %s." : "Added new stock %s to catalog.", ticker);
        return 1;
    }
    if (id == ID_HOLD_BTN_UPDATE) {
        GetWindowTextA(ctx->hold_ticker, ticker, sizeof(ticker));
        GetWindowTextA(ctx->hold_price, price_str, sizeof(price_str));
        double price = strtod(price_str, NULL);
        if (!portfolio_update_price(p, ticker, price)) {
            MessageBoxA(ctx->hwnd_main, "Enter an existing stock ticker and positive price.",
                        "Update Price Failed", MB_ICONWARNING);
            return 1;
        }
        db_save_portfolio(p, p->db_path);
        gui_holdings_refresh(ctx);
        gui_set_status(ctx, "Updated price for %s to %.2f.", ticker, price);
        return 1;
    }
    if (id == ID_HOLD_BTN_REMOVE) {
        GetWindowTextA(ctx->hold_ticker, ticker, sizeof(ticker));
        if (!ticker[0]) {
            MessageBoxA(ctx->hwnd_main, "Enter the ticker symbol to remove from catalog.", "Input Required", MB_ICONWARNING);
            return 1;
        }
        if (MessageBoxA(ctx->hwnd_main, "Remove this stock from catalog? (Only stocks with 0 owned shares can be removed)",
                        "Confirm Removal", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            if (!portfolio_remove_stock(p, ticker)) {
                MessageBoxA(ctx->hwnd_main, "Cannot remove stock: either ticker not found or shares are still held.",
                            "Removal Blocked", MB_ICONWARNING);
            } else {
                db_save_portfolio(p, p->db_path);
                gui_holdings_refresh(ctx);
                gui_set_status(ctx, "Removed %s from catalog.", ticker);
            }
        }
        return 1;
    }
    return 0;
}

/* Buy/Sell & Pending Order Queue View */
void gui_buysell_create(GuiContext *ctx) {
    int cx = 210, cy = 20;
    make_ctrl(ctx, VIEW_BUY_SELL, "STATIC", "BUY / SELL STOCKS & ORDER QUEUE", SS_LEFT, cx, cy, 420, 26, 0);

    make_ctrl(ctx, VIEW_BUY_SELL, "STATIC", "Stock Ticker:", SS_LEFT, cx, cy + 45, 110, 22, 0);
    ctx->trade_ticker = make_ctrl(ctx, VIEW_BUY_SELL, "EDIT", "RELIANCE", WS_BORDER | ES_AUTOHSCROLL, cx + 115, cy + 42, 130, 28, ID_TRADE_TICKER);

    make_ctrl(ctx, VIEW_BUY_SELL, "STATIC", "Quantity (Shares):", SS_LEFT, cx + 265, cy + 45, 125, 22, 0);
    ctx->trade_qty = make_ctrl(ctx, VIEW_BUY_SELL, "EDIT", "10", WS_BORDER | ES_AUTOHSCROLL, cx + 395, cy + 42, 100, 28, ID_TRADE_QTY);

    make_ctrl(ctx, VIEW_BUY_SELL, "STATIC", "Execution Price:", SS_LEFT, cx + 515, cy + 45, 115, 22, 0);
    ctx->trade_price = make_ctrl(ctx, VIEW_BUY_SELL, "EDIT", "1450.00", WS_BORDER | ES_AUTOHSCROLL, cx + 635, cy + 42, 110, 28, ID_TRADE_PRICE);

    make_ctrl(ctx, VIEW_BUY_SELL, "BUTTON", "BUY SHARES", BS_PUSHBUTTON, cx, cy + 86, 140, 34, ID_TRADE_BTN_BUY);
    make_ctrl(ctx, VIEW_BUY_SELL, "BUTTON", "SELL SHARES", BS_PUSHBUTTON, cx + 155, cy + 86, 140, 34, ID_TRADE_BTN_SELL);
    make_ctrl(ctx, VIEW_BUY_SELL, "BUTTON", "Enqueue Buy (FIFO Queue)", BS_PUSHBUTTON, cx + 310, cy + 86, 185, 34, ID_TRADE_BTN_QUEUE);
    make_ctrl(ctx, VIEW_BUY_SELL, "BUTTON", "Process Pending Queue", BS_PUSHBUTTON, cx + 510, cy + 86, 175, 34, ID_TRADE_BTN_EXEC_Q);
    make_ctrl(ctx, VIEW_BUY_SELL, "BUTTON", "Undo Last Trade (Stack)", BS_PUSHBUTTON, cx + 700, cy + 86, 175, 34, ID_TRADE_BTN_UNDO);

    make_ctrl(ctx, VIEW_BUY_SELL, "STATIC", "Pending Order FIFO Queue & Current Position Status:", SS_LEFT, cx, cy + 136, 420, 24, 0);

    ctx->trade_queue_list = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
        WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
        cx, cy + 164, 1020, 420, ctx->hwnd_main, (HMENU)(INT_PTR)ID_TRADE_QUEUE_LIST,
        GetModuleHandle(NULL), NULL);
    ListView_SetExtendedListViewStyle(ctx->trade_queue_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    gui_register_control(ctx, VIEW_BUY_SELL, ctx->trade_queue_list);

    lv_add_col(ctx->trade_queue_list, 0, "Record Type", 130);
    lv_add_col(ctx->trade_queue_list, 1, "Ticker", 110);
    lv_add_col(ctx->trade_queue_list, 2, "Position / Task", 160);
    lv_add_col(ctx->trade_queue_list, 3, "Quantity", 100);
    lv_add_col(ctx->trade_queue_list, 4, "Avg Cost / Price", 130);
    lv_add_col(ctx->trade_queue_list, 5, "Market Value / Total", 150);
    lv_add_col(ctx->trade_queue_list, 6, "Unrealized / Realized P/L", 190);
}

void gui_buysell_refresh(GuiContext *ctx) {
    if (!ctx || !ctx->trade_queue_list || !ctx->portfolio) return;
    Portfolio *p = ctx->portfolio;
    ListView_DeleteAllItems(ctx->trade_queue_list);
    int row = 0;
    char buf[128];

    /* Show queued tasks first */
    for (const QueueNode *qn = p->pending_queue.front; qn != NULL; qn = qn->next) {
        lv_set_cell(ctx->trade_queue_list, row, 0, "QUEUED TASK (FIFO)");
        lv_set_cell(ctx->trade_queue_list, row, 1, qn->task.ticker);
        lv_set_cell(ctx->trade_queue_list, row, 2, qn->task.type == TASK_BUY_ORDER ? "PENDING BUY" : "PENDING ORDER");
        snprintf(buf, sizeof(buf), "%d", qn->task.quantity);
        lv_set_cell(ctx->trade_queue_list, row, 3, buf);
        snprintf(buf, sizeof(buf), "%.2f", qn->task.price);
        lv_set_cell(ctx->trade_queue_list, row, 4, buf);
        snprintf(buf, sizeof(buf), "%.2f", qn->task.price * qn->task.quantity);
        lv_set_cell(ctx->trade_queue_list, row, 5, buf);
        lv_set_cell(ctx->trade_queue_list, row, 6, qn->task.created_at);
        row++;
    }

    /* Show active holdings */
    for (size_t i = 0; i < p->stocks.count; ++i) {
        const Stock *s = &p->stocks.items[i];
        lv_set_cell(ctx->trade_queue_list, row, 0, s->quantity > 0 ? "ACTIVE HOLDING" : "CATALOG STOCK");
        lv_set_cell(ctx->trade_queue_list, row, 1, s->ticker);
        lv_set_cell(ctx->trade_queue_list, row, 2, s->name);
        snprintf(buf, sizeof(buf), "%d", s->quantity);
        lv_set_cell(ctx->trade_queue_list, row, 3, buf);
        snprintf(buf, sizeof(buf), "Avg: %.2f | Cur: %.2f", s->average_cost, s->current_price);
        lv_set_cell(ctx->trade_queue_list, row, 4, buf);
        snprintf(buf, sizeof(buf), "%.2f", s->market_value);
        lv_set_cell(ctx->trade_queue_list, row, 5, buf);
        snprintf(buf, sizeof(buf), "UPL: %+.2f | RPL: %+.2f", s->unrealized_pl, s->realized_pl);
        lv_set_cell(ctx->trade_queue_list, row, 6, buf);
        row++;
    }
}

void gui_buysell_paint(GuiContext *ctx, HDC hdc, RECT client_rc) {
    (void)ctx; (void)hdc; (void)client_rc;
}

void gui_buysell_layout(GuiContext *ctx, int content_x, int content_y, int content_w, int content_h) {
    if (!ctx || !ctx->trade_queue_list) return;
    MoveWindow(ctx->trade_queue_list, content_x, content_y + 164, content_w, content_h - 176, TRUE);
}

int gui_buysell_command(GuiContext *ctx, int id, int notify_code) {
    (void)notify_code;
    Portfolio *p = ctx->portfolio;
    char ticker[32] = "", qty_str[32] = "", price_str[32] = "";
    GetWindowTextA(ctx->trade_ticker, ticker, sizeof(ticker));
    GetWindowTextA(ctx->trade_qty, qty_str, sizeof(qty_str));
    GetWindowTextA(ctx->trade_price, price_str, sizeof(price_str));
    int qty = (int)strtol(qty_str, NULL, 10);
    double price = strtod(price_str, NULL);

    if (id == ID_TRADE_BTN_BUY || id == ID_TRADE_BTN_SELL) {
        int is_buy = (id == ID_TRADE_BTN_BUY);
        if (!ticker[0] || qty <= 0 || price <= 0.0) {
            MessageBoxA(ctx->hwnd_main, "Enter a valid stock ticker, positive share quantity, and positive price.",
                        "Invalid Trade Input", MB_ICONWARNING);
            return 1;
        }
        if (!portfolio_trade(p, ticker, qty, price, is_buy)) {
            MessageBoxA(ctx->hwnd_main,
                        is_buy ? "Purchase failed: verify that the ticker exists in the stock catalog."
                               : "Sale rejected: cannot sell more shares than currently owned.",
                        "Trade Rejected", MB_ICONWARNING);
            return 1;
        }
        portfolio_record_snapshot(p, NULL);
        db_save_portfolio(p, p->db_path);
        gui_buysell_refresh(ctx);
        gui_set_status(ctx, "%s %d shares of %s @ %.2f completed and persisted to SQLite.",
                       is_buy ? "BUY" : "SELL", qty, ticker, price);
        return 1;
    }
    if (id == ID_TRADE_BTN_QUEUE) {
        if (!ticker[0] || qty <= 0 || price <= 0.0) {
            MessageBoxA(ctx->hwnd_main, "Enter valid ticker, quantity, and price to enqueue.", "Queue Input", MB_ICONWARNING);
            return 1;
        }
        portfolio_enqueue_task(p, TASK_BUY_ORDER, ticker, qty, price);
        gui_buysell_refresh(ctx);
        gui_set_status(ctx, "Enqueued BUY order for %d %s @ %.2f into FIFO TaskQueue (Size=%zu).",
                       qty, ticker, price, queue_size(&p->pending_queue));
        return 1;
    }
    if (id == ID_TRADE_BTN_EXEC_Q) {
        size_t done = portfolio_process_pending_queue(p, 0);
        db_save_portfolio(p, p->db_path);
        gui_buysell_refresh(ctx);
        gui_set_status(ctx, "Executed %zu pending order(s) from FIFO Queue.", done);
        return 1;
    }
    if (id == ID_TRADE_BTN_UNDO) {
        if (!portfolio_undo(p)) {
            MessageBoxA(ctx->hwnd_main, "Undo Stack is empty: no recent transaction available to reverse.",
                        "Undo Stack Empty", MB_ICONINFORMATION);
            return 1;
        }
        db_save_portfolio(p, p->db_path);
        gui_buysell_refresh(ctx);
        gui_set_status(ctx, "Reversed most recent transaction via LIFO UndoStack.");
        return 1;
    }
    return 0;
}
#endif
