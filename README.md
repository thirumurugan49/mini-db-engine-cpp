# Mini In-Memory Database Engine in C++

This project is a fully functional in-memory database engine built from scratch in C++17. It implements the core internals of real-world database systems like MySQL, PostgreSQL, and SQLite — with a custom SQL tokenizer, parser, query executor, B-tree index, and Write Ahead Log (WAL) for crash recovery.

---

## 📂 `main.cpp` - Mini Database Engine

This program accepts SQL-style queries, parses them, executes them against in-memory tables, and recovers data after a simulated crash using a Write Ahead Log.

### Key Features

* **Storage Engine**: Stores tables, rows, and typed columns in memory using C++ STL.
    * Supports `int` and `string` column types using `std::variant<int, std::string>`
    * Tables stored in `std::unordered_map` for O(1) lookup by name

* **SQL Tokenizer**: Splits a raw SQL string into meaningful tokens.
    * `"SELECT name FROM students WHERE age > 20"` becomes `["SELECT", "name", "FROM", "students", "WHERE", "age", ">", "20"]`
    * Handles symbols (`>`, `<`, `=`, `,`, `;`) as standalone tokens

* **SQL Parser**: Converts token list into a structured `SelectQuery` object.
    * Extracts table name, column names, and WHERE condition
    * `SELECT` → column names, `FROM` → table name, `WHERE` → condition

* **Query Executor**: Executes parsed queries using a full table scan.
    * Scans every row and applies WHERE condition — O(n)
    * Supports `>`, `<`, `=` operators

* **B-tree Index**: Fast lookups using a sorted `std::map` index.
    * Reduces range query time from O(n) to O(log n)
    * `upper_bound()` for `>` queries, `lower_bound()` for `<` queries

* **Write Ahead Log (WAL)**: Crash recovery by logging every INSERT to disk.
    * Writes every operation to `wal.log` before updating memory
    * On restart, replays the log to restore all lost data
    * Same mechanism used by PostgreSQL and SQLite internally

---

## 🏗️ Architecture — 6 Layers

```
Raw SQL String
      │
      ▼
Layer 2 — Tokenizer       →  splits string into tokens
      │
      ▼
Layer 3 — Parser          →  tokens into SelectQuery struct
      │
      ▼
Layer 4 — Query Executor  →  scans rows, applies WHERE
      │
      ▼
Layer 5 — B-tree Index    →  fast O(log n) range lookups
      │
      ▼
Layer 1 — Storage Engine  →  tables and rows in memory
      │
      ▼
Layer 6 — WAL             →  every INSERT logged to disk
```

---

## 🛠️ How to Compile and Run

To use this program, you'll need a C++17 compatible compiler like GCC.

1. **Clone the repository:**
    ```bash
    git clone https://github.com/thirumurugan49/mini-db-engine-cpp.git
    cd mini-db-engine-cpp
    ```

2. **Compile the engine:**
    ```bash
    g++ -std=c++17 main.cpp -o minidb
    ```

3. **Run on Linux / Mac:**
    ```bash
    ./minidb
    ```

4. **Run on Windows (MinGW):**
    ```bash
    minidb.exe
    ```

5. **Expected Output:**
    ```
    [Layer 1] Raw table in memory:
    id    name    age
    -----------------------------
    1     Ravi    22
    2     Priya   19
    3     Arjun   25
    4     Meena   17
    5     Kiran   21

    [Layer 2] Tokens:
    [SELECT] [name] [FROM] [students] [WHERE] [age] [>] [20]

    [Layer 3] Parsed query:
      Table  : students
      Column : name
      Where  : age > 20

    [Layer 4] Full table scan result:
    name
    ----------
    Ravi
    Arjun
    Kiran

    [Layer 5] Index lookup result:
    name
    ----------
    Kiran
    Ravi
    Arjun

    [Layer 6] Simulating CRASH...
    Rows in memory after crash: 0

    [Layer 6] Restarting engine...
    WAL Recovery: 5 rows restored!

    [Layer 6] Table after WAL recovery:
    id    name    age
    -----------------------------
    1     Ravi    22
    2     Priya   19
    3     Arjun   25
    4     Meena   17
    5     Kiran   21
    ```

6. **Check the WAL file on disk:**
    ```bash
    # Linux / Mac
    cat wal.log

    # Windows
    type wal.log

    # Output:
    # INSERT students 1 Ravi 22
    # INSERT students 2 Priya 19
    # INSERT students 3 Arjun 25
    # INSERT students 4 Meena 17
    # INSERT students 5 Kiran 21
    ```

---

## 📝 Limitations and Future Improvements

* **SQL Support**: Only `SELECT` with a single `WHERE` condition is supported. `INSERT`, `UPDATE`, `DELETE` via SQL string are not yet implemented.
* **Multi-column WHERE**: Currently supports only one condition (e.g., `age > 20`). `AND` / `OR` operators are not handled.
* **String comparisons**: The WHERE condition only works on integer columns. String-based conditions like `WHERE name = 'Ravi'` are not supported yet.
* **Persistent storage**: The storage engine is in-memory only. Full disk-based persistence (beyond WAL) is not implemented.
* **Concurrency**: No thread safety — concurrent reads and writes are not handled. A production engine would use mutex locks or MVCC.
* **Multiple tables**: JOIN operations across multiple tables are not supported.
* **Type inference**: Column types are hardcoded. A more robust engine would infer types from the CREATE TABLE statement.
