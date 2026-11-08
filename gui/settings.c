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

void gui_settings_create(GuiContext *ctx) {
    int cx = 210, cy = 20;
    make_ctrl(ctx, VIEW_SETTINGS, "STATIC", "USER PROFILE, SQLITE DATABASE BACKUP & MARKET DATA", SS_LEFT, cx, cy, 520, 26, 0);

    make_ctrl(ctx, VIEW_SETTINGS, "STATIC", "Username:", SS_LEFT, cx, cy + 48, 120, 22, 0);
    ctx->set_username = make_ctrl(ctx, VIEW_SETTINGS, "EDIT", "", WS_BORDER | ES_AUTOHSCROLL, cx + 130, cy + 45, 220, 28, ID_SET_USERNAME);

    make_ctrl(ctx, VIEW_SETTINGS, "STATIC", "Email Address:", SS_LEFT, cx, cy + 88, 120, 22, 0);
    ctx->set_email = make_ctrl(ctx, VIEW_SETTINGS, "EDIT", "", WS_BORDER | ES_AUTOHSCROLL, cx + 130, cy + 85, 220, 28, ID_SET_EMAIL);

    make_ctrl(ctx, VIEW_SETTINGS, "STATIC", "Base Currency:", SS_LEFT, cx, cy + 128, 120, 22, 0);
    ctx->set_currency = make_ctrl(ctx, VIEW_SETTINGS, "EDIT", "INR", WS_BORDER | ES_AUTOHSCROLL, cx + 130, cy + 125, 100, 28, ID_SET_CURRENCY);

    make_ctrl(ctx, VIEW_SETTINGS, "STATIC", "Cash Balance:", SS_LEFT, cx, cy + 168, 120, 22, 0);
    ctx->set_cash = make_ctrl(ctx, VIEW_SETTINGS, "EDIT", "1000000.00", WS_BORDER | ES_AUTOHSCROLL, cx + 130, cy + 165, 160, 28, ID_SET_CASH);

    make_ctrl(ctx, VIEW_SETTINGS, "BUTTON", "Save User Profile", BS_PUSHBUTTON, cx + 130, cy + 208, 180, 32, ID_SET_BTN_SAVE_USER);

    /* Database Backup, Restore, and Integrity Verification */
    make_ctrl(ctx, VIEW_SETTINGS, "STATIC", "Backup Database File Path:", SS_LEFT, cx, cy + 270, 200, 22, 0);
    ctx->set_backup_path = make_ctrl(ctx, VIEW_SETTINGS, "EDIT", "data/portfolio_backup.db", WS_BORDER | ES_AUTOHSCROLL, cx + 210, cy + 267, 290, 28, ID_SET_BACKUP_PATH);

    make_ctrl(ctx, VIEW_SETTINGS, "BUTTON", "Backup SQLite Database", BS_PUSHBUTTON, cx, cy + 312, 200, 34, ID_SET_BTN_BACKUP);
    make_ctrl(ctx, VIEW_SETTINGS, "BUTTON", "Restore from Backup", BS_PUSHBUTTON, cx + 215, cy + 312, 200, 34, ID_SET_BTN_RESTORE);
    make_ctrl(ctx, VIEW_SETTINGS, "BUTTON", "Verify SQLite & DSA Integrity", BS_PUSHBUTTON, cx + 430, cy + 312, 230, 34, ID_SET_BTN_INTEGRITY);

    /* Sample Market Data Simulator & Snapshot Controls */
    make_ctrl(ctx, VIEW_SETTINGS, "STATIC", "Sample Market Data Module (Offline Simulation — Clearly Labeled Sample Prices):", SS_LEFT, cx, cy + 375, 580, 22, 0);
    make_ctrl(ctx, VIEW_SETTINGS, "BUTTON", "Simulate Sample Price Tick", BS_PUSHBUTTON, cx, cy + 408, 220, 34, ID_SET_BTN_SIMULATE);
    make_ctrl(ctx, VIEW_SETTINGS, "BUTTON", "Record Portfolio Snapshot", BS_PUSHBUTTON, cx + 235, cy + 408, 220, 34, ID_SET_BTN_SNAPSHOT);
}

void gui_settings_refresh(GuiContext *ctx) {
    if (!ctx || !ctx->portfolio) return;
    Portfolio *p = ctx->portfolio;
    char buf[64];
    SetWindowTextA(ctx->set_username, p->user.username);
    SetWindowTextA(ctx->set_email, p->user.email);
    SetWindowTextA(ctx->set_currency, p->user.currency);
    snprintf(buf, sizeof(buf), "%.2f", p->user.cash_balance);
    SetWindowTextA(ctx->set_cash, buf);
}

void gui_settings_paint(GuiContext *ctx, HDC hdc, RECT client_rc) {
    (void)ctx; (void)hdc; (void)client_rc;
}

int gui_settings_command(GuiContext *ctx, int id, int notify_code) {
    (void)notify_code;
    Portfolio *p = ctx->portfolio;
    char backup_path[260] = "";

    if (id == ID_SET_BTN_SAVE_USER) {
        char u[48] = "", e[80] = "", c[8] = "", cash_s[32] = "";
        GetWindowTextA(ctx->set_username, u, sizeof(u));
        GetWindowTextA(ctx->set_email, e, sizeof(e));
        GetWindowTextA(ctx->set_currency, c, sizeof(c));
        GetWindowTextA(ctx->set_cash, cash_s, sizeof(cash_s));
        double cash = strtod(cash_s, NULL);
        if (!u[0] || !e[0] || !c[0] || cash < 0.0) {
            MessageBoxA(ctx->hwnd_main, "Enter valid username, email, currency code, and non-negative cash balance.",
                        "Validation Error", MB_ICONWARNING);
            return 1;
        }
        stock_copy_str(p->user.username, sizeof(p->user.username), u);
        stock_copy_str(p->user.email, sizeof(p->user.email), e);
        stock_copy_str(p->user.currency, sizeof(p->user.currency), c);
        p->user.cash_balance = cash;
        db_save_portfolio(p, p->db_path);
        gui_set_status(ctx, "Saved user profile (%s) to SQLite database.", p->user.username);
        return 1;
    }
    if (id == ID_SET_BTN_BACKUP) {
        GetWindowTextA(ctx->set_backup_path, backup_path, sizeof(backup_path));
        db_save_portfolio(p, p->db_path);
        if (db_backup(p->db_path, backup_path)) {
            MessageBoxA(ctx->hwnd_main, "SQLite database backup completed successfully.", "Database Backup", MB_ICONINFORMATION);
            gui_set_status(ctx, "Backed up SQLite database to %s.", backup_path);
        } else {
            MessageBoxA(ctx->hwnd_main, "Database backup failed. Check path and permissions.", "Backup Error", MB_ICONERROR);
        }
        return 1;
    }
    if (id == ID_SET_BTN_RESTORE) {
        GetWindowTextA(ctx->set_backup_path, backup_path, sizeof(backup_path));
        if (MessageBoxA(ctx->hwnd_main, "Restore portfolio state from backup database file?",
                        "Confirm Restore", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            if (db_restore(backup_path, p->db_path, p)) {
                gui_settings_refresh(ctx);
                MessageBoxA(ctx->hwnd_main, "Database restored and reloaded successfully.", "Restore Complete", MB_ICONINFORMATION);
                gui_set_status(ctx, "Restored portfolio state from %s.", backup_path);
            } else {
                MessageBoxA(ctx->hwnd_main, "Could not restore from backup file.", "Restore Failed", MB_ICONERROR);
            }
        }
        return 1;
    }
    if (id == ID_SET_BTN_INTEGRITY) {
        char dsa_rep[512] = "", db_rep[512] = "", combined[1024] = "";
        int ok1 = portfolio_verify_integrity(p, dsa_rep, sizeof(dsa_rep));
        int ok2 = db_integrity_check(p->db_path, db_rep, sizeof(db_rep));
        snprintf(combined, sizeof(combined), "%s\n\n%s", dsa_rep, db_rep);
        MessageBoxA(ctx->hwnd_main, combined,
                    (ok1 && ok2) ? "Integrity Check Passed" : "Integrity Warning",
                    (ok1 && ok2) ? MB_ICONINFORMATION : MB_ICONWARNING);
        gui_set_status(ctx, "%s", dsa_rep);
        return 1;
    }
    if (id == ID_SET_BTN_SIMULATE) {
        size_t n = market_data_simulate_step(p, &ctx->rng_seed);
        db_save_portfolio(p, p->db_path);
        gui_set_status(ctx, "Simulated sample price step across %zu stocks and recorded portfolio snapshot.", n);
        return 1;
    }
    if (id == ID_SET_BTN_SNAPSHOT) {
        portfolio_record_snapshot(p, NULL);
        db_save_portfolio(p, p->db_path);
        gui_set_status(ctx, "Recorded portfolio valuation snapshot (#%zu).", p->snapshot_count);
        return 1;
    }
    return 0;
}
#endif
