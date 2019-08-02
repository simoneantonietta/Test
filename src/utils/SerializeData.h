/**----------------------------------------------------------------------------
 * PROJECT: woodbusclient
 * PURPOSE:
 * Serialize in a buffer the data, useful to send to a stream
 *-----------------------------------------------------------------------------  
 * CREATION: 11 Oct 2013
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

#ifndef SERIALIZEDATA_H_
#define SERIALIZEDATA_H_

#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/stat.h>

#define SERIALIZEDATA_DEFAULTBUFSIZE	256

using namespace std;

class SerializeData
{
public:

	/**
	 * ctor
	 */
	SerializeData()
	{
	_allocate(SERIALIZEDATA_DEFAULTBUFSIZE);
	ndata=0;
	}

	/**
	 * ctor
	 * @param bufferSize
	 */
	SerializeData(int bufferSize)
	{
	_allocate(bufferSize);
	ndata=0;
	}

	/**
	 * dtor
	 */
	~SerializeData()
	{
	delete buffer;
	}

	/**
	 * concatenate data in the buffer
	 * @param data data to serialize
	 * @param size size of data
	 * @return
	 */
	bool push(void *data, int size)
	{
	if((ndata+size)<=bufferSize)
		{
		memcpy(buffer+ndata,data,size);
		ndata+=size;
		}
	else
		{
		cerr << "ERROR: buffer overrun of " << ndata+size-bufferSize<< endl;
		return false;
		}
	return true;
	}

	/**
	 * clear all data
	 */
	void clear()
	{
	ndata=0;
	}

	/**
	 * set a new size of an already allocated serializer buffer.
	 * NOTE: all data will be lost!
	 * @param size new size
	 */
	void reallocate(int newsize)
	{
	if(bufferSize==newsize)
		{
		clear();
		}
	else
		{
		delete [] buffer;
		_allocate(newsize);
		ndata=0;
		}
	}

	/**
	 * resize an already allocated serializer buffer.
	 * NOTE: all data will be preserved, if new size >= old size, else will be truncated
	 * @param size new size
	 */
	void resize(int newsize)
	{
	if(bufferSize==newsize)
		{
		return;
		}
	else
		{
		char *tmp=new char[newsize];
		int mov=min(this->bufferSize,newsize);
		memcpy(tmp,buffer,mov);
		buffer=tmp;
		delete [] buffer;
		ndata=0;
		}
	}

	/**
	 * get the data stored
	 * @return bytes serialized
	 */
	int getNdata()
	{
	return ndata;
	}

	// buffer exposed :)
	char *buffer;


	/**
	 * save data to file
	 * @param fname
	 * @return true:ok
	 */
	bool writeFile(string fname)
	{
	bool ret=true;
	ofstream file(fname.c_str(), ios::out | ios::binary);
	if(file.good())
		{
		file.write (buffer, ndata);
  	cerr << "file write error: " << fname << endl;
		ret=false;
		}
  file.close();
  return ret;
	}

	/**
	 * load data from file
	 * reallocates the buffer to read all the file data
	 * @param fname
	 * @return true:ok
	 */
	bool readFile(string fname)
	{
	bool ret=false;
	struct stat results;

  if (stat(fname.c_str(), &results) == 0)
  	{
  	reallocate(results.st_size);
  	}
  else
  	{
    // An error occurred
  	cerr << "file read error: " << fname << endl;
  	return false;
  	}

  ifstream file(fname.c_str(), ios::in | ios::binary);
	if(file.good())
		{
		file.read (buffer, ndata);
		if(file)	// no error
			{
			ret=true;
			}
		else	// error
			{
			ret=false;
	  	cerr << "file read error: " << fname << endl;
			}
		}
	file.close();
  return ret;
	}

private:
	int ndata;
	int bufferSize;

	/**
	 * allocates memory
	 * @param size
	 */
	void _allocate(int size)
	{
	this->bufferSize=size;
	buffer=new char[this->bufferSize];
	}
};


//-----------------------------------------------
#endif /* SERIALIZEDATA_H_ */
