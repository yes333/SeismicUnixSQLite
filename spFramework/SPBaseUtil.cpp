//============================================================================
// Name        : SPBaseUtil.cpp
// Author      : Lin Li, Aristool AG Switzerland
// Version     : 1.0, July 2008
// Copyright   : READ group Norway, all rights reserved
//============================================================================
#include "SPBaseUtil.hh"

using namespace std;
using namespace SP;

int SPVerbose::verboseLevel = 0;

string SP::getTimeString() {
	time_t t;
	time(&t);
	tm& lt = *localtime(&t);

	stringstream ss;
	ss << lt.tm_mday << "/" << lt.tm_mon + 1 << "/" << (lt.tm_year + 1900) << " ";
	ss << lt.tm_hour << ":" << lt.tm_min << ":" << lt.tm_sec;

	return ss.str();
}
