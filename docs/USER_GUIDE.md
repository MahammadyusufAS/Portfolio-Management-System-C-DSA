# User Manual, GUI Wireframes & Testing Report

## 1. Installation & Startup Guide

### Prerequisites on Windows 11
- **Compiler**: MinGW-w64 (`gcc.exe` targeting `x86_64-w64-mingw32`) in system `PATH`.
- **IDE**: Visual Studio Code with C/C++ Extension.
- **Database**: Built-in Windows 11 `winsqlite3.dll` (or `sqlite3.dll`) is loaded automatically at runtime by `src/database.c`.

### Building & Launching
1. Open a terminal in the `StockPortfolio/` directory and run `.\build.bat`.
2. Launch `.\build\StockPortfolio.exe`.
3. On first launch, if `data/portfolio.db` does not exist, the application initializes all 7 SQLite tables and seeds 8 sample stocks (`RELIANCE`, `TCS`, `INFY`, `HDFCBANK`, `ICICIBANK`, `ITC`, `SUNPHARMA`, `LT`), sample transactions, price alerts, and historical valuation snapshots.

---

## 2. GUI Screen-by-Screen Manual

The application window (`1280 x 780`, resizable) features a persistent dark-slate left **Navigation Sidebar** (`194px` wide) and a dynamic main workspace.

```text
+-----------------------+-----------------------------------------------------------------------+
| STOCK PORTFOLIO       |  FINANCIAL PORTFOLIO DASHBOARD                                        |
| C17 + DSA ENGINE      |  +------------+ +------------+ +------------+ +----------+ +--------+ |
|                       |  | Market Val | | Invested   | | Unreal P/L | | Real P/L | | Divers | |
| [1. Dashboard       ] |  +------------+ +------------+ +------------+ +----------+ +--------+ |
| [2. Stock Holdings  ] |  +------------------------------------+ +---------------------------+ |
| [3. Buy/Sell Stocks ] |  | Historical Portfolio Value Curve   | | Sector Asset Allocation   | |
| [4. Transactions    ] |  |   (GDI Line & Area Plot)           | |   (GDI Horizontal Bars)   | |
| [5. Watchlist/Alerts] |  +------------------------------------+ +---------------------------+ |
| [6. Analytics & DSA ] |  +------------------------------------------------------------------+ |
| [7. Settings/Backup ] |  | Individual Stock Holdings Performance (Market Value & P/L Bars)  | |
|                       |  +------------------------------------------------------------------+ |
|                       |  Ready | SQLite Backend: winsqlite3.dll | Stocks: 8 | MV: 226680.00   |
+-----------------------+-----------------------------------------------------------------------+
```

### Screen 1: Dashboard
- Displays 5 KPI summary cards at the top: **Total Market Value**, **Total Invested**, **Unrealized P/L** (with return %), **Realized P/L**, and **Holdings Count & Diversification Score**.
- Renders the **Portfolio Historical Value Curve** from `portfolio_snapshots`, **Asset Allocation by Sector**, and **Individual Stock Holdings Performance** bars.

### Screen 2: Stock Holdings & Catalog
- **Add / Update Stock**: Enter `Ticker`, `Company`, `Sector`, and `Price`, then click **Add / Update**.
- **Update Price**: Enter an existing `Ticker` and new `Price`, then click **Update Price**.
- **Remove Stock**: Removes a stock from the catalog if `Quantity Owned == 0` (blocks removal if shares are currently held).
- **Search & Sort**: Filter rows in real time by typing a substring in `Search`, or select any sort field (`Market Value`, `Current Price`, `Quantity Owned`, `Profit / Loss`, `Total Invested`, `Ticker Symbol`), algorithm (`Merge Sort`, `Quick Sort`, `Insertion Sort`), and direction (`Descending`, `Ascending`). Status bar reports exact comparison count, move count, and elapsed microseconds.

### Screen 3: Buy / Sell Stocks & Order Queue
- **Immediate Execution**: Enter `Stock Ticker`, `Quantity`, and `Execution Price`, then click **BUY SHARES** or **SELL SHARES**.
  - Purchases update the weighted-average cost:
    $$\text{AvgCost}_{\text{new}} = \frac{\text{AvgCost}_{\text{old}} \times Q_{\text{old}} + P_{\text{buy}} \times Q_{\text{buy}}}{Q_{\text{old}} + Q_{\text{buy}}}$$
  - Sales verify $Q_{\text{sell}} \le Q_{\text{owned}}$ and accumulate realized profit/loss:
    $$\Delta \text{Realized P/L} = (P_{\text{sell}} - \text{AvgCost}) \times Q_{\text{sell}}$$
- **FIFO Order Queue**: Click **Enqueue Buy (FIFO Queue)** to stage orders in `TaskQueue`, and **Process Pending Queue** to execute them sequentially.
- **Undo Last Trade**: Click **Undo Last Trade (Stack)** to pop the latest `UndoRecord` from `UndoStack`, restoring holdings and removing the transaction from both the Doubly Linked List and SQLite database.

### Screen 4: Transaction History
- Displays all `BUY` and `SELL` records stored in the **Doubly Linked List** (`TransactionList`).
- Filter by `Ticker`, `Type` (`ALL`, `BUY`, `SELL`), `From Date` (`YYYY-MM-DD`), and `To Date` (`YYYY-MM-DD`), or click **Undo Latest**.

### Screen 5: Watchlist & Min-Heap Price Alerts
- Top table displays Watchlist stocks grouped with their sector, previous price, current price, net change, and percentage change.
- Bottom table displays **Min-Heap Priority Queue Price Alerts** (`AlertMinHeap`). Triggered alerts (`priority_key <= 0`) automatically bubble to index `[0]` at the root of the heap.

### Screen 6: Portfolio Analytics & DSA Benchmarks
- Displays Net Total P/L, Overall Return %, Herfindahl Diversification Score (`0-100`), and **Adjacency List Graph** average pairwise correlation.
- Lists sector allocation breakdowns and runs a live benchmark comparing **Hash Table Search**, **AVL Tree Search**, **Binary Search**, **Linear Search**, **Merge Sort**, **Quick Sort**, and **Insertion Sort**.

### Screen 7: Settings, User Profile & Database Backup
- Edit and save `Username`, `Email`, `Base Currency`, and `Cash Balance`.
- Click **Backup SQLite Database** or **Restore from Backup** to run online SQLite backup (`sqlite3_backup_init`).
- Click **Verify SQLite & DSA Integrity** to execute `PRAGMA integrity_check`, `PRAGMA foreign_key_check`, and verify Hash Table, AVL balance, and Min-Heap invariants.
- Click **Simulate Sample Price Tick** to advance sample prices deterministically via the decoupled Market Data Module.

---

## 3. Unit Testing & Verification Report

All four unit test suites in `tests/` were compiled with `gcc -std=c17 -Wall -Wextra -Werror -O2` and verified against `libsqlite3`:

| Test Suite Executable | Modules & Edge Cases Verified | Result |
| :--- | :--- | :---: |
| **`test_hashmap`** | Separate chaining with 600 keys in 257 buckets (>343 forced collisions), case-insensitive lookup, key update, deletion, load factor, and 4-algorithm search parity (`Linear`, `Binary`, `Hash`, `AVL`). | **PASSED (0 warnings, 0 errors)** |
| **`test_avl`** | Explicit LL, RR, LR, and RL tree rotations, 250-node sequential insertion & 125-node deletion stress test verifying `avl_is_balanced` and height $\le 10$ at every step, in-order sorting check, ticker range query, and price range query. | **PASSED (0 warnings, 0 errors)** |
| **`test_transactions`** | Doubly Linked List bidirectional links & multi-field date/type filter, LIFO `UndoStack` dynamic expansion, FIFO `TaskQueue` order processing, `AlertMinHeap` trigger ordering, `Merge`/`Quick`/`Insertion` sort equivalence, and `StockGraph` BFS/DFS cluster discovery. | **PASSED (0 warnings, 0 errors)** |
| **`test_portfolio`** | Input validation (rejecting empty/invalid tickers, zero/negative prices & quantities, overselling more shares than owned, deleting held stocks), weighted-average cost math, realized/unrealized P/L, undo consistency, SQLite ACID save/load, online backup/restore, and `PRAGMA integrity_check`. | **PASSED (0 warnings, 0 errors)** |
