//============================================================================
// Name        : SPAccessor.cpp
// Author      : Lin Li, Aristool AG Switzerland
// Version     : 1.0, July 2008
// Copyright   : READ group Norway, all rights reserved
//============================================================================
#ifndef SPACCESSORS_H_
#define SPACCESSORS_H_

#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <typeinfo>
#include "SPBaseUtil.hh"

using namespace std;

namespace SP {

/**
 */
template<typename V> class SPMap : public map<string, V> {
	vector<string> names;
public:
	SPMap() :
		map<string, V>(), names() {
	}
	~SPMap() {
	}

	bool hasName(const string& name) {
		return this->count(name)> 0;
	}

	virtual vector<string>& getNames() {
		names.clear();

		typename map<string, V>::iterator i;
		for (i = this->begin(); i != this->end(); ++i) {
			names.push_back(i->first);
		}

		return names;
	}
};

class SPAbstractPicker {

public:
	SPAbstractPicker(int offset, int size) :
		offset(offset), size(size) {
	}

	virtual ~SPAbstractPicker() {
	}

	int getOffset() {
		return offset;
	}

	int getSize() {
		return size;
	}

	const void* getProperties() {
		return properties;
	}

	void setProperties(const void* p) {
		properties = p;
	}

	string& getString(void* area);

	virtual void setInt(int data, void* area) = 0;

	virtual void setDouble(double data, void* area) = 0;

	virtual int getInt(void* area) = 0;

	virtual double getDouble(void* area) = 0;

	virtual bool isInt() = 0;

	virtual SPAbstractPicker* duplicate(int offset = -1) = 0;

private:
	int offset;
	int size;
	const void *properties;
};

template<typename T> class SPPicker : public SPAbstractPicker {
public:
	SPPicker(int offset) :
		SPAbstractPicker(offset, sizeof(T)) {
	}

	virtual ~SPPicker() {
	}

	T& get(void* area) {
		return *(T*)(((char*)area) + getOffset());
	}

	void get(T& data, void* area) {
		data = *(T*)(((char*)area) + getOffset());
	}

	void set(T& data, void* area) {
		*(T*)(((char*)area) + getOffset()) = data;
	}

	void setInt(int data, void* area) {
		*(T*)(((char*)area) + getOffset()) = (T)data;
	}

	void setDouble(double data, void* area) {
		throw SPException("Can not assign double to int type");
	}

	int getInt(void* area) {
		return (int)*(T*)(((char*)area) + getOffset());
	}

	double getDouble(void* area) {
		throw SPException("Can not convert int type to double!");
	}

	bool isInt() {
		return typeid(float) != typeid(T) && typeid(double) != typeid(T) ;
	}

	SPAbstractPicker* duplicate(int os = -1) {
		if (os == -1) {
			os = getOffset();
		}
		return new SPPicker<T>(os);
	}
};

class SPPickerBox : public SPMap<SPAbstractPicker*> {
	string test;
public:
	SPPickerBox() :
		SPMap<SPAbstractPicker*>() {
	}
	~SPPickerBox() {
	}

	template<typename T> SPPicker<T>* pick(string name) {
		return (SPPicker<T>*)operator[](name);
	}

	template<typename T> T& get(string name, void* area) {
		return pick<T>(name)->get(area);
	}

	template<typename T> void get(T& data, string name, void* area) {
		pick<T>(name)->get(data, area);
	}

	template<typename T> void set(T& data, string name, void* area) {
		return pick<T>(name)->set(data, area);
	}
};

class SPCopyItem {
public:
	SPCopyItem(SPAbstractPicker& from, SPAbstractPicker& to) :
		from(from), to(to) {
	}

	~SPCopyItem() {
	}

	void run(void* fromBase, void* toBase);

private:
	SPAbstractPicker& from;
	SPAbstractPicker& to;
};

class SPCopyMachine {
public:
	SPCopyMachine() {
	}

	~SPCopyMachine() {
	}

	void addCopy(SPAbstractPicker& from, SPAbstractPicker& to);
	void run(void* fromBase, void* toBase);
private:
	vector<SPCopyItem*> items;
};
}

#endif /*SPACCESSORS_H_*/
