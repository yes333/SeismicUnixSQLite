//============================================================================
// Name        : test.cpp
// Author      : Sanyu Ye,  SoftSeis,  Norway
// Version     : 1.1, Sept. 2020
// Copyright   : SoftSeis, Norway, all rights reserved
//============================================================================

#include <iostream>
#include <stdlib.h>
#include <SPProcessor.hh>
#include <SPAccessors.hh>
#include <SPTable.hh>
#include <string>
#include <sqlite3.h>

using namespace std;
using namespace SP;

void tableTest() {

	SPTable* t = new SPTable();
	t->addColumn<int>("good");
	t->addColumn<char>("bad");
	t->addColumn<short>("ugli");

	int r = t->addRow();
	t->set<int>("good", r, (int)200);
	t->set<char>("bad", r, (char)100);
	t->set<short>("ugli", r, (short)200);

	r = t->addRow();
	t->set<int>("good", r, (int)100);
	t->set<char>("bad", r, (char)70);
	t->set<short>("ugli", r, (short)100);

	t->setPrimaryKey("good");

	t->createTable("test1.db", "test");
	delete t;
}

void stringTableWriteTest() {
	map<string, string> data;
	data["one"] = "1000";
	data["two"] = "2000";
	data["three"] = "3000";

	SPKVTable* t = new SPKVTable();
	t->createTable("test.db", "meta", data);
}

void stringTableReadTest() {
	SPKVTable* t = new SPKVTable();
	map<string, string>& res = t->read("test.db", "meta");

	for (map<string, string>::iterator i = res.begin(); i != res.end(); ++i) {
		cerr << i->first << ": " << i->second << endl;
	}
}

void stringReadTest() {
	sqlite3_stmt* res;
	stringstream ss;
	SPDB db("test.db");

	ss.str("select key, value from meta;");
	res = db.prepareStatement(ss, "test");

	for (;;) {
		int rc = sqlite3_step(res);
		switch (rc) {
		case SQLITE_DONE:
			sqlite3_finalize(res);
			return;
		case SQLITE_ROW:
			cerr << sqlite3_column_text(res, 0) << ": " <<sqlite3_column_text(
					res, 1)<< endl;
			break;
		default:
			throw SPException("unknown stepping result: ", 0, rc);
		}
	}
}

int main(int argc, char **argv) {
	stringTableReadTest();
}
