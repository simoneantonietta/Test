/*
 *-----------------------------------------------------------------------------
 * PROJECT:
 * PURPOSE: see module TaggedFiles.h file
 *-----------------------------------------------------------------------------
 */

#include "TaggedTxtFile.h"

#include <iostream>
#include "Utils.h"

/**
 * ctor
 */
TaggedTxtFile::TaggedTxtFile()
{
setSeparator(TAGGEDFILE_DEFAULT_SEPARATOR);
loaded=false;
}

/**
 * ctor
 * @param filename
 */
TaggedTxtFile::TaggedTxtFile(string filename)
{
setFilename(filename);
setSeparator(TAGGEDFILE_DEFAULT_SEPARATOR);
loaded=false;
}

/**
 * dtor
 */
TaggedTxtFile::~TaggedTxtFile()
{
if(loaded)
	{
	for(map<string,dataElem_t>::iterator it=fileStructure.begin();it!=fileStructure.end();it++)
		{
		delete it->second.data;
		if(it->second.dataHex!=NULL)	delete it->second.dataHex;
		}
	}
}

/**
 * set the filename
 * @param filename
 */
void TaggedTxtFile::setFilename(string filename)
{
this->filename=filename;
}

/**
 * add a field to the file structure
 * @param tag tag name of the field
 * @param typ type
 * @param dataPtr pointer of the data
 * @param linebreak after linebreak fields go insert a CR. if -1 (TAGGEDFILE_NO_LINEBREAK) is ignored
 */
void TaggedTxtFile::addField(string tag, dataTypes_e typ, vector<string> *data, int linebreak)
{
dataElem_t e;
e.tag=tag;
e.type=typ;
e.data=data;
e.linebreak=linebreak;
fileStructure[tag]=e;
}

/**
 * add a single value to the file structure
 * @param tag tag name of the field
 * @param typ type
 * @param value value data
 */
void TaggedTxtFile::addField(string tag, dataTypes_e typ,string value)
{
dataElem_t e;
e.tag=tag;
e.type=typ;
e.value=value;
fileStructure[tag]=e;
}

/**
 * add a binary field that will be written as a hex string. linebreak means the number of bytes for each line
 * @param tag tag name
 * @param data binary data
 * @param size size of the data in bytes
 * @param linebreak number of bytes for each line (default=32)
 */
void TaggedTxtFile::addField(string tag, unsigned char* data, size_t size,int linebreak)
{
dataElem_t e;
int lb=(linebreak & 1) ? (linebreak+1) : (linebreak);	// must be even
e.dataHex=new char[size*2+5];
Hex2AsciiHex(e.dataHex,data,size,false,0);
e.type=tf_hex;
e.linebreak=lb;
e.tag=tag;
fileStructure[tag]=e;
}

/**
 * set the separator for fields data
 * @param sep separator char
 */
void TaggedTxtFile::setSeparator(char sep)
{
separator=sep;
}

/**
 * load file to memory
 * @return true:ok; false: error
 */
bool TaggedTxtFile::load()
{
ifstream f;
string val,tag;
dataElem_t e;
string sep;
char _sep=TAGGEDFILE_CR;

sep.append((const char *)&separator);
sep.append(&_sep);

f.open(filename.c_str());
if(f.good())
	{
	fileStructure.clear();
	// read the file
	while (!f.eof())
		{
		val.clear();
		e.tag=readTaggedData(f,&val,&e.type,&e.linebreak);
		if(val.find(separator)!=string::npos)	// if single value
			{
			e.value=val;
			}
		else
			{
			vector<string> *v=new vector<string>();
			e.data=v;
			switch(e.type)
				{
				//.....................................................
				case tf_string:
				case tf_integer:
				case tf_float:
				case tf_double:
					Split(val,*e.data,sep);
					break;
				case tf_hex:
					e.dataHex=new char[val.size()+1];
					strcpy(e.dataHex,val.c_str());
					break;
				}
			}
		fileStructure[e.tag]=e;	// add to the database
		//cout << "tag: " << e.tag << " -> fields: " << e.data->size() << endl;
		}
	loaded=true;
	f.close();
	}
else
	return false;
return true;
}

/**
 * write data to file
 * @return true:ok; false: error
 */
bool TaggedTxtFile::save()
{
ofstream f;
stringstream s;
unsigned int n=0;

f.open(filename.c_str());
if(f.good())
	{
	//for(map<string,dataElem_t>::iterator it=fileStructure.begin();it!=fileStructure.end();it++)
	for(map<string,dataElem_t>::reverse_iterator it=fileStructure.rbegin();it!=fileStructure.rend();it++)
		{
		s << TAGGEDFILE_TAG_BEGIN
		  << it->first 	<< TAGGEDFILE_PARAM_SEP <<  it->second.type
		  							<< TAGGEDFILE_PARAM_SEP <<  it->second.linebreak
		  << TAGGEDFILE_TAG_END << TAGGEDFILE_CR;
		switch(it->second.type)
			{
			//.....................................................
			case tf_string:
			case tf_integer:
			case tf_float:
			case tf_double:
				if(it->second.data->empty())
					{
					s << it->second.value << TAGGEDFILE_CR;
					}
				else
					{
					for(vector<string>::iterator datait=it->second.data->begin(); datait<it->second.data->end(); datait++)
						{
						s << *datait;
						n++;
						if(it->second.linebreak>0)
							{
							if(n>=it->second.linebreak)
								{
								s << TAGGEDFILE_CR;
								n=0;
								}
							else
								{
								if(datait<it->second.data->end()-1)
									s << separator;
								else
									{
									n=0;
									s << TAGGEDFILE_CR;
									}
								}
							}
						}
					}
				break;
			//.....................................................
			case tf_hex:
				while(n<strlen(it->second.dataHex))
					{
					for(int t=0;t<it->second.linebreak*2 && n<strlen(it->second.dataHex);t++)
						{
						s << it->second.dataHex[n];
						n++;
						}
					s << TAGGEDFILE_CR;
					}
				break;
			}
		}
	}
string tmp=s.str();
f << tmp;
f.close();
return true;
}

/**
 * clear all the database and variables
 */
void TaggedTxtFile::clear()
{
fileStructure.clear();
filename.clear();
}


//-----------------------------------------------------------------------------
// PRIVATE METHODS
//-----------------------------------------------------------------------------
/**
 * read a tagged data
 * @param f file to read
 * @param data tag data of the tag
 * @param type type of the data
 * @param lb line break
 * @return tag readed
 * <_ERROR_> error reading data
 * <_EOF_>
 */
string TaggedTxtFile::readTaggedData(ifstream &f,string *data,dataTypes_e *typ,int *lb)
{
string tag;
string _lb,_type;
int nparam=0;
char ch;
bool done=false;
enum st_e {pidle,ptag,pparam,pdata} st=pidle;

while(!done && !f.eof())	// read a tag and its data
	{
	f.read(&ch,1);	// read c
	switch(st)
		{
		case pidle:
			if(ch==TAGGEDFILE_TAG_BEGIN) st=ptag;
			break;

		case ptag:
			if(ch!=TAGGEDFILE_TAG_END)
				{
				if(ch==TAGGEDFILE_PARAM_SEP)
					{
					nparam++;
					st=pparam;
					}
				else
					{
					tag.append(&ch);	// rebuild the tag
					}
				}
			else
				st=pdata;
			break;

		case pparam:
			if(ch!=TAGGEDFILE_TAG_END && ch!=TAGGEDFILE_PARAM_SEP)
				{
				switch(nparam)
					{
					case 1:	// type
						_type.append(&ch);
						break;
					case 2:	// break line
						_lb.append(&ch);
						break;
					}
				}
			else if(ch==TAGGEDFILE_PARAM_SEP || ch==TAGGEDFILE_TAG_END)
				{
				switch(nparam)
					{
					case 1:	// type
						*typ=(dataTypes_e)to_number<int>(_type);
						break;
					case 2:	// break line
						*lb=to_number<int>(_lb);
						break;
					}
				f.unget();
				st=ptag;	// restart
				}
			break;

		case pdata:
			switch(*typ)
				{
				//.....................................................
				case tf_string:
				case tf_integer:
				case tf_float:
				case tf_double:
					if(ch!='<')
						{
						if(ch=='\n')
							data->append(&separator);
						else
							data->append(&ch);
						}
					else
						{
						f.unget();
						done=true;
						}
					break;
				//.....................................................
				case tf_hex:
					if(ch!='<')
						{
						if(ch!='\n') data->append(&ch);
						}
					else
						{
						f.unget();
						done=true;
						}
					break;
				}
			break;
		}
	}
return tag;
}

