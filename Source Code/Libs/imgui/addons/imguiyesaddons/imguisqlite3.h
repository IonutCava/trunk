/* 
 CppSQLite was originally developed by Rob Groves on CodeProject:
 <http://www.codeproject.com/KB/database/CppSQLite.aspx>
 
 Maintenance and updates are Copyright (C) 2011 NeoSmart Technologies
 <http://neosmart.net/>
 
 Original copyright information:
 Copyright (C) 2004 Rob Groves. All Rights Reserved.
 <rob.groves@btinternet.com>
 
 Permission to use, copy, modify, and distribute this software and its
 documentation for any purpose, without fee, and without a written
 agreement, is hereby granted, provided that the above copyright notice, 
 this paragraph and the following two paragraphs appear in all copies, 
 modifications, and distributions.

 IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT,
 INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST
 PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
 EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF
 ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS". THE AUTHOR HAS NO OBLIGATION
 TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
*/
////////////////////////////////////////////////////////////////////////////////
// Modifications to the original CppSQLite3 library:
// -> added CppSQLite3 namespace
// -> removed prefix "CppSQLite3" to all classes
// -> removed #include <sqlite3.h> from the .h file
////////////////////////////////////////////////////////////////////////////////
// USAGE:
// -> needs -lsqlite3
// -> or the "plain C" amalgamated file "sqlite.c" compiled with a "plain C"
//    compiler linked to the executable. (Not included because it's too big)
////////////////////////////////////////////////////////////////////////////////
#ifndef CppSQLite3_H
#define CppSQLite3_H
#define IMGUISQLITE3_H_

/*#ifndef IMGUI_API
#include <imgui.h>
#endif //IMGUI_API*/

// Define attributes of all API symbols declarations, e.g. for DLL under Windows.
#ifndef IMGUI_API
#define IMGUI_API
#endif


//#include <sqlite3.h>	// see if we can remove it
// Attempt to move "sqlite3.h" to imguisqlite3.cpp:----
struct sqlite3;
struct sqlite3_stmt;
#ifndef SQLITE_INT64_TYPE
#if defined(_MSC_VER) || defined(__BORLANDC__)
  #define SQLITE_INT64_TYPE __int64
#else
  #define SQLITE_INT64_TYPE long long int
#endif
#endif //SQLITE_INT64_TYPE
typedef SQLITE_INT64_TYPE sqlite_int64;
typedef unsigned SQLITE_INT64_TYPE sqlite_uint64;
typedef sqlite_int64 sqlite3_int64;
typedef sqlite_uint64 sqlite3_uint64;
// End Attempt ------------------------------------------


#define CPPSQLITE_ERROR 1000

namespace CppSQLite3 {

class Exception
{
public:

    IMGUI_API Exception(const int nErrCode,
                    const char* szErrMess,
                    bool bDeleteMsg=true);

    IMGUI_API Exception(const Exception&  e);

    IMGUI_API virtual ~Exception();

    int errorCode() const { return mnErrCode; }

    const char* errorMessage() const { return mpszErrMess; }

    static const char* errorCodeAsString(int nErrCode);

private:

    int mnErrCode;
    char* mpszErrMess;
};


class Buffer
{
public:

    IMGUI_API Buffer();

    IMGUI_API ~Buffer();

    IMGUI_API const char* format(const char* szFormat, ...);

    operator const char*() const { return mpBuf; }

    IMGUI_API void clear();

private:

    char* mpBuf;
};


class Binary
{
public:

    IMGUI_API Binary();

    IMGUI_API ~Binary();

    IMGUI_API void setBinary(const unsigned char* pBuf, int nLen);
    IMGUI_API void setEncoded(const unsigned char* pBuf);

    IMGUI_API const unsigned char* getEncoded();
    IMGUI_API const unsigned char* getBinary();

    IMGUI_API int getBinaryLength();

    IMGUI_API unsigned char* allocBuffer(int nLen);

    IMGUI_API void clear();

private:

    unsigned char* mpBuf;
    int mnBinaryLen;
    int mnBufferLen;
    int mnEncodedLen;
    bool mbEncoded;
};


class Query
{
public:

    IMGUI_API Query();

    IMGUI_API Query(const Query& rQuery);

    IMGUI_API Query(sqlite3* pDB,
                sqlite3_stmt* pVM,
                bool bEof,
                bool bOwnVM=true);

    IMGUI_API Query& operator=(const Query& rQuery);

    IMGUI_API virtual ~Query();

    IMGUI_API int numFields() const;

    IMGUI_API int fieldIndex(const char* szField) const;
    IMGUI_API const char* fieldName(int nCol) const;

    IMGUI_API const char* fieldDeclType(int nCol) const;
    IMGUI_API int fieldDataType(int nCol) const;

    IMGUI_API const char* fieldValue(int nField) const;
    IMGUI_API const char* fieldValue(const char* szField) const;

    IMGUI_API int getIntField(int nField, int nNullValue=0) const;
    IMGUI_API int getIntField(const char* szField, int nNullValue=0) const;

    IMGUI_API long long getInt64Field(int nField, long long nNullValue=0) const;
    IMGUI_API long long getInt64Field(const char* szField, long long nNullValue=0) const;

    IMGUI_API double getFloatField(int nField, double fNullValue=0.0) const;
    IMGUI_API double getFloatField(const char* szField, double fNullValue=0.0) const;

    IMGUI_API const char* getStringField(int nField, const char* szNullValue="") const;
    IMGUI_API const char* getStringField(const char* szField, const char* szNullValue="") const;

    IMGUI_API const unsigned char* getBlobField(int nField, int& nLen) const;
    IMGUI_API const unsigned char* getBlobField(const char* szField, int& nLen) const;

    IMGUI_API bool fieldIsNull(int nField) const;
    IMGUI_API bool fieldIsNull(const char* szField) const;

    IMGUI_API bool eof() const;

    IMGUI_API void nextRow();

    IMGUI_API void finalize();

private:

    void checkVM() const;

    sqlite3* mpDB;
    sqlite3_stmt* mpVM;
    bool mbEof;
    int mnCols;
    bool mbOwnVM;
};


class Table
{
public:

    IMGUI_API Table();

    IMGUI_API Table(const Table& rTable);

    IMGUI_API Table(char** paszResults, int nRows, int nCols);

    IMGUI_API virtual ~Table();

    IMGUI_API Table& operator=(const Table& rTable);

    IMGUI_API int numFields() const;

    IMGUI_API int numRows() const;

    IMGUI_API const char* fieldName(int nCol) const;

    IMGUI_API const char* fieldValue(int nField) const;
    IMGUI_API const char* fieldValue(const char* szField) const;

    IMGUI_API int getIntField(int nField, int nNullValue=0) const;
    IMGUI_API int getIntField(const char* szField, int nNullValue=0) const;

    IMGUI_API double getFloatField(int nField, double fNullValue=0.0) const;
    IMGUI_API double getFloatField(const char* szField, double fNullValue=0.0) const;

    IMGUI_API const char* getStringField(int nField, const char* szNullValue="") const;
    IMGUI_API const char* getStringField(const char* szField, const char* szNullValue="") const;

    IMGUI_API bool fieldIsNull(int nField) const;
    IMGUI_API bool fieldIsNull(const char* szField) const;

    IMGUI_API void setRow(int nRow);

    IMGUI_API void finalize();

private:

    void checkResults() const;

    int mnCols;
    int mnRows;
    int mnCurrentRow;
    char** mpaszResults;
};


class Statement
{
public:

    IMGUI_API Statement();

    IMGUI_API Statement(const Statement& rStatement);

    IMGUI_API Statement(sqlite3* pDB, sqlite3_stmt* pVM);

    IMGUI_API virtual ~Statement();

    IMGUI_API Statement& operator=(const Statement& rStatement);

    IMGUI_API int execDML();

    IMGUI_API Query execQuery();

    IMGUI_API void bind(int nParam, const char* szValue);
    IMGUI_API void bind(int nParam, const int nValue);
    IMGUI_API void bind(int nParam, const long long nValue);
    IMGUI_API void bind(int nParam, const double dwValue);
    IMGUI_API void bind(int nParam, const unsigned char* blobValue, int nLen);
    IMGUI_API void bindNull(int nParam);

    IMGUI_API void reset();

    IMGUI_API void finalize();

private:

    void checkDB() const;
    void checkVM() const;

    sqlite3* mpDB;
    sqlite3_stmt* mpVM;
};


class DB
{
public:

    IMGUI_API DB();

    IMGUI_API virtual ~DB();

    IMGUI_API void open(const char* szFile);

    IMGUI_API void close();

    IMGUI_API bool tableExists(const char* szTable);

    IMGUI_API int execDML(const char* szSQL);

    IMGUI_API Query execQuery(const char* szSQL);

    IMGUI_API int execScalar(const char* szSQL);

    IMGUI_API Table getTable(const char* szSQL);

    IMGUI_API Statement compileStatement(const char* szSQL);

    IMGUI_API sqlite_int64 lastRowId() const;

    IMGUI_API void interrupt();

    IMGUI_API void setBusyTimeout(int nMillisecs);

    IMGUI_API static const char* SQLiteVersion();

private:

    DB(const DB& db);
    DB& operator=(const DB& db);

    sqlite3_stmt* compile(const char* szSQL);

    void checkDB() const;

    sqlite3* mpDB;
    int mnBusyTimeoutMs;
};
} // namespace CppSQLite3

#endif //IMGUISQLITE3_H_

