#ifndef deebee_transaction_h
#define deebee_transaction_h

namespace deebee
{

template<typename CT, typename T>
void trans(CT db, T t)
{
    db.exec("BEGIN");
    try
    {
        t(db);
        db.exec("COMMIT");
    }
    catch(...)
    {
        db.exec("ROLLBACK");
        throw;
    }
}

}

#endif
