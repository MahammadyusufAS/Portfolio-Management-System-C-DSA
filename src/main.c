#include "portfolio.h"
#include "database.h"
#include "sqlite3_api.h"
#include "market_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include "gui.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")

static Portfolio g_portfolio;
static GuiContext g_gui;

HWND gui_register_control(GuiContext *ctx, AppView view, HWND ctrl) {
    if (!ctx || !ctrl || view < 0 || view >= VIEW_COUNT) return ctrl;
    if (ctx->view_control_counts[view] < 48) {
        ctx->view_controls[view][ctx->view_control_counts[view]++] = ctrl;
    }
    return ctrl;
}

void gui_set_status(GuiContext *ctx, const char *fmt, ...) {
    if (!ctx || !ctx->hwnd_status || !fmt) return;
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    SetWindowTextA(ctx->hwnd_status, buf);
}

void gui_refresh_active_view(GuiContext *ctx) {
    if (!ctx) return;
    switch (ctx->current_view) {
        case VIEW_DASHBOARD:    gui_dashboard_refresh(ctx); break;
        case VIEW_HOLDINGS:     gui_holdings_refresh(ctx); break;
        case VIEW_BUY_SELL:     gui_buysell_refresh(ctx); break;
        case VIEW_TRANSACTIONS: gui_transactions_refresh(ctx); break;
        case VIEW_WATCHLIST:    gui_watchlist_refresh(ctx); break;
        case VIEW_ANALYTICS:    gui_analytics_refresh(ctx); break;
        case VIEW_SETTINGS:     gui_settings_refresh(ctx); break;
        default: break;
    }
}

void gui_switch_view(GuiContext *ctx, AppView view) {
    if (!ctx || view < 0 || view >= VIEW_COUNT) return;
    ctx->current_view = view;
    for (int v = 0; v < VIEW_COUNT; ++v) {
        int show = (v == (int)view) ? SW_SHOW : SW_HIDE;
        for (size_t i = 0; i < ctx->view_control_counts[v]; ++i) {
            ShowWindow(ctx->view_controls[v][i], show);
        }
    }
    gui_refresh_active_view(ctx);
    InvalidateRect(ctx->hwnd_main, NULL, TRUE);
}

void gui_resize_layout(GuiContext *ctx, int client_w, int client_h) {
    if (!ctx) return;
    int content_x = 210;
    int content_y = 20;
    int content_w = client_w - content_x - 20;
    int content_h = client_h - 60;
    if (content_w < 600) content_w = 600;
    if (content_h < 350) content_h = 350;

    gui_holdings_layout(ctx, content_x, content_y, content_w, content_h);
    gui_buysell_layout(ctx, content_x, content_y, content_w, content_h);
    gui_transactions_layout(ctx, content_x, content_y, content_w, content_h);
    gui_watchlist_layout(ctx, content_x, content_y, content_w, content_h);
    gui_analytics_layout(ctx, content_x, content_y, content_w, content_h);

    if (ctx->hwnd_status) {
        MoveWindow(ctx->hwnd_status, 200, client_h - 30, client_w - 210, 24, TRUE);
    }
    InvalidateRect(ctx->hwnd_main, NULL, TRUE);
}

static void create_sidebar_and_views(HWND hwnd) {
    memset(&g_gui, 0, sizeof(g_gui));
    g_gui.hwnd_main = hwnd;
    g_gui.portfolio = &g_portfolio;
    g_gui.rng_seed = 20261108u;

    g_gui.font_title = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    g_gui.font_bold = CreateFontA(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    g_gui.font_body = CreateFontA(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");

    static const char *nav_labels[VIEW_COUNT] = {
        "1. Dashboard",
        "2. Stock Holdings",
        "3. Buy / Sell Stocks",
        "4. Transactions",
        "5. Watchlist & Alerts",
        "6. Portfolio Analytics",
        "7. Settings & Backup"
    };
    for (int i = 0; i < VIEW_COUNT; ++i) {
        HWND btn = CreateWindowExA(0, "BUTTON", nav_labels[i],
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            14, 70 + i * 46, 166, 38, hwnd, (HMENU)(INT_PTR)(ID_NAV_DASHBOARD + i),
            GetModuleHandle(NULL), NULL);
        SendMessage(btn, WM_SETFONT, (WPARAM)g_gui.font_bold, TRUE);
        g_gui.nav_buttons[i] = btn;
    }

    g_gui.hwnd_status = CreateWindowExA(0, "STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        210, 720, 1020, 24, hwnd, NULL, GetModuleHandle(NULL), NULL);
    SendMessage(g_gui.hwnd_status, WM_SETFONT, (WPARAM)g_gui.font_body, TRUE);

    gui_dashboard_create(&g_gui);
    gui_holdings_create(&g_gui);
    gui_buysell_create(&g_gui);
    gui_transactions_create(&g_gui);
    gui_watchlist_create(&g_gui);
    gui_analytics_create(&g_gui);
    gui_settings_create(&g_gui);

    gui_switch_view(&g_gui, VIEW_DASHBOARD);
    gui_set_status(&g_gui,
        "Ready | SQLite Backend: %s | Stocks: %zu | Market Value: %.2f | Unrealized P/L: %+.2f",
        sp_sqlite_backend_name(), g_portfolio.stocks.count,
        portfolio_market_value(&g_portfolio), portfolio_unrealized_pl(&g_portfolio));
}

static void paint_sidebar(HDC hdc, RECT client_rc) {
    RECT side = { 0, 0, 194, client_rc.bottom };
    HBRUSH sb = CreateSolidBrush(RGB(15, 23, 42));
    FillRect(hdc, &side, sb);
    DeleteObject(sb);

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, g_gui.font_bold);
    SetTextColor(hdc, RGB(248, 250, 252));
    RECT brand = { 16, 18, 184, 58 };
    DrawTextA(hdc, "STOCK PORTFOLIO\nC17 + DSA ENGINE", -1, &brand, DT_LEFT);
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE:
            create_sidebar_and_views(hwnd);
            return 0;

        case WM_SIZE:
            gui_resize_layout(&g_gui, LOWORD(lp), HIWORD(lp));
            return 0;

        case WM_COMMAND: {
            int id = LOWORD(wp);
            int code = HIWORD(wp);
            if (id >= ID_NAV_DASHBOARD && id < ID_NAV_DASHBOARD + VIEW_COUNT) {
                gui_switch_view(&g_gui, (AppView)(id - ID_NAV_DASHBOARD));
                return 0;
            }
            if (gui_holdings_command(&g_gui, id, code)) return 0;
            if (gui_buysell_command(&g_gui, id, code)) return 0;
            if (gui_transactions_command(&g_gui, id, code)) return 0;
            if (gui_watchlist_command(&g_gui, id, code)) return 0;
            if (gui_analytics_command(&g_gui, id, code)) return 0;
            if (gui_settings_command(&g_gui, id, code)) return 0;
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            HBRUSH bg = CreateSolidBrush(RGB(241, 245, 249));
            FillRect(hdc, &rc, bg);
            DeleteObject(bg);

            paint_sidebar(hdc, rc);
            if (g_gui.current_view == VIEW_DASHBOARD) {
                gui_dashboard_paint(&g_gui, hdc, rc);
            } else if (g_gui.current_view == VIEW_ANALYTICS) {
                gui_analytics_paint(&g_gui, hdc, rc);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            db_save_portfolio(&g_portfolio, g_portfolio.db_path);
            portfolio_destroy(&g_portfolio);
            if (g_gui.font_title) DeleteObject(g_gui.font_title);
            if (g_gui.font_bold)  DeleteObject(g_gui.font_bold);
            if (g_gui.font_body)  DeleteObject(g_gui.font_body);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev, LPSTR cmdline, int show) {
    (void)prev; (void)cmdline;
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    portfolio_init(&g_portfolio);
    if (!db_load_portfolio(&g_portfolio, g_portfolio.db_path)) {
        portfolio_seed_sample_data(&g_portfolio);
        db_save_portfolio(&g_portfolio, g_portfolio.db_path);
    }

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = instance;
    wc.lpszClassName = "StockPortfolioDSAWin32";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(0, wc.lpszClassName,
        "Stock Portfolio Management System (C17 + Manual DSA + SQLite + Win32 GUI)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 780,
        NULL, NULL, instance, NULL);
    if (!hwnd) {
        portfolio_destroy(&g_portfolio);
        return 1;
    }

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}
#else
/* Non-Windows CLI entry point for headless verification on POSIX systems */
int main(void) {
    Portfolio p;
    portfolio_init(&p);
    if (!db_load_portfolio(&p, p.db_path)) {
        portfolio_seed_sample_data(&p);
        db_save_portfolio(&p, p.db_path);
    }
    char report[512];
    portfolio_verify_integrity(&p, report, sizeof(report));
    printf("Stock Portfolio Management System (Headless Verification Mode)\n");
    printf("%s\n", report);
    portfolio_destroy(&p);
    return 0;
}
#endif
