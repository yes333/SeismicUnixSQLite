//============================================================================
// Name        : SPBaseUtil.hh
// Author      : Lin Li, Aristool AG Switzerland
// Version     : 1.0, July 2008
// Copyright   : READ group Norway, all rights reserved
//============================================================================

#ifndef SPBASEUTIL_H_
#define SPBASEUTIL_H_

#include <string>
#include <cstdarg>
#include <sstream>
#include <iostream>

using namespace std;

namespace SP {

string getTimeString();

template<typename T> string cat(T s) {
	stringstream ss;
	ss << s;
	return ss.str();
}

template<typename T1, typename T2> string cat(T1 s1, T2 s2) {
	stringstream ss;
	ss << s1 << s2;
	return ss.str();
}

template<typename T1, typename T2, typename T3> string cat(T1 s1, T2 s2,
		T3 s3) {
	stringstream ss;
	ss << s1 << s2 << s3;
	return ss.str();
}

template<typename T1, typename T2, typename T3, typename T4> string cat(T1 s1,
		T2 s2, T3 s3, T4 s4) {
	stringstream ss;
	ss << s1 << s2 << s3 << s4;
	return ss.str();
}

template<typename T1, typename T2, typename T3, typename T4, typename T5> string cat(
		T1 s1, T2 s2, T3 s3, T4 s4, T5 s5) {
	stringstream ss;
	ss << s1 << s2 << s3 << s4 << s5;
	return ss.str();
}

template<typename T1, typename T2, typename T3, typename T4, typename T5,
		typename T6> string cat(T1 s1, T2 s2, T3 s3, T4 s4, T5 s5, T6 s6) {
	stringstream ss;
	ss << s1 << s2 << s3 << s4 << s5 << s6;
	return ss.str();
}

template<typename T1, typename T2, typename T3, typename T4, typename T5,
		typename T6, typename T7> string cat(T1 s1, T2 s2, T3 s3, T4 s4,
		T5 s5, T6 s6, T7 s7) {
	stringstream ss;
	ss << s1 << s2 << s3 << s4 << s5 << s6 << s7;
	return ss.str();
}

template<typename T> string catln(T s) {
	stringstream ss;
	ss << getTimeString() << ":  " << s << endl;
	return ss.str();
}

template<typename T1, typename T2> string catln(T1 s1, T2 s2) {
	stringstream ss;
	ss << getTimeString() << ":  " << s1 << s2 << endl;
	return ss.str();
}

template<typename T1, typename T2, typename T3> string catln(T1 s1, T2 s2,
		T3 s3) {
	stringstream ss;
	ss << getTimeString() << ":  " << s1 << s2 << s3 << endl;
	return ss.str();
}

template<typename T1, typename T2, typename T3, typename T4> string catln(
		T1 s1, T2 s2, T3 s3, T4 s4) {
	stringstream ss;
	ss << getTimeString() << ":  " << s1 << s2 << s3 << s4 << endl;
	return ss.str();
}

template<typename T1, typename T2, typename T3, typename T4, typename T5> string catln(
		T1 s1, T2 s2, T3 s3, T4 s4, T5 s5) {
	stringstream ss;
	ss << getTimeString() << ":  " << s1 << s2 << s3 << s4 << s5 << endl;
	return ss.str();
}

template<typename T1, typename T2, typename T3, typename T4, typename T5,
		typename T6> string catln(T1 s1, T2 s2, T3 s3, T4 s4, T5 s5, T6 s6) {
	stringstream ss;
	ss << getTimeString() << ":  " << s1 << s2 << s3 << s4 << s5 << s6 << endl;
	return ss.str();
}

template<typename T1, typename T2, typename T3, typename T4, typename T5,
		typename T6, typename T7> string catln(T1 s1, T2 s2, T3 s3, T4 s4,
		T5 s5, T6 s6, T7 s7) {
	stringstream ss;
	ss << getTimeString() << ":  " << s1 << s2 << s3 << s4 << s5 << s6 << s7
			<< endl;
	return ss.str();
}

/**
 * This is the general exception class for the framework. It should be
 * enough for most cases in a batch environment. 
 */

class SPException {
	/**
	 * The text description of the exception condition.
	 */
	string text;

public:
	/**
	 * The default constructor with unknown reason.
	 */
	SPException() :
		text("SPprocessor encountered an exception") {
	}

	/**
	 * The constructor with one reason element
	 */
	template<typename T> SPException(T s) :
		text(cat(s)) {
	}

	/**
	 * The constructor with two reason elements.
	 */
	template<typename T1, typename T2> SPException(T1 s1, T2 s2) :
		text(cat(s1, s2)) {
	}

	/**
	 * The constructor with three reason elements.
	 */
	template<typename T1, typename T2, typename T3> SPException(T1 s1, T2 s2,
			T3 s3) :
		text(cat(s1, s2, s3)) {
	}

	/**
	 * The constructor with four reason elements.
	 */
	template<typename T1, typename T2, typename T3, typename T4> SPException(
			T1 s1, T2 s2, T3 s3, T4 s4) :
		text(cat(s1, s2, s3, s4)) {
	}

	/**
	 * The constructor with five reason elements.
	 */
	template<typename T1, typename T2, typename T3, typename T4, typename T5> SPException(
			T1 s1, T2 s2, T3 s3, T4 s4, T5 s5) :
		text(cat(s1, s2, s3, s4, s5)) {
	}

	/**
	 * retrieve the reason
	 */
	string what(void) {
		return text;
	}
};

class SPVerbose {

public:
	static const int NO = 0;
	static const int ERROR = 1;
	static const int ESSENTIAL = 3;
	static const int DATA = 5;
	static const int EVERYTHING = 10;

public:
	static void setVerboseLevel(int level) {
		verboseLevel = level;
	}

	static int getVerboseLevel() {
		return verboseLevel;
	}

	static void show(int level, stringstream ss) {
		if (level <= verboseLevel) {
			string s = ss.str();
			s = catln(s);
			cerr << s;
		}
	}

	template<typename T> static void show(int level, T s) {
		if (level <= verboseLevel) {
			string ss = catln(s);
			cerr << ss;
		}
	}

	template<typename T1, typename T2> static void show(int level, T1 s1,
			T2 s2) {
		if (level <= verboseLevel) {
			string s = catln(s1, s2);
			cerr << s;
		}
	}

	template<typename T1, typename T2, typename T3> static void show(int level,
			T1 s1, T2 s2, T3 s3) {
		if (level <= verboseLevel) {
			string s = catln(s1, s2, s3);
			cerr << s;
		}
	}

	template<typename T1, typename T2, typename T3, typename T4> static void show(
			int level, T1 s1, T2 s2, T3 s3, T4 s4) {
		if (level <= verboseLevel) {
			string s = catln(s1, s2, s3, s4);
			cerr << s;
		}
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5> static void show(
			int level, T1 s1, T2 s2, T3 s3, T4 s4, T5 s5) {
		if (level <= verboseLevel) {
			string s = catln(s1, s2, s3, s4, s5);
			cerr << s;
		}
	}

private:
	static int verboseLevel;
};
}

#endif /*SPBASEUTIL_H_*/
