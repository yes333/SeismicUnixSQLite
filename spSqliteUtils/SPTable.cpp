//============================================================================
// Name        : SPTable.cpp
// Author      : Sanyu Ye,  SoftSeis,  Norway
// Version     : 1.1, Sept. 2020
// Copyright   : SoftSeis, Norway, all rights reserved
//============================================================================
#include "SPTable.hh"
#include <sstream>

using namespace std;
using namespace SP;

void SPDB::init() {
	stringstream ss;
	db = 0;
	if (dbs.size() == 1) {
		if (sqlite3_open_v2(dbs[0].c_str(), &db, SQLITE_OPEN_READWRITE
		| SQLITE_OPEN_CREATE, 0) != SQLITE_OK) {
			throw SPException("Database open failed");
		}
		return;
	}
	if (sqlite3_open_v2("", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0)
			!= SQLITE_OK) {
		throw SPException("Database open failed");
	}
	for (unsigned int i = 0; i < dbs.size(); ++i) {
		ss.str("");
		ss << "attach '" << dbs[i] << "' as db" << i << ";";
		executeStatement(ss, "attaching database");
	}
}

SPDB::SPDB(vector<string>& fileNames) {
	dbs = fileNames;
	init();
}

SPDB::SPDB(string fileName) {
	dbs.push_back(fileName);
	init();
}

SPDB::~SPDB() {
	if (db != 0) {
		sqlite3_close(db);
	}
}

void SPDB::beginTransaction() {
	stringstream ss("begin transaction;");
	executeStatement(ss, "transaction start");
}

void SPDB::commit() {
	stringstream ss("commit;");
	executeStatement(ss, "transaction commit");
}

sqlite3_stmt* SPDB::prepareStatement(stringstream& sql, string op) {
	const char* s_end;
	sqlite3_stmt* res;
	string sqlstring = sql.str();
	const char* s = sqlstring.c_str();

	SPVerbose::show(SPVerbose::DATA, "Prepare statement: ", sqlstring);
	int err = sqlite3_prepare_v2(db, s, -1, &res, &s_end);
	if (err != SQLITE_OK) {
		stringstream ss;
		ss << "operation " << op << " statement preparation failed with code: "
				<< err << " (sql: " << s << ")";
		throw SPException(ss.str());
	}
	return res;
}

void SPDB::executeStatement(stringstream& sql, string op) {
	string sqlstring = sql.str();
	const char* s = sqlstring.c_str();
	char *err;

	SPVerbose::show(SPVerbose::DATA, "Executing statement: ", sqlstring);

	if (sqlite3_exec(db, s, 0, 0, &err) != SQLITE_OK && op != "") {
		stringstream ss;
		ss << "operation " << op << " failed: " << err << " (sql: "
				<< sqlstring << ")";
		throw SPException(ss.str());
	}
}

string SPDB::getUnionTable(const string& table, const string& fields,
		const string& where, const string& dbColumn) {
	stringstream ss;
	for (unsigned int i = 0; i < dbs.size(); ++i) {
		if (i > 0) {
			ss << " union ";
		}
		ss << "select " << fields << "indexnumber, " << i << " as " << dbColumn
				<< " from ";
		if (dbs.size() > 1) {
			ss << "db" << i << ".";
		}
		ss << table;
		if (where.length() > 0) {
			ss << " where " << where;
		}
	}
	return ss.str();

}

void SPTable::clean() {
	for (unsigned int i = 0; i < data.size(); ++i) {
		delete (char*)data[i];
	}
	data.clear();
	columns.clear();
}

void SPTable::createTable(const string& dbName, const string& name) {
	SPDB db(dbName);
	stringstream ss;
	sqlite3_stmt* statement;

	ss << "drop table " << name << ";";
	db.executeStatement(ss, "");

	ss.str("");
	ss << "create table " << name << " (";
	for (SPPickerBox::iterator i = columns.begin(); i != columns.end(); ++i) {
		ss << i->first << " ";
		ss << (i->second->isInt() ? "integer" : "real") << ", ";
	}
	ss << " primary key (" << primaryKey << "));";
	db.executeStatement(ss, "create data table");

	ss.str("");
	ss << "insert into " << name << " values (";
	for (SPPickerBox::iterator i = columns.begin(); i != columns.end(); ++i) {
		if (i != columns.begin()) {
			ss << ", ";
		}
		ss << "?";
	}
	ss << ");";
	statement = db.prepareStatement(ss, "data table inserts");

	db.beginTransaction();
	for (unsigned int i = 0; i < data.size(); ++i) {
		int k = 1;
		for (SPPickerBox::iterator iter = columns.begin(); iter
				!= columns.end(); ++iter) {
			SPAbstractPicker* p = iter->second;
			if (p->isInt()) {
				sqlite3_bind_int(statement, k, p->getInt(data[i]));
			} else {
				sqlite3_bind_double(statement, k, p->getDouble(data[i]));
			}
			++k;
		}
		int rc = sqlite3_step(statement);
		if (rc != SQLITE_DONE) {
			throw SPException("data insertion failed: ", rc);
		}
		sqlite3_reset(statement);
		if ((i + 1) % 1000 == 0) {
			db.commit();
			db.beginTransaction();
		}
	}
	db.commit();

	sqlite3_finalize(statement);
}

void SPTable::readBySQL(SPDB& db, stringstream& sql, SPPickerBox& typedefs,
		int start, int stop) {
	sqlite3_stmt* statement = db.prepareStatement(sql, "data select statement");

	int cols = sqlite3_column_count(statement);
	SPAbstractPicker* fillers[cols];
	for (int a = 0; a < cols; a++) {
		string n = sqlite3_column_name(statement, a);
		SPAbstractPicker* p = typedefs[n];
		addColumn(n, p);
		fillers[a] = getColumnPicker(n);
	}

	for (;;) {
		int rc = sqlite3_step(statement);
		int i;
		switch (rc) {
		case SQLITE_DONE:
			sqlite3_finalize(statement);
			return;
		case SQLITE_ROW:
			i = addRow();
			for (int a = 0; a < cols; a++) {
				switch (sqlite3_column_type(statement, a)) {
				case SQLITE_INTEGER:
					fillers[a]->setInt(sqlite3_column_int(statement, a),
							getRowStart(i));
					break;
				case SQLITE_FLOAT: {
					fillers[a]->setDouble(sqlite3_column_double(statement, a),
							getRowStart(i));
					break;
				}
				}
			}
			break;
		default:
			throw SPException("unknown stepping result: ", rc);
		}
	}
}

void SPKVTable::createTable(const string& dbName, const string& name,
		map<string, string>& data) {
	SPDB db(dbName);
	sqlite3_stmt* statement;
	stringstream ss;

	ss << "drop table " << name << ";";
	db.executeStatement(ss, "");

	ss.str("");
	ss << "create table " << name << " (" << keyColumn << " string, "
			<< valueColumn << " string, primary key (" << keyColumn << "));";
	db.executeStatement(ss, "create meta table");

	ss.str("");
	ss << "insert into " << name << " values (?, ?);";
	statement = db.prepareStatement(ss, "meta table insert");

	db.beginTransaction();
	for (map<string, string>::iterator i = data.begin(); i != data.end(); ++i) {
		sqlite3_bind_text(statement, 1, i->first.c_str(), -1, 0);
		sqlite3_bind_text(statement, 2, i->second.c_str(), -1, 0);
		int rc = sqlite3_step(statement);
		if (rc != SQLITE_DONE) {
			throw SPException("data insertion failed: ", rc);
		}
		sqlite3_reset(statement);
	}
	db.commit();

	sqlite3_finalize(statement);
}

map<string, string>& SPKVTable::read(const string& dbName, const string& name) {
	static map<string, string> res;
	SPDB db(dbName);
	sqlite3_stmt* statement;
	stringstream ss;

	ss << "select " << keyColumn << ", " << valueColumn << " from " << name
			<< ";";
	statement = db.prepareStatement(ss, "read meta table");

	for (;;) {
		int rc = sqlite3_step(statement);
		switch (rc) {
		case SQLITE_DONE:
			sqlite3_finalize(statement);
			return res;
		case SQLITE_ROW:
			res[(const char*)sqlite3_column_text(statement, 0)] = (const char*)sqlite3_column_text(
					statement, 1);
			break;
		default:
			throw SPException("unknown stepping result: ", rc);
		}
	}
	return res;
}
