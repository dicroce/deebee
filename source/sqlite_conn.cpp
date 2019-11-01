
#include "deebee/sqlite_conn.h"
#include "cppkit/ck_exception.h"
#include "cppkit/ck_string_utils.h"
#include <unistd.h>

using namespace deebee;
using namespace cppkit;
using namespace std;

static const int DEFAULT_NUM_RETRIES = 60;
static const int BASE_SLEEP_MICROS = 4000;

sqlite_conn::sqlite_conn(const string& fileName) :
    _db(nullptr)
{
    int numRetries = DEFAULT_NUM_RETRIES;

    while(numRetries > 0)
    {
        int flags = SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX;

        if(sqlite3_open_v2(fileName.c_str(), &_db, flags, nullptr ) == SQLITE_OK)
        {
            sqlite3_busy_timeout(_db, BASE_SLEEP_MICROS / 1000);
            return;
        }

        if(_db != nullptr)
            _clear();

        usleep(((DEFAULT_NUM_RETRIES-numRetries)+1) * BASE_SLEEP_MICROS);

        --numRetries;
    }

    CK_STHROW(ck_not_found_exception, ( "Unable to open database connection." ));
}

sqlite_conn::sqlite_conn(sqlite_conn&& obj) noexcept :
    _db(std::move(obj._db))
{
    obj._db = nullptr;
}

sqlite_conn::~sqlite_conn() noexcept
{
    _clear();
}

sqlite_conn& sqlite_conn::operator=(sqlite_conn&& obj) noexcept
{
    _clear();

    _db = std::move(obj._db);
    obj._db = nullptr;

    return *this;
}

vector<unordered_map<string, ck_nullable<string>>> sqlite_conn::exec(const string& query) const
{
    return sqlite_conn::exec(query.c_str());
}

vector<unordered_map<string, ck_nullable<string>>> sqlite_conn::exec(const char* query, ...) const
{
    va_list args;
    va_start(args, query);
    auto results = sqlite_conn::exec(query, args);
    va_end(args);
    return results;
}

vector<unordered_map<string, ck_nullable<string>>> sqlite_conn::exec(const char* q, va_list& args) const
{
    auto query = ck_string_utils::format(q, args);

    if(!_db)
        CK_STHROW(ck_internal_exception, ("Cannot exec() on moved out instance of sqlite_conn."));

    vector<unordered_map<string, ck_nullable<string>>> results;

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(_db, query.c_str(), query.length(), &stmt, nullptr);
    if(rc != SQLITE_OK)
        CK_STHROW(ck_internal_exception, ("sqlite3_prepare_v2(%s) failed with: %s", query.c_str(), sqlite3_errmsg(_db)));

    try
    {
        bool done = false;
        while(!done)
        {
            rc = sqlite3_step(stmt);

            if(rc == SQLITE_DONE)
                done = true;
            else if(rc == SQLITE_ROW)
            {
                int columnCount = sqlite3_column_count(stmt);

                unordered_map<string, ck_nullable<string>> row;

                for(int i = 0; i < columnCount; ++i)
                {
                    string val;
                    bool isNull = false;

                    switch(sqlite3_column_type(stmt, i))
                    {
                        case SQLITE_INTEGER:
                            val = ck_string_utils::int64_to_s(sqlite3_column_int64(stmt, i));
                        break;
                        case SQLITE_FLOAT:
                            val = ck_string_utils::double_to_s(sqlite3_column_double(stmt, i));
                        break;
                        case SQLITE_NULL:
                            isNull = true;
                        break;
                        case SQLITE_TEXT:
                        default:
                        {
                            const char* tp = (const char*)sqlite3_column_text(stmt, i);
                            val = (tp)?string(tp):string();
                        }
                    }

                    ck_nullable<string> vc;
                    if(!isNull)
                        vc.set_value(val);

                    row[sqlite3_column_name(stmt, i)] = val;
                }

                results.push_back(row);
            }
            else CK_STHROW(ck_internal_exception, ("Query (%s) to db failed. Cause:", query.c_str(), sqlite3_errmsg(_db)));
        }

        if( stmt )
        {
            sqlite3_clear_bindings(stmt);
            sqlite3_reset(stmt);
            sqlite3_finalize(stmt);
            stmt = nullptr;
        }
    }
    catch(...)
    {
        if(stmt)
        {
            sqlite3_clear_bindings(stmt);
            sqlite3_reset(stmt);
            sqlite3_finalize(stmt);
            stmt = nullptr;
        }

        throw;
    }

    return results;
}

string sqlite_conn::last_insert_id() const
{
    if(!_db)
        CK_STHROW(ck_internal_exception, ("Cannot last_insert_id() on moved out instance of sqlite_conn."));

    return ck_string_utils::int64_to_s(sqlite3_last_insert_rowid(_db));
}

void sqlite_conn::_clear() noexcept
{
    if(_db)
    {
        if(sqlite3_close(_db) != SQLITE_OK)
            CK_LOG_ERROR("Unable to close database. Leaking db handle. Cause: %s", sqlite3_errmsg(_db));

        _db = nullptr;
    }
}

int sqlite_conn::_sqlite3_busy_handlerS(void* obj, int callCount)
{
    return ((sqlite_conn*)obj)->_sqlite3_busy_handler(callCount);
}

int sqlite_conn::_sqlite3_busy_handler(int callCount)
{
    if(callCount >= DEFAULT_NUM_RETRIES)
        return 0;

    usleep(callCount * BASE_SLEEP_MICROS);

    return 1;
}
