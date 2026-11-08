#ifndef GUI_H
#define GUI_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>

#include "portfolio.h"
#include "database.h"
#include "market_data.h"

typedef enum {
    VIEW_DASHBOARD    = 0,
    VIEW_HOLDINGS     = 1,
    VIEW_BUY_SELL     = 2,
    VIEW_TRANSACTIONS = 3,
    VIEW_WATCHLIST    = 4,
    VIEW_ANALYTICS    = 5,
    VIEW_SETTINGS     = 6,
    VIEW_COUNT        = 7
} AppView;

/* Control Command IDs */
#define ID_NAV_DASHBOARD      1001
#define ID_NAV_HOLDINGS       1002
#define ID_NAV_BUY_SELL       1003
#define ID_NAV_TRANSACTIONS   1004
#define ID_NAV_WATCHLIST      1005
#define ID_NAV_ANALYTICS      1006
#define ID_NAV_SETTINGS       1007

/* Holdings View Controls */
#define ID_HOLD_LIST          1101
#define ID_HOLD_SEARCH        1102
#define ID_HOLD_SORT_FIELD    1103
#define ID_HOLD_SORT_ALG      1104
#define ID_HOLD_SORT_ORDER    1105
#define ID_HOLD_BTN_SORT      1106
#define ID_HOLD_TICKER        1107
#define ID_HOLD_NAME          1108
#define ID_HOLD_SECTOR        1109
#define ID_HOLD_PRICE         1110
#define ID_HOLD_BTN_ADD       1111
#define ID_HOLD_BTN_UPDATE    1112
#define ID_HOLD_BTN_REMOVE    1113

/* Buy/Sell View Controls */
#define ID_TRADE_TICKER       1201
#define ID_TRADE_QTY          1202
#define ID_TRADE_PRICE        1203
#define ID_TRADE_BTN_BUY      1204
#define ID_TRADE_BTN_SELL     1205
#define ID_TRADE_BTN_QUEUE    1206
#define ID_TRADE_BTN_EXEC_Q   1207
#define ID_TRADE_BTN_UNDO     1208
#define ID_TRADE_QUEUE_LIST   1209

/* Transactions View Controls */
#define ID_TX_LIST            1301
#define ID_TX_FILTER_TICKER   1302
#define ID_TX_FILTER_TYPE     1303
#define ID_TX_FILTER_START    1304
#define ID_TX_FILTER_END      1305
#define ID_TX_BTN_FILTER      1306
#define ID_TX_BTN_CLEAR       1307
#define ID_TX_BTN_UNDO        1308

/* Watchlist & Alerts View Controls */
#define ID_WL_LIST            1401
#define ID_WL_TICKER          1402
#define ID_WL_BTN_ADD         1403
#define ID_WL_BTN_REMOVE      1404
#define ID_ALERT_LIST         1405
#define ID_ALERT_TICKER       1406
#define ID_ALERT_PRICE        1407
#define ID_ALERT_COND         1408
#define ID_ALERT_NOTES        1409
#define ID_ALERT_BTN_ADD      1410
#define ID_ALERT_BTN_REMOVE   1411

/* Analytics View Controls */
#define ID_ANA_BENCH_TICKER   1501
#define ID_ANA_BTN_BENCH      1502
#define ID_ANA_SECTOR_LIST    1503
#define ID_ANA_BENCH_LIST     1504

/* Settings View Controls */
#define ID_SET_USERNAME       1601
#define ID_SET_EMAIL          1602
#define ID_SET_CURRENCY       1603
#define ID_SET_CASH           1604
#define ID_SET_BTN_SAVE_USER  1605
#define ID_SET_BACKUP_PATH    1606
#define ID_SET_BTN_BACKUP     1607
#define ID_SET_BTN_RESTORE    1608
#define ID_SET_BTN_INTEGRITY  1609
#define ID_SET_BTN_SIMULATE   1610
#define ID_SET_BTN_SNAPSHOT   1611

typedef struct {
    HWND hwnd_main;
    HWND hwnd_status;
    HFONT font_title;
    HFONT font_Sub;
    HFONT font_body;
    HFONT font_bold;
    HFONT font_mono;
    AppView current_view;
    Portfolio *portfolio;
    SortField sort_field;
    SortAlgorithm sort_alg;
    SortOrder sort_order;
    SortStats last_sort_stats;
    SearchComparisonReport last_search_report;
    unsigned int rng_seed;

    /* Navigation sidebar buttons */
    HWND nav_buttons[VIEW_COUNT];

    /* View-specific control arrays for clean show/hide */
    HWND view_controls[VIEW_COUNT][48];
    size_t view_control_counts[VIEW_COUNT];

    /* Key shared control handles */
    HWND hold_list, hold_search, hold_sort_field, hold_sort_alg, hold_sort_order;
    HWND hold_ticker, hold_name, hold_sector, hold_price;

    HWND trade_ticker, trade_qty, trade_price, trade_queue_list;
    HWND tx_list, tx_filter_ticker, tx_filter_type, tx_filter_start, tx_filter_end;
    HWND wl_list, wl_ticker, alert_list, alert_ticker, alert_price, alert_cond, alert_notes;
    HWND ana_bench_ticker, ana_sector_list, ana_bench_list;
    HWND set_username, set_email, set_currency, set_cash, set_backup_path;
} GuiContext;

/* Helper registration & status update */
HWND gui_register_control(GuiContext *ctx, AppView view, HWND ctrl);
void gui_set_status(GuiContext *ctx, const char *fmt, ...);
void gui_switch_view(GuiContext *ctx, AppView view);
void gui_refresh_active_view(GuiContext *ctx);
void gui_resize_layout(GuiContext *ctx, int client_w, int client_h);

/* Module-specific GUI builders, painters, and command handlers */
void gui_dashboard_create(GuiContext *ctx);
void gui_dashboard_refresh(GuiContext *ctx);
void gui_dashboard_paint(GuiContext *ctx, HDC hdc, RECT client_rc);

void gui_holdings_create(GuiContext *ctx);
void gui_holdings_refresh(GuiContext *ctx);
void gui_holdings_layout(GuiContext *ctx, int content_x, int content_y, int content_w, int content_h);
int  gui_holdings_command(GuiContext *ctx, int id, int notify_code);

void gui_buysell_create(GuiContext *ctx);
void gui_buysell_refresh(GuiContext *ctx);
void gui_buysell_paint(GuiContext *ctx, HDC hdc, RECT client_rc);
void gui_buysell_layout(GuiContext *ctx, int content_x, int content_y, int content_w, int content_h);
int  gui_buysell_command(GuiContext *ctx, int id, int notify_code);

void gui_transactions_create(GuiContext *ctx);
void gui_transactions_refresh(GuiContext *ctx);
void gui_transactions_layout(GuiContext *ctx, int content_x, int content_y, int content_w, int content_h);
int  gui_transactions_command(GuiContext *ctx, int id, int notify_code);

void gui_watchlist_create(GuiContext *ctx);
void gui_watchlist_refresh(GuiContext *ctx);
void gui_watchlist_layout(GuiContext *ctx, int content_x, int content_y, int content_w, int content_h);
int  gui_watchlist_command(GuiContext *ctx, int id, int notify_code);

void gui_analytics_create(GuiContext *ctx);
void gui_analytics_refresh(GuiContext *ctx);
void gui_analytics_paint(GuiContext *ctx, HDC hdc, RECT client_rc);
void gui_analytics_layout(GuiContext *ctx, int content_x, int content_y, int content_w, int content_h);
int  gui_analytics_command(GuiContext *ctx, int id, int notify_code);

void gui_settings_create(GuiContext *ctx);
void gui_settings_refresh(GuiContext *ctx);
void gui_settings_paint(GuiContext *ctx, HDC hdc, RECT client_rc);
int  gui_settings_command(GuiContext *ctx, int id, int notify_code);

#endif /* _WIN32 */
#endif /* GUI_H */
