
#ifndef deebee_sqlite_conn_h
#define deebee_sqlite_conn_h

#include "sqlite3/sqlite3.h"
#include "cppkit/ck_nullable.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace deebee
{

class sqlite_conn final
{
public:
    sqlite_conn(const std::string& fileName);
    sqlite_conn(const sqlite_conn&) = delete;
    sqlite_conn(sqlite_conn&& obj) noexcept;

    ~sqlite_conn() noexcept;

    sqlite_conn& operator=(const sqlite_conn&) = delete;
    sqlite_conn& operator=(sqlite_conn&&) noexcept;

    std::vector<std::unordered_map<std::string, cppkit::ck_nullable<std::string>>> exec(const std::string& query) const;
    std::vector<std::unordered_map<std::string, cppkit::ck_nullable<std::string>>> exec(const char* query, ...) const;
    std::vector<std::unordered_map<std::string, cppkit::ck_nullable<std::string>>> exec(const char* q, va_list& args) const;

    std::string last_insert_id() const;

private:
    void _clear() noexcept;
    static int _sqlite3_busy_handlerS(void* obj, int callCount);
    int _sqlite3_busy_handler(int callCount);

    sqlite3* _db;
};

}

#endif
