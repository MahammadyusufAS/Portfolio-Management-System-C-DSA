# Stock Portfolio Management System Using C17 and DSA

A complete, native Windows 11 desktop application written in **ISO C17** using the **Win32 API**, **10 manual Data Structures & Algorithms (DSA)** implementations, and **SQLite3** relational persistence.

## 1. Key Capabilities

- **7-Screen Native Windows GUI (`gui/`)**:
  1. **Dashboard**: 5 KPI summary cards, GDI historical portfolio performance line chart, sector asset allocation bar chart, and individual holding performance bars.
  2. **Stock Holdings & Catalog**: Dynamic stock catalog, live substring search, multi-criteria sorting using **Merge Sort**, **Quick Sort**, or **Insertion Sort** with microsecond telemetry, and stock add/update/remove operations.
  3. **Buy / Sell Stocks & Order Queue**: Weighted-average cost calculation, realized and unrealized profit/loss computation, oversell protection, **FIFO Queue** for pending orders, and **LIFO Stack** for transaction undo.
  4. **Transaction History**: Bidirectional **Doubly Linked List** history with filtering by ticker, transaction type (`BUY`/`SELL`), and date range (`YYYY-MM-DD`).
  5. **Watchlist & Price Alerts**: Category/sector organization, price change tracking, and a **Binary Min-Heap Priority Queue** that orders price alerts by trigger urgency.
  6. **Portfolio Analytics & DSA Benchmarks**: Herfindahl-Hirschman diversification score, **Adjacency List Graph** sector correlation metrics, and live side-by-side step/nanosecond benchmarking of **Linear Search**, **Binary Search**, **Hash Table Search**, and **AVL Tree Search**.
  7. **Settings & Database Management**: User profile configuration, SQLite online backup & restore (`sqlite3_backup_init`), `PRAGMA integrity_check` + DSA invariant verification, and a decoupled **Market Data Module** for sample price simulation.

## 2. Project Directory Structure

```text
StockPortfolio/
├── CMakeLists.txt
├── build.bat
├── README.md
├── .gitignore
├── .vscode/
│   ├── tasks.json
│   ├── launch.json
│   └── c_cpp_properties.json
├── include/
│   ├── dsa.h
│   ├── stock.h
│   ├── portfolio.h
│   ├── transaction.h
│   ├── hashmap.h
│   ├── avl_tree.h
│   ├── linkedlist.h
│   ├── stack.h
│   ├── queue.h
│   ├── heap.h
│   ├── sorting.h
│   ├── searching.h
│   ├── graph.h
│   ├── market_data.h
│   ├── database.h
│   ├── sqlite3_api.h
│   └── gui.h
├── src/
│   ├── main.c
│   ├── stock.c
│   ├── portfolio.c
│   ├── transaction.c
│   ├── market_data.c
│   └── database.c
├── dsa/
│   ├── hashmap.c
│   ├── avl_tree.c
│   ├── linkedlist.c
│   ├── stack.c
│   ├── queue.c
│   ├── heap.c
│   ├── sorting.c
│   ├── searching.c
│   └── graph.c
├── gui/
│   ├── dashboard.c
│   ├── holdings.c
│   ├── transactions.c
│   ├── watchlist.c
│   ├── analytics.c
│   └── settings.c
├── database/
│   └── schema.sql
├── tests/
│   ├── test_portfolio.c
│   ├── test_hashmap.c
│   ├── test_avl.c
│   └── test_transactions.c
└── docs/
    ├── architecture.md
    ├── DSA_DESIGN.md
    └── USER_GUIDE.md
```

## 3. Build & Execution Instructions (Windows 11 + VS Code + MinGW-w64)

### Option A: One-Click Batch Build & Test (`build.bat`)
1. Install **MinGW-w64** (GCC for Windows) and ensure its `bin` folder is in your `PATH`.
2. Open the `StockPortfolio` folder in **Visual Studio Code**.
3. Open the integrated terminal (`Ctrl + ~`) and run:
   ```bat
   .\build.bat
   ```
   This compiles `build\StockPortfolio.exe` AND compiles and executes all 4 unit test suites (`test_hashmap.exe`, `test_avl.exe`, `test_transactions.exe`, `test_portfolio.exe`).
4. Launch the desktop application:
   ```bat
   .\build\StockPortfolio.exe
   ```
   *(Note: On Windows 10/11, the application automatically binds to `sqlite3.dll` or the built-in Windows system library `C:\Windows\System32\winsqlite3.dll`, so no external SQLite installation is required.)*

### Option B: CMake Build + CTest
```bat
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
ctest --output-on-failure
.\StockPortfolio.exe
```

### Option C: Direct Manual GCC Command (Windows)
```bat
mkdir build
gcc -std=c17 -Wall -Wextra -O2 -Iinclude src\main.c src\stock.c src\transaction.c src\portfolio.c src\market_data.c src\database.c dsa\hashmap.c dsa\avl_tree.c dsa\linkedlist.c dsa\stack.c dsa\queue.c dsa\heap.c dsa\sorting.c dsa\searching.c dsa\graph.c gui\dashboard.c gui\holdings.c gui\transactions.c gui\watchlist.c gui\analytics.c gui\settings.c -o build\StockPortfolio.exe -mwindows -lcomctl32 -lgdi32 -luser32
```

## 4. Documentation Index
- **[System Architecture & UML (`docs/architecture.md`)](docs/architecture.md)**: Abstract, problem statement, requirements, 4-layer architecture, database ER schema, UML diagrams, and flowcharts.
- **[Data Structures & Algorithms Design (`docs/DSA_DESIGN.md`)](docs/DSA_DESIGN.md)**: Deep-dive into all 10 manual DSA implementations, rotation invariants, complexity proofs, and benchmark analysis.
- **[User Manual & Testing Report (`docs/USER_GUIDE.md`)](docs/USER_GUIDE.md)**: Screen-by-screen walkthrough, UI layout diagrams, operational workflows, and unit test verification report.
