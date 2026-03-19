#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <map>
#include <fstream>
#include <sstream>

// LAYER 1
using Value = std::variant<int, std::string>;
using Row   = std::vector<Value>;
using Token = std::string;

struct Table {
    std::string name;
    std::vector<std::string> columns;
    std::vector<Row> rows;

    void insert(Row row) { rows.push_back(row); }

    void print() {
        for (auto& col : columns)
            std::cout << col << "\t";
        std::cout << "\n";
        std::cout << "-----------------------------\n";
        for (auto& row : rows) {
            for (auto& val : row)
                std::visit([](auto& v){ std::cout << v << "\t"; }, val);
            std::cout << "\n";
        }
    }
};

struct Database {
    std::unordered_map<std::string, Table> tables;

    void createTable(std::string name, std::vector<std::string> columns) {
        tables[name] = {name, columns, {}};
    }
};

// LAYER 2
std::vector<Token> tokenize(const std::string& query) {
    std::vector<Token> tokens;
    std::string current = "";

    for (char c : query) {
        if (c == ' ') {
            if (!current.empty()) {
                tokens.push_back(current);
                current = "";
            }
        }
        else if (c=='>' || c=='<' || c=='=' ||
                 c==',' || c==';' || c=='(' || c==')') {
            if (!current.empty()) {
                tokens.push_back(current);
                current = "";
            }
            tokens.push_back(std::string(1, c));
        }
        else current += c;
    }
    if (!current.empty()) tokens.push_back(current);
    return tokens;
}

// LAYER 3
struct SelectQuery {
    std::string              table;
    std::vector<std::string> columns;
    std::string              whereCol;
    std::string              whereOp;
    std::string              whereVal;
};

SelectQuery parse(const std::vector<Token>& tokens) {
    SelectQuery q;
    int i = 0;
    i++;

    while (tokens[i] != "FROM") {
        if (tokens[i] != ",") q.columns.push_back(tokens[i]);
        i++;
    }
    i++;
    q.table = tokens[i++];

    if (i < (int)tokens.size() && tokens[i] == "WHERE") {
        i++;
        q.whereCol = tokens[i++];
        q.whereOp  = tokens[i++];
        q.whereVal = tokens[i++];
    }
    return q;
}

// LAYER 4
void executeSelect(const SelectQuery& query, Database& db) {
    if (db.tables.find(query.table) == db.tables.end()) {
        std::cout << "Table not found!\n";
        return;
    }
    Table& table = db.tables[query.table];

    int whereColIdx  = -1;
    int selectColIdx = -1;
    for (int i = 0; i < (int)table.columns.size(); i++) {
        if (table.columns[i] == query.whereCol)   whereColIdx  = i;
        if (table.columns[i] == query.columns[0]) selectColIdx = i;
    }

    std::cout << query.columns[0] << "\n";
    std::cout << "----------\n";

    for (auto& row : table.rows) {
        int rowAge  = std::get<int>(row[whereColIdx]);
        int condAge = std::stoi(query.whereVal);

        bool match = false;
        if      (query.whereOp == ">") match = rowAge > condAge;
        else if (query.whereOp == "<") match = rowAge < condAge;
        else if (query.whereOp == "=") match = rowAge == condAge;

        if (match)
            std::visit([](auto& v){ std::cout << v << "\n"; },
                       row[selectColIdx]);
    }
}

// LAYER 5
struct Index {
    std::string columnName;
    std::map<int, std::vector<int>> data;

    void build(const Table& table, const std::string& colName) {
        columnName = colName;
        int colIdx = -1;
        for (int i = 0; i < (int)table.columns.size(); i++)
            if (table.columns[i] == colName) colIdx = i;

        if (colIdx == -1) { std::cout << "Column not found!\n"; return; }

        for (int rowIdx = 0; rowIdx < (int)table.rows.size(); rowIdx++) {
            int val = std::get<int>(table.rows[rowIdx][colIdx]);
            data[val].push_back(rowIdx);
        }
    }

    std::vector<int> greaterThan(int val) {
        std::vector<int> result;
        auto it = data.upper_bound(val);
        while (it != data.end()) {
            for (int r : it->second) result.push_back(r);
            it++;
        }
        return result;
    }

    std::vector<int> lessThan(int val) {
        std::vector<int> result;
        auto it = data.begin();
        while (it != data.lower_bound(val)) {
            for (int r : it->second) result.push_back(r);
            it++;
        }
        return result;
    }

    std::vector<int> equalTo(int val) {
        std::vector<int> result;
        auto it = data.find(val);
        if (it != data.end())
            for (int r : it->second) result.push_back(r);
        return result;
    }
};

void executeSelectWithIndex(const SelectQuery& query,
                             Database& db,
                             Index& index) {
    Table& table = db.tables[query.table];

    int selectColIdx = -1;
    for (int i = 0; i < (int)table.columns.size(); i++)
        if (table.columns[i] == query.columns[0])
            selectColIdx = i;

    std::vector<int> matchingRows;
    int condVal = std::stoi(query.whereVal);

    if      (query.whereOp == ">") matchingRows = index.greaterThan(condVal);
    else if (query.whereOp == "<") matchingRows = index.lessThan(condVal);
    else if (query.whereOp == "=") matchingRows = index.equalTo(condVal);

    std::cout << query.columns[0] << "\n";
    std::cout << "----------\n";
    for (int rowIdx : matchingRows)
        std::visit([](auto& v){ std::cout << v << "\n"; },
                   table.rows[rowIdx][selectColIdx]);
}

// LAYER 6
struct WAL {
    std::string filename;

    WAL(std::string fname) : filename(fname) {}

    void logInsert(const std::string& tableName, const Row& row) {
        std::ofstream log(filename, std::ios::app);
        log << "INSERT " << tableName << " ";
        for (auto& val : row)
            std::visit([&](auto& v){ log << v << " "; }, val);
        log << "\n";
        log.close();
    }

    void recover(Database& db) {
        std::ifstream log(filename);
        if (!log.is_open()) {
            std::cout << "No WAL file found — fresh start.\n";
            return;
        }

        std::string line;
        int recovered = 0;

        while (std::getline(log, line)) {
            std::istringstream ss(line);
            std::string action;
            ss >> action;

            if (action == "INSERT") {
                std::string tableName;
                ss >> tableName;

                if (db.tables.find(tableName) == db.tables.end()) {
                    std::cout << "Table " << tableName << " not found!\n";
                    continue;
                }

                Table& table = db.tables[tableName];
                Row row;

                for (int i = 0; i < (int)table.columns.size(); i++) {
                    std::string val;
                    ss >> val;
                    try {
                        row.push_back(std::stoi(val));
                    } catch (...) {
                        row.push_back(val);
                    }
                }

                table.insert(row);
                recovered++;
            }
        }
        log.close();
        std::cout << "WAL Recovery: " << recovered << " rows restored!\n";
    }

    void clear() {
        std::ofstream log(filename, std::ios::trunc);
        log.close();
        std::cout << "WAL cleared after checkpoint.\n";
    }
};

int main() { // MAIN
    Database db;
    db.createTable("students", {"id", "name", "age"});
    WAL wal("wal.log");

    Row r1 = {1, std::string("Ravi"),  22};
    Row r2 = {2, std::string("Priya"), 19};
    Row r3 = {3, std::string("Arjun"), 25};
    Row r4 = {4, std::string("Meena"), 17};
    Row r5 = {5, std::string("Kiran"), 21};

    for (auto& row : std::vector<Row>{r1,r2,r3,r4,r5}) {
        wal.logInsert("students", row);
        db.tables["students"].insert(row);
    }

    std::string sql = "SELECT name FROM students WHERE age > 20";
    auto tokens = tokenize(sql);
    auto query = parse(tokens);

    executeSelect(query, db);

    Index ageIndex;
    ageIndex.build(db.tables["students"], "age");
    executeSelectWithIndex(query, db, ageIndex);

    db.tables["students"].rows.clear();
    wal.recover(db);
    db.tables["students"].print();
    wal.clear();

    return 0;
}