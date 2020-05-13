//============================================================================
// Name        : spdbread.cpp
// Author      : Lin Li, Aristool AG Switzerland
// Version     : 1.0, July 2008
// Copyright   : READ group Norway, all rights reserved
//============================================================================

#define _FILE_OFFSET_BITS 64

#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <set>
#include <SPProcessor.hh>
#include <SPAccessors.hh>
#include "SPParsers.hh"
#include <SPTable.hh>
#include <header.h>

using namespace std;
using namespace SP;

#undef open

bool bigEndianMachine() {
	long l = 0;
	char *b = (char *)&l;
	b[3] = 1;
	return l == 1;
}

class filereader {
public:
	filereader(const string& dbPath, const string& dataPath);
	~filereader() {
	}
	bool compatible(const filereader& other);
	void overrideByteswap(bool on);
	void setFloatFormat(bool on);
	void ibm_to_float(int from[], int to[], int n, int endian);

	segy* read(int id);

private:
	long long fileSize(const string& fileName);

private:
	ifstream file;

	string datapath;
	bool segytape;
	bool fortran;
	bool byteswap;
	bool ibmfloat;
	int nrTraces;

	int dt;
	int ns;
	int scalel;
	int scalco;

	int traceSize;
	int headerOffset;
	int recordLength;
	segy* store;
};

filereader::filereader(const string& dbPath, const string& dataPath) {
	SPVerbose::show(SPVerbose::DATA, "Initializing for db: ", dbPath);

	if (!fileSize(dbPath)) {
		throw SPException("File for database not found: ", dbPath);
	}

	map<string, string>& meta = (new SPKVTable())->read(dbPath, "meta");

	datapath = dataPath == "" ? meta["datapath"] : dataPath;
	segytape = meta["segytape"] == "true";
	fortran = meta["fortran"] == "true";

	byteswap = segytape != bigEndianMachine();
	ibmfloat = true;

	dt = atoi(meta["dt"].c_str());
	ns = atoi(meta["ns"].c_str());
	scalel = atoi(meta["scalel"].c_str());
	scalco = atoi(meta["scalco"].c_str());
	nrTraces = atoi(meta["numberoftraces"].c_str());

	traceSize = 240 + ns * 4;
	recordLength = traceSize;
	headerOffset = 0;
	if (fortran) {
		recordLength += 8;
		headerOffset += 4;
	}
	if (segytape) {
		headerOffset += 3600;
		if (fortran) {
			headerOffset += 16;
		}
	}
	store = (segy*)new char[traceSize];

	long long fs = fileSize(datapath);
	long long dl = ((long long)recordLength) * nrTraces + headerOffset - 4;
	if (fs < dl) {
		throw SPException("Data file ", datapath, " length error: ", dl, " bytes required");
	}
	file.open(datapath.c_str(), ios::in | ios::binary);

	SPVerbose::show(SPVerbose::DATA, "datapath: ", datapath);
	SPVerbose::show(SPVerbose::DATA, "segytape: ", segytape);
	SPVerbose::show(SPVerbose::DATA, "fortran: ", fortran);
	SPVerbose::show(SPVerbose::DATA, "byteswap: ", byteswap);
	SPVerbose::show(SPVerbose::DATA, "nrTraces: ", nrTraces);

	SPVerbose::show(SPVerbose::DATA, "dt: ", dt);
	SPVerbose::show(SPVerbose::DATA, "ns: ", ns);
	SPVerbose::show(SPVerbose::DATA, "scalel: ", scalel);
	SPVerbose::show(SPVerbose::DATA, "scalco: ", scalco);

	SPVerbose::show(SPVerbose::DATA, "traceSize: ", traceSize);
	SPVerbose::show(SPVerbose::DATA, "headerOffset: ", headerOffset);
	SPVerbose::show(SPVerbose::DATA, "recordLength: ", recordLength);
}

bool filereader::compatible(const filereader& other) {
	return dt == other.dt || ns == other.ns || scalel == other.scalel || scalco
			== other.scalco;
}

void filereader::overrideByteswap(bool on) {
	SPVerbose::show(SPVerbose::ESSENTIAL, "Overriding byteswap of ", datapath,
			" to ", on);
	byteswap = on;
}

void filereader::setFloatFormat(bool on) {
	if (segytape) SPVerbose::show(SPVerbose::ESSENTIAL, datapath,
			" has ", on? "IBM" : "IEEE", " floating point format");
	ibmfloat = on;
}

segy* filereader::read(int id) {
	long long p = ((long long)recordLength) * id + headerOffset;
	SPVerbose::show(SPVerbose::EVERYTHING, datapath, ": reading trace ", id,
			" at ", p);

	file.seekg(p);
	file.read((char*)store, traceSize);

	if (byteswap) {  // swap trace headers
		for (int i = 0; i < SU_NKEYS; ++i) {
			swaphval(store, i);
		}
	}
	if (byteswap && !ibmfloat) {
		for (int i = 0; i < ns; ++i) {
			swap_float_4((float*)((char*)store + (240 + i * 4)));
		}
	} else if (byteswap && segytape && ibmfloat) {
	    ibm_to_float((int *) ((char*)store + 240), (int *) ((char*)store + 240), ns, 0);
	}
	return store;
}

long long filereader::fileSize(const string& name) {
	struct stat res;

	int rc= stat(name.c_str(), &res);
	if (rc) {
		throw SPException("Cannot get stat of file ", name, ": ", rc);
	}
	return res.st_size;
}

void filereader::ibm_to_float(int from[], int to[], int n, int endian)
/***********************************************************************
ibm_to_float - convert between 32 bit IBM and IEEE floating numbers
 ************************************************************************
Input::
from		input vector
to		output vector, can be same as input vector
endian		byte order =0 little endian (DEC, PC's)
                            =1 other systems
 *************************************************************************
Notes:
Up to 3 bits lost on IEEE -> IBM

Assumes sizeof(int) == 4

IBM -> IEEE may overflow or underflow, taken care of by
substituting large number or zero

Only integer shifting and masking are used.
 *************************************************************************
Credits: CWP: Brian Sumner,  c.1985
 *************************************************************************/
{
    int fconv, fmant, i, t;

    for (i = 0; i < n; ++i) {

        fconv = from[i];

        /* if little endian, i.e. endian=0 do this */
        if (endian == 0) fconv = (fconv << 24) | ((fconv >> 24) & 0xff) |
            ((fconv & 0xff00) << 8) | ((fconv & 0xff0000) >> 8);

        if (fconv) {
            fmant = 0x00ffffff & fconv;
            /* The next two lines were added by Toralf Foerster */
            /* to trap non-IBM format data i.e. conv=0 data  */
            if (fmant == 0)
                SPVerbose::show(SPVerbose::ESSENTIAL, "mantissa is zero data may not be in IBM FLOAT Format !");
            t = (int) ((0x7f000000 & fconv) >> 22) - 130;
            while (!(fmant & 0x00800000)) {
                --t;
                fmant <<= 1;
            }
            if (t > 254) fconv = (0x80000000 & fconv) | 0x7f7fffff;
            else if (t <= 0) fconv = 0;
            else fconv = (0x80000000 & fconv) | (t << 23)
                | (0x007fffff & fmant);
        }
        to[i] = fconv;
    }
    return;
}


class spdbread : public SPProcessor {
public:
	void init();

private:
	stringstream& getSQL(SPDB& db, SPGroup* group);
	bool checkData();

private:
	SPTable table;
	SPParserBase* overrides;
	SPIndexFileSpec* fileSpec;
	filereader** files;
	SPSelection* select;
};

void spdbread::init() {

	stop();

	SPVerbose::setVerboseLevel(getIntParameter("verbose", 0));

	if (hasParameter("paths")) {
		string p = getStringParameter("paths");
		SPVerbose::show(SPVerbose::ESSENTIAL, "Parameter found: paths=", p);

		fileSpec = new SPIndexFileSpec(p);
		if (fileSpec->getLength() == 0) {
			SPVerbose::show(SPVerbose::ERROR, "No db file specified");
			return;
		}
		if (!checkData()) {
			throw SPException("Data in the files are not compatible");
		}
	} else {
		SPVerbose::show(SPVerbose::ERROR, "No db file specified");
		return;
	}

	if (hasParameter("overrides")) {
		overrides = new SPParserBase(getStringParameter("overrides"), ",");
		SPVerbose::show(SPVerbose::ESSENTIAL, "Parameter found: overrides=",
				getStringParameter("overrides"));
	}

	if (hasParameter("select")) {
		string s = getStringParameter("select");
		select = new SPSelection(s);
		SPVerbose::show(SPVerbose::ESSENTIAL, "Parameter found: select=", s);
	} else {
		select = new SPSelection((char*)"");
		SPVerbose::show(SPVerbose::ESSENTIAL, "Selection not specified");
	}
	
	SPCopyMachine* copy = 0;

	SPVerbose::show(SPVerbose::ESSENTIAL,
			"Openning data base connection for selected read");
	vector<string> names;
	for (int i = 0; i < fileSpec->getLength(); ++i) {
		string n = fileSpec->getFiles()[i]->getDBFileName();
		names.push_back(n);
		SPVerbose::show(SPVerbose::DATA, "Adding database file: ", n);
	}
	SPDB db(names);

	SPSegy::getPicker()["indexnumber"] = new SPPicker<int>(0);
	SPSegy::getPicker()["fileid"] = new SPPicker<int>(0);
	for (int j = 0; j < select->getLength(); ++j) {
		table.clean();
		SPVerbose::show(SPVerbose::ESSENTIAL,
				"Reading data from database for group #", j);
		table.readBySQL(db, getSQL(db, select->getGroups()[j]), SPSegy::getPicker());

		if (overrides != 0 && overrides->getLength() > 0 && copy == 0) {
			SPVerbose::show(SPVerbose::ESSENTIAL,
					"Building copy machine for header overrides");
			copy = new SPCopyMachine();
			for (int i = 0; i < overrides->getLength(); ++i) {
				string f = overrides->getFractions()[i];
				copy->addCopy(*table.getColumnPicker(f), *SPSegy::getPicker()[f]);
			}
		}

		SPVerbose::show(SPVerbose::ESSENTIAL,
				"Reading selected data from trace files");
		int n = table.numberOfRows();
		for (int i = 0; i < n; ++i) {
			void* row = table.getRowStart(i);
			int fid = table.getColumnPicker("fileid")->getInt(row);
			int index = table.getColumnPicker("indexnumber")->getInt(row);
			segy* s = files[fid]->read(index);
			if (copy != 0) {
				copy->run(row, (void*)s);
			}
			fputtr(stdout, s);
		}
		SPVerbose::show(SPVerbose::ESSENTIAL, "End of group #", j,
				", number of records written: ", n);
	}
}

bool spdbread::checkData() {
	files = new filereader*[fileSpec->getLength()];
	for (int i = 0; i < fileSpec->getLength(); ++i) {
		SPDBFilePath* f = fileSpec->getFiles()[i];
		files[i] = new filereader(f->getDBFileName(), f->getDataFileName());
		if (hasParameter("byteswap")) {
			files[i]->overrideByteswap(getBooleanParameter("byteswap", false));
		}
		if (hasParameter("ibmfloat")) {
			files[i]->setFloatFormat(getBooleanParameter("ibmfloat", true));
		}
		if (i > 0) {
			if (!files[i]->compatible(*files[i-1])) {
				return false;
			}
		}
	}
	return true;
}

stringstream& spdbread::getSQL(SPDB& db, SPGroup *group) {
	set<string> names;
	static stringstream ss;

	ss.str("");

	for (int i = 0; i < group->getLength(); ++i) {
		names.insert(group->getColumns()[i]->getName());
	}
	if (overrides != 0) {
		for (int i = 0; i < overrides->getLength(); ++i) {
			names.insert(overrides->getFractions()[i]);
		}
	}

	for (set<string>::iterator i = names.begin(); i != names.end(); i++) {
		ss << *i << ", ";
	}

	string table = db.getUnionTable("headers", ss.str(), group->getWhere(),
			"fileid");

	ss.str("");

	if (db.getNumberOfFiles() == 1) {
		ss << table;
	} else {
		ss << "select * from (" << table << ")";
	}
	ss << " order by " << group->getOrders() << " indexnumber;";
	return ss;
}

/// This is the normal code for the program driver.
int main(int argc, char **argv) {
	return (new spdbread())->localMain(argc, argv);
}

// make SU doc happy
const char
		* sdoc[] = {
				"SPDBREAD - stream traces selected and sorted using the index database",
				"",
				" spdbread >stdout dbpaths= [optional parameters]",
				"",
				" Required parameter:",
				"",
				"      paths=     paths to the files that contain the SQLite databases and",
				"                 data sets. The syntax is explained below.",
				"",
				" Optional parameters:",
				"",
				"      select=    specify the trace selection and sequencing",
				"                 criteria for the SU data stream to be produced. Syntax",
				"                 is explained below.",
				"",
				"      overrides= the header fields where the data in the index file",
				"                 replaces the value from data file. This is useful when",
				"                 the head field data is manipulated through SQLite ",
				"                 facilities and those result should be used for down",
				"                 stream processing.",
				"",
				"      byteswap=0 or 1 for whether the numbers in the data file ",
				"                 are to be swapped for endian change. ",
				"                 !!!! IMPORTANT: this parameter must be missing from the",
				"                 command line if the sudbread is to determine the need",
				"                 for byte swap automatically",
				"                 !!!! IMPORTANT: this parameter, if set, overrides byte",
				"                 swapping for all data files. This means if the files",
				"                 are of mixed format, you should not override them.",
				"",
				"      ibmfloat=1 default assuming IBM floating point for segy data.",
				"                 =0 IEEE floating point.",
				"                 This parameter is ignored if data file is in SU format",
				"",
				" Path specification syntax:",
				"",
				"      The paths to both the data set files and the index files are as",
				"      follows: ",
				"",
				"          ip = <index file path>",
				"          dp = <data file path>",
				"          setpath = p['('dp')']",
				"          paths = setpath[','setpath]*",
				"",
				"      Where <index file path>s are to the \".db\" files. If the data",
				"      file to the index is wrong in the meta data of the index",
				"      database, the <data file path> can be added to correct it. So",
				"      the file spec test1.db(oldTest.su) means use test1.db as index",
				"      file for oldTest.su. A complete path spec could be:",
				"",
				"              t1.db(tape0.import),t2.db,tnew.db(data.su)",
				"",
				" Trace stream selection syntax:",
				"",
				"      The selection and ordering of the traces are done with the",
				"      following syntax:",
				"",
				"          range=<limit1>':'<limit2>[':'<increment>]",
				"          valueset=(<value>|range)[','(<value>|range)]*",
				"          order=['+'|'-']",
				"          column::=<column name>[order]['('valueset')']",
				"          group::=column['|'column]*",
				"          spec::=group['/'group]*",
				"",
				"      Valueset specifies the \"in\" values. For example",
				"          \"1:10,-20,100,502:10000:5\"",
				"      specifies the following value set:",
				"          all values between 1 and 10 (including both 1 and 10)",
				"          plus -20, plus 100, plus all values between 502 and ",
				"          10000 at increment of 5 (502, 507, 512...)",
				"",
				"      The order part defines the ordering of the traces to the",
				"      stream. '+' means result with ascending value for the field;",
				"      '-' means descending order. If this part is missing the ",
				"      column is not used in ordering of the stream. The following",
				"      is a group:",
				"",
				"  cdp+(1:100)|fldr(1000,7000:8000:20,50000)|sx+(-12000:12000)|sy-",
				"",
				"      The traces produced by this groups is selected by the header",
				"      values of fields cdp, fldr, and sx. The resulting trace stream",
				"      is ordered by cdpt value ascending first and then sx value",
				"      ascending and sy value descending.",
				"",
				"      The selection can include many groups, concatenated with a",
				"      '/' in between. In this case, the output stream will also",
				"      be data selected by each of them in their group order.",
				"",
				" !!!! IMPORTANT: the syntax specified above contain characters used",
				"      by shells in their scripts. In order to prevent the shell",
				"      from hijacking the parameters, they have to be quoted (\"\")",
				"      in the command line as the examples below shows.", "",
				"",
				" Examples:", "",
				"    read two files sorted by component and stack",
				"	   spdbread dbpaths=pdata.db,zdata.db overrides=unscale \\",
				"	     \"select=cdp+|nhs+|fldr-|ep-|offset(-3000:3000)\" |\\",
				"	   suweight a=0 b=1 key=unscale | sustack key=fldr > pz.su",
				"", 
				"    select every 100 shot gather for QC",
				"	   spdbread \"dbpaths=mydata.db(/newfolder/mydata.su)\" \\",
						"		\"select=fldr+(1000:10000:100)|gx-\" |\\",
						"	  suxmovie n2=240 n3=901 perc=98 loop=2 ", 0 };
