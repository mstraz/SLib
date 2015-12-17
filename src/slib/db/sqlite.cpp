#include "../../../inc/thirdparty/sqlite/sqlite3.h"

#include "../../../inc/slib/db/sqlite.h"
#include "../../../inc/slib/core/file.h"

SLIB_DB_NAMESPACE_BEGIN
SQLiteDatabase::SQLiteDatabase()
{
}

class _Sqlite3Database : public SQLiteDatabase
{
public:
	sqlite3* m_db;

	_Sqlite3Database()
	{
		m_db = sl_null;
	}

	~_Sqlite3Database()
	{
		::sqlite3_close(m_db);
	}

	static Ref<_Sqlite3Database> connect(const String& _filePath)
	{
		Ref<_Sqlite3Database> ret;
		sqlite3* db = sl_null;
		String8 filePath = _filePath;
		if (File::exists(filePath)) {
			sl_int32 iResult = ::sqlite3_open(filePath.getBuf(), &db);
			if (SQLITE_OK == iResult) {
				ret = new _Sqlite3Database();
				if (ret.isNotNull()) {
					ret->m_db = db;
					return ret;
				}
				::sqlite3_close(db);
			}
		}
		return ret;
	}

	sl_bool execute(const String& _sql, sl_size* pOutAffectedRowsCount)
	{
		MutexLocker lock(getLocker());
		char* zErrMsg = 0;
		String8 sql = _sql;
		if (SQLITE_OK == ::sqlite3_exec(m_db, sql.getBuf(), 0, 0, &zErrMsg)) {
			if (pOutAffectedRowsCount) {
				*pOutAffectedRowsCount = ::sqlite3_changes(m_db);
			}
			return sl_true;
		} else {
			::sqlite3_free(zErrMsg);
		}
		return sl_false;
	}

	static Map<String, Variant> _getRow(sqlite3_stmt* stmt, String* columns, sl_uint32 nColumns)
	{
		Map<String, Variant> row = Map<String, Variant>::createList();
		for (sl_uint32 cIndex = 0; cIndex < nColumns; cIndex++) {
			String cValue = String::fromUtf8(::sqlite3_column_text(stmt, (int)cIndex));
			row.put(columns[cIndex], cValue);
		}
		return row;
	}

	class _DatabaseCursor : public DatabaseCursor
	{
	public:
		Ref<DatabaseStatement> m_statementObj;
		sqlite3_stmt* m_statement;

		List<String> m_listColumnNames;
		sl_uint32 m_nColumnNames;
		String* m_columnNames;
		Map<String, sl_int32> m_mapColumnIndexes;

		_DatabaseCursor(Database* db, DatabaseStatement* statementObj, sqlite3_stmt* statement)
		{
			m_db = db;
			m_statementObj = statementObj;
			m_statement = statement;

			sl_int32 cols = ::sqlite3_column_count(statement);
			for (sl_int32 i = 0; i < cols; i++) {
				const char* buf = ::sqlite3_column_name(statement, (int)i);
				String name(buf);
				m_listColumnNames.push(name);
				m_mapColumnIndexes.put(name, i);
			}
			m_nColumnNames = (sl_uint32)(m_listColumnNames.getCount());
			m_columnNames = m_listColumnNames.getBuffer();

			db->lock();
		}

		~_DatabaseCursor()
		{
			::sqlite3_reset(m_statement);
			::sqlite3_clear_bindings(m_statement);
			m_db->unlock();
		}

		sl_uint32 getColumnsCount()
		{
			return m_nColumnNames;
		}

		String getColumnName(sl_uint32 index)
		{
			if (index < m_nColumnNames) {
				return m_columnNames[index];
			}
			return String::null();
		}

		sl_int32 getColumnIndex(const String& name)
		{
			return m_mapColumnIndexes.getValue(name, -1);
		}

		Map<String, Variant> getRow()
		{
			Map<String, Variant> ret;
			ret = Map<String, Variant>::createHash();
			for (sl_uint32 index = 0; index < m_nColumnNames; index++) {
				Variant var = _getValue(index);
				ret.put(m_columnNames[index], var);
			}
			return ret;
		}

		String _getString(sl_uint32 index)
		{
			int n = sqlite3_column_bytes(m_statement, index);
			const void* buf = sqlite3_column_text(m_statement, index);
			if (buf && n > 0) {
				return String8::fromUtf8(buf, n);
			}
			return String8::getEmpty();
		}

		Memory _getBlob(sl_uint32 index)
		{
			int n = sqlite3_column_bytes(m_statement, index);
			const void* buf = sqlite3_column_blob(m_statement, index);
			if (buf && n > 0) {
				return Memory::create(buf, n);
			}
			return Memory::null();
		}

		Variant _getValue(sl_uint32 index)
		{
			int type = ::sqlite3_column_type(m_statement, index);
			switch (type) {
			case SQLITE_INTEGER:
				{
					sl_int64 v64 = sqlite3_column_int64(m_statement, index);
					sl_int32 v32 = (sl_int32)v64;
					if (v64 == v32) {
						return v32;
					} else {
						return v64;
					}
				}
				break;
			case SQLITE_FLOAT:
				return sqlite3_column_double(m_statement, index);
			case SQLITE_TEXT:
				return _getString(index);
			case SQLITE_BLOB:
				return _getBlob(index);
			}
			return Variant::null();
		}

		Variant getValue(sl_uint32 index)
		{
			if (index < m_nColumnNames) {
				return _getValue(index);
			}
			return Variant::null();
		}

		String getString(sl_uint32 index)
		{
			if (index < m_nColumnNames) {
				int type = ::sqlite3_column_type(m_statement, index);
				switch (type) {
				case SQLITE_INTEGER:
					return String::fromInt64(sqlite3_column_int64(m_statement, index));
				case SQLITE_FLOAT:
					return String::fromDouble(sqlite3_column_double(m_statement, index));
				case SQLITE_TEXT:
					return _getString(index);
				}
			}
			return String::null();
		}

		sl_int64 getInt64(sl_uint32 index, sl_int64 defaultValue)
		{
			if (index < m_nColumnNames) {
				int type = ::sqlite3_column_type(m_statement, index);
				switch (type) {
				case SQLITE_INTEGER:
					return sqlite3_column_int64(m_statement, index);
				case SQLITE_FLOAT:
					return (sl_int64)(sqlite3_column_double(m_statement, index));
				case SQLITE_TEXT:
					return _getString(index).parseInt64(10, defaultValue);
				}
			}
			return defaultValue;
		}

		sl_uint64 getUint64(sl_uint32 index, sl_uint64 defaultValue)
		{
			if (index < m_nColumnNames) {
				int type = ::sqlite3_column_type(m_statement, index);
				switch (type) {
				case SQLITE_INTEGER:
					return sqlite3_column_int64(m_statement, index);
				case SQLITE_FLOAT:
					return (sl_uint64)(sqlite3_column_double(m_statement, index));
				case SQLITE_TEXT:
					return _getString(index).parseUint64(10, defaultValue);
				}
			}
			return defaultValue;
		}

		sl_int32 getInt32(sl_uint32 index, sl_int32 defaultValue)
		{
			if (index < m_nColumnNames) {
				int type = ::sqlite3_column_type(m_statement, index);
				switch (type) {
				case SQLITE_INTEGER:
					return sqlite3_column_int(m_statement, index);
				case SQLITE_FLOAT:
					return (sl_int32)(sqlite3_column_double(m_statement, index));
				case SQLITE_TEXT:
					return _getString(index).parseInt32(10, defaultValue);
				}
			}
			return defaultValue;
		}

		sl_uint32 getUint32(sl_uint32 index, sl_uint32 defaultValue)
		{
			if (index < m_nColumnNames) {
				int type = ::sqlite3_column_type(m_statement, index);
				switch (type) {
				case SQLITE_INTEGER:
					return (sl_uint32)(sqlite3_column_int64(m_statement, index));
				case SQLITE_FLOAT:
					return (sl_uint32)(sqlite3_column_double(m_statement, index));
				case SQLITE_TEXT:
					return _getString(index).parseUint32(10, defaultValue);
				}
			}
			return defaultValue;
		}

		float getFloat(sl_uint32 index, float defaultValue)
		{
			if (index < m_nColumnNames) {
				int type = ::sqlite3_column_type(m_statement, index);
				switch (type) {
				case SQLITE_INTEGER:
					return (float)(sqlite3_column_int64(m_statement, index));
				case SQLITE_FLOAT:
					return (float)(sqlite3_column_double(m_statement, index));
				case SQLITE_TEXT:
					return _getString(index).parseFloat(defaultValue);
				}
			}
			return defaultValue;
		}

		double getDouble(sl_uint32 index, double defaultValue)
		{
			if (index < m_nColumnNames) {
				int type = ::sqlite3_column_type(m_statement, index);
				switch (type) {
				case SQLITE_INTEGER:
					return (double)(sqlite3_column_int64(m_statement, index));
				case SQLITE_FLOAT:
					return (sqlite3_column_double(m_statement, index));
				case SQLITE_TEXT:
					return _getString(index).parseDouble(defaultValue);
				}
			}
			return defaultValue;
		}

		Memory getBlob(sl_uint32 index)
		{
			if (index < m_nColumnNames) {
				int type = ::sqlite3_column_type(m_statement, index);
				switch (type) {
				case SQLITE_TEXT:
				case SQLITE_BLOB:
					return _getBlob(index);
				}
			}
			return Memory::null();
		}

		sl_bool moveNext()
		{
			sl_int32 nRet = ::sqlite3_step(m_statement);
			if (nRet == SQLITE_ROW) {
				return sl_true;
			} else {
				return sl_false;
			}
		}

	};

	class _DatabaseStatement : public DatabaseStatement
	{
	public:
		sqlite3* m_sqlite;
		sqlite3_stmt* m_statement;
		Array<Variant> m_boundParams;

		_DatabaseStatement(_Sqlite3Database* db, sqlite3_stmt* statement)
		{
			m_db = db;
			m_sqlite = db->m_db;
			m_statement = statement;
		}

		~_DatabaseStatement()
		{
			::sqlite3_finalize(m_statement);
		}

		sl_bool _execute(const Variant* _params, sl_uint32 nParams)
		{
			::sqlite3_reset(m_statement);
			::sqlite3_clear_bindings(m_statement);
			m_boundParams.setNull();

			Array<Variant> params = Array<Variant>::create(_params, nParams);
			if (params.isNull()) {
				return sl_false;
			}
			sl_uint32 n = (sl_uint32)(::sqlite3_bind_parameter_count(m_statement));
			if (n == nParams) {
				if (n > 0) {
					for (sl_uint32 i = 0; i < n; i++) {
						int iRet = SQLITE_ABORT;
						Variant& var = params[i];
						switch (var.getType()) {
						case Variant::typeEmpty:
							iRet = ::sqlite3_bind_null(m_statement, i);
							break;
						case Variant::typeBoolean:
						case Variant::typeInt32:
							iRet = ::sqlite3_bind_int(m_statement, i, var.getInt32());
							break;
						case Variant::typeUint32:
						case Variant::typeInt64:
						case Variant::typeUint64:
							iRet = ::sqlite3_bind_int64(m_statement, i, var.getInt64());
							break;
						case Variant::typeFloat:
						case Variant::typeDouble:
							iRet = ::sqlite3_bind_double(m_statement, i, var.getDouble());
							break;
						default:
							if (var.isMemory()) {
								Memory mem = var.getMemory();
								sl_size size = mem.getSize();
								if (size > 0x7fffffff) {
									iRet = ::sqlite3_bind_blob64(m_statement, i, mem.getBuf(), size, SQLITE_STATIC);
								} else {
									iRet = ::sqlite3_bind_blob(m_statement, i, mem.getBuf(), (sl_uint32)size, SQLITE_STATIC);
								}
							} else {
								String8 str = var.getString();
								var = str;
								iRet = ::sqlite3_bind_text(m_statement, i, str.getBuf(), str.getLength(), SQLITE_STATIC);
							}
						}
						if (iRet != SQLITE_OK) {
							return sl_false;
						}
					}
				}
				m_boundParams = params;
				return sl_true;
			}
			return sl_false;
		}

		sl_bool execute(const Variant* params, sl_uint32 nParams, sl_size* pOutAffectedRowsCount)
		{
			MutexLocker lock(m_db->getLocker());
			if (_execute(params, nParams)) {
				if (::sqlite3_step(m_statement) == SQLITE_DONE) {
					::sqlite3_reset(m_statement);
					::sqlite3_clear_bindings(m_statement);
					if (pOutAffectedRowsCount) {
						*pOutAffectedRowsCount = ::sqlite3_changes(m_sqlite);
					}
					return sl_true;
				}
			}
			return sl_false;
		}

		Ref<DatabaseCursor> query(const Variant* params, sl_uint32 nParams)
		{
			MutexLocker lock(m_db->getLocker());
			Ref<DatabaseCursor> ret;
			if (_execute(params, nParams)) {
				ret = new _DatabaseCursor(m_db.get(), this, m_statement);
				if (ret.isNotNull()) {
					return ret;
				}
				::sqlite3_reset(m_statement);
				::sqlite3_clear_bindings(m_statement);
			}
			return ret;
		}
	};

	Ref<DatabaseStatement> prepareStatement(const String& _sql)
	{
		MutexLocker lock(getLocker());
		Ref<DatabaseStatement> ret;
		String8 sql = _sql;
		sqlite3_stmt* statement = sl_null;
		if (SQLITE_OK == ::sqlite3_prepare_v2(m_db, sql.getBuf(), -1, &statement, sl_null)) {
			ret = new _DatabaseStatement(this, statement);
			if (ret.isNotNull()) {
				return ret;
			}
			::sqlite3_finalize(statement);
		}
		return ret;
	}

	String getErrorMessage()
	{
		return ::sqlite3_errmsg(m_db);
	}
};

Ref<SQLiteDatabase> SQLiteDatabase::connect(const String& path)
{
	Ref<SQLiteDatabase> ret = _Sqlite3Database::connect(path);
	return ret;
}
SLIB_DB_NAMESPACE_END
