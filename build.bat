@echo off
setlocal
echo ============================================================================
echo Building Stock Portfolio Management System (C17 + Manual DSA + SQLite + Win32)
echo ============================================================================

where gcc >nul 2>nul
if errorlevel 1 (
    echo [ERROR] GCC was not found. Install MinGW-w64 and add its bin folder to PATH.
    pause
    exit /b 1
)

if not exist build mkdir build
if not exist data mkdir data

set CORE_SRC=src\stock.c src\transaction.c src\portfolio.c src\market_data.c src\database.c dsa\hashmap.c dsa\avl_tree.c dsa\linkedlist.c dsa\stack.c dsa\queue.c dsa\heap.c dsa\sorting.c dsa\searching.c dsa\graph.c
set GUI_SRC=gui\dashboard.c gui\holdings.c gui\transactions.c gui\watchlist.c gui\analytics.c gui\settings.c

echo [1/5] Compiling main Windows GUI application (build\StockPortfolio.exe)...
gcc -std=c17 -Wall -Wextra -O2 -Iinclude src\main.c %CORE_SRC% %GUI_SRC% -o build\StockPortfolio.exe -mwindows -lcomctl32 -lgdi32 -luser32
if errorlevel 1 (
    echo [ERROR] GUI build failed.
    pause
    exit /b 1
)

echo [2/5] Compiling and running Hash Table ^& Search Unit Tests...
gcc -std=c17 -Wall -Wextra -O2 -Iinclude tests\test_hashmap.c %CORE_SRC% -o build\test_hashmap.exe
if errorlevel 1 exit /b 1
build\test_hashmap.exe
if errorlevel 1 exit /b 1

echo [3/5] Compiling and running AVL Tree Unit Tests...
gcc -std=c17 -Wall -Wextra -O2 -Iinclude tests\test_avl.c %CORE_SRC% -o build\test_avl.exe
if errorlevel 1 exit /b 1
build\test_avl.exe
if errorlevel 1 exit /b 1

echo [4/5] Compiling and running Linked List, Stack, Queue, Heap, Sort ^& Graph Tests...
gcc -std=c17 -Wall -Wextra -O2 -Iinclude tests\test_transactions.c %CORE_SRC% -o build\test_transactions.exe
if errorlevel 1 exit /b 1
build\test_transactions.exe
if errorlevel 1 exit /b 1

echo [5/5] Compiling and running Portfolio ^& SQLite Persistence Unit Tests...
gcc -std=c17 -Wall -Wextra -O2 -Iinclude tests\test_portfolio.c %CORE_SRC% -o build\test_portfolio.exe
if errorlevel 1 exit /b 1
build\test_portfolio.exe
if errorlevel 1 exit /b 1

echo ============================================================================
echo Build and all Unit Tests succeeded!
echo Launch the desktop application with: .\build\StockPortfolio.exe
echo ============================================================================
