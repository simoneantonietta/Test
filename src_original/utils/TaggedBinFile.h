/**----------------------------------------------------------------------------
 * PROJECT: WUtilities
 * PURPOSE: handle simple binary tagged file
 *
 *
 * Example of usage:
 * 
 *     TaggedBinFile tb;
 *     int dat1=5;
 *     float dat2=123.45;
 *     string dat3="test for a std string";
 *     vector<double> dat4;
 *
 *     int rdat1=0;
 *     float rdat2=0.0;
 *     string rdat3="......................";
 *     vector<double> rdat4;
 *
 *     dat4.push_back(1.1);
 *     dat4.push_back(2.2);
 *     dat4.push_back(3.3);
 *     dat4.push_back(4.4);
 *     dat4.push_back(5.5);
 *
 *     tb.setFilename("test.bin");
 *     tb.open('w');
 *     tb.setHeader("test","1.0","Wood","Ebanister");
 *     tb.writeHeader();
 *     tb.writeField("dat1",sizeof(int),&dat1);
 *     tb.writeField("dat2",sizeof(float),&dat2);
 *     tb.writeField("dat3",dat3.size(),(char*)dat3.c_str());
 *     tb.writeArrayDef("dat4",dat4.size());
 *     FOR_EACH(it,double,dat4)
 *     	{
 *     	//printf("val %f\n",*it);
 *     	tb.writeArrayElem(sizeof(double),(char*)&*it);
 *     	}
 *     tb.close();
 *
 *     tb.open('r');
 *     tb.load();
 *     printf("File length: %i\n",tb.getFilelength());
 *     if(tb.checkVersion(2,0)==TBF_FILEVERSION_LOWER) cout << "file version lower" <<endl;
 *     if(tb.checkVersion(1,0)==TBF_FILEVERSION_EQUAL) cout << "file version equal" <<endl;
 *     if(tb.checkVersion(0,1)==TBF_FILEVERSION_HIGHER) cout << "file version higher" <<endl;
 *     tb.readField("dat1",&rdat1);
 *     tb.readField("dat2",&rdat2);
 *     tb.readField("dat3",(char *)rdat3.c_str());
 *     tb.readField("dat4",NULL);
 *     int n=tb.getSize("dat4");
 *     double v;
 *     for(int i=0;i<n;i++)
 *     	{
 *     	tb.readField(tb.getElementUniqueName(i),&v);
 *     	rdat4.push_back(v);
 *     	printf("val %f\n",v);
 *     	}
 *     tb.close();
 *
 *-----------------------------------------------------------------------------  
 * CREATION: 25 Sep 2013
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

#ifndef TAGGEDBINFILE_H_
#define TAGGEDBINFILE_H_

#include <string>
#include <cstring>
#include <fstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdlib>

//------------------------------------------------------------------------------
// DEFINES (globals)
//------------------------------------------------------------------------------
#define TBF_HEADER_MARK									"TBF"
#define TBF_HEADER_HDR_SIZE							(sizeof(TBF_HEADER_MARK)+1)
#define TBF_HEADER_TYPE_SIZE						(32+1)
#define TBF_HEADER_VER_SIZE							(8+1)
#define TBF_HEADER_MANUFACTURER_SIZE		(32+1)
#define TBF_HEADER_PRODUCT_SIZE					(32+1)


#define TBF_TAG_SIZE						(32+1)			// number of chars in the tag (asciiz)
#define TBF_ELEMENT_PREFIX			"E)"
#define TBF_ARRAY_PART_NAME			5						// number of chars of port of the array name, first n chars
#define TBF_STRING_NUMBER_SIZE	7
#define TBF_ELEM_UNIQUE_PART		"_%02X_%04X"


#define TBF_FILEVERSION_LOWER			-1
#define TBF_FILEVERSION_EQUAL			0
#define TBF_FILEVERSION_HIGHER		1
//------------------------------------------------------------------------------
// TYPES (globals)
//------------------------------------------------------------------------------
typedef unsigned long int tbf_size_t;
typedef unsigned char byte;

//------------------------------------------------------------------------------
// MACROS
//------------------------------------------------------------------------------
#define ERR(m) 			cerr << m << endl
//==============================================================================


enum tbf_dataType_e
{
	tbf_undef				=0,
	tbf_data				=1,
	tbf_array_def		=2,
	tbf_array_elem	=3,
};

// file header
struct FileHeader
{
	char header[TBF_HEADER_HDR_SIZE];		// always "TBF"
	char fileType[TBF_HEADER_TYPE_SIZE];	// i.e: "waveforms"
	char version[TBF_HEADER_VER_SIZE];	// i.e: "1.2"
	char manufacturer[TBF_HEADER_MANUFACTURER_SIZE];	// i.e: "WOOD"
	char product[TBF_HEADER_PRODUCT_SIZE]; // i.e: "Ebanister"
};

// generic structure of each tag
struct BinTag
{
	char tag[TBF_TAG_SIZE];
	tbf_dataType_e type;
	tbf_size_t nElement; // if is an array element, it contains the position number of the element it represets; in other cases it will be 0
	tbf_size_t size; // if is an array def, it contains the number of elements
	void *data;	// if is an array def, it will be NULL
	size_t filePos;	// position in the file of the data (read operation)

	BinTag()
	{
	for(int i=0;i<TBF_TAG_SIZE;i++) this->tag[i]=0;
	type=tbf_undef;
	size=0;
	data=0;
	}

	BinTag(char *tag,tbf_dataType_e type,tbf_size_t nElement,tbf_size_t size,void *data)
	{
	for(int i=0;i<TBF_TAG_SIZE;i++) this->tag[i]=0;
	strcpy(this->tag,tag);
	this->type=type;
	this->nElement=nElement;
	this->size=size;
	this->data=data;
	}
	/*
	 * overload for sort
	 */
	bool operator<(const BinTag&other) const
	{
	return 	(strcmp(this->tag,other.tag)<0);
	}

	/*
	 * overload for find
	 */
	bool operator==(const BinTag&other) const
	{
	return (strcmp(this->tag,other.tag)==0);
	}

	/*
	 * overload for find
	 */
	bool operator==(const char *other) const
	{
	return (strcmp(this->tag,other)==0);
	}

};


using namespace std;

/**
 * main class
 */
class TaggedBinFile
{
public:
	TaggedBinFile();
	virtual ~TaggedBinFile();

	const string& getFilename() const
	{
	return filename;
	}

	void setFilename(const string& filename)
	{
	this->filename = filename;
	}

	bool open(char mode);
	/**
	 * open a
	 * @param fname
	 * @param mode
	 * @return
	 */
	bool open(string fname,char mode) {setFilename(fname); return open(mode); }
	void close();

	void setHeader(const char *fileType,const char *version,const char *manufacturer,const char *product);

	bool writeHeader();
	bool writeField(const char *tag, tbf_size_t size, void *data);
	bool writeArrayDef(const char* tag, tbf_size_t nelem);
	bool writeArrayElem(tbf_size_t size, void *data);

	bool load();
	bool isEr() const
		{
		return er;
		}
	tbf_size_t getSize(const char *tag);
	bool readField(const char *tag, void* data);
	char * getElementUniqueName(int nelem);

	int checkVersion(int major,int minor);
	char *getFileType() {return fHeader.fileType;}
	char *getFileVersion() {return fHeader.version;}
	char *getManufacturer() {return fHeader.manufacturer;}
	char *getProductr() {return fHeader.product;}
	int getFilelength();

private:
	bool isOpen;
	string filename;
	FileHeader fHeader;
	ifstream ifile;
	ofstream ofile;
	char mode;
	int fileLen;
	vector<BinTag> dataFields;
	bool er;		// error condition true=error, used for the members cannot return explicitly an error condition

	// array handling
	unsigned int elemCount;
	unsigned int nElements;
	char arrayName[TBF_TAG_SIZE];
	char arrayElementName[TBF_TAG_SIZE];
	unsigned int arrayCounter;	// count the number of arrays in the file from the beginning (suet to generate the unique name for elements)

	bool _writeBinTag(BinTag &bt);
	bool _readHeader();
	bool _readBinTag(BinTag &bt);
};

//-----------------------------------------------
#endif /* TAGGEDBINFILE_H_ */
