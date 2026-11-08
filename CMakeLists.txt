cmake_minimum_required(VERSION 3.16)
project(StockPortfolio VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

set(CORE_SOURCES
    src/stock.c
    src/transaction.c
    src/portfolio.c
    src/market_data.c
    src/database.c
    dsa/hashmap.c
    dsa/avl_tree.c
    dsa/linkedlist.c
    dsa/stack.c
    dsa/queue.c
    dsa/heap.c
    dsa/sorting.c
    dsa/searching.c
    dsa/graph.c
)

set(GUI_SOURCES
    gui/dashboard.c
    gui/holdings.c
    gui/transactions.c
    gui/watchlist.c
    gui/analytics.c
    gui/settings.c
)

add_library(stock_portfolio_core STATIC ${CORE_SOURCES})
target_include_directories(stock_portfolio_core PUBLIC include)
if(UNIX)
    target_link_libraries(stock_portfolio_core PUBLIC dl m)
endif()

add_executable(StockPortfolio WIN32
    src/main.c
    ${GUI_SOURCES}
)
target_link_libraries(StockPortfolio PRIVATE stock_portfolio_core)
if(WIN32)
    target_link_libraries(StockPortfolio PRIVATE comctl32 gdi32 user32)
endif()

enable_testing()

add_executable(test_hashmap tests/test_hashmap.c)
target_link_libraries(test_hashmap PRIVATE stock_portfolio_core)
add_test(NAME TestHashMap COMMAND test_hashmap)

add_executable(test_avl tests/test_avl.c)
target_link_libraries(test_avl PRIVATE stock_portfolio_core)
add_test(NAME TestAVL COMMAND test_avl)

add_executable(test_transactions tests/test_transactions.c)
target_link_libraries(test_transactions PRIVATE stock_portfolio_core)
add_test(NAME TestTransactions COMMAND test_transactions)

add_executable(test_portfolio tests/test_portfolio.c)
target_link_libraries(test_portfolio PRIVATE stock_portfolio_core)
add_test(NAME TestPortfolio COMMAND test_portfolio)
