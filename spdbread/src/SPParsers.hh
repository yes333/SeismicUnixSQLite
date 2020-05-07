//============================================================================
// Name        : SPParsers.hh
// Author      : Lin Li, Aristool AG Switzerland
// Version     : 1.0, Aug. 2008
// Copyright   : READ group Norway, all rights reserved
//============================================================================
#ifndef SPPARSERS_H_
#define SPPARSERS_H_

#include <string>
#include <iostream>
#include <cstdlib>

using namespace std;

namespace SP {

template<typename T> T** buildSub(int length, char** texts) {
	T** res = new T*[length];
	for (int i = 0; i < length; ++i) {
		res[i] = new T(texts[i]);
	}
	return res;
}

class SPParserBase {
public:
	SPParserBase(char* data, const char* separators) {
		init(data, separators);

	}
	SPParserBase(const string& data, const char* separators) {
		init(data, separators);
	}

	virtual ~SPParserBase() {
	}

	char** getFractions() {
		return fractions;
	}

	int getLength() {
		return length;
	}

private:
	void init(char* data, const char* separators);
	void init(const string& data, const char* separators);
private:
	char** fractions;
	int length;
};

class SPDBFilePath : public SPParserBase {
public:
	SPDBFilePath(char* path) :
		SPParserBase(path, "()") {
	}
	virtual ~SPDBFilePath() {
	}

	bool hasFileName() {
		return getLength() > 1;
	}

	string getDBFileName() {
		return getFractions()[0];
	}

	string getDataFileName() {
		return getLength() > 1 ? getFractions()[1] : "";
	}
};

class SPIndexFileSpec : public SPParserBase {
public:

	SPIndexFileSpec(char* fileSet) :
		SPParserBase(fileSet, ",") {
		indices = buildSub<SPDBFilePath>(getLength(), getFractions());
	}

	SPIndexFileSpec(const string& fileSet) :
		SPParserBase(fileSet, ",") {
		indices = buildSub<SPDBFilePath>(getLength(), getFractions());
	}

	virtual ~SPIndexFileSpec() {
	}

	SPDBFilePath** getFiles() {
		return indices;
	}

private:
	SPDBFilePath** indices;
};

class SPRangeSpec : public SPParserBase {
public:

	SPRangeSpec(char* fileSet) :
		SPParserBase(fileSet, ":") {
	}

	virtual ~SPRangeSpec() {
	}

	bool hasLimits() {
		return getLength() > 1;
	}

	int getValue() {
		return atoi(getFractions()[0]);
	}

	int getLowerLimit() {
		return atoi(getFractions()[0]);
	}

	int getUpperLimit() {
		return atoi(getFractions()[1]);
	}

	int getMultiple() {
		return getLength() > 2 ? atoi(getFractions()[2]) : 0;
	}

	void collect(ostream& o);
};

class SPValueSelection : public SPParserBase {
public:

	SPValueSelection(char* selection) :
		SPParserBase(selection, ",") {
		ranges = buildSub<SPRangeSpec>(getLength(), getFractions());
	}

	virtual ~SPValueSelection() {
	}

	SPRangeSpec** getRanges() {
		return ranges;
	}

	void collect(ostream& o);

private:
	SPRangeSpec** ranges;
};

class SPColumnSpec : public SPParserBase {
public:
	SPColumnSpec(char* spec);
	virtual ~SPColumnSpec() {
	}

	string getName() {
		return name;
	}

	char getSort() {
		return sort;
	}

	SPValueSelection* getSelection() {
		return selection;
	}

	void collect(ostream& o);
	void getPredicate(ostream& o);

private:
	char sort;
	char* name;
	SPValueSelection* selection;
};

class SPGroup : public SPParserBase {
public:

	SPGroup(char* selection) :
		SPParserBase(selection, "|") {
		columns = buildSub<SPColumnSpec>(getLength(), getFractions());
	}

	virtual ~SPGroup() {
	}

	SPColumnSpec** getColumns() {
		return columns;
	}

	void collect(ostream& o);
	string getWhere();
	string getOrders();

private:
	SPColumnSpec** columns;
};

class SPSelection : public SPParserBase {

public:
	SPSelection(char* selection) :
		SPParserBase(selection, "/") {
		groups = buildSub<SPGroup>(getLength(), getFractions());
	}

	SPSelection(const string& selection) :
		SPParserBase(selection, "/") {
		groups = buildSub<SPGroup>(getLength(), getFractions());
	}

	virtual ~SPSelection() {
	}

	SPGroup** getGroups() {
		return groups;
	}

	void collect(ostream& o);

private:
	SPGroup** groups;
};
}
#endif /*SPPARSERBASE_H_*/
