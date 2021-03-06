//============================================================================
// Name        : SPAccessors.cpp
// Author      : Sanyu Ye,  SoftSeis,  Norway
// Version     : 1.1, Sept. 2020
// Copyright   : SoftSeis, Norway, all rights reserved
//============================================================================
#include "SPAccessors.hh"

using namespace std;
using namespace SP;

namespace SP {

template<> void SPPicker<double>::setInt(int data, void* area) {
	throw SPException("Can not assign int type to double");
}

template<> void SPPicker<double>::setDouble(double data, void* area) {
	*(double*)(((char*)area) + getOffset()) = data;
}

template<> int SPPicker<double>::getInt(void* area) {
	throw SPException("Can not convert double type to int");
}

template<> double SPPicker<double>::getDouble(void* area) {
	return *(double*)(((char*)area) + getOffset());
}

template<> void SPPicker<float>::setInt(int data, void* area) {
	throw SPException("Can not assign int type to float");
}

template<> void SPPicker<float>::setDouble(double data, void* area) {
	*(float*)(((char*)area) + getOffset()) = (float)data;
}

template<> int SPPicker<float>::getInt(void* area) {
	throw SPException("Can not convert int type to float");
}

template<> double SPPicker<float>::getDouble(void* area) {
	return (double)*(float*)(((char*)area) + getOffset());
}

template<> int SPPicker<int>::getInt(void* area) {
	return (int)*(int*)(((char*)area) + getOffset());
}

template<> int SPPicker<short>::getInt(void* area) {
	return (int)*(short*)(((char*)area) + getOffset());
}

template<> int SPPicker<unsigned short>::getInt(void* area) {
	return (int)*(unsigned short*)(((char*)area) + getOffset());
}

/** dosen't work, tested by Sanyu
template<> double SPPicker<int>::getDouble(void* area) {
	int v = *(int*)(((char*)area) + getOffset());
	return static_cast<double> (v);
} */

string& SPAbstractPicker::getString(void* area) {
	static string buf;
	stringstream s;
	if (isInt()) {
		s << getInt(area);
	} else {
		s << getDouble(area);
	}
	buf.clear();
	buf.append(s.str());
	return buf;
}

void SPCopyItem::run(void* fromBase, void* toBase) {
	if (from.isInt()) {
		int i = from.getInt(fromBase);
		if (to.isInt()) {
			to.setInt(i, toBase);
		} else {
			to.setDouble((double)i, toBase);
		}
	} else {
		double d = from.getDouble(fromBase);
		if (to.isInt()) {
			to.setInt((int)d, toBase);
		} else {
			to.setDouble(d, toBase);
		}
	}
}

void SPCopyMachine::addCopy(SPAbstractPicker& from, SPAbstractPicker& to) {
	if (from.getSize() != to.getSize()) {
		throw SPException("Incompatible copy size: ", from.getSize(), " for ", to.getSize());
	}
	items.push_back(new SPCopyItem(from, to));
}

void SPCopyMachine::run(void* fromBase, void* toBase) {
	for (unsigned int i = 0; i < items.size(); ++i) {
		items[i]->run(fromBase, toBase);
	}
}

}
