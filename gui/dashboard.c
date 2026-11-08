#include "gui.h"
#ifdef _WIN32
#include <stdio.h>
#include <math.h>

void gui_dashboard_create(GuiContext *ctx) {
    /* Dashboard uses custom GDI vector cards and charts rendered in WM_PAINT */
    (void)ctx;
}

void gui_dashboard_refresh(GuiContext *ctx) {
    if (!ctx || !ctx->hwnd_main) return;
    InvalidateRect(ctx->hwnd_main, NULL, TRUE);
}

static void draw_kpi_card(HDC hdc, HFONT font_sub, HFONT font_title,
                          int x, int y, int w, int h,
                          const char *label, const char *value_str,
                          COLORREF accent_color, COLORREF value_color) {
    RECT rc = { x, y, x + w, y + h };
    HBRUSH card_brush = CreateSolidBrush(RGB(255, 255, 255));
    HPEN border_pen = CreatePen(PS_SOLID, 1, RGB(218, 224, 235));
    HGDIOBJ old_b = SelectObject(hdc, card_brush);
    HGDIOBJ old_p = SelectObject(hdc, border_pen);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 10, 10);
    SelectObject(hdc, old_b);
    SelectObject(hdc, old_p);
    DeleteObject(card_brush);
    DeleteObject(border_pen);

    /* Top accent bar */
    RECT bar = { x + 2, y + 2, x + w - 2, y + 7 };
    HBRUSH acc_b = CreateSolidBrush(accent_color);
    FillRect(hdc, &bar, acc_b);
    DeleteObject(acc_b);

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, font_sub);
    SetTextColor(hdc, RGB(95, 109, 126));
    RECT lbl_rc = { x + 14, y + 16, x + w - 10, y + 38 };
    DrawTextA(hdc, label, -1, &lbl_rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    SelectObject(hdc, font_title);
    SetTextColor(hdc, value_color);
    RECT val_rc = { x + 14, y + 40, x + w - 10, y + h - 10 };
    DrawTextA(hdc, value_str, -1, &val_rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
}

void gui_dashboard_paint(GuiContext *ctx, HDC hdc, RECT client_rc) {
    if (!ctx || !ctx->portfolio) return;
    Portfolio *p = ctx->portfolio;
    PortfolioAnalytics ana;
    portfolio_compute_analytics(p, &ana);

    int cx = 210;
    int cy = 20;
    int cw = (client_rc.right - cx - 25);
    if (cw < 720) cw = 720;

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, ctx->font_title);
    SetTextColor(hdc, RGB(22, 34, 56));
    RECT hdr = { cx, cy, cx + cw, cy + 32 };
    DrawTextA(hdc, "FINANCIAL PORTFOLIO DASHBOARD", -1, &hdr, DT_LEFT | DT_SINGLELINE);

    /* 5 KPI Cards */
    int card_y = cy + 42;
    int gap = 12;
    int card_w = (cw - gap * 4) / 5;
    int card_h = 86;
    char buf[96];

    snprintf(buf, sizeof(buf), "%s %.2f", p->user.currency, ana.total_market_value);
    draw_kpi_card(hdc, ctx->font_body, ctx->font_bold, cx, card_y, card_w, card_h,
                  "Total Market Value", buf, RGB(30, 102, 245), RGB(20, 32, 50));

    snprintf(buf, sizeof(buf), "%s %.2f", p->user.currency, ana.total_invested);
    draw_kpi_card(hdc, ctx->font_body, ctx->font_bold, cx + (card_w + gap), card_y, card_w, card_h,
                  "Total Invested", buf, RGB(92, 107, 192), RGB(20, 32, 50));

    snprintf(buf, sizeof(buf), "%+.2f (%+.1f%%)", ana.total_unrealized_pl, ana.overall_return_pct);
    draw_kpi_card(hdc, ctx->font_body, ctx->font_bold, cx + (card_w + gap) * 2, card_y, card_w, card_h,
                  "Unrealized P/L", buf,
                  ana.total_unrealized_pl >= 0 ? RGB(22, 163, 74) : RGB(220, 38, 38),
                  ana.total_unrealized_pl >= 0 ? RGB(21, 128, 61) : RGB(185, 28, 28));

    snprintf(buf, sizeof(buf), "%+.2f", ana.total_realized_pl);
    draw_kpi_card(hdc, ctx->font_body, ctx->font_bold, cx + (card_w + gap) * 3, card_y, card_w, card_h,
                  "Realized P/L", buf,
                  ana.total_realized_pl >= 0 ? RGB(13, 148, 136) : RGB(225, 29, 72),
                  ana.total_realized_pl >= 0 ? RGB(15, 118, 110) : RGB(190, 18, 60));

    snprintf(buf, sizeof(buf), "%zu Held / Score %.0f", ana.active_holdings_count, ana.diversification_score);
    draw_kpi_card(hdc, ctx->font_body, ctx->font_bold, cx + (card_w + gap) * 4, card_y, card_w, card_h,
                  "Holdings & Diversif.", buf, RGB(217, 119, 6), RGB(20, 32, 50));

    /* Left Panel: Historical Portfolio Performance Line Chart */
    int chart_y = card_y + card_h + 18;
    int left_w = (cw * 56) / 100;
    int right_w = cw - left_w - 16;
    int panel_h = 250;

    RECT perf_rc = { cx, chart_y, cx + left_w, chart_y + panel_h };
    HBRUSH white_b = CreateSolidBrush(RGB(255, 255, 255));
    HPEN border_p = CreatePen(PS_SOLID, 1, RGB(218, 224, 235));
    SelectObject(hdc, white_b);
    SelectObject(hdc, border_p);
    RoundRect(hdc, perf_rc.left, perf_rc.top, perf_rc.right, perf_rc.bottom, 10, 10);

    SelectObject(hdc, ctx->font_bold);
    SetTextColor(hdc, RGB(25, 38, 62));
    RECT ptitle = { perf_rc.left + 16, perf_rc.top + 12, perf_rc.right - 16, perf_rc.top + 34 };
    DrawTextA(hdc, "Portfolio Historical Value Curve (Snapshots)", -1, &ptitle, DT_LEFT | DT_SINGLELINE);

    int plot_x = perf_rc.left + 64;
    int plot_y = perf_rc.top + 44;
    int plot_w = left_w - 84;
    int plot_h = panel_h - 74;

    /* Draw horizontal gridlines */
    HPEN grid_pen = CreatePen(PS_DOT, 1, RGB(230, 235, 242));
    SelectObject(hdc, grid_pen);
    for (int g = 0; g <= 4; ++g) {
        int gy = plot_y + (plot_h * g) / 4;
        MoveToEx(hdc, plot_x, gy, NULL);
        LineTo(hdc, plot_x + plot_w, gy);
    }
    DeleteObject(grid_pen);

    if (p->snapshot_count > 0) {
        double min_v = p->snapshots[0].total_market_value;
        double max_v = p->snapshots[0].total_market_value;
        for (size_t i = 0; i < p->snapshot_count; ++i) {
            if (p->snapshots[i].total_market_value < min_v) min_v = p->snapshots[i].total_market_value;
            if (p->snapshots[i].total_market_value > max_v) max_v = p->snapshots[i].total_market_value;
            if (p->snapshots[i].total_invested < min_v) min_v = p->snapshots[i].total_invested;
            if (p->snapshots[i].total_invested > max_v) max_v = p->snapshots[i].total_invested;
        }
        double span = (max_v - min_v);
        if (span < 100.0) { min_v -= 50.0; max_v += 50.0; span = max_v - min_v; }

        /* Y-axis labels */
        SelectObject(hdc, ctx->font_body);
        SetTextColor(hdc, RGB(100, 116, 139));
        snprintf(buf, sizeof(buf), "%.0f", max_v);
        TextOutA(hdc, perf_rc.left + 10, plot_y - 6, buf, (int)strlen(buf));
        snprintf(buf, sizeof(buf), "%.0f", min_v);
        TextOutA(hdc, perf_rc.left + 10, plot_y + plot_h - 10, buf, (int)strlen(buf));

        /* Plot Invested baseline (dashed slate) and Market Value line (solid blue) */
        HPEN inv_pen = CreatePen(PS_DASH, 1, RGB(148, 163, 184));
        SelectObject(hdc, inv_pen);
        for (size_t i = 0; i < p->snapshot_count; ++i) {
            int offset_x = (p->snapshot_count > 1)
                ? (int)((i * (size_t)plot_w) / (p->snapshot_count - 1))
                : (plot_w / 2);
            int px = plot_x + offset_x;
            int py = plot_y + plot_h - (int)(((p->snapshots[i].total_invested - min_v) / span) * plot_h);
            if (i == 0) MoveToEx(hdc, px, py, NULL);
            else LineTo(hdc, px, py);
        }
        DeleteObject(inv_pen);

        HPEN mv_pen = CreatePen(PS_SOLID, 3, RGB(37, 99, 235));
        SelectObject(hdc, mv_pen);
        for (size_t i = 0; i < p->snapshot_count; ++i) {
            int offset_x = (p->snapshot_count > 1)
                ? (int)((i * (size_t)plot_w) / (p->snapshot_count - 1))
                : (plot_w / 2);
            int px = plot_x + offset_x;
            int py = plot_y + plot_h - (int)(((p->snapshots[i].total_market_value - min_v) / span) * plot_h);
            if (i == 0) MoveToEx(hdc, px, py, NULL);
            else LineTo(hdc, px, py);
            Ellipse(hdc, px - 4, py - 4, px + 4, py + 4);
        }
        DeleteObject(mv_pen);
    }

    /* Right Panel: Asset Allocation by Sector Chart */
    RECT alloc_rc = { cx + left_w + 16, chart_y, cx + left_w + 16 + right_w, chart_y + panel_h };
    SelectObject(hdc, white_b);
    SelectObject(hdc, border_p);
    RoundRect(hdc, alloc_rc.left, alloc_rc.top, alloc_rc.right, alloc_rc.bottom, 10, 10);

    SelectObject(hdc, ctx->font_bold);
    SetTextColor(hdc, RGB(25, 38, 62));
    RECT atitle = { alloc_rc.left + 16, alloc_rc.top + 12, alloc_rc.right - 16, alloc_rc.top + 34 };
    DrawTextA(hdc, "Asset Allocation by Sector", -1, &atitle, DT_LEFT | DT_SINGLELINE);

    static const COLORREF sector_colors[6] = {
        RGB(37, 99, 235), RGB(16, 185, 129), RGB(245, 158, 11),
        RGB(139, 92, 246), RGB(236, 72, 153), RGB(14, 165, 233)
    };
    int sy = alloc_rc.top + 44;
    for (size_t k = 0; k < ana.sector_count && k < 6; ++k) {
        SelectObject(hdc, ctx->font_body);
        SetTextColor(hdc, RGB(51, 65, 85));
        snprintf(buf, sizeof(buf), "%s (%.1f%%)", ana.sectors[k].sector, ana.sectors[k].weight_percent);
        TextOutA(hdc, alloc_rc.left + 16, sy, buf, (int)strlen(buf));

        int bar_x = alloc_rc.left + 160;
        int max_bw = right_w - 180;
        int bw = (int)((ana.sectors[k].weight_percent / 100.0) * max_bw);
        if (bw < 4) bw = 4;
        RECT brc = { bar_x, sy + 2, bar_x + bw, sy + 18 };
        HBRUSH sb = CreateSolidBrush(sector_colors[k % 6]);
        FillRect(hdc, &brc, sb);
        DeleteObject(sb);
        sy += 32;
    }

    /* Bottom Panel: Individual Stock Performance Horizontal Bar Chart */
    int bot_y = chart_y + panel_h + 16;
    int bot_h = 215;
    RECT ind_rc = { cx, bot_y, cx + cw, bot_y + bot_h };
    SelectObject(hdc, white_b);
    SelectObject(hdc, border_p);
    RoundRect(hdc, ind_rc.left, ind_rc.top, ind_rc.right, ind_rc.bottom, 10, 10);

    SelectObject(hdc, ctx->font_bold);
    SetTextColor(hdc, RGB(25, 38, 62));
    RECT ititle = { ind_rc.left + 16, ind_rc.top + 12, ind_rc.right - 16, ind_rc.top + 34 };
    DrawTextA(hdc, "Individual Stock Holdings Performance (Market Value & Unrealized P/L)", -1, &ititle, DT_LEFT | DT_SINGLELINE);

    double max_stock_mv = 1.0;
    for (size_t i = 0; i < p->stocks.count; ++i) {
        if (p->stocks.items[i].market_value > max_stock_mv) {
            max_stock_mv = p->stocks.items[i].market_value;
        }
    }

    int row_y = ind_rc.top + 42;
    size_t drawn = 0;
    for (size_t i = 0; i < p->stocks.count && drawn < 6; ++i) {
        const Stock *s = &p->stocks.items[i];
        if (s->quantity <= 0) continue;
        SelectObject(hdc, ctx->font_body);
        SetTextColor(hdc, RGB(30, 41, 59));
        snprintf(buf, sizeof(buf), "%-10s Qty:%d", s->ticker, s->quantity);
        TextOutA(hdc, ind_rc.left + 16, row_y, buf, (int)strlen(buf));

        int bx = ind_rc.left + 165;
        int max_w = cw - 390;
        if (max_w < 100) max_w = 100;
        int bw = (int)((s->market_value / max_stock_mv) * max_w);
        if (bw < 6) bw = 6;
        RECT bar = { bx, row_y + 2, bx + bw, row_y + 18 };
        HBRUSH hb = CreateSolidBrush(s->unrealized_pl >= 0 ? RGB(34, 197, 94) : RGB(239, 68, 68));
        FillRect(hdc, &bar, hb);
        DeleteObject(hb);

        snprintf(buf, sizeof(buf), "MV: %.2f | P/L: %+.2f", s->market_value, s->unrealized_pl);
        SetTextColor(hdc, s->unrealized_pl >= 0 ? RGB(21, 128, 61) : RGB(185, 28, 28));
        TextOutA(hdc, bx + bw + 12, row_y, buf, (int)strlen(buf));

        row_y += 26;
        drawn++;
    }

    DeleteObject(white_b);
    DeleteObject(border_p);
}
#endif
