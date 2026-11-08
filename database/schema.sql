-- SQLite Database Schema for Stock Portfolio Management System
-- Enforces referential integrity via foreign key constraints and ACID transactions.

PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS users (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT NOT NULL UNIQUE,
    email         TEXT NOT NULL,
    currency      TEXT NOT NULL DEFAULT 'INR',
    cash_balance  REAL NOT NULL DEFAULT 1000000.0 CHECK (cash_balance >= 0.0),
    created_at    TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);

CREATE TABLE IF NOT EXISTS stocks (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    ticker         TEXT NOT NULL UNIQUE,
    name           TEXT NOT NULL,
    sector         TEXT NOT NULL DEFAULT 'General',
    current_price  REAL NOT NULL CHECK (current_price > 0.0),
    previous_price REAL NOT NULL DEFAULT 0.0 CHECK (previous_price >= 0.0)
);

CREATE TABLE IF NOT EXISTS holdings (
    user_id        INTEGER NOT NULL,
    stock_id       INTEGER NOT NULL,
    quantity       INTEGER NOT NULL DEFAULT 0 CHECK (quantity >= 0),
    average_cost   REAL NOT NULL DEFAULT 0.0 CHECK (average_cost >= 0.0),
    realized_pl    REAL NOT NULL DEFAULT 0.0,
    updated_at     TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    PRIMARY KEY (user_id, stock_id),
    FOREIGN KEY (user_id)  REFERENCES users(id)  ON DELETE CASCADE,
    FOREIGN KEY (stock_id) REFERENCES stocks(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS transactions (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id        INTEGER NOT NULL,
    stock_id       INTEGER NOT NULL,
    ticker         TEXT NOT NULL,
    type           TEXT NOT NULL CHECK (type IN ('BUY', 'SELL')),
    quantity       INTEGER NOT NULL CHECK (quantity > 0),
    price          REAL NOT NULL CHECK (price > 0.0),
    total_amount   REAL NOT NULL CHECK (total_amount > 0.0),
    realized_pl    REAL NOT NULL DEFAULT 0.0,
    timestamp      TEXT NOT NULL,
    FOREIGN KEY (user_id)  REFERENCES users(id)  ON DELETE CASCADE,
    FOREIGN KEY (stock_id) REFERENCES stocks(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS watchlist (
    user_id        INTEGER NOT NULL,
    stock_id       INTEGER NOT NULL,
    category       TEXT NOT NULL DEFAULT 'General',
    added_at       TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    PRIMARY KEY (user_id, stock_id),
    FOREIGN KEY (user_id)  REFERENCES users(id)  ON DELETE CASCADE,
    FOREIGN KEY (stock_id) REFERENCES stocks(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS price_alerts (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id        INTEGER NOT NULL,
    stock_id       INTEGER NOT NULL,
    ticker         TEXT NOT NULL,
    target_price   REAL NOT NULL CHECK (target_price > 0.0),
    condition_type INTEGER NOT NULL CHECK (condition_type IN (1, 2)),
    triggered      INTEGER NOT NULL DEFAULT 0 CHECK (triggered IN (0, 1)),
    notes          TEXT NOT NULL DEFAULT '',
    FOREIGN KEY (user_id)  REFERENCES users(id)  ON DELETE CASCADE,
    FOREIGN KEY (stock_id) REFERENCES stocks(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS portfolio_snapshots (
    id                 INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id            INTEGER NOT NULL,
    timestamp          TEXT NOT NULL,
    total_invested     REAL NOT NULL DEFAULT 0.0,
    total_market_value REAL NOT NULL DEFAULT 0.0,
    unrealized_pl      REAL NOT NULL DEFAULT 0.0,
    realized_pl        REAL NOT NULL DEFAULT 0.0,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_stocks_ticker ON stocks(ticker);
CREATE INDEX IF NOT EXISTS idx_tx_ticker     ON transactions(ticker);
CREATE INDEX IF NOT EXISTS idx_tx_timestamp  ON transactions(timestamp);
CREATE INDEX IF NOT EXISTS idx_alerts_ticker ON price_alerts(ticker);
