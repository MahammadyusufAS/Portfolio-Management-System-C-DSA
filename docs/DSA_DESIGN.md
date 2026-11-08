# Data Structures & Algorithms (DSA) Design & Complexity Analysis

All ten mandatory data structures and algorithms are implemented manually in ISO C17 without relying on external container libraries.

---

## 1. Dynamic Array (`StockArray` in `include/stock.h`, `src/stock.c`)
- **Purpose**: Contiguous storage for stock catalog records and active holdings, enabling $O(1)$ random access by index for sorting and GUI rendering.
- **Implementation**: Geometric capacity doubling (`capacity * 2`, starting at 16) via `realloc`, with zero-initialization of newly allocated slots and `memmove` compaction on removal.
- **Complexity**:
  - Index Access: $O(1)$
  - Amortized Append (`stock_array_push`): $O(1)$
  - Removal at Index (`stock_array_remove_at`): $O(n)$
  - Space: $O(n)$

## 2. Doubly Linked List (`TransactionList` in `include/linkedlist.h`, `dsa/linkedlist.c`)
- **Purpose**: Stores complete chronological transaction history and supports both forward (`head -> next`) and reverse (`tail -> prev`) traversal as well as $O(1)$ tail unlinking during Undo operations.
- **Implementation**: Each `TransactionNode` maintains `prev` and `next` pointers. `dll_append` inserts at `tail` in $O(1)$ time. `dll_filter` traverses in either direction while applying ticker, transaction type, and date-prefix predicates.
- **Complexity**:
  - Append / Prepend: $O(1)$
  - Unlink Known Node (`dll_remove_node`): $O(1)$
  - Filter / Search by ID: $O(m)$ where $m$ is transaction count

## 3. Hash Table with Separate Chaining (`HashMap` in `include/hashmap.h`, `dsa/hashmap.c`)
- **Purpose**: Maps ticker symbols (`"RELIANCE"`, `"TCS"`, etc.) to their `StockArray` index in $O(1)$ average time.
- **Hash Function**: Case-insensitive DJB2 hash modulo prime bucket capacity ($M = 257$):
  $$h(S) = \left(\sum_{i} h_{i-1} \cdot 33 + \text{toupper}(S[i])\right) \bmod 257$$
- **Collision Resolution**: Separate chaining with singly linked `HashNode` lists per bucket and explicit collision counting (`map->collision_count`).
- **Complexity**:
  - Average Insert / Lookup / Delete: $O(1 + \alpha)$ where load factor $\alpha = n / 257$
  - Worst-Case Lookup: $O(n)$

## 4. Self-Balancing AVL Tree (`AVLTree` in `include/avl_tree.h`, `dsa/avl_tree.c`)
- **Purpose**: Maintains stock records in strict alphabetical order by ticker symbol and supports $O(\log n)$ worst-case search, insertion, deletion, and pruned range queries (`[low_ticker, high_ticker]`).
- **Balancing Invariant**: Every node satisfies $|\text{height}(\text{left}) - \text{height}(\text{right})| \le 1$. Four rotation cases restore balance after insertion or deletion:
  - **LL (Balance > +1, Left Child $\ge 0$)**: Single Right Rotation
  - **RR (Balance < -1, Right Child $\le 0$)**: Single Left Rotation
  - **LR (Balance > +1, Left Child $< 0$)**: Left Rotation on left child, then Right Rotation on node
  - **RL (Balance < -1, Right Child $> 0$)**: Right Rotation on right child, then Left Rotation on node
- **Complexity**:
  - Search / Insert / Delete: $O(\log n)$ guaranteed worst-case
  - Ticker Range Query: $O(\log n + k)$ where $k$ is number of matching tickers

## 5. LIFO Stack (`UndoStack` in `include/stack.h`, `dsa/stack.c`)
- **Purpose**: Reverses the most recent BUY or SELL transaction (`portfolio_undo`), restoring previous share quantity, weighted-average cost, and realized profit/loss.
- **Implementation**: Dynamic array-backed LIFO stack of `UndoRecord` structs.
- **Complexity**:
  - `stack_push` / `stack_pop` / `stack_peek`: $O(1)$ amortized

## 6. FIFO Queue (`TaskQueue` in `include/queue.h`, `dsa/queue.c`)
- **Purpose**: Buffers pending buy/sell orders and scheduled portfolio tasks in First-In, First-Out order (`portfolio_enqueue_task`, `portfolio_process_pending_queue`).
- **Implementation**: Singly linked list with `front` and `rear` pointers.
- **Complexity**:
  - `queue_enqueue` (at `rear`) / `queue_dequeue` (at `front`) / `queue_peek`: $O(1)$

## 7. Priority Queue / Binary Min-Heap (`AlertMinHeap` in `include/heap.h`, `dsa/heap.c`)
- **Purpose**: Manages stock price alerts (`ALERT_ABOVE` and `ALERT_BELOW`) so the most urgent or already-triggered alert is always accessible at index `0` in $O(1)$ time.
- **Priority Key Formulation**:
  - When triggered: $\text{priority\_key} = -|P_{\text{current}} - P_{\text{target}}| / P_{\text{target}} \le 0.0$
  - When untriggered: $\text{priority\_key} = |P_{\text{target}} - P_{\text{current}}| / P_{\text{target}} > 0.0$
- **Operations**: `sift_up` on insertion ($O(\log n)$), `sift_down` on root extraction ($O(\log n)$), and Floyd's bottom-up `heapify` ($O(n)$) when stock prices update.

## 8. Sorting Algorithms (`include/sorting.h`, `dsa/sorting.c`)
All three sorting algorithms operate over `SortItem` projections (`MARKET_VALUE`, `PRICE`, `QUANTITY`, `PROFIT_LOSS`, `INVESTMENT`, `TICKER`) in ascending or descending order and record exact comparison and move counts:
1. **Merge Sort (`merge_sort_items`)**: Stable divide-and-conquer sort with $O(n \log n)$ worst-case time and $O(n)$ auxiliary buffer.
2. **Quick Sort (`quick_sort_items`)**: In-place partitioning using **median-of-three pivot selection** (`low`, `mid`, `high`) to avoid $O(n^2)$ degradation on already-sorted holdings; $O(n \log n)$ average time and $O(\log n)$ stack space.
3. **Insertion Sort (`insertion_sort_items`)**: Adaptive in-place sort running in $O(n)$ time on nearly-sorted holdings and $O(n^2)$ worst-case time.

## 9. Searching Algorithms (`include/searching.h`, `dsa/searching.c`)
Implemented side-by-side with step and nanosecond benchmarking (`search_compare_all`):
1. **Linear Search (`search_linear_ticker`, `search_linear_substring`)**: $O(n)$ sequential scan supporting both exact ticker lookup and case-insensitive substring matching across ticker, company name, and sector.
2. **Binary Search (`search_binary_ticker`)**: $O(\log n)$ dichotomous search over a Merge-Sorted ticker index array.
3. **Hash Table Search (`search_hash_ticker`)**: $O(1)$ average lookup via `HashMap`.
4. **AVL Tree Search (`search_avl_ticker`)**: $O(\log n)$ worst-case tree traversal via `AVLTree`.

## 10. Adjacency List Graph (`StockGraph` in `include/graph.h`, `dsa/graph.c`)
- **Purpose**: Models sector peer relationships and pairwise return correlations between stocks in the portfolio.
- **Implementation**: Array of `GraphVertex` nodes, each owning a singly linked list of weighted `GraphEdge` adjacency nodes (`correlation` $\in [-1.0, 1.0]$).
- **Traversals**:
  - **Breadth-First Search (`graph_bfs_related`)**: Discovers all stocks correlated above a threshold $\tau$ in $O(V + E)$ time.
  - **Depth-First Search (`graph_dfs_sector_cluster`)**: Identifies connected sector clusters in $O(V + E)$ time.

---

## Summary Complexity Matrix

| Data Structure / Algorithm | Average Time | Worst-Case Time | Auxiliary Space | Primary Application in Project |
| :--- | :---: | :---: | :---: | :--- |
| **Dynamic Array (`StockArray`)** | $O(1)$ access | $O(n)$ resize | $O(n)$ | Holdings & catalog contiguous storage |
| **Doubly Linked List** | $O(1)$ append | $O(m)$ filter | $O(m)$ | Bidirectional transaction history |
| **Hash Table (Separate Chaining)** | $O(1)$ | $O(n)$ | $O(n + M)$ | Instant ticker symbol lookup |
| **AVL Tree** | $O(\log n)$ | $O(\log n)$ | $O(n)$ | Sorted ticker index & range queries |
| **LIFO Stack (`UndoStack`)** | $O(1)$ | $O(n)$ resize | $O(u)$ | Reversing recent BUY/SELL trades |
| **FIFO Queue (`TaskQueue`)** | $O(1)$ | $O(1)$ | $O(q)$ | Pending order & batch task processing |
| **Min-Heap (`AlertMinHeap`)** | $O(\log k)$ | $O(\log k)$ | $O(k)$ | Priority-ordered stock price alerts |
| **Merge Sort** | $O(n \log n)$ | $O(n \log n)$ | $O(n)$ | Stable multi-field holdings sorting |
| **Quick Sort (Median-of-3)** | $O(n \log n)$ | $O(n^2)$ | $O(\log n)$ | Fast in-place holdings sorting |
| **Insertion Sort** | $O(n^2)$ | $O(n^2)$ | $O(1)$ | Adaptive sorting for small/sorted sets |
| **Binary Search** | $O(\log n)$ | $O(\log n)$ | $O(1)$ | Ticker lookup on sorted index array |
| **Adjacency List Graph (BFS/DFS)** | $O(V + E)$ | $O(V + E)$ | $O(V + E)$ | Sector clustering & correlation score |
