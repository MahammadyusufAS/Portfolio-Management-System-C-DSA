# System Architecture & Software Engineering Specification

## 1. Project Abstract
The **Stock Portfolio Management System Using C and DSA** is a native Windows 11 desktop application engineered in ISO C17. It integrates ten foundational Data Structures and Algorithms (Dynamic Arrays, Doubly Linked Lists, Separate-Chaining Hash Tables, Self-Balancing AVL Trees, LIFO Stacks, FIFO Queues, Binary Min-Heaps, Merge/Quick/Insertion Sorting, Linear/Binary/Hash/AVL Searching, and Adjacency-List Graphs) with an ACID-compliant SQLite3 relational database and a multi-screen Win32 GUI. The system provides weighted-average cost accounting, realized and unrealized profit/loss tracking, reversible trade execution, priority-driven price alerts, graph-based sector diversification analytics, and empirical algorithm benchmarking.

## 2. Problem Statement
Individual investors and computer science students often face a disconnect between theoretical data structures taught in coursework and real-world financial software engineering:
1. Simple spreadsheet or flat-file trackers suffer from $O(n)$ lookup degradation, lack transactional atomicity, and cannot reverse erroneous trades safely.
2. High-level scripting frameworks obscure memory management, pointer manipulation, tree balancing, and algorithmic complexity trade-offs.
3. Existing educational C projects rarely integrate a responsive native GUI, a normalized relational database with foreign-key constraints, and verifiable unit tests in a clean, layered architecture.

## 3. Objectives
- Design and implement 10 core data structures and algorithms from scratch in standard C17 with zero memory leaks or dangling pointers.
- Enforce accurate financial accounting using weighted-average cost basis, separating unrealized P/L from realized P/L, and preventing short/oversell errors.
- Persist all user profiles, stock catalogs, holdings, transactions, watchlists, price alerts, and historical valuation snapshots in SQLite3 using prepared statements and ACID transactions.
- Provide a decoupled **Presentation Layer (Win32 GUI)**, **Business Logic Layer**, **DSA Layer**, and **Database Layer**.

## 4. Functional & Non-Functional Requirements

### Functional Requirements
| ID | Module | Requirement Description |
| :--- | :--- | :--- |
| FR-01 | Dashboard | Display total market value, total invested capital, unrealized P/L, realized P/L, active holdings count, historical value line chart, and sector allocation chart. |
| FR-02 | Stock Management | Add, update, remove (when 0 shares held), search, and sort stocks by price, quantity, market value, investment, ticker, or P/L. |
| FR-03 | Portfolio Trading | Execute BUY and SELL trades, update weighted-average cost on purchases, compute realized P/L on sales, and reject selling more shares than owned. |
| FR-04 | Transaction History | Record every trade in a Doubly Linked List and SQLite table; filter by ticker, type (`BUY`/`SELL`), and date range; undo the latest trade via LIFO Stack. |
| FR-05 | Watchlist & Alerts | Track watchlist stocks by sector, compute price change %, and manage target price alerts (`ABOVE`/`BELOW`) via a Binary Min-Heap Priority Queue. |
| FR-06 | Portfolio Analytics | Compute Herfindahl diversification score, Graph-based pairwise sector correlation, sector weights, and live Search/Sort algorithm benchmarks. |
| FR-07 | User & Data Admin | Manage user profile, create/restore SQLite database backups, verify `PRAGMA integrity_check` and DSA invariants, and simulate sample market ticks. |

### Non-Functional Requirements
- **Language & Compiler Compliance**: Strict ISO C17 (`-std=c17 -Wall -Wextra -Werror`), compatible with MinGW-w64 GCC on Windows 11 and GCC on POSIX for headless testing.
- **Performance**: $O(1)$ average ticker lookup via Hash Table, $O(\log n)$ ordered lookup and range queries via AVL Tree, and $O(n \log n)$ sorting via Merge/Quick Sort.
- **Data Integrity**: SQLite foreign keys (`PRAGMA foreign_keys = ON`), `CHECK` constraints on positive prices and quantities, and atomic `BEGIN IMMEDIATE ... COMMIT` transactions.

## 5. Four-Layer Modular Architecture

```text
+-----------------------------------------------------------------------------------+
|                          PRESENTATION LAYER (Win32 API)                           |
|  src/main.c | gui/dashboard.c | gui/holdings.c | gui/transactions.c               |
|  gui/watchlist.c | gui/analytics.c | gui/settings.c                               |
+-----------------------------------------+-----------------------------------------+
                                          |
                                          v
+-----------------------------------------------------------------------------------+
|                               BUSINESS LOGIC LAYER                                |
|  src/portfolio.c (Weighted-Avg Cost, Realized/Unrealized P/L, Analytics, Undo)    |
|  src/stock.c (Validation, Normalization) | src/transaction.c | src/market_data.c  |
+-------------------+-------------------------------------------+-------------------+
                    |                                           |
                    v                                           v
+---------------------------------------+   +---------------------------------------+
|               DSA LAYER               |   |            DATABASE LAYER             |
|  dsa/hashmap.c    dsa/avl_tree.c      |   |  src/database.c (Prepared Statements, |
|  dsa/linkedlist.c dsa/stack.c         |   |    ACID Transactions, Online Backup,  |
|  dsa/queue.c      dsa/heap.c          |   |    PRAGMA integrity_check)            |
|  dsa/sorting.c    dsa/searching.c     |   |  database/schema.sql (7 Tables)       |
|  dsa/graph.c      src/stock.c (Array) |   |  winsqlite3.dll / sqlite3.dll         |
+---------------------------------------+   +---------------------------------------+
```

## 6. UML Structural Diagram (Core Entities)

```text
+----------------------------------+          +----------------------------------+
|            Portfolio             |          |              Stock               |
+----------------------------------+          +----------------------------------+
| + user: UserProfile              |1        *| + id: int                        |
| + stocks: StockArray             |<>------->| + ticker: char[24]               |
| + ticker_index: HashMap          |          | + name: char[80]                 |
| + avl_index: AVLTree             |          | + sector: char[40]               |
| + transactions: TransactionList  |          | + current_price: double          |
| + undo_stack: UndoStack          |          | + previous_price: double         |
| + pending_queue: TaskQueue       |          | + quantity: int                  |
| + price_alerts: AlertMinHeap     |          | + average_cost: double           |
| + correlation_graph: StockGraph  |          | + total_investment: double       |
| + snapshots: PortfolioSnapshot[] |          | + market_value: double           |
+----------------------------------+          | + unrealized_pl: double          |
         |1                 |1                | + realized_pl: double            |
         |                  |                 +----------------------------------+
         v*                 v*
+--------------------+  +--------------------+
|  TransactionNode   |  |     UndoRecord     |
+--------------------+  +--------------------+
| + id: ulong        |  | + tx_id: ulong     |
| + ticker: char[24] |  | + ticker: char[24] |
| + type: char[8]    |  | + qty_delta: int   |
| + quantity: int    |  | + prev_qty: int    |
| + price: double    |  | + prev_avg: double |
| + realized_pl: dbl |  | + prev_rpl: double |
| + prev / next      |  +--------------------+
+--------------------+
```

## 7. Trade Execution & Undo Flowchart

```text
[User Submits BUY / SELL Trade]
              |
              v
  <Validate Ticker, Qty > 0, Price > 0?> --(No)--> [Return Validation Error]
              | (Yes)
              v
  <Lookup Stock via HashMap O(1)> --------(Not Found)--> [Reject Trade]
              | (Found)
              v
  <Is SELL and Qty > Owned?> -------------(Yes)--> [Reject Oversell]
              | (No)
              v
  [Capture Pre-Trade Snapshot in UndoRecord]
              |
              v
  [Push UndoRecord onto LIFO UndoStack]
              |
              v
  [Update Weighted-Average Cost (BUY) or Realized P/L (SELL)]
              |
              v
  [Append TransactionNode to Doubly Linked List Tail O(1)]
              |
              v
  [Persist State in SQLite3 Atomic Transaction (BEGIN ... COMMIT)]
```

## 8. Relational Database Schema (`database/schema.sql`)

The SQLite database enforces `PRAGMA foreign_keys = ON` across 7 normalized tables:
1. **`users`**: Stores investor profile (`id`, `username`, `email`, `currency`, `cash_balance`, `created_at`).
2. **`stocks`**: Master security catalog (`id`, `ticker UNIQUE`, `name`, `sector`, `current_price`, `previous_price`).
3. **`holdings`**: Current user position per stock (`user_id`, `stock_id`, `quantity`, `average_cost`, `realized_pl`).
4. **`transactions`**: Immutable audit log of executed trades (`id`, `user_id`, `stock_id`, `ticker`, `type`, `quantity`, `price`, `total_amount`, `realized_pl`, `timestamp`).
5. **`watchlist`**: User-monitored securities (`user_id`, `stock_id`, `category`, `added_at`).
6. **`price_alerts`**: Target price triggers (`id`, `user_id`, `stock_id`, `ticker`, `target_price`, `condition_type`, `triggered`, `notes`).
7. **`portfolio_snapshots`**: Time-series valuation records for historical charting (`id`, `user_id`, `timestamp`, `total_invested`, `total_market_value`, `unrealized_pl`, `realized_pl`).

## 9. Future Enhancements
- Live REST/WebSocket market feed adapter using Windows `WinHTTP` inside `src/market_data.c`.
- Tax-lot accounting modes (FIFO / LIFO / Specific Identification) alongside weighted-average cost.
- Multi-currency FX conversion and dividend reinvestment tracking.
