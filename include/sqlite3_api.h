#ifndef SQLITE3_API_H
#define SQLITE3_API_H

/*
 * Standard SQLite3 C API Declarations & Dynamic/Static Binding Layer
 * Supports:
 * 1. Direct linking with system/MinGW libsqlite3 (-lsqlite3)
 * 2. Runtime dynamic loading of sqlite3.dll / winsqlite3.dll (built into Windows 10/11)
 *    or libsqlite3.so.0 on Linux.
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
typedef struct sqlite3_backup sqlite3_backup;
typedef long long sqlite3_int64;

#define SQLITE_OK         0
#define SQLITE_ERROR      1
#define SQLITE_BUSY       5
#define SQLITE_ROW        100
#define SQLITE_DONE       101
#define SQLITE_TRANSIENT  ((void(*)(void*))-1)
#define SQLITE_STATIC     ((void(*)(void*))0)

int sp_sqlite_initialize(void);
int sp_sqlite_is_available(void);
const char *sp_sqlite_backend_name(void);

int sp_sqlite3_open(const char *filename, sqlite3 **ppDb);
int sp_sqlite3_close(sqlite3 *db);
int sp_sqlite3_exec(sqlite3 *db, const char *sql, int (*callback)(void*,int,char**,char**), void *arg, char **errmsg);
void sp_sqlite3_free(void *ptr);
const char *sp_sqlite3_errmsg(sqlite3 *db);
sqlite3_int64 sp_sqlite3_last_insert_rowid(sqlite3 *db);

int sp_sqlite3_prepare_v2(sqlite3 *db, const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail);
int sp_sqlite3_step(sqlite3_stmt *pStmt);
int sp_sqlite3_reset(sqlite3_stmt *pStmt);
int sp_sqlite3_finalize(sqlite3_stmt *pStmt);

int sp_sqlite3_bind_int(sqlite3_stmt *pStmt, int idx, int val);
int sp_sqlite3_bind_int64(sqlite3_stmt *pStmt, int idx, sqlite3_int64 val);
int sp_sqlite3_bind_double(sqlite3_stmt *pStmt, int idx, double val);
int sp_sqlite3_bind_text(sqlite3_stmt *pStmt, int idx, const char *val, int n, void(*destructor)(void*));

int sp_sqlite3_column_int(sqlite3_stmt *pStmt, int iCol);
sqlite3_int64 sp_sqlite3_column_int64(sqlite3_stmt *pStmt, int iCol);
double sp_sqlite3_column_double(sqlite3_stmt *pStmt, int iCol);
const unsigned char *sp_sqlite3_column_text(sqlite3_stmt *pStmt, int iCol);

sqlite3_backup *sp_sqlite3_backup_init(sqlite3 *pDest, const char *zDestName, sqlite3 *pSource, const char *zSourceName);
int sp_sqlite3_backup_step(sqlite3_backup *p, int nPage);
int sp_sqlite3_backup_finish(sqlite3_backup *p);

#ifdef __cplusplus
}
#endif

#endif /* SQLITE3_API_H */
