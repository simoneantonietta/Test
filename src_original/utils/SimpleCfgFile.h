/*
 * PROJECT:
 *
 * FILENAME: simpleCfgFile.h
 * 
 * PURPOSE:
 * simple class to handle very simple config files
 * format:
 *
 * [varname] = [value] #comment
 *
 * to use sections, before write all parameters without sections, and then
 * write section parameters
 *
 * <varname1> = <value> #comment
 * [section1]
 * <varname2> = <value> #comment
 * <varname3> = <value> #comment
 *
 * varname2 and varname3 are under section1. Sections are defined between []
 * NOTE: section cannot have comment in line
 *
 * so to refer varname2 or varname3 you must refer it in this mode:
 * "section1.varname2"
 *
 * each var is separed from section by a dot.
 *
 * TYPE:
 * you can define a type that will be checked
 * @typename = list|alist_i|alist_f|range_i|range_f|(values)
 *
 * and the parameters sepcifies its type between () as the following:
 * parm = (typename) value
 *
 * ARRAY:
 * You can define an array of values:
 * @ARRAYBEGIN name
 *
 * values...
 *
 * @ARRAYEND
 *
 * and they are recalled as nameNumber
 * where Number is sequential integer number from 1 and is positional.
 * There is an additional parameter for each array called _ARRAY_SIZE_name that
 * give the number of elements in the array
 *
 *
 * an example:
# prova config files
#
#----------------------------------------
#@_title = CFG di prova
#@_version = 1.0

#@lista1 = list(item1,item2,item3)
#@lista2 = alist_i(-10, 10, 1)
#@lista3 = alist_f(-20.0, 50.5, 10.0)
#@range1 = range_i(10, 100)
#@range2 = range_f(10.0, 100.0)
#----------------------------------------
@include prova_inc.cfg

param1 = (lista1) item2
param2 = prova1
param3 = prova2 # commento in linea

[section1]
par5 = (range1) 20 # commento di prova
par6  = (range2) 15.56
par7 = (range2) 99.9
par8 = (lista2) 9
par9 = (lista3) -10.0

[section2]
par10 = 100
par11 = prove congiunte # commento nuovo
 *
 * LICENSE: please refer to LICENSE.TXT
 * CREATED: 22/dic/2009
 * HISTORY:
 * 					18/feb/2010		added save capabilities
 * 					22/feb/2010		added feature to trimming strings
 * 					10/gen/2011		added type and rules
 * 					13/apr/2012		check types; array handling
 * 					13/mar/2013		added direct conversion functions for bool, int, float and double
 * 					05/dec/2013		added array element updates and some utilities
 * AUTHOR:  Luca Mini
 */

#ifndef SIMPLECFGFILE_H_
#define SIMPLECFGFILE_H_

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

#include "Utils.h"


using namespace std;

class SimpleCfgFile
{
private:
#define MAX_INC_DEPTH_LEVEL		 				4
	//-------------------------------
	typedef struct entry_st
	{
	bool isComment;		// true if a comment line
	bool inlineComment;	// true if a couple var=val contain a comment
	bool hasType;	// contain a type
	bool isType;	// is type definition
	bool valueFromFile;	// if the value is in a file
	bool isBinaryFile;
	bool isArrayStart;
	bool isArrayEnd;
	bool isArrayEntry;
	bool isArraySize;
	bool hasSection;
	bool isSection;
	bool isEmptyLine;
	string varname;
	string value;		// contain value for vars or section name
	string comment;
	string type;

	// data for values contained in a file
	string filename;
	string txtFileContent;
	char * binFileContent;	// ptr to destination var
	long int binFileSize;		// bytes readed

	// ctor
	entry_st()
	{
	isComment=false;
	inlineComment=false;
	hasType=false;
	isType=false;
	valueFromFile=false;
	isBinaryFile=false;
	isArrayStart=false;
	isArrayEnd=false;
	isArrayEntry=false;
	isArraySize=false;
	hasSection=false;
	isSection=false;
	isEmptyLine=false;
	varname.clear();
	value.clear();
	comment.clear();
	type.clear();
	filename.clear();
	txtFileContent.clear();
	binFileContent=(char*)NULL;
	binFileSize=0;
	}

	// for search algorithm
	bool operator ==(const string &vname) const
		{
		// check if the search is for name and value
		if((*this).varname==vname)
			return(true);
		else
			return(false);
		}
	}entry_t;
	//-------------------------------

	string filename;
	int nline[MAX_INC_DEPTH_LEVEL];
	bool hasSection;
	string actualSection;
	vector<entry_t> entries;
	vector<string> sections;
	struct SectionPos_st
	{
		string section;
		int startPos;
		int endPos;

		bool operator ==(const string &sec) const
		{
		if(this->section==sec) return true;
		return false;
		}
	}; // positions of each section (updated by the findSectionsPos)

	bool hasInclude;
	string incFile;
	int depthLevel;	// to handle include files
	int autonumber;
	string arrayVar;
	bool isArrayEntry;

	bool parseLine(string l);

	bool loadValueFromFile(char type, entry_t &elem);
	string getRuleOf(string varname);
	string getVarname(string varname);
	string getSection(string varname);

	int getArrayStartPos(string arrayName);
	int getArrayEndPos(string arrayName);
	int incdecArraySize(string ar,int incdec);
	int getVarPosition(string varname);
	bool insertNewVar(entry_t etmp);
	void findSectionsPos(vector<SectionPos_st> &pos);

	bool checkRule(string value, string rule);
	bool checkValue_list(string value, vector<string> rule);
	bool checkValue_alist_i(string value, vector<string> rule);
	bool checkValue_alist_f(string value, vector<string> rule);
	bool checkValue_range_i(string value, vector<string> rule);
	bool checkValue_range_f(string value, vector<string> rule);

public:
	SimpleCfgFile();
	virtual ~SimpleCfgFile();

	bool loadCfgFile(string filename);
	bool saveCfgFile(string filename);
	bool saveCfgFile();
	bool updateVariable(string varname, string value, string inlineComment="" );
	bool updateVariable(string varname, unsigned char* value, int len, string inlineComment="");
	bool addArrayElement(string arrayName,string value, string inlineComment="");
	bool removeAllArrayElements(string arrayName, bool removeArray=false);

	string getValueOf(string varname);
	string getValueOf(string varname, string defValue);
	int getValueOf(string varname, unsigned char *val, int len);
	int getValue_int(string varname, string defValue="");
	float getValue_float(string varname, string defValue="");
	double getValue_double(string varname, string defValue="");
	bool getValue_bool(string varname, string defValue="");


	vector<string> getAllSections();
	bool hasSections();
	bool hasParameter(string varname);
	int getArraySize(string ar);

	bool hasType(string varname);
	bool isType(string varname);
	bool checkValue(string varname);
	void printDatabaseInfo();

	string getTitle();
	string getVersion();
	string getFiletype();

};

#endif /* SIMPLECFGFILE_H_ */
