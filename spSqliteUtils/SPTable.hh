//============================================================================
// Name        : SPTable.cpp
// Author      : Lin Li, Aristool AG Switzerland
// Version     : 1.0, July 2008
// Copyright   : READ group Norway, all rights reserved
//============================================================================
#ifndef SPTABLE_HH_
#define SPTABLE_HH_

#include <SPProcessor.hh>
#include <sqlite3.h>
#include <vector>
#include <sstream>

namespace SP {

class SPDB {

public:
	SPDB(string file);
	SPDB(vector<string>& files);
	virtual ~SPDB();

	void beginTransaction();
	void commit();

	sqlite3_stmt* prepareStatement(stringstream& sql, string op);
	void executeStatement(stringstream& sql, string op);
	string getUnionTable(const string& table, const string& fields,
			const string& where, const string& dbColumn);

	sqlite3* getDB() {
		return db;
	}
	
	int getNumberOfFiles() {
		return dbs.size();
	}
	
private:
	void init();

private:
	vector<string> dbs;
	sqlite3* db;
};

class SPTable {

public:
	SPTable() {
	}

	~SPTable() {
		clean();
	}

	template<typename T> SPPicker<T>* addColumn(const string& name,
			void* description = 0) {
		int size = sizeof(T);
		int pos = ((end + size - 1)/size) * size;
		end = pos + size;

		SPPicker<T>* p = new SPPicker<T>(pos);
		p->setProperties(description);

		columns[name] = p;
		return p;
	}

	void addColumn(const string& name, SPAbstractPicker* ref,
			void* description = 0) {
		int size = ref->getSize();
		int pos = ((end + size - 1)/size) * size;
		end = pos + size;

		SPAbstractPicker* p = ref->duplicate(pos);
		p->setProperties(description);

		columns[name] = p;
	}

	SPAbstractPicker* getColumnPicker(const string& name) {
		return columns[name];
	}

	vector<string>& getColumnNames() {
		return columns.getNames();
	}

	void* getRowStart(int row) {
		return data[row];
	}

	template<typename T> T get(const string& column, int row) {
		return ((SPPicker<T>)
				columns[column])->get(getRowStart(row));
	}

	template<typename T> void set(const string& column, int row, T value) {
		((SPPicker<T>*)columns[column])->set(value,
				getRowStart(row));
	}

	int addRow() {
		data.push_back((void*)new char[end]);
		return data.size() - 1;
	}

	int numberOfRows() {
		return data.size();
	}

	void setPrimaryKey(const string& key) {
		primaryKey = key;
	}

	void createTable(const string& fileName, const string& name);
	void readBySQL(SPDB& db, stringstream& sql, SPPickerBox& typedefs,
			int start = 0, int stop = -1);

	void clean();

private:
	SPPickerBox columns;
	int end;
	string primaryKey;
	vector<void*> data;
};

class SPKVTable {

public:
	SPKVTable() :
		keyColumn("key"), valueColumn("value") {
	}

	SPKVTable(string key, string value) :
		keyColumn(key), valueColumn(value) {
	}

	~SPKVTable() {
	}

	void createTable(const string& fileName, const string& name,
			map<string, string>& data);
	map<string, string>& read(const string& fileName, const string& name);

private:
	string keyColumn;
	string valueColumn;
};
}
#endif /*SPTABLE_HH_*/
