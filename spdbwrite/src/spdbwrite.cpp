//============================================================================
// Name        : spdbwrite.cpp
// Author      : Lin Li, Aristool AG Switzerland
// Version     : 1.0, July 2008
// Copyright   : READ group Norway, all rights reserved
//============================================================================

#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <fstream>
#include <set>
#include <stdlib.h>
#include <errno.h>
#include <SPProcessor.hh>
#include <SPAccessors.hh>
#include <SPTable.hh>
#include <string>
#include <sqlite3.h>
#include <ctime>

#undef open
#undef fopen

using namespace std;
using namespace SP;

class spdbwrite : public SPProcessor {
public:
	const static string defaultFields[];

public:
	void init();
	void process(SPSegy* data);
	void cleanup();

private:

	SPTable table;
	SPCopyMachine copy;
	SPPicker<int>* id;
	string dbpath;

	int dt; // sampling rate for all traces in the data set all traces must have identical values 
	int ns; // size of all traces in the data set all traces must have identical values
	int scalel; // scale used for elevation values
	int scalco; // scale used for coordinate values

	int max;
};

const string spdbwrite::defaultFields[] = { "fldr", "tracf", "ep", "cdp",
		"cdpt", "trid", "sx", "sy", "gx", "gy", "offset", "" };

void spdbwrite::init() {

	SPVerbose::setVerboseLevel(getIntParameter("verbose", 0));

	if (hasParameter("dbpath")) {
		dbpath.append(getStringParameter("dbpath"));
		SPVerbose::show(SPVerbose::ESSENTIAL, "Parameter found: dbpath=",
				dbpath);
	} else {
		throw SPException("No dbpath given, shutting down");
	}

	if (hasParameter("input")) {
		string s = getStringParameter("input");
		SPVerbose::show(SPVerbose::ESSENTIAL, "Input from file: ", s);
		FILE* fin = fopen(s.c_str(), "r");
		if (fin == 0) {
			throw SPException("Input file open failed: ", errno);
		}
		setinput(fin);
	}

	if (hasParameter("output")) {
		string s = getStringParameter("output");
		SPVerbose::show(SPVerbose::ESSENTIAL, "Output to file: ", s);
		FILE* fout = fopen(s.c_str(), "w");
		if (fout == 0) {
			throw SPException("Output file open failed: ", errno);
		}
		setoutput(fout);
	}

	max = getIntParameter("max", 0);

	ifstream f;
	f.open(dbpath.c_str(), ios_base::binary | ios_base::in);
	if (f.good() && !f.eof() && f.is_open()) {
		throw SPException("The file ", dbpath, " already exists, shutting down");
	}
	f.close();

	set<string> fields;

	for (int i = 0; defaultFields[i] != ""; ++i) {
		fields.insert(defaultFields[i]);
	}
	if (hasParameter("columns")) {
		char sp[1024];
		strcpy(sp, getStringParameter("columns").c_str());
		SPVerbose::show(SPVerbose::ESSENTIAL, "Parameter found: columns=", sp);
		char* p = sp;
		for (int i = 0; i < 1024 && sp[i] > 0; ++i) {
			if (sp[i] == ',') {
				sp[i] = 0;
				fields.insert(p);
				p = sp + i + 1;
			}
		}
		if (*p != 0) {
			fields.insert(p);
		}
	}

	SPVerbose::show(SPVerbose::ESSENTIAL,
			"Preparing columns in the headers table");
	id = table.addColumn<int>("indexnumber");
	SPVerbose::show(SPVerbose::DATA, "indexnumber");
	for (set<string>::iterator i = fields.begin(); i != fields.end(); i++) {
		SPAbstractPicker* p = SPSegy::getPicker()[*i];
		if(p == 0) {
			throw SPException("No such field in SEGY headers: ", *i);
		}
		table.addColumn(*i, p);
		copy.addCopy(*p, *table.getColumnPicker(*i));
		SPVerbose::show(SPVerbose::DATA, *i);
	}

	SPVerbose::show(SPVerbose::ESSENTIAL, "Start reading traces from input");
}

void spdbwrite::process(SPSegy* data) {
	if (table.numberOfRows() == 0) {
		dt = data->get<unsigned short>("dt");
		ns =data->get<unsigned short>("ns");
		scalel =data->get<short>("scalel");
		scalco =data->get<short>("scalco");

		SPVerbose::show(SPVerbose::ESSENTIAL, "Trace matric data dt: ", dt);
		SPVerbose::show(SPVerbose::ESSENTIAL, "Trace matric data ns: ", ns);
		SPVerbose::show(SPVerbose::ESSENTIAL, "Trace matric data scalel: ",
				scalel);
		SPVerbose::show(SPVerbose::ESSENTIAL, "Trace matric data scalco: ",
				scalco);

	} else if (dt !=data->get<unsigned short>("dt") || ns
			!=data->get<unsigned short>("ns") || scalel
			!=data->get<short>("scalel") || scalco !=data->get<short>("scalco")) {
		throw SPException("Inconsistent data detected");
	}

	int i = table.addRow();
	id->set(i, table.getRowStart(i));
	copy.run(data->getTrace(), table.getRowStart(i));
	dispatch(data);

	if (max > 0 && table.numberOfRows() >= max) {
		stop();
	}
}

void spdbwrite::cleanup() {
	SPVerbose::show(SPVerbose::ESSENTIAL, "Input file end");

	map<string, string> meta;
	meta["datapath"] = getStringParameter("datapath", "data.su");
	meta["comment"] = getStringParameter("comment", "");
	meta["segytape"] = getBooleanParameter("segytape", false) ? "true"
			: "false";
	meta["fortran"] = getBooleanParameter("fortran", false) ? "true" : "false";
	meta["creationdate"] = getTimeString();
	meta["creator"] = getenv("USER");

	meta["dt"] = cat(dt);
	meta["ns"] = cat(ns);
	meta["scalel"] = cat(scalel);
	meta["scalco"] = cat(scalco);
	int nr = table.numberOfRows();
	meta["numberoftraces"] = cat(nr);

	SPVerbose::show(SPVerbose::ESSENTIAL, "Dumping meta data to database");
	SPKVTable* t = new SPKVTable();
	t->createTable(dbpath, "meta", meta);

	SPVerbose::show(SPVerbose::ESSENTIAL, "Meta table content");
	for (map<string, string>::iterator i = meta.begin(); i != meta.end(); i++) {
		SPVerbose::show(SPVerbose::ESSENTIAL, i->first, ": ", i->second);
	}
	
	SPVerbose::show(SPVerbose::ESSENTIAL, "Dumping header data to database");
	table.setPrimaryKey("indexnumber");
	table.createTable(dbpath, "headers");
}

/// This is the normal code for the program driver.
int main(int argc, char **argv) {
	return (new spdbwrite())->localMain(argc, argv);
}

// make SU doc happy
const char
		* sdoc[] = {
				"SPDBWRITE - create the index db for traces in a single file",
				"",
				" spdbwrite <stdin >stdout dbpath= [optional parameters]",
				"",
				" Required parameter:",
				"",
				"      dbpath=Path to the file that contains the SQLite database",
				"             indexing the input stream. The file usually has the",
				"             extension \".db\". The file must be none existent,",
				"             otherwise this module exits with an error code.",
				"",
				" Optional parameters:",
				"",
				"      columns= :add more columns (segy field names) to the table in",
				"             addition to the fixed ones. Columns already exist are",
				"             not added again. Any column name that is not defined ",
				"             in segy header causes the module to exit with an error",
				"             code.Defaults keys:",
                "                   fldr,tracf,ep,cdp,cdpt,trid,sx,sy,gx,gy,offset",
				"      datapath=data.su: set the file path to the file containing",
				"             the data set to be indexed. This is only used to set ",
				"             the meta data. The module itself gets the data from ",
				"             the input stream.",
				"      segytape=0: or 1 if the data file has segy tape format, which",
				"             means a tape header, a binary header, and big endian",
				"             numbers",
				"      fortran=0: or 1 if the data is written by Fortran with leading",
				"             and trailing delimiters for each record",
				"      comment= : add comments to this index database.",
				"",
				" Notes:",
				"",
				"      Because this module gets its data from input stream in",
				"      standard SU format, it is the user's responsibility to make",
				"      sure the data file name, tape header and FORTRAN data format",
				"      setting are correctly set. If not, the index data will not be",
				"      applicable to the data set, resulting in wild crashes in the",
				"      read module.",
				"",
				"      On the other hand, all meta data fields can be repaired through",
				"      SQLite tools after generation.",
				"",
				" Examples:",
				"",
				"    create a database file for existing segy data",
				"        segyread tape=seisdata.sgy | \\ ",
				"        spdbwrite dbpath=seisdata.db datapath=seisdata.sgy \\ ",
						"        segytape=1 > /dec/null", "",
						"    create a database after processing",
				"        ... | suchw key1=tstat key2=offset d=1.480 |\\ ",
				"        spdbwrite dbpath=mydata.db datapath=mydata.su \\ ",
						"        columns=tstat > mydata.su", 0 };
