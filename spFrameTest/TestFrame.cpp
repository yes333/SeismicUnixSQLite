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

/**
 * This is a test class to show how a normal SU module looks like.
 */
class Testing : public SPProcessor {
public:
	/**
	 * This method is called at the start up of the module. It allows the
	 * module to do the one time setup work before the traces start comming.
	 */
	void init();

	/**
	 * This method is called for every trace from the input. It should call
	 * ::discard or ::dispatch when the SPSegy is not needed afterwards.
	 */
	void process(SPSegy* data);

	/**
	 * This is called before the module exits. It allows the module to cleanup
	 * or conclude the job.
	 */
	void cleanup();

private:
	/** local variable example: how many traces so far */
	int counter;
	/** local variable example: some intermediate result */
	long long sigma;
	/** local variable example: maximum number of traces to process */
	int max;

	SPPicker<int>* testAccessor;
};

void Testing::init() {
	// how did they call me?
	cerr << "Command: " << getCommand() << endl;

	// the parameter "input" sets the traces source from a file, similar to <
	if (hasParameter("input")) {
		FILE* f= fopen(getStringParameter("input").c_str(), "r");
		setinput(f);
	}

	// the parameter "output" redirects the output, similar to >
	if (hasParameter("output")) {
		FILE* f= fopen(getStringParameter("output").c_str(), "w");
		setoutput(f);
	}

	// max can be set on the command line so that the module does not have to go through all traces
	max = getIntParameter("max", 0);

	testAccessor = SPSegy::getPicker().pick<int>("offset");

	// initializing the rest of local variables.
	counter = 0;
	this->sigma = 0;
}

void Testing::process(SPSegy* data) {

	// this is the way to access header field of a trace.
	// int v = data->get<int>("offset");
	int v = testAccessor->get((void*)data->getTrace());

	// In named access is not working properly. This is also the prefered way to stop the module
	// violently due to some error conditions.
	if (v != data->getTrace()->offset) {
		throw SPException("Bad named value ", v, " at ", data->getTrace()->offset);
	}
	this->sigma += v;
	counter++;

	// This is very important !! Otherwise the module keeps the memory to the end.
	dispatch(data);

	// stop() is the way to gently stop the module.
	if (max > 0 && counter > max) {
		stop();
	}
}

void Testing::cleanup() {
	cerr << "cleaning up" << endl;
	if (counter == 0) {
		cerr << "No trace read. Nothing to report" << endl;
		return;
	}
	cerr << "number of traces through: " << counter << endl;
	cerr << "total offsets: " << sigma << endl;
	cerr << "average offset: " << sigma/counter << endl;
} 

// make SU doc happy
char* sdoc[] = { (char*)"Example for SP framework in C++", 0 };

int main(int argc, char **argv) {
	return (new Testing())->localMain(argc, argv);
}
