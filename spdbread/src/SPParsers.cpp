//============================================================================
// Name        : SPParsers.cpp
// Author      : Lin Li, Aristool AG Switzerland
// Version     : 1.0, Aug. 2008
// Copyright   : READ group Norway, all rights reserved
//============================================================================
#include "SPParsers.hh"
#include <sstream>
#include <SPTable.hh>

using namespace SP;
using namespace std;

void SPParserBase::init(char* data, const char* separators) {
	if (separators[0] == 0) {
		fractions = new char*[1];
		fractions[0] = data;
		length = 1;
		return;
	}

	const char* ns = separators;
	bool more = separators[1] != 0;

	char* temp[1000];
	char* d = data;
	temp[0] = data;
	length = 1;
	while (*d != 0) {
		if ( *ns == 0 || *d != *ns) {
			++d;
			continue;
		}
		*d = 0;
		temp[length] = d + 1;
		++length;
		if (length > 999) {
			throw SPException("Too many elements for ", separators);
		}
		if (more) {
			++ns;
		}
		++d;
	}

	fractions = new char*[length];
	for (int i = 0; i < length; ++i) {
		fractions[i] = temp[i];
	}
}

void SPParserBase::init(const string& s, const char* separators) {
	int l = s.size();
	char* d = new char[l + 1];
	strcpy(d, s.c_str());
	init(d, separators);
}

void SPValueSelection::collect(ostream& o) {
	for (int i = 0; i < getLength(); ++i) {
		if (i > 0) {
			o << ",";
		}
		ranges[i]->collect(o);
	}
}

void SPRangeSpec::collect(ostream& o) {
	for (int i = 0; i < getLength(); ++i) {
		if (i > 0) {
			o << ":";
		}
		o << getFractions()[i];
	}
}

SPColumnSpec::SPColumnSpec(char* spec) :
	SPParserBase(spec, "()") {
	name = getFractions()[0];
	int n = strlen(name);
	char c = name[n - 1];
	if (c == '+' || c == '-') {
		sort = c;
		name[n - 1] = 0;
	} else {
		sort = ' ';
	}
	if (getLength() > 1 && *getFractions()[1] != 0) {
		selection = new SPValueSelection(getFractions()[1]);
	} else {
		selection = 0;
	}
}

void SPColumnSpec::collect(ostream& o) {
	o << name;
	if (sort != ' ') {
		o << sort;
	}
	if (selection != 0) {
		o << "(";
		selection->collect(o);
		o << ")";
	}
}

void SPColumnSpec::getPredicate(ostream& o) {
	if (selection == 0) {
		o << "1 = 1";
		return;
	}
	SPRangeSpec** r = selection->getRanges();
	int n = selection->getLength();
	bool has = false;
	for (int i = 0; i < n; ++i) {
		if (has) {
			o << " OR ";
		} else {
			has = true;
		}
		if (n > 1) {
			o << "(";
		}
		o << name;
		if (r[i]->hasLimits()) {
			o << " between " << r[i]->getLowerLimit() << " AND "
					<< r[i]->getUpperLimit();

			if (r[i]->getMultiple() > 0) {
				o << " AND (" << name << " - ((" << name << " - "
						<< r[i]->getLowerLimit() << ")/" << r[i]->getMultiple()
						<< ") * " << r[i]->getMultiple() << ") = "
						<< r[i]->getLowerLimit();
			}
		} else {
			o << " = " << r[i]->getValue();
		}
		if (n > 1) {
			o << ")";
		}
	}
}

void SPGroup::collect(ostream& o) {
	for (int i = 0; i < getLength(); ++i) {
		if (i > 0) {
			o << "!";
		}
		columns[i]->collect(o);
	}
}

string SPGroup::getWhere() {
	stringstream ss;
	int n = getLength();
	SPColumnSpec** s = getColumns();
	bool has = false;
	for (int i = 0; i < n; ++i) {
		if (s[i]->getSelection() == 0) {
			continue;
		}
		if (has) {
			ss << " AND ";
		} else {
			has = true;
		}
		ss << "(";
		s[i]->getPredicate(ss);
		ss << ")";
	}
	return ss.str();
}

string SPGroup::getOrders() {
	stringstream ss;
	int n = getLength();
	SPColumnSpec** s = getColumns();
	for (int i = 0; i < n; ++i) {
		char c = s[i]->getSort();
		if (c == ' ') {
			continue;
		}
		ss << s[i]->getName() << (c == '-' ? " DESC" : " ASC") << ", ";
	}
	return ss.str();
}

void SPSelection::collect(ostream& o) {
	for (int i = 0; i < getLength(); ++i) {
		if (i > 0) {
			o << "/";
		}
		groups[i]->collect(o);
	}
}
