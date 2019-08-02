/**----------------------------------------------------------------------------
 * PROJECT: general
 * PURPOSE:
 * permits save and load of tagged files
 *-----------------------------------------------------------------------------  
 * CREATION: Jul 3, 2013
 * Author: Luca Mini
 * 
 * LICENCE: please see LICENCE.TXT file
 * 
 * HISTORY (of the module):
 *-----------------------------------------------------------------------------
 * Author              | Date        | Description
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */

#ifndef TAGGEDTXTFILES_H_
#define TAGGEDTXTFILES_H_

#include <string>
#include <vector>
#include <map>
#include "Utils.h"

#define TAGGEDFILE_DEFAULT_SEPARATOR	','
#define TAGGEDFILE_TAG_BEGIN					'<'
#define TAGGEDFILE_TAG_END						'>'
#define TAGGEDFILE_PARAM_SEP					':'
#define TAGGEDFILE_CR									'\n'
#define TAGGEDFILE_NO_LINEBREAK				-1


using namespace std;


/*
 * main class
 */
class TaggedTxtFile
{
public:
	enum dataTypes_e
	{
	tf_string=1,
	tf_integer=2,
	tf_float=3,
	tf_double=4,
	tf_hex=5,
	};

	TaggedTxtFile();
	TaggedTxtFile(string filename);
	virtual ~TaggedTxtFile();

	void setFilename(string filename);
	void addField(string tag,dataTypes_e typ,vector<string> *data,int linebreak);
	void addField(string tag,dataTypes_e typ,string value);
	void addField(string tag,unsigned char* data,size_t size,int linebreak=32);
	void setSeparator(char sep);
	bool load();
	bool save();
	void clear();

	/**
	 * get the data in string format
	 * @param tag data tag name
	 * @param nfld field (default=0)
	 * @return string data
	 */
	string getData(string tag,int nfld=0)
	{
	return fileStructure[tag].data->at(nfld);
	}

	/**
	 * get data in numeric format (if compatible)
	 * @param tag tag name
	 * @param nfld fied (default=0)
	 * @return numeric data
	 */
	template <typename T>
	T getNumber(string tag, int nfld=0)
	{
	return to_number<T>(fileStructure[tag].data->at(nfld));
	}

	/**
	 * get the array of all tag data
	 * @param tag tag name
	 * @return vector of strings
	 */
	vector<string> *getAllData(string tag)
	{
	return fileStructure[tag].data;
	}

	/**
	 * get hex data in binary form
	 * @param tag tag name
	 * @param data destination data
	 * @param size size of the data destination
	 */
	void getHexData(string tag, unsigned char *data, size_t size)
	{
	AsciiHex2Hex(data,fileStructure[tag].dataHex,size);
	}

	/**
	 * get the type of the tag
	 * @param tag tag name
	 * @return type
	 */
	dataTypes_e getType(string tag)
	{
	return fileStructure[tag].type;
	}

	/**
	 * get the number of fields available in the tag specified
	 * NOTE if the type is hex, return the bytes, not the hex string length
	 * @param tag tag name
	 * @return name of fields available
	 */
	int getNData(string tag)
	{
	if(fileStructure[tag].type==tf_hex)
		{
		return strlen(fileStructure[tag].dataHex)/2;
		}
	else
		{
		return fileStructure[tag].data->size();
		}

	}


private:
	bool loaded;
	typedef struct dataElem_st
	{
	string tag;
	dataTypes_e type;
	int linebreak;
	vector<string> *data;	// used for multivalue field
	char *dataHex;		// used for binary/hex
	string value;			// used for single values

	dataElem_st()
		{
		type=tf_string;
		linebreak=TAGGEDFILE_NO_LINEBREAK;
		dataHex=(char*)NULL;
		}
	} dataElem_t;

	string filename;
	char separator;
	map<string,dataElem_t> fileStructure;

	string readTaggedData(ifstream &f,string *data, dataTypes_e *typ, int *lb);
};

//-----------------------------------------------
#endif /* TAGGEDTXTFILES_H_ */
