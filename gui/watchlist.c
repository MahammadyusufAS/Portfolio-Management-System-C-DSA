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

void gui_watchlist_create(GuiContext *ctx) {
    int cx = 210, cy = 20;
    make_ctrl(ctx, VIEW_WATCHLIST, "STATIC", "WATCHLIST & MIN-HEAP PRICE ALERTS", SS_LEFT, cx, cy, 420, 26, 0);

    make_ctrl(ctx, VIEW_WATCHLIST, "STATIC", "Ticker:", SS_LEFT, cx, cy + 36, 55, 22, 0);
    ctx->wl_ticker = make_ctrl(ctx, VIEW_WATCHLIST, "EDIT", "LT", WS_BORDER | ES_AUTOHSCROLL, cx + 60, cy + 33, 100, 26, ID_WL_TICKER);
    make_ctrl(ctx, VIEW_WATCHLIST, "BUTTON", "Add to Watchlist", BS_PUSHBUTTON, cx + 170, cy + 32, 135, 28, ID_WL_BTN_ADD);
    make_ctrl(ctx, VIEW_WATCHLIST, "BUTTON", "Remove from Watchlist", BS_PUSHBUTTON, cx + 315, cy + 32, 165, 28, ID_WL_BTN_REMOVE);

    ctx->wl_list = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
        WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
        cx, cy + 68, 1020, 215, ctx->hwnd_main, (HMENU)(INT_PTR)ID_WL_LIST,
        GetModuleHandle(NULL), NULL);
    ListView_SetExtendedListViewStyle(ctx->wl_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    gui_register_control(ctx, VIEW_WATCHLIST, ctx->wl_list);

    lv_add_col(ctx->wl_list, 0, "Ticker", 110);
    lv_add_col(ctx->wl_list, 1, "Company Name", 230);
    lv_add_col(ctx->wl_list, 2, "Category / Sector", 145);
    lv_add_col(ctx->wl_list, 3, "Previous Price", 120);
    lv_add_col(ctx->wl_list, 4, "Current Price", 120);
    lv_add_col(ctx->wl_list, 5, "Price Change", 120);
    lv_add_col(ctx->wl_list, 6, "Change %", 120);

    /* Min-Heap Priority Queue Price Alerts Controls */
    int ay = cy + 296;
    make_ctrl(ctx, VIEW_WATCHLIST, "STATIC", "Configure Price Alert (Min-Heap Priority Queue):", SS_LEFT, cx, ay, 360, 22, 0);

    make_ctrl(ctx, VIEW_WATCHLIST, "STATIC", "Ticker:", SS_LEFT, cx, ay + 30, 50, 22, 0);
    ctx->alert_ticker = make_ctrl(ctx, VIEW_WATCHLIST, "EDIT", "RELIANCE", WS_BORDER | ES_AUTOHSCROLL, cx + 55, ay + 27, 95, 26, ID_ALERT_TICKER);

    make_ctrl(ctx, VIEW_WATCHLIST, "STATIC", "Target:", SS_LEFT, cx + 160, ay + 30, 50, 22, 0);
    ctx->alert_price = make_ctrl(ctx, VIEW_WATCHLIST, "EDIT", "1500.00", WS_BORDER | ES_AUTOHSCROLL, cx + 215, ay + 27, 95, 26, ID_ALERT_PRICE);

    ctx->alert_cond = make_ctrl(ctx, VIEW_WATCHLIST, "COMBOBOX", "", CBS_DROPDOWNLIST, cx + 320, ay + 26, 110, 120, ID_ALERT_COND);
    SendMessageA(ctx->alert_cond, CB_ADDSTRING, 0, (LPARAM)"ABOVE (>=)");
    SendMessageA(ctx->alert_cond, CB_ADDSTRING, 0, (LPARAM)"BELOW (<=)");
    SendMessageA(ctx->alert_cond, CB_SETCURSEL, 0, 0);

    make_ctrl(ctx, VIEW_WATCHLIST, "STATIC", "Note / ID:", SS_LEFT, cx + 440, ay + 30, 65, 22, 0);
    ctx->alert_notes = make_ctrl(ctx, VIEW_WATCHLIST, "EDIT", "Breakout target", WS_BORDER | ES_AUTOHSCROLL, cx + 510, ay + 27, 175, 26, ID_ALERT_NOTES);

    make_ctrl(ctx, VIEW_WATCHLIST, "BUTTON", "Add Alert (Heap Push)", BS_PUSHBUTTON, cx + 695, ay + 26, 160, 28, ID_ALERT_BTN_ADD);
    make_ctrl(ctx, VIEW_WATCHLIST, "BUTTON", "Pop Top Alert", BS_PUSHBUTTON, cx + 865, ay + 26, 140, 28, ID_ALERT_BTN_REMOVE);

    ctx->alert_list = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
        WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
        cx, ay + 64, 1020, 220, ctx->hwnd_main, (HMENU)(INT_PTR)ID_ALERT_LIST,
        GetModuleHandle(NULL), NULL);
    ListView_SetExtendedListViewStyle(ctx->alert_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    gui_register_control(ctx, VIEW_WATCHLIST, ctx->alert_list);

    lv_add_col(ctx->alert_list, 0, "Heap Idx / ID", 105);
    lv_add_col(ctx->alert_list, 1, "Ticker", 105);
    lv_add_col(ctx->alert_list, 2, "Condition", 105);
    lv_add_col(ctx->alert_list, 3, "Target Price", 115);
    lv_add_col(ctx->alert_list, 4, "Current Price", 115);
    lv_add_col(ctx->alert_list, 5, "Priority Key", 115);
    lv_add_col(ctx->alert_list, 6, "Status", 120);
    lv_add_col(ctx->alert_list, 7, "Alert Notes", 220);
}

void gui_watchlist_refresh(GuiContext *ctx) {
    if (!ctx || !ctx->wl_list || !ctx->alert_list || !ctx->portfolio) return;
    Portfolio *p = ctx->portfolio;
    ListView_DeleteAllItems(ctx->wl_list);
    ListView_DeleteAllItems(ctx->alert_list);

    char buf[128];
    int row = 0;
    for (size_t i = 0; i < p->stocks.count; ++i) {
        const Stock *s = &p->stocks.items[i];
        if (!s->in_watchlist) continue;
        double prev = s->previous_price > 0.0 ? s->previous_price : s->current_price;
        double diff = s->current_price - prev;
        double pct = prev > 0.0 ? (diff / prev) * 100.0 : 0.0;

        lv_set_cell(ctx->wl_list, row, 0, s->ticker);
        lv_set_cell(ctx->wl_list, row, 1, s->name);
        lv_set_cell(ctx->wl_list, row, 2, s->sector);
        snprintf(buf, sizeof(buf), "%.2f", prev);
        lv_set_cell(ctx->wl_list, row, 3, buf);
        snprintf(buf, sizeof(buf), "%.2f", s->current_price);
        lv_set_cell(ctx->wl_list, row, 4, buf);
        snprintf(buf, sizeof(buf), "%+.2f", diff);
        lv_set_cell(ctx->wl_list, row, 5, buf);
        snprintf(buf, sizeof(buf), "%+.2f%%", pct);
        lv_set_cell(ctx->wl_list, row, 6, buf);
        row++;
    }

    for (size_t i = 0; i < p->price_alerts.size; ++i) {
        const PriceAlert *a = &p->price_alerts.items[i];
        snprintf(buf, sizeof(buf), "[%zu] #%d", i, a->id);
        lv_set_cell(ctx->alert_list, (int)i, 0, buf);
        lv_set_cell(ctx->alert_list, (int)i, 1, a->ticker);
        lv_set_cell(ctx->alert_list, (int)i, 2, a->condition == ALERT_ABOVE ? "ABOVE >=" : "BELOW <=");
        snprintf(buf, sizeof(buf), "%.2f", a->target_price);
        lv_set_cell(ctx->alert_list, (int)i, 3, buf);
        snprintf(buf, sizeof(buf), "%.2f", a->current_price);
        lv_set_cell(ctx->alert_list, (int)i, 4, buf);
        snprintf(buf, sizeof(buf), "%.4f", a->priority_key);
        lv_set_cell(ctx->alert_list, (int)i, 5, buf);
        lv_set_cell(ctx->alert_list, (int)i, 6, a->triggered ? "TRIGGERED!" : "ARMED");
        lv_set_cell(ctx->alert_list, (int)i, 7, a->notes);
    }
}

void gui_watchlist_layout(GuiContext *ctx, int content_x, int content_y, int content_w, int content_h) {
    if (!ctx || !ctx->wl_list || !ctx->alert_list) return;
    MoveWindow(ctx->wl_list, content_x, content_y + 68, content_w, 215, TRUE);
    int ay = content_y + 296;
    MoveWindow(ctx->alert_list, content_x, ay + 64, content_w, content_h - 372, TRUE);
}

int gui_watchlist_command(GuiContext *ctx, int id, int notify_code) {
    (void)notify_code;
    Portfolio *p = ctx->portfolio;
    char ticker[32] = "", price_str[32] = "", notes[64] = "";

    if (id == ID_WL_BTN_ADD || id == ID_WL_BTN_REMOVE) {
        GetWindowTextA(ctx->wl_ticker, ticker, sizeof(ticker));
        int add = (id == ID_WL_BTN_ADD);
        if (!portfolio_set_watchlist(p, ticker, add)) {
            MessageBoxA(ctx->hwnd_main, "Ticker not found in catalog.", "Watchlist", MB_ICONWARNING);
            return 1;
        }
        db_save_portfolio(p, p->db_path);
        gui_watchlist_refresh(ctx);
        gui_set_status(ctx, "%s %s watchlist.", ticker, add ? "added to" : "removed from");
        return 1;
    }
    if (id == ID_ALERT_BTN_ADD) {
        GetWindowTextA(ctx->alert_ticker, ticker, sizeof(ticker));
        GetWindowTextA(ctx->alert_price, price_str, sizeof(price_str));
        GetWindowTextA(ctx->alert_notes, notes, sizeof(notes));
        double target = strtod(price_str, NULL);
        int csel = (int)SendMessageA(ctx->alert_cond, CB_GETCURSEL, 0, 0);
        AlertCondition cond = (csel == 1) ? ALERT_BELOW : ALERT_ABOVE;
        if (!portfolio_add_alert(p, ticker, target, cond, notes)) {
            MessageBoxA(ctx->hwnd_main, "Enter an existing stock ticker and positive target price.",
                        "Alert Validation", MB_ICONWARNING);
            return 1;
        }
        db_save_portfolio(p, p->db_path);
        gui_watchlist_refresh(ctx);
        gui_set_status(ctx, "Pushed price alert for %s @ %.2f into Min-Heap.", ticker, target);
        return 1;
    }
    if (id == ID_ALERT_BTN_REMOVE) {
        PriceAlert popped;
        if (!heap_pop(&p->price_alerts, &popped)) {
            MessageBoxA(ctx->hwnd_main, "Alert Min-Heap is empty.", "Min-Heap", MB_ICONINFORMATION);
            return 1;
        }
        db_save_portfolio(p, p->db_path);
        gui_watchlist_refresh(ctx);
        gui_set_status(ctx, "Popped root alert #%d (%s target %.2f) from Min-Heap.",
                       popped.id, popped.ticker, popped.target_price);
        return 1;
    }
    return 0;
}
#endif
