//============================================================================
// Name        : SPProcessor.hh
// Author      : Sanyu Ye,  SoftSeis,  Norway
// Version     : 1.1, Sept. 2020
// Copyright   : SoftSeis, Norway, all rights reserved
//============================================================================

#ifndef SPPROCESSOR_H_
#define SPPROCESSOR_H_

#include "SPAccessors.hh"
#include "SPBaseUtil.hh"
#include <vector>
#include <map>
#include <string>
#include <su.h>
#include <segy.h>
#undef open

using namespace std;

namespace SP {

/**
 * Wrapper class for segy structure of SU. Its main purpose is to provide named
 * access to all the header fields of an SU trace.
 *
 * It uses the memory from external. In most cases, this is enough because the
 * trace is used once and discarded. In case the traces are accumulated, two things
 * must happen: the memory should be allocated to hold the trace data for each
 * trace, and the allocated memory must be of the right size for the trace to avoid
 * excessive waste of memory. This is done here by a call to ::detach. From that
 * point, the allocated memory belong to the object and will be cleaned up properly
 * upon destruction.
 *
 * Note: SPProcessor automatically detects whether ::detach is necessary. So in most
 * cases, properly using SPProcessor class is enough for proper memory management.
 */
class SPSegy {
public:
	/** as defined in SU */
	const static int HEADERLENGTH = 240;

public:
	/**
	 * Constructor with the segy structure to be wrapped.
	 */
	SPSegy(segy* trace) :
		data(trace), myMemory(false) {
	}

	/**
	 * Copy constructor, causes the new one to own its memory.
	 */
	SPSegy(SPSegy& o);

	/**
	 * Destructor handles proper memory cleanup.
	 */
	~SPSegy() {
		if (myMemory)
			free(data);
	}

	/**
	 * Get the trace data wrapped.
	 */
	segy* getTrace() {
		return data;
	}

	/**
	 * Retrieve pointer to the trace data
	 */
	float* getValue() {
		return data->data;
	}

	/**
	 * retrieve named header data
	 */
	template<typename T> T& get(const string name) {
		return picker.get<T>(name, (void*)data);
	}

	/**
	 * Check if header field with the name exists
	 */
	bool hasValue(string name) {
		return picker.hasName(name);
	}

	/**
	 * Let the object own the memory.
	 */
	void takeMamory() {
		myMemory = true;
	}

	/**
	 * Get the value holder for the named field
	 */
	static SPPickerBox& getPicker() {
		return picker;
	}

	static vector<string>& getNames() {
		return names;
	}

	static void initAccessor();

private:
	/** the segy structure wrapped */
	segy* data;
	/** Whether the wrapped structure does not use externally assigned memory */
	bool myMemory;

	/** Static accessor for header fields */
	static SPPickerBox& picker;
	static vector<string>& names;
};

/**
 * This is the super class for standard SU modules implemented in C++. Each
 * module is a subclass of SPProcessor plus a very simple main() method.
 * The subclass overrides a few selected method to perform its tasks. The
 * purpose of this class is to relive the modules from the usual logistic
 * concerns so that they can concentrate on their task.
 */

class SPProcessor {
public:

	/**
	 * This is the main call to run the module. In most cases, the only line
	 * in the main method of the module is simply this one line:
	 *
	 * <tt>return (new [your class name]())->localMain(argc, argv);</tt>
	 *
	 * For very unconventional modules, it is it might want to write more
	 * or less the logics in this method in the main directly so as to
	 * bypass all the restrictions. This is possible but not recommended
	 * because the SP* classes are highly integrated. It is probably easier
	 * to completely discard the framework rather than using it for something
	 * that the framework is not built for.
	 */
	int localMain(int argc, char **argv);

	/**
	 * This method builds the command line parameter storage. If you write your
	 * own main method, you have to call this before it enters main loop.
	 */
	void initParam(int argc, char **argv);

	/**
	 * Fetch the next trace. If the module has to do unconventional things to
	 * get traces (like generating simulated traces), it can override this
	 * method. Each call should produce a single SPSegy wrapped trace.
	 */
	virtual SPSegy* fetchNext() {
		return fgettr(input, &tr) ? new SPSegy(&tr) : 0;
	}

	/**
	 * Writer a trace to the output and discard it. The module can also override
	 * this method to do unconventional things to the trace. Remember to discard
	 * the SPSegy after processing.
	 */
	virtual void dispatch(SPSegy* data) {
		if (output != 0) {
			fputtr(output, data->getTrace());
		}
	}

	/**
	 * Delete the object and signal to SPProcessor that this trace is disposed
	 * of already. It is important to call this method for discarding the trace
	 * wrapper rather than delete it directly. The framework need to know its
	 * objects to do the house keeping properly.
	 */
	virtual void discard(SPSegy* data) {
		delete data;
	}

	/**
	 * This call makes the main loop to stop reading the next trace and stop
	 * processing completely. (Goes on to ::cleanup as if the input is finished).
	 */
	void stop() {
		stopProcessing = true;
	}

	/**
	 * If the traces are not from standard in of the process, this method can
	 * make the module get the traces from a different source. However, it is
	 * important that the file produces the trace in a SU standard stream.
	 */
	void setinput(FILE* f) {
		input = f;
	}

	/**
	 * Redirect the output stream to a different destination. If f == 0, the
	 * output traces will be discarded.
	 */
	void setoutput(FILE* f) {
		output = f;
	}

	/**
	 * Whether the command line has a parameter with this name
	 */
	bool hasParameter(string name) {
		return params.hasName(name);
	}

	/**
	 * List names of all parameters in the command line. Repeated names are overriden
	 * (last value is effective).
	 */
	vector<string> getParameterNames() {
		return params.getNames();
	}

	/**
	 * Get the value of the parameter with name as a string
	 */
	string getStringParameter(string name) {
		return params[name];
	}

	/**
	 * Get the value of the parameter with name as a string. If the parameter is
	 * not in the commandline, the defaultValue is returned,
	 */
	string getStringParameter(string name, string defaultValue);

	/**
	 * Get the value of the parameter with name as an int. The parsing is down with
	 * simple atoi() so this method never failes. If the parameter with the name
	 * does not exist, the default value is returned.
	 */
	int getIntParameter(string name, int defValue);

	/**
	 * Get the value of the parameter with name as a double. The parsing is down with
	 * simple atof() so this method never failes. If the parameter with the name
	 * does not exist, the default value is returned.
	 */
	double getDoubleParameter(string name, double defValue);

	/**
	 * Get the value of the parameter with name as a boolean. The result is true when
	 * the value part starts with '1' (as in 1=true; 0=false) or 'y'/'Y' for yes, or
	 * 't'/'T' for true.  If the parameter with the name does not exist, the default
	 * value is returned.
	 */
	bool getBooleanParameter(string name, bool defValue);

	/**
	 * Get the shell command for this module (argv[0])
	 */
	string& getCommand() {
		return command;
	}

	/**
	 * This is to be overriden by the subclasses. It is called after the
	 * command line is processed and before the first trace is read in.
	 */
	virtual void init() {
	}

	/**
	 * This is to be overriden by the subclasses. It is called once for
	 * every trace read in. If the method finishes dealing with the segy
	 * record, it should call either ::dispatch or ::discard to free up
	 * the record. Otherwise, it is very important that one of them is
	 * called for each of the record at some point later. Otherwise massive
	 * memory leak is the result.
	 */
	virtual void process(SPSegy* data) {
		dispatch(data);
	}

	/**
	 * This is to be overriden by the subclasses. It is called when the
	 * input stream failed to deliver and more trace and the module is
	 * about to shutdown.
	 */
	virtual void cleanup() {
	}

private:

	/** the tempporary buffer to hold data read from the input */
	segy tr;

	/** the input channel for traces, usually stdin */
	FILE* input;
	/** the output channel for traces, usually stdout */
	FILE* output;

	/** name of the command calling this module */
	string command;

	/** whether the processing should stop here */
	bool stopProcessing;

	/** digested command line parameters */
	SPMap<string> params;
};
}

#endif /*SPPROCESSOR_H_*/
