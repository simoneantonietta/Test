/*
 * PROJECT:  WUtilities
 *
 * FILENAME: Serializer.h
 *
 * PURPOSE: Serialize most of STL data structure to
 * save/load data to/from a file
 *
 * Each entry data has this format:
 * <S:name:n_elem:elem_size:type> (data) <E(:cksum)>
 * cksum may not be present. Cksum is calculated only on data
 * type specifies the data types stored
 * A file is a collection of these entries
 *
 * USAGE:
 * ---------
 * SAVE (to serialize and save a file in the more versatile fashion)
 * 1. initSave
 * for each data
 * 3. initBlock
 * 4. addDataToBlock or addDataToBlock_iter
 * restart from 3. for all data to store
 * 5. finalizeBlock
 * 6. finalizeBuffer
 * 7. saveFile
 * 8. closeOperation
 *
 * LOAD
 *
 * 1. initLoad
 * 2. loadFile
 * for each block of data:
 * 3. getNextBlock
 * 4. getBlockInfo (if need)
 * 5. retrieveBlockData or retrieveBlockData_iter
 * at the end
 * 6. closeOperation
 *
 * A correct use is to implement in each important structure
 * two members:
 *
 * void Serialize();
 * void Deserialize();
 *
 * to implement save/load blocks data
 *
 * LICENSE: please refer to LICENSE.TXT
 * CREATED: 11-giu-2009
 * AUTHOR:  Luca Mini
 */

#ifndef SERIALIZER_H_
#define SERIALIZER_H_

#include <queue>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <iterator>
#include <algorithm>

#include "Utils.h"
using namespace std;

#define SERIALIZER_VERSION 		"1.0"

#define SERIALIZE_DATATYPE_BASE				1
#define SERIALIZE_DATATYPE_VECTOR			2

class Serializer
{
public:
	Serializer()	{buffer=NULL;	block=NULL;this->hdrName="WOOD";};
	Serializer(string hdrName)	{buffer=NULL;	block=NULL;this->hdrName=hdrName;};
	virtual ~Serializer()
	{
	if(buffer!=NULL) delete buffer;
	if(block!=NULL) delete block;
	};

	typedef queue<char> qserializer_t;
	unsigned int blockCount;
	qserializer_t *block; // data buffer for a single block for serialization
	qserializer_t *buffer; // data buffer for serialization of the entire data file (collections of blocks)
	// store all information for a block
	struct blockInfo_st
	{
	string name;
	unsigned int nelem;
	size_t size;
	int type;
	} blockInfo;

	// store information readed from header of a loaded file
	struct bufferInfo_st
	{
	string name;
	string serializerVersion;
	string appName;
	string appVersion;
	string fileVersion;
	unsigned int nBytes;
	unsigned int blockCount;
	} bufferInfo;

	void setHdrName(string s) {hdrName=s;};
	void initSave(string appName,string appVersion,string fileVersion,string name);
	void initLoad();
	void finalizeBuffer();
	void initBlock(string blockName, unsigned int nelem, size_t size, int type);
	void closeOperation();

	/**
	 * add data to the block
	 * @param value data to add
	 */
	template<typename T>
	void addDataToBlock(const T value)
	{
	qserializer_t v = baseType2Queue(value);
	appendToQueue(v, *block);
	}

	/**
	 * add data to the block
	 * specialized function to store data which support iterators
	 * Note that the first data is the quantity of base data (i.e. element
	 * of vectors)
	 * Nested structure cannot be handle correctly by this routine.
	 * data must be recovered by retrieveBlockData_iter
	 * @param dat data structure (like vector or string)
	 *
	 */
	template<typename T>
	void addDataToBlock_iter(T &dat)
	{
	addDataToBlock(dat.size());
	for(typename T::iterator it=dat.begin(); it<dat.end();it++)
		{
		addDataToBlock(*it);
		}
	}

	void finalizeBlock();
	void appendBlock2Buffer();

	bool getNextBlock();
	bool getBlockInfo();

	/**
	 * retrieve some data of base types from a block.
	 * @param value 	value retrieved
	 */
	template <typename T>
	void retrieveBlockData(T &value)
	{
	T *data=&value;	// ptr to value
	char *d=(char*)data;	// transform to a char ptr

	for(unsigned int i=0;i<sizeof(T);i++)
		{
		if(!block->empty())
			{
			*(d+i)=block->front();
			block->pop();
			}
		}
	}

	/**
	 * retrieve some data of base types from a block.
	 * specialized function to store data which support iterators
	 * Data retrieved must be stored with addDataToBlock_iter
	 * Nested structure cannot be handle correctly by this routine.
	 * T is the structure
	 * E is the type of elements in the structure
	 *
	 * @param dat 	data striucture retrieved
	 */
	template <typename T, typename E>
	void retrieveBlockData_iter(T &dat)
	{
	size_t size;
	E elem;

	retrieveBlockData(size);
	dat.resize(size);
	typename T::iterator it=dat.begin();

	for(unsigned int i=0;i<size;i++)
		{
		retrieveBlockData(elem);
		*it=elem;
		it++;
		}
	}


	//----- FILE -----
	bool saveFile(string filename);
	bool loadFile(string filename);



private:
	string hdrName;
	/**
	 * convert a type data to a byte vector queue serializer_t
	 * @param value		value to convert
	 * @return vector of bytes
	 */
	template<typename T> qserializer_t baseType2Queue(const T value)
	{
	qserializer_t v;
	char *t = (char*) ((&value));
	for (unsigned int i = 0; i < sizeof(T); i++)
		{
		v.push(*(t + i));
		}
	return (v);
	}

	void initBuffer();
	void initBlock();

	void setupBufferHeader(string appName,string appVersion,string fileVersion,string name);
	void appendToQueue(qserializer_t &to_append, qserializer_t &dest);

};

#endif /* SERIALIZER_H_ */
