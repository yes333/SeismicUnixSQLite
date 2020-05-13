//============================================================================
// Name        : SPProcessor.cpp
// Author      : Lin Li, Aristool AG Switzerland
// Version     : 1.0, July 2008
// Copyright   : READ group Norway, all rights reserved
//============================================================================

#include <iostream>
#include <hdr.h>
#include <header.h>
#include "SPProcessor.hh"
#include "SPBaseUtil.hh"

using namespace std;
using namespace SP;

SPPickerBox& SPSegy::picker = *(new SPPickerBox());
vector<string>& SPSegy::names = *(new vector<string>());
bool initiated = false;

void SPSegy::initAccessor() {
	names.clear();
	picker.clear();
	for (int i = 0; i < SU_NKEYS; ++i) {
		switch (hdr[i].type[0]) {
		case 'i':
			picker[hdr[i].key] = new SPPicker<int>(hdr[i].offs);
			break;
		case 'h':
			picker[hdr[i].key] = new SPPicker<short>(hdr[i].offs);
			break;
		case 'u':
			picker[hdr[i].key] = new SPPicker<unsigned short>(hdr[i].offs);
			break;
		case 'f':
			picker[hdr[i].key] = new SPPicker<float>(hdr[i].offs);
			break;
		}
		names.push_back(hdr[i].key);
	}
}

SPSegy::SPSegy(SPSegy& o) :
	myMemory(true) {

	int length = ((o.data->ns + 1) * sizeof(float) + HEADERLENGTH - 1)/4;
	long* x = new long[length];
	long* y = (long*)o.data;
	for (register int i = 0; i < length; ++i) {
		x[i] = y[i];
	}
	data = (segy*)x;
}

/**
 *
 */
int SPProcessor::localMain(int argc, char **argv) {
	try {
		input = stdin;
		output = stdout;
		stopProcessing = false;

        /* Initialize */
        initargs(argc, argv) ;
        requestdoc(1) ;

		initParam(argc, argv);
		SPSegy::initAccessor();

		init();
		while(SPSegy* d = stopProcessing ? 0 : fetchNext() ) {
			process(d);
		}

		cleanup();
		return 0;
	} catch (SPException e) {
		cerr << e.what() << endl;
		return 1;
	} catch(...) {
		cerr << "Unexpected exception from application" << endl;
	}
	return 2;
}

void SPProcessor::initParam(int argc, char **argv) {
	command = argv[0];
	for (int i = 1; i < argc; ++i) {
		char* b = argv[i];
		while(*b != '=' && *b != 0){
			++b;
		}
		if(*b == '=') {
			*b = 0;
			++b;
		}
		params[argv[i]] = b;
	}
}

string SPProcessor::getStringParameter(string name, string defValue) {
	if(params.hasName(name)) {
		return params[name];
	}
	return defValue;
}

int SPProcessor::getIntParameter(string name, int defValue) {
	if(params.hasName(name)) {
		return atoi(params[name].c_str());
	}
	return defValue;
}

double SPProcessor::getDoubleParameter(string name, double defValue) {
	if(params.hasName(name)) {
		return (double)atof(params[name].c_str());
	}
	return defValue;
}

bool SPProcessor::getBooleanParameter(string name, bool defValue) {
	if(params.hasName(name)) {
		char c = params[name].c_str()[0];
		return c == '1' || c == 'T' || c == 't' || c == 'Y' || c == 'y';
	}
	return defValue;
}
