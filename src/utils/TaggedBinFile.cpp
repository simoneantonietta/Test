/*
 *-----------------------------------------------------------------------------
 * PROJECT: WUtilities
 * PURPOSE: see module TaggedBinFile.h file
 *-----------------------------------------------------------------------------
 */

#include "TaggedBinFile.h"

/**
 * ctor
 */
TaggedBinFile::TaggedBinFile()
{
arrayCounter=0;
isOpen=false;
}

/**
 * dtor
 */
TaggedBinFile::~TaggedBinFile()
{
}

/**
 * open the file
 * @param mode 'w' write mode; 'r' read mode
 * @return true:ok;false:error
 */
bool TaggedBinFile::open(char mode)
{
bool ret=true;
this->mode=mode;
if(mode=='w')
	{
	ofile.open(filename.c_str(), ios::out|ios::binary|ios::ate);
	if (ofile.is_open())
		{
		ofile.seekp(0,ios::beg);
		}
	else
		ret=false;
	}
else if(mode=='r')
	{
	dataFields.clear();

	ifile.open(filename.c_str(), ios::in|ios::binary|ios::ate);
	if (ifile.is_open())
		{
    // get length of file:
    ifile.seekg(0, ifile.end);
    fileLen = ifile.tellg();
    ifile.seekg(0, ifile.beg);
		}
	else
		ret=false;
	}
else
	ret=false;

if(!ret)
	{
	ERR("error opening file " << filename);
	isOpen=false;
	}
else
	isOpen=true;
return ret;
}

int TaggedBinFile::getFilelength()
{
if(!isOpen)
	{
	ifile.open(filename.c_str(), ios::in|ios::binary|ios::ate);
	if (ifile.is_open())
		{
    // get length of file:
    ifile.seekg(0, ifile.end);
    fileLen = ifile.tellg();
    ifile.seekg(0, ifile.beg);
		}
	}
else if(isOpen && mode=='w')
	return 0;	// write mode

return fileLen;
}

/**
 * close the file opened
 */
void TaggedBinFile::close()
{
if(mode=='w')
	{
	ofile.close();
	}
else if(mode=='r')
	{
	ifile.close();
	}
}

/**
 * write the header. This is the first think to be written
 * @return true:ok;false:error
 */
void TaggedBinFile::setHeader(const char *fileType,const char *version,const char *manufacturer,const char *product)
{
memset(&fHeader,0,sizeof(FileHeader));
strcpy(fHeader.fileType,fileType);
strcpy(fHeader.header,TBF_HEADER_MARK);
strcpy(fHeader.version,version);
strcpy(fHeader.manufacturer,manufacturer);
strcpy(fHeader.product,product);
}


/**
 * write the header. This must be the first write operation
 * @return true:ok;false:error
 */
bool TaggedBinFile::writeHeader()
{
bool ret=true;
if(ofile.good())
	{
	ofile.write((char *)&fHeader,sizeof(FileHeader));
	}
else
	{
	ERR("error writing header file " << filename);
	ret=false;
	}
return ret;
}

/**
 * write a field to file
 * @param tag tag name
 * @param size size in bytes
 * @param data data to be written
 * @return true:ok; false: error
 */
bool TaggedBinFile::writeField(const char* tag, tbf_size_t size, void* data)
{
bool ret=true;
BinTag *bt;
bt=new BinTag((char*)tag,tbf_data,0,size,data);
ret=_writeBinTag(*bt);
dataFields.push_back(*bt);
delete bt;
return ret;
}

/**
 * write an array definition to file
 * @param tag tag name
 * @param nelem number of elements in the array
 * @return true:ok; false: error
 */
bool TaggedBinFile::writeArrayDef(const char* tag, tbf_size_t nelem)
{
bool ret=true;
BinTag *bt;
bt=new BinTag((char*)tag,tbf_array_def,0,nelem,NULL);
strcpy(arrayName,tag);	// store array name
ret=_writeBinTag(*bt);
dataFields.push_back(*bt);
elemCount=0;
nElements=nelem;
arrayCounter++;

delete bt;
return ret;
}

/**
 * write an array element to file into the previously array definition
 * The tag name is derived from the array definition
 * @param tbf_dataType_e type type of the field
 * @param size size in bytes
 * @param data data to be written
 * @return true:ok; false: error
 */
bool TaggedBinFile::writeArrayElem(tbf_size_t size, void *data)
{
bool ret=true;
BinTag *bt;
getElementUniqueName(elemCount);
bt=new BinTag(arrayElementName,tbf_array_elem,elemCount,size,data);
ret=_writeBinTag(*bt);
dataFields.push_back(*bt);
elemCount++;
//if(elemCount==nElements) elemCount=0;	// end of elements array
delete bt;
return ret;
}

/**
 * read the file entirely, taking the data structure.
 * Data contents will be read from the application
 * @returntrue:ok;false:error
 */
bool TaggedBinFile::load()
{
bool ret=true;
BinTag bt;

arrayCounter=0;
elemCount=0;
dataFields.clear();

ret=_readHeader();
if(ret)
	{
	while(!ifile.eof() && ifile.tellg()<fileLen)
		{
		//printf("pos: %X\n",ifile.tellg());
		ret=_readBinTag(bt);
		if(ret)
			{
			dataFields.push_back(bt);
			}
		else
			{
			ERR("ERROR: load operation failed");
			break;
			}
		}
	}
return ret;
}

/**
 * get the size of the tag
 * NOTE: execute a load operation before its call
 * @param tag tag to search
 * @return size of the data (0 if none or error, in this case please check the 'er' status)
 */
tbf_size_t TaggedBinFile::getSize(const char *tag)
{
er=false;
vector<BinTag>::iterator it=find(dataFields.begin(),dataFields.end(),(const char*)tag);
if(it!=dataFields.end())
	{
	return it->size;
	}
else
	{
	ERR("ERROR: tag [" << tag << "] not found");
	er=true;
	return 0;
	}
}

/**
 * get the data fron the file of the tag specified.
 * NOTE: execute a load operation before its call
 * @param tag tag data to retrieve
 * @param data ptr to the destination buffer (must be >= in size of the data to be readed)
 * @return
 */
bool TaggedBinFile::readField(const char *tag, void* data)
{
vector<BinTag>::iterator it=find(dataFields.begin(),dataFields.end(),(const char*)tag);
if(it==dataFields.end())
	{
	ERR("ERROR: tag [" << tag << "] not found");
	return false;
	}
if(it->type!=tbf_array_def)
	{
	ifile.seekg(it->filePos,ios::beg);
	ifile.read((char *)data,it->size);
	}

if(!ifile.good()) return false;
return true;
}

/**
 * used to check the version written in the header in the format "major.minor"
 * @param major
 * @param minor
 * @return -1: the file has an older version number; 0: is the same; +1: the file has a version number most recent that this one
 */
int TaggedBinFile::checkVersion(int major,int minor)
{
int hminor,hmajor;
char ver[TBF_HEADER_VER_SIZE],*mi;
strcpy(ver,fHeader.version);
mi=strchr(ver,'.')+1;
*(mi-1)=0;
hminor=atoi(mi);
hmajor=atoi(ver);
if(hmajor<major) return -1;
if(major==hmajor && hminor<minor) return -1;
if(major==hmajor && hminor==minor) return 0;
if(major==hmajor && hminor>minor) return 1;
if(hmajor>major) return 1;

return -2;	// not possible
}
//-----------------------------------------------------------------------------
// PRIVATE MEMBERS
//-----------------------------------------------------------------------------

/**
 * write a bin tag
 * @param bt bin tag to write
 * @return true:ok; false:error
 */
bool TaggedBinFile::_writeBinTag(BinTag &bt)
{
bool ret=true;
if(ofile.good())
	{
	ofile.write(bt.tag,TBF_TAG_SIZE);
	ofile.write((char *)&bt.type,sizeof(tbf_dataType_e));
	ofile.write((char *)&bt.nElement,sizeof(tbf_size_t));
	ofile.write((char *)&bt.size,sizeof(tbf_size_t));
	if(bt.size>0 && bt.type!=tbf_array_def && bt.data==NULL)
		ERR("ERROR: inconsistent type/size/data for tag [" << bt.tag << "]");
	if(bt.data!=NULL) ofile.write((char *)bt.data,bt.size);
	}
else
	{
	ERR("error writing tag [" << bt.tag << "] in file " << filename);
	ret=false;
	}
return ret;
}

/**
 * read the file header. This must be the first read operation
 * @return true:ok;false:error
 */
bool TaggedBinFile::_readHeader()
{
bool ret=true;
if(ifile.good())
	{
	ifile.read((char *)&fHeader,sizeof(FileHeader));
	}
else
	{
	ERR("error reading header file " << filename);
	ret=false;
	}
return ret;
}

/**
 * read a bin tag
 * NOTE: data is no readed, but in its place it read the position in the file to retrieve data
 * @param bt data filled with the data readed
 * @return true:ok;false:error
 */
bool TaggedBinFile::_readBinTag(BinTag &bt)
{
bool ret=true;
if(ifile.good())
	{
	//printf("pos: %X\n",ifile.tellg());
	ifile.read(bt.tag,TBF_TAG_SIZE);
	ifile.read((char *)&bt.type,sizeof(tbf_dataType_e));
	switch(bt.type)
		{
		case tbf_data:
			if(elemCount!=0)
				ERR("ERROR: expected an array element");
			else
				{
				ifile.read((char *)&bt.nElement,sizeof(tbf_size_t));
				ifile.read((char *)&bt.size,sizeof(tbf_size_t));
				//ifile.read((char *)bt.data,bt.size);
				bt.filePos=ifile.tellg();	// get the position of the data in the file
				//printf("pos: %X\n",ifile.tellg());
				ifile.seekg(bt.size,ios::cur);	// skip data
				}
			break;
		case tbf_array_def:
			if(elemCount!=0)
				ERR("ERROR: expected an array element");
			else
				{
				strcpy(arrayName,bt.tag);
				ifile.read((char *)&bt.nElement,sizeof(tbf_size_t));
				ifile.read((char *)&bt.size,sizeof(tbf_size_t));
				bt.filePos=ifile.tellg();	// get the position of the first element in the file
				//strcpy(arrayElementName,TBF_ELEMENT_PREFIX);
				//strncat(arrayElementName,bt.tag,TBF_ARRAY_PART_NAME);
				// this type has no data
				nElements=bt.size;
				elemCount=0;
				arrayCounter++;
				}
			break;
		case tbf_array_elem:
			if(elemCount < nElements)
				{
				ifile.read((char *)&bt.nElement,sizeof(tbf_size_t));
				if(elemCount==bt.nElement)	// found as exepcted
					{
					ifile.read((char *)&bt.size,sizeof(tbf_size_t));
					//ifile.read((char *)bt.data,bt.size);
					bt.filePos=ifile.tellg();	// get the position of data in the file
					ifile.seekg(bt.size,ios::cur);	// skip data
					elemCount++;
					}
				else
					{
					ERR("ERROR: unexpected array element number");
					}
				if(elemCount==nElements)
					{
					nElements=0;
					elemCount=0;
					}
				}
			else
				{
				ERR("ERROR: found element but no array definition or too many elements with respect the declared one");
				}
			break;

		case tbf_undef:
		default:
			ERR("ERROR: undefined type found -> skipped");
			break;
		}
	}
else
	{
	ERR("error writing tag [" << bt.tag << "] in file " << filename);
	ret=false;
	}
return ret;
}

/**
 * generate an unique name for each element of an array in the file
 * @param nelem position of the element
 * @return
 */
char* TaggedBinFile::getElementUniqueName(int nelem)
{
char num[TBF_STRING_NUMBER_SIZE];

strcpy(arrayElementName,TBF_ELEMENT_PREFIX);
strncat(arrayElementName,arrayName,TBF_ARRAY_PART_NAME);
sprintf(num,TBF_ELEM_UNIQUE_PART,arrayCounter,nelem);	// generate unique part
strcat(arrayElementName,num);
return arrayElementName;
}
