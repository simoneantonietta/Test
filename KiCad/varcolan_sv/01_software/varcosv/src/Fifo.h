/**----------------------------------------------------------------------------
 * PROJECT: gpsnif
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 16 Mar 2016
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

#ifndef COMM_FIFO_H_
#define COMM_FIFO_H_

#include <string>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define FIFO_BUFFER_SIZE		512

using namespace std;

class Fifo
{
public:
	Fifo() {fifo=0;};
	virtual ~Fifo() {};

	/**
	 * open and creates the fifo for sniffing
	 * @param fifoName
	 * @param mode 'w' write, 'r' read
	 * @param create specifies if have to be created
	 * @return true fifo ok; false: error
	 */
	bool openFifo(string fifoName, char mode, bool create)
	{
	// delete if it exists
	this->fifoname=fifoName;
	try
		{
		if(create)
			{
			unlink(fifoName.c_str());
			fifo=mkfifo (fifoName.c_str(),0666);
			if(fifo<0)
				{
				throw false;
				}
			}

		if(mode=='w')	// write
			{
			writeEnabled=true;
			if(fifo<0)
				{
				//cerr << "cannot create FIFO " << fifoName << endl;
				throw false;
				}
			// check if sniffer is present with a non blocking open
			fifo=open(fifoName.c_str(), O_WRONLY | O_NONBLOCK);
			if(fifo<0)
				{
				//cerr << "cannot open FIFO " << fifoName << endl;
				throw false;
				}
			fifoEnabled=true;
			}
		else	// 'r' read
			{
			writeEnabled=false;
			if(fifo<0)
				{
				//cerr << "cannot create FIFO " << fifoName << endl;
				throw false;
				}
			// the reader is blocking
			fifo=open(fifoName.c_str(), O_RDONLY);
			if(fifo<0)
				{
				//cerr << "cannot open FIFO " << fifoName << endl;
				throw false;
				}
			fifoEnabled=true;
			}
		}
	catch(...)
		{
		fifoEnabled=false;
		}
	return fifoEnabled;
	}

	/**
	 * close the fifo for sniffing
	 * @param fifoName
	 */
	void closeFifo()
	{
	if(fifoEnabled)
		{
		close(fifo);
		}
	}

	/**
	 * remove the fifo
	 */
	void removeFifo()
	{
	if(fifoEnabled)
		{
		remove(fifoname.c_str());
		}
	}

	/**
	 * write data to fifo
	 * @param data
	 * @param size
	 */
	void writeFifo(uint8_t *data,int size)
	{
	int ret;
	if(fifoEnabled && writeEnabled)
		{
		if(fifo != 0)
			{
			ret=write(fifo,data,size);
			if(ret<0)
				{
				//cerr << "error writing FIFO" << endl;
				}
			}
		}
	usleep(1000); // it seems usefull !
	}

	/**
	 * read data from the fifo
	 * @param data data read buffer
	 * @param size size of the buffer
	 * @return ndata data effectively read
	 */
	int readFifo(uint8_t *data, int size)
	{
	int ret;
	if(fifoEnabled && !writeEnabled)
		{
		ret=read(fifo,data,FIFO_BUFFER_SIZE);
		if(ret<0)
			{
			//cerr << "error reading FIFO" << endl;
			}
		}
	return ret;
	}

private:
	bool fifoEnabled;
	bool writeEnabled;
	string fifoname;
	int fifo;
};

//-----------------------------------------------
#endif /* COMM_FIFO_H_ */
