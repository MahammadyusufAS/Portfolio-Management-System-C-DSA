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

void gui_transactions_create(GuiContext *ctx) {
    int cx = 210, cy = 20;
    make_ctrl(ctx, VIEW_TRANSACTIONS, "STATIC", "TRANSACTION HISTORY (DOUBLY LINKED LIST)", SS_LEFT, cx, cy, 450, 26, 0);

    make_ctrl(ctx, VIEW_TRANSACTIONS, "STATIC", "Ticker:", SS_LEFT, cx, cy + 38, 50, 22, 0);
    ctx->tx_filter_ticker = make_ctrl(ctx, VIEW_TRANSACTIONS, "EDIT", "", WS_BORDER | ES_AUTOHSCROLL, cx + 55, cy + 35, 95, 26, ID_TX_FILTER_TICKER);

    make_ctrl(ctx, VIEW_TRANSACTIONS, "STATIC", "Type:", SS_LEFT, cx + 165, cy + 38, 40, 22, 0);
    ctx->tx_filter_type = make_ctrl(ctx, VIEW_TRANSACTIONS, "COMBOBOX", "", CBS_DROPDOWNLIST, cx + 210, cy + 34, 95, 140, ID_TX_FILTER_TYPE);
    SendMessageA(ctx->tx_filter_type, CB_ADDSTRING, 0, (LPARAM)"ALL");
    SendMessageA(ctx->tx_filter_type, CB_ADDSTRING, 0, (LPARAM)"BUY");
    SendMessageA(ctx->tx_filter_type, CB_ADDSTRING, 0, (LPARAM)"SELL");
    SendMessageA(ctx->tx_filter_type, CB_SETCURSEL, 0, 0);

    make_ctrl(ctx, VIEW_TRANSACTIONS, "STATIC", "From Date:", SS_LEFT, cx + 320, cy + 38, 75, 22, 0);
    ctx->tx_filter_start = make_ctrl(ctx, VIEW_TRANSACTIONS, "EDIT", "", WS_BORDER | ES_AUTOHSCROLL, cx + 400, cy + 35, 105, 26, ID_TX_FILTER_START);

    make_ctrl(ctx, VIEW_TRANSACTIONS, "STATIC", "To Date:", SS_LEFT, cx + 520, cy + 38, 60, 22, 0);
    ctx->tx_filter_end = make_ctrl(ctx, VIEW_TRANSACTIONS, "EDIT", "", WS_BORDER | ES_AUTOHSCROLL, cx + 585, cy + 35, 105, 26, ID_TX_FILTER_END);

    make_ctrl(ctx, VIEW_TRANSACTIONS, "BUTTON", "Filter", BS_PUSHBUTTON, cx + 705, cy + 34, 85, 28, ID_TX_BTN_FILTER);
    make_ctrl(ctx, VIEW_TRANSACTIONS, "BUTTON", "Reset", BS_PUSHBUTTON, cx + 800, cy + 34, 80, 28, ID_TX_BTN_CLEAR);
    make_ctrl(ctx, VIEW_TRANSACTIONS, "BUTTON", "Undo Latest", BS_PUSHBUTTON, cx + 890, cy + 34, 120, 28, ID_TX_BTN_UNDO);

    ctx->tx_list = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
        WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        cx, cy + 75, 1020, 510, ctx->hwnd_main, (HMENU)(INT_PTR)ID_TX_LIST,
        GetModuleHandle(NULL), NULL);
    ListView_SetExtendedListViewStyle(ctx->tx_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    gui_register_control(ctx, VIEW_TRANSACTIONS, ctx->tx_list);

    lv_add_col(ctx->tx_list, 0, "Tx ID", 75);
    lv_add_col(ctx->tx_list, 1, "Ticker", 115);
    lv_add_col(ctx->tx_list, 2, "Type", 90);
    lv_add_col(ctx->tx_list, 3, "Quantity", 95);
    lv_add_col(ctx->tx_list, 4, "Unit Price", 120);
    lv_add_col(ctx->tx_list, 5, "Total Amount", 140);
    lv_add_col(ctx->tx_list, 6, "Realized P/L", 135);
    lv_add_col(ctx->tx_list, 7, "Timestamp", 210);
}

void gui_transactions_refresh(GuiContext *ctx) {
    if (!ctx || !ctx->tx_list || !ctx->portfolio) return;
    ListView_DeleteAllItems(ctx->tx_list);

    char ticker_f[32] = "", type_f[16] = "ALL", start_f[32] = "", end_f[32] = "";
    GetWindowTextA(ctx->tx_filter_ticker, ticker_f, sizeof(ticker_f));
    int tsel = (int)SendMessageA(ctx->tx_filter_type, CB_GETCURSEL, 0, 0);
    if (tsel == 1) strcpy(type_f, "BUY");
    else if (tsel == 2) strcpy(type_f, "SELL");
    GetWindowTextA(ctx->tx_filter_start, start_f, sizeof(start_f));
    GetWindowTextA(ctx->tx_filter_end, end_f, sizeof(end_f));

    const TransactionNode *nodes[512];
    size_t n = dll_filter(&ctx->portfolio->transactions, ticker_f, type_f, start_f, end_f, nodes, 512, 1);

    char buf[128];
    for (size_t i = 0; i < n; ++i) {
        const TransactionNode *tx = nodes[i];
        snprintf(buf, sizeof(buf), "#%lu", tx->id);
        lv_set_cell(ctx->tx_list, (int)i, 0, buf);
        lv_set_cell(ctx->tx_list, (int)i, 1, tx->ticker);
        lv_set_cell(ctx->tx_list, (int)i, 2, tx->type);
        snprintf(buf, sizeof(buf), "%d", tx->quantity);
        lv_set_cell(ctx->tx_list, (int)i, 3, buf);
        snprintf(buf, sizeof(buf), "%.2f", tx->price);
        lv_set_cell(ctx->tx_list, (int)i, 4, buf);
        snprintf(buf, sizeof(buf), "%.2f", tx->total);
        lv_set_cell(ctx->tx_list, (int)i, 5, buf);
        snprintf(buf, sizeof(buf), "%+.2f", tx->realized_pl);
        lv_set_cell(ctx->tx_list, (int)i, 6, buf);
        lv_set_cell(ctx->tx_list, (int)i, 7, tx->timestamp);
    }
}

void gui_transactions_layout(GuiContext *ctx, int content_x, int content_y, int content_w, int content_h) {
    if (!ctx || !ctx->tx_list) return;
    MoveWindow(ctx->tx_list, content_x, content_y + 75, content_w, content_h - 88, TRUE);
}

int gui_transactions_command(GuiContext *ctx, int id, int notify_code) {
    (void)notify_code;
    if (id == ID_TX_BTN_FILTER) {
        gui_transactions_refresh(ctx);
        gui_set_status(ctx, "Filtered transaction history via Doubly Linked List traversal.");
        return 1;
    }
    if (id == ID_TX_BTN_CLEAR) {
        SetWindowTextA(ctx->tx_filter_ticker, "");
        SendMessageA(ctx->tx_filter_type, CB_SETCURSEL, 0, 0);
        SetWindowTextA(ctx->tx_filter_start, "");
        SetWindowTextA(ctx->tx_filter_end, "");
        gui_transactions_refresh(ctx);
        gui_set_status(ctx, "Cleared transaction history filters.");
        return 1;
    }
    if (id == ID_TX_BTN_UNDO) {
        if (!portfolio_undo(ctx->portfolio)) {
            MessageBoxA(ctx->hwnd_main, "No eligible transaction in UndoStack to reverse.", "Undo", MB_ICONINFORMATION);
            return 1;
        }
        db_save_portfolio(ctx->portfolio, ctx->portfolio->db_path);
        gui_transactions_refresh(ctx);
        gui_set_status(ctx, "Most recent transaction undone and unlinked from Doubly Linked List.");
        return 1;
    }
    return 0;
}
#endif
