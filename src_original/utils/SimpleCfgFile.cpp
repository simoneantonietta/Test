/*
 * PROJECT:  dockstationPC
 *
 * FILENAME: SimpleCfgFile.cpp
 * 
 * PURPOSE:
 *
 *
 * LICENSE: please refer to LICENSE.TXT
 * CREATED: 22/dic/2009
 * AUTHOR:  Luca Mini
 */
//_____________________________________________________________________________

#include "SimpleCfgFile.h"

#define SCF_COMMENT						'#'
#define SCF_SEPARATOR					'='
#define SCF_SECTION						"[]"
#define SCF_TYPE_OPEN					'('
#define SCF_TYPE_CLOSE				')'
#define SCF_SEPARATORS				"=#"

#define SCF_INT_SECTION_SEP		"."
#define SCF_INCLUDE_KEYWORD 	"@include"
#define SCF_TYPE_DEFINITION		'@'
#define SCF_INTERNAL_VARS		  "@_"

#define SCF_VALUEINFILE_TXT_L			"@@FILE_T("	// ex: @@FILE(test.txt)
#define SCF_VALUEINFILE_TXT_R			")"
#define SCF_VALUEINFILE_BIN_L			"@@FILE_B("	// ex: @@FILE(test.bin)
#define SCF_VALUEINFILE_BIN_R			")"

#define SCF_ARRAY_BEGIN						"@ARRAYBEGIN"
#define SCF_ARRAY_END							"@ARRAYEND"
#define SCF_ARRAY_SIZE_PREFIX			"_ARRAY_SIZE_"
#define SCF_ARRAY_AUTONUMB_START	1
#define SCF_ARRAY_SEPARATOR				" \t"

#define SCF_GLOBAL_SECTION_NAME			"*Global*"
/**
 * ctor
 *
 * @param
 * @return
 */
SimpleCfgFile::SimpleCfgFile()
{
hasSection=false;
hasInclude=false;
depthLevel=0;
autonumber=SCF_ARRAY_AUTONUMB_START;
isArrayEntry=false;
for(int i=0; i<MAX_INC_DEPTH_LEVEL;i++) nline[i]=0;
}


/**
 * dtor
 *
 * @param
 * @return
 */
SimpleCfgFile::~SimpleCfgFile()
{
FOR_EACH(it,entry_t,entries)
	{
	if(it->valueFromFile && it->isBinaryFile && it->binFileSize>0) delete [] it->binFileContent;
	}
}


/**
 * load file
 *
 * @param
 * @return true Ok; false on error
 */
bool SimpleCfgFile::loadCfgFile(string filename)
{
ifstream f;
string line;
bool ret=true;
bool result;

if(depthLevel==0)  // only the main file
	{
	sections.clear();
	this->filename=filename;
	entries.clear();
	}
f.open(filename.c_str());
if(f.is_open())
	{
	// read the file
	while (!f.eof())
		{
		nline[depthLevel]++;
		getline(f, line);
		result=parseLine(line);
		if(!result)
			{
			cerr << "syntax error in config file at line " << nline[depthLevel] << endl;
			}
		if(hasInclude)
			{
			depthLevel++;
			if(depthLevel>=MAX_INC_DEPTH_LEVEL)
				{
				cerr << "error: too many nested include!" << endl;
				ret=false;
				}
			else
				{
				hasInclude=false;
				loadCfgFile(incFile);	// reenter in this routine
				depthLevel--;
				}
			}
		//cout << line << endl;
		}
	f.close();
	}
else
	{
	ret=false;
	cerr << "Unable to open file " << filename << endl;
	}
return(ret);
}


/**
 * save a config file based on the memory database
 *
 * @param filename
 * @return true:ok
 */
bool SimpleCfgFile::saveCfgFile(string filename)
{
#define ALIGN_COMMENT_POSITION	39
string var;
string tmp,tmp1;
ofstream f;
stringstream s;

if(entries.empty())
	{
	cout << "cfg: warning: no data to save" << endl;
	return(false);
	}

this->filename=filename;
f.open(filename.c_str());
for(vector<entry_t>::iterator it=entries.begin();it<entries.end();it++)
	{
	if(it->isEmptyLine)
		{
		s << "\n";
		}
	else if(it->isComment)
		{
		s << it->comment << "\n";
		}
	else if(it->isSection)
		{
		s << SCF_SECTION[0] << it->value << SCF_SECTION[1] << "\n";
		}
	else if(it->isArrayStart)
		{
		s << it->varname << " " << it->value << "\n";
		}
	else if(it->isArrayEnd)
		{
		s << it->varname << "\n";
		}
	else if(it->isArraySize)
		{
		// do nothing
		}
	else	// normal entry
		{
		string var=getVarname(it->varname);
		if(it->isArrayEntry)
			{
			// array entry
			if(it->hasType)
				{
				s << it->type << " " << it->value;
				}
			else
				{
				s << it->value;
				}
			}
		else
			{
			// normal entry
			if(it->hasType)
				{
				s << var << " " << SCF_SEPARATOR << " " << it->type << " " << it->value;
				}
			else
				{
				s << var << " " << SCF_SEPARATOR << " " << it->value;
				}
			}
		if(it->inlineComment)
			{
			tmp1=s.str();
			alignFields(tmp,0,tmp1.c_str(),ALIGN_COMMENT_POSITION,((string)"# " + it->comment).c_str(),-1);
			s << tmp;
			}
		s << "\n";
		}
	// write to file
	tmp=s.str();
	//cout << tmp << endl;
	f << tmp;
	s.str("");
	}
f.flush();
f.close();
return(true);
}

/**
 * save a config file based on the memory database over the file previous loaded
 *
 * @return true:ok
 */
bool SimpleCfgFile::saveCfgFile()
{
return(saveCfgFile(this->filename));
}


/**
 * update a variable, if it doesn't exists it will be added
 * IMPORTANT: if the variable is a new array element don't use this method but use "addArrayElement"
 * for update the array instead use this
 * @param varname name of the var to update/add
 * @param value value of the var
 * @param inlineComment (optional) comment to add at the variable
 * @return true:ok
 */
bool SimpleCfgFile::updateVariable(string varname, string value, string inlineComment)
{
entry_t etmp;
vector<entry_t>::iterator it;

if(varname.empty()) return(false);
if(value.empty()) return(false);

it=find(entries.begin(),entries.end(),varname);	// before search if exists
if(it!=entries.end())
	{
	// update
	it->value=value;
	etmp=*it;
	}
else
	{
	// add
	etmp.isComment=false;
	etmp.inlineComment=false;
	etmp.varname=varname;
	etmp.value=value;
	etmp.hasType=false;
	etmp.isArrayEntry=false;
	if(inlineComment!="")
		{
		etmp.inlineComment=true;
		etmp.comment=SCF_COMMENT + " " + inlineComment;
		}
	insertNewVar(etmp);
	}

// this section is for check the type
if(etmp.hasType)
	{
	bool ret=checkValue(etmp.varname);
	if(!ret) cerr << "ERROR: " << etmp.varname << " has bad value (rule defined by: " << etmp.type << ")" << endl;
	return(ret);
	}

return(true);
}


/**
 * update a variable, if it doesn't exists it will be added.
 * This version threats the binary data, stored as strings of hex
 * @param varname variable to update
 * @param value value buffer (ptr)
 * @param len length of the buffer (value) in bytes
 * @param inlineComment (optional) comment to add at the variable
 * @return true:ok; false: error
 */
bool SimpleCfgFile::updateVariable(string varname, unsigned char* value, int len, string inlineComment)
{
bool ret=false;
// before all convert to string the value
char *buf=new char[len*2+1];

Hex2AsciiHex(buf,value,len,false,0);
ret=updateVariable(varname,buf,inlineComment); // call the standard method
delete [] buf;
return(ret);
}

/**
 * add an element to an array
 * @param arrayName name of the array
 * @param value value to add
 * @return true:ok
 */
bool SimpleCfgFile::addArrayElement(string arrayName,string value, string inlineComment)
{
entry_t etmp;
int nelm=getArraySize(arrayName);
string varname=arrayName+to_string(nelm+1);
int endpos=getArrayEndPos(arrayName);

// create the element
etmp.isComment=false;
etmp.inlineComment=false;
etmp.varname=varname;
etmp.value=value;
etmp.hasType=false;
etmp.isArrayEntry=true;
if(inlineComment!="")
	{
	etmp.inlineComment=true;
	etmp.comment=SCF_COMMENT + " " + inlineComment;
	}

// insert the element and update the size indicator
entries.insert(entries.begin()+endpos,etmp);
incdecArraySize(arrayName,+1);
return true;
}

/**
 * remove alla the elements of an array
 * useful to update the entire array writing it from the beginning using addArrayElement
 * NOTE:
 * @param arrayName
 * @param removeArray true if you wanto to remove the array entirely; false to left the its definition in the file
 * @return
 */
bool SimpleCfgFile::removeAllArrayElements(string arrayName, bool removeArray)
{
entry_t etmp;
int nelm=getArraySize(arrayName);
if(nelm>0)
	{
	int startpos=getArrayStartPos(arrayName);
	int endpos=getArrayEndPos(arrayName);
	entries.erase(entries.begin()+startpos+1,entries.begin()+endpos-1);	// remain: arraystart,arraysize,arrayend
	updateVariable(SCF_ARRAY_SIZE_PREFIX+arrayName,"0");

	if(removeArray)
		{
		entries.erase(entries.begin()+startpos);
		entries.erase(entries.begin()+endpos);
		int p=getVarPosition(SCF_ARRAY_SIZE_PREFIX+arrayName);
		entries.erase(entries.begin()+p);
		}
	}
return true;
}

/**
 * verifies if the parameter is defined
 *
 * @param varname		name of the parametere
 * @return true: is defined; false: not exists
 */
bool SimpleCfgFile::hasParameter(string varname)
{
vector<entry_t>::iterator it;

it=find(entries.begin(),entries.end(),varname);
if(it!=entries.end()) return(true);
return(false);
}


/**
 * get the value of a variable
 *
 * @param varname name of the variable
 * @return value of varname; if not found, return an empty string
 */
string SimpleCfgFile::getValueOf(string varname)
{
vector<entry_t>::iterator it;

it=find(entries.begin(),entries.end(),varname);
if(it>=entries.end())
	return("");
else
	return(it->value);
}


/**
 * get the value of a variable, and if not defined return the default
 *
 * @param varname name of the variable
 * @param defValue default string value
 * @return value of varname; if not found, return the default value
 */
string SimpleCfgFile::getValueOf(string varname, string defValue)
{
string res;

res=getValueOf(varname);
if(res.empty())
	return(defValue);
else
	return(res);
}

/**
 * get the value of a variable as integer
 * @param varname name of the variable
 * @param defValue default string value
 * @return
 */
int SimpleCfgFile::getValue_int(string varname, string defValue)
{
string res;
int r=0;
res=getValueOf(varname);
if(res.empty())
	{
	if(defValue.empty())
		{
		cerr << __FUNCTION__ << " ERROR: var " << varname << " not found" << endl;
		}
	else
		{
		r=to_number<int>(defValue);
		}
	}
else
	{
	r=to_number<int>(res);
	}
return r;
}

/**
 * get the value of a variable as float
 * @param varname name of the variable
 * @param defValue default string value
 * @return
 */
float SimpleCfgFile::getValue_float(string varname, string defValue)
{
string res;
float r=0;
res=getValueOf(varname);
if(res.empty())
	{
	if(defValue.empty())
		{
		cerr << __FUNCTION__ << " ERROR: var " << varname << " not found" << endl;
		}
	else
		{
		r=to_number<float>(defValue);
		}
	}
else
	{
	r=to_number<float>(res);
	}
return r;
}

/**
 * get the value of a variable as double
 * @param varname name of the variable
 * @param defValue default string value
 * @return
 */
double SimpleCfgFile::getValue_double(string varname, string defValue)
{
string res;
double r=0;
res=getValueOf(varname);
if(res.empty())
	{
	if(defValue.empty())
		{
		cerr << __FUNCTION__ << " ERROR: var " << varname << " not found" << endl;
		}
	else
		{
		r=to_number<double>(defValue);
		}
	}
else
	{
	r=to_number<double>(res);
	}
return r;
}


/**
 * get the value of a variable as boolean
 * @param varname name of the variable
 * @param defValue default string value
 * @return
 */
bool SimpleCfgFile::getValue_bool(string varname, string defValue)
{
string res;
bool r=false;
res=getValueOf(varname);
if(res.empty())
	{
	if(defValue.empty())
		{
		cerr << __FUNCTION__ << " ERROR: var " << varname << " not found" << endl;
		}
	else
		{
		r=to_bool(defValue);
		}
	}
else
	{
	r=to_bool(res);
	}
return r;
}


/**
 * get the value of a variable, and recover it's binary values
 * @param varname variable to be readed
 * @param val buffer that contain the value readed. In the file it is specified as a string of hex bytes
 * @param len length in bytes
 * @return bytes readed (0 may indicates an error)
 */
int SimpleCfgFile::getValueOf(string varname, unsigned char *val, int len)
{
int ret=0;
vector<entry_t>::iterator it;

it=find(entries.begin(),entries.end(),varname);
if(it<entries.end())
	{
	ret=AsciiHex2Hex(val,(char *)it->value.c_str(),len);
	}
return(ret);
}


/**
 * ritorna un vettore di stringhe contenente tutte le sezioni trovate nel file
 *
 * @param
 * @return vettore di stringhe
 */
vector<string> SimpleCfgFile::getAllSections()
{
return(sections);
}


/**
 * dice se il file possiede delle sezioni
 *
 * @param
 * @return true: si, ha delle sezioni
 */
bool SimpleCfgFile::hasSections()
{
return(hasSection);
}


/**
 * check the type (if the parameter has one)
 *
 * @param vasname	name of the variable to check
 * @return  true:ok (even if has no type defined); false: parameter is not good
 */
bool SimpleCfgFile::checkValue(string varname)
{
string rule=getRuleOf(varname);
return(checkRule(getValueOf(varname),rule));
}


/**
 * get the title of the cfg file if specified, or the filename
 *
 * @param
 * @return  title, or if undefined, the filename
 */
string SimpleCfgFile::getTitle()
{
return(getValueOf("@_title",filename));
}


/**
 * get the version of the cfg file if specified, or '-'
 *
 * @param
 * @return  version, or '-' if undefined
 */
string SimpleCfgFile::getVersion()
{
return(getValueOf("@_version","-"));
}


/**
 * get the filetypeof the cfg file if specified, or '-'
 *
 * @param
 * @return  version, or '-' if undefined
 */
string SimpleCfgFile::getFiletype()
{
return(getValueOf("@_filetype","-"));
}


/**
 * tell if a parameter has a type defined
 *
 * @param	varname parameter to check
 * @return true: has type definition; false: no
 */
bool SimpleCfgFile::hasType(string varname)
{
vector<entry_t>::iterator it,itrule;

it=find(entries.begin(),entries.end(),varname);
if(it>=entries.end())
	return(it->hasType);
else
	return(false);
}


/**
 * tell if a varname is a type definition
 *
 * @param	varname parameter to check
 * @return  true: is a type definition; false no
 */
bool SimpleCfgFile::isType(string varname)
{
if(varname[0]=='@' && varname[1]!='_') return(true); else return(false);
return(false);
}


/**
 * get the size of an array
 *
 * @param	ar array base name
 * @return size
 */
int SimpleCfgFile::getArraySize(string ar)
{
string r=getValueOf(SCF_ARRAY_SIZE_PREFIX+ar);
int val=to_number<int>(r);

return(val);
}

/**
 * print some info
 */
void SimpleCfgFile::printDatabaseInfo()
{
cout << "SECTIONS:\n---------------" << endl;
FOR_EACH(it,string,sections)
	{
	cout << *it << endl;
	}
cout << "\n ENTRIES:\n---------------"<< endl;
FOR_EACH(it,entry_t,entries)
	{
	if(it->isComment) cout << it->comment << endl;
	else if(it->isSection) cout << "SECTION: " << it->value << endl;
	else if(it->isEmptyLine) cout << "" << endl;
	else cout << it->varname << " = " << it->value << endl;
	}
}


// PRIVATE MEMBERS




/**
 * parse the line and store date in the data vector
 *
 * @param l line to parse
 * @return true: ok; false: syntax error
 */
bool SimpleCfgFile::parseLine(string line)
{
string var,data,type,comment,l,sect;
entry_t etmp;
vector<string> tokens;
string fn;

etmp.isComment=false;
etmp.inlineComment=false;
etmp.hasType=false;
etmp.isType=false;
etmp.filename="";
etmp.valueFromFile=false;
etmp.isBinaryFile=false;
etmp.binFileSize=0;
etmp.isArrayEnd=false;
etmp.isArrayStart=false;
etmp.isArrayEntry=false;
etmp.hasSection=false;
etmp.isSection=false;
etmp.isEmptyLine=false;
etmp.isArraySize=false;

l=line;
if(l[l.size()-1]=='\r')	l.erase(l.size()-1,1);	// erase "enter"
if(l.empty())
	{
	etmp.isEmptyLine=true;
	entries.push_back(etmp);
	return(true);
	}

trim(l);

if(l[0]==SCF_COMMENT)	// comment line
	{
	etmp.isComment=true;
	etmp.hasType=false;
	etmp.inlineComment=false;
	comment=l;
	data.clear();
	var.clear();
	}
else if(l[0]==SCF_SECTION[0])	// if it's a section
	{
	sect=l.substr(1,l.size()-2);
	trim(sect);
	sections.push_back(sect);	// store as section
	etmp.isSection=true;
	etmp.value=sect;
	entries.push_back(etmp);	// store as entries
	actualSection=sect;
	hasSection=true;
	return(true);
	}
else if(l.find(SCF_INCLUDE_KEYWORD)!=string::npos) // if declare an include file
	{
	Split(l,tokens," ");
	incFile=tokens[1];
	hasInclude=true;
	}
else if(l.find(SCF_ARRAY_BEGIN)!=string::npos) // if array init
	{
	Split(l,tokens,SCF_ARRAY_SEPARATOR);
	var=tokens[0];
	data=tokens[1];
	arrayVar=data;
	etmp.varname=tokens[0];
	etmp.value=tokens[1];
	etmp.isArrayStart=true;
	}
else if(l.find(SCF_ARRAY_END)!=string::npos) // if array end
	{
	// store an additional var to get the number of array entries. It's called 'SCF_ARRAY_SIZE_PREFIXname'
	entry_t as;
	as.varname=SCF_ARRAY_SIZE_PREFIX+arrayVar;
	as.value=to_string(autonumber-1);
	as.isArraySize=true;
	entries.push_back(as);	// store as entries (inside the block [arraystart-arrayend]
	autonumber=SCF_ARRAY_AUTONUMB_START;
	isArrayEntry=false;

	// data for the array end
	var=SCF_ARRAY_END;
	data=arrayVar;
	etmp.isArrayEnd=true;
	}
//---------------------
else // PARSER
	{
	string separator=(string)SCF_SEPARATORS;

	if(l[0]==SCF_TYPE_DEFINITION && l[1]!=SCF_INTERNAL_VARS[1])	// if is a type definition
		{
		etmp.isType=true;
		}

	// split the string
	Split(l,tokens,separator);
	if(isArrayEntry)
		{
		// must insert the varname because arrays entry have not a varname itself
		tokens.insert(tokens.begin(),arrayVar+to_string(autonumber));
		autonumber++;
		}
	var=trim(tokens[0]);

	if(	l.find(SCF_TYPE_OPEN)!=string::npos && \
			!etmp.isType  && \
			l.find(SCF_VALUEINFILE_BIN_L)==string::npos && \
			l.find(SCF_VALUEINFILE_TXT_L)==string::npos)	// entry with type definition
		{
		etmp.hasType=true;
		type=tokens[1].substr(tokens[1].find(SCF_TYPE_OPEN)+1,tokens[1].find(SCF_TYPE_CLOSE)-tokens[1].find(SCF_TYPE_OPEN)-1);
		trim(type);
		data=tokens[1].substr(tokens[1].find(SCF_TYPE_CLOSE)+1);
		trim(data);
		if(l.find(SCF_COMMENT)!=string::npos)
			{
			etmp.inlineComment=true;
			comment=trim(tokens[2]);
			}
		}
	else
		{
		// simple value (or type definition)
		data=trim(tokens[1]);
		if(l.find(SCF_COMMENT)!=string::npos)
			{
			etmp.inlineComment=true;
			comment=trim(tokens[2]);
			}

		// check if value is contained in a file
		fn=getDelimitedStr(data,(string)SCF_VALUEINFILE_TXT_L,(string)SCF_VALUEINFILE_TXT_R);
		if(!fn.empty())
			{
			trim(fn);
			etmp.filename=fn;
			etmp.valueFromFile=true;
			etmp.isBinaryFile=false;
			loadValueFromFile('t',etmp);
			}
		fn=getDelimitedStr(data,(string)SCF_VALUEINFILE_BIN_L,(string)SCF_VALUEINFILE_BIN_R);
		if(!fn.empty())
			{
			trim(fn);
			etmp.filename=fn;
			etmp.valueFromFile=true;
			etmp.isBinaryFile=true;
			loadValueFromFile('b',etmp);
			}
		}
	if(var.empty()) return(false);
	if(data.empty()) return(false);
	}

// check for array definition and setup
if(etmp.varname==SCF_ARRAY_BEGIN)
	{
	arrayVar=etmp.value;			// set the base variable name for the parameters
	isArrayEntry=true;				// next parameters are array entry
	}

// store data
if(!etmp.isComment)
	{
	if(hasSection)
		{
		etmp.varname=actualSection+(string)SCF_INT_SECTION_SEP+var;
		etmp.hasSection=true;
		}
	else
		etmp.varname=var;
	if(etmp.hasType) etmp.type=type;
	}
etmp.value=data;
etmp.comment=comment;
etmp.isArrayEntry=isArrayEntry;
entries.push_back(etmp);

// this section is for check the type
if(etmp.hasType)
	{
	bool ret=checkValue(etmp.varname);
	if(!ret) cerr << "ERROR: " << etmp.varname << " has bad value (rule defined by: " << etmp.type << ")" << endl;
	return(ret);
	}

return(true);
}


/**
 * return the section if present in the varname
 * @param varname varname to handle
 * @return section; "" if none
 */
string SimpleCfgFile::getSection(string varname)
{
string sect="";

size_t sp=varname.find(SCF_INT_SECTION_SEP);
if(sp!=string::npos)
	{
	sect=varname.substr(0,sp);
	}
return(sect);
}


/**
 * return the varname without section
 * @param varname varname to handle
 * @return varname
 */
string SimpleCfgFile::getVarname(string varname)
{
string var;
size_t sp=varname.find(SCF_INT_SECTION_SEP);
if(sp!=string::npos)
	var=varname.substr(sp+1);
else
	var=varname;
return(var);
}


/**
 * load value from file
 * NOTE: path is relative to the cfgfile
 * @param type type of file 't' = text; 'b' for binary file
 * @param elem element where to load data readed
 * @return true:ok; false: error
 */
bool SimpleCfgFile::loadValueFromFile(char type, entry_t &elem)
{
bool ret=false;
string line;
int nline=0;

// get the current path
char *path=NULL;
size_t size=0;
path=getcwd(path,size);

// threat the filename
string fpath=filenameGetDir(this->filename,true);
if(fpath.at(0)=='.')
	{
	fpath=fpath.substr(1);
	}
if(fpath.at(0)!='/')
	{
	fpath=_S("/")+fpath;
	}

// create the right path relative to the cfgfile
fpath=string(path) + fpath;
string fname=fpath+elem.filename;

if(type=='t')
	{
  ifstream vfile(fname.c_str());

  if (vfile.is_open())
		{
  	elem.txtFileContent.clear();
		while (vfile.good())
			{
			getline(vfile, line);
			elem.txtFileContent+=line;
			nline++;
			}
		vfile.close();
		cout << __FUNCTION__ << " > readed " << nline << " lines (" << elem.txtFileContent.size() << " bytes) from " << elem.filename << endl;
		ret=true;
		}
  else
  	cerr << "ERROR reading " << elem.filename << endl;
	}

if(type=='b')
	{
  ifstream vfile(fname.c_str(), ios::in | ios::binary | ios::ate);
	if (vfile.is_open())
		{
		elem.binFileSize = vfile.tellg();
		elem.binFileContent = new char[elem.binFileSize];
		vfile.seekg(0, ios::beg);
		vfile.read(elem.binFileContent, (long int)elem.binFileSize);
		vfile.close();
		cout << "> readed " << elem.binFileSize << " bytes from " << elem.filename << endl;
		ret=true;
		}
	else
  	cerr << "ERROR reading " << elem.filename << endl;
	}
return(ret);
}


/**
 * return the type of a variable, if defined
 *
 * @param	varname variable to search for the type
 * @return  type of the varname (rule); empty string if not found a rule
 */
string SimpleCfgFile::getRuleOf(string varname)
{
vector<entry_t>::iterator it,itrule;
// get the type
it=find(entries.begin(),entries.end(),varname);
if(it>=entries.end())
	return("");
else
	{
	// get the rule
	string stype=SCF_TYPE_DEFINITION + it->type;	// compose the name
	itrule=find(entries.begin(),entries.end(),stype);
	if(itrule>=entries.end())
		return("");
	else
		return(itrule->value);
	}
return("");
}



/**
 * check if the value complies with the rules. This is the main function to be called
 * some examples of rules (the second member)
 * @lista1 = list(item1,item2,item3)
 * @lista2 = alist_i(-10, 10, 1)
 * @lista3 = alist_f(-20.0, 50.5, 10.0)
 * @range1 = range_i(10, 100)
 * @range2 = range_f(10.0, 100.0)
 *
 * @param	value value to check
 * @param rule rule for the value
 * @return  true: ok; false: not complies
 */
bool SimpleCfgFile::checkRule(string value, string rule)
{
bool ret=false;
string ruleType, stmp, sitm;
vector<string> items;

// split the rule
items.push_back(rule.substr(0,rule.find('(')));
trim(items[0]);
//cout << items[0] << endl;
stmp=rule.substr(rule.find('('));
for(unsigned int i=0;i<stmp.length();i++)
	{
	if(stmp[i]!=' ' && stmp[i]!=',' && stmp[i]!='(' && stmp[i]!=')')	// split the items of the rule
		{
		sitm.append(1,stmp[i]);
		}
	else
		{
		if(sitm.length()>0)
			{
			items.push_back(sitm);
			//cout << sitm << endl;
			sitm.clear();
			}
		}
	}
//cout << items.size() << " items" << endl;

// dispatch to specialized checker
if(items[0]=="list") ret=checkValue_list(value,items);
if(items[0]=="alist_i") ret=checkValue_alist_i(value,items);
if(items[0]=="alist_f") ret=checkValue_alist_f(value,items);
if(items[0]=="range_i") ret=checkValue_range_i(value,items);
if(items[0]=="range_f") ret=checkValue_range_f(value,items);

return(ret);
}


/**
 * check a value for a list type
 *
 * @param value value to check
 * @param rule vector string where at position 0 there is the type (list) and at next position there
 * 				are paramenters
 * @return true: ok; false: not complies
 */
bool SimpleCfgFile::checkValue_list(string value, vector<string> rule)
{
bool ret=false;
for(vector<string>::iterator it=rule.begin()+1; it<rule.end(); it++)
	{
	if(*it==value)
		{
		ret=true;
		break;
		}
	}
return(ret);
}


/**
 * check a value for an automatic integer list type
 *
 * @param value value to check
 * @param rule vector string where at position 0 there is the type (list) and at next position there
 * 				are paramenters
 * @return true: ok; false: not complies
 */
bool SimpleCfgFile::checkValue_alist_i(string value, vector<string> rule)
{
bool ret=false;
int val;

int lmin=to_number<int>(rule[1]);
int lmax=to_number<int>(rule[2]);
int lstep=to_number<int>(rule[3]);
if(IsIntNumber(value))
	{
	val=to_number<int>(value);
	for(int l=lmin;l<lmax;l+=lstep)
		{
		if(l==val)
			{
			ret=true;
			break;
			}
		}
	}

return(ret);
}


/**
 * check a value for an automatic float list type
 *
 * @param value value to check
 * @param rule vector string where at position 0 there is the type (list) and at next position there
 * 				are paramenters
 * @return true: ok; false: not complies
 */
bool SimpleCfgFile::checkValue_alist_f(string value, vector<string> rule)
{
bool ret=false;
float val;

float lmin=to_number<float>(rule[1]);
float lmax=to_number<float>(rule[2]);
float lstep=to_number<float>(rule[3]);
if(IsFloatNumber(value))
	{
	val=to_number<float>(value);
	for(float l=lmin;l<lmax;l+=lstep)
		{
		if(l==val)
			{
			ret=true;
			break;
			}
		}
	}

return(ret);
}


/**
 * check a value if it stay in the range specified
 *
 * @param value value to check
 * @param rule vector string where at position 0 there is the type (list) and at next position there
 * 				are paramenters
 * @return true: ok; false: not complies
 */
bool SimpleCfgFile::checkValue_range_i(string value, vector<string> rule)
{
bool ret=false;
int val;

int rmin=to_number<int>(rule[1]);
int rmax=to_number<int>(rule[2]);

if(IsIntNumber(value))
	{
	val=to_number<int>(value);
	if(val>=rmin && val <=rmax) ret=true;
	}

return(ret);
}



/**
 * check a value if it stay in the range specified
 *
 * @param value value to check
 * @param rule vector string where at position 0 there is the type (list) and at next position there
 * 				are paramenters
 * @return true: ok; false: not complies
 */
bool SimpleCfgFile::checkValue_range_f(string value, vector<string> rule)
{
bool ret=false;
float val;

float rmin=to_number<float>(rule[1]);
float rmax=to_number<float>(rule[2]);

if(IsFloatNumber(value))
	{
	val=to_number<float>(value);
	if(val>=rmin && val <=rmax) ret=true;
	}

return(ret);
}

/**
 * get the position in the vector of the variable specified
 * @param arrayName
 * @return position, -1 not found
 */
int SimpleCfgFile::getVarPosition(string varname)
{
vector<entry_t>::iterator it;

it=find(entries.begin(),entries.end(),varname);
if(it>=entries.end())
	return(-1);
else
	return distance(entries.begin(),it);
}

/**
 * insert new element.
 * If is in a section it will be put at the end of the section
 * if the section does not exists, it will be created at the end of the file and the new element will be inserted
 * NOTE: before use this you must be sure that the variable does not exists!
 * @param etmp entry
 * @return true: ok
 */
bool SimpleCfgFile::insertNewVar(entry_t etmp)
{
unsigned int pos;
vector<SectionPos_st> sPos;

string sec=getSection(etmp.varname);
findSectionsPos(sPos);

if(sec.empty())
	{
	// global section (no section)
	pos=sPos[0].endPos;
	if(pos>0)	while(entries[pos-1].isEmptyLine) pos--;		// avoid insertion after empty rows, to compact data on the top
	entries.insert(entries.begin()+pos,etmp);
	}
else
	{
	// in a section
	// first see if the section is present
	vector<SectionPos_st>::iterator found=find(sPos.begin(),sPos.end(),sec);
	if(found!=sPos.end()) // section already exists
		{
		pos=found->endPos;
		if(pos>0) while(entries[pos-1].isEmptyLine) pos--;		// avoid insertion after empty rows, to compact data on the top
		entries.insert(entries.begin()+pos,etmp);
		}
	else // section must be created
		{
		entry_t esec;
		// create the section at the end of the vector
		sections.push_back(sec);	// store as section
		esec.isSection=true;
		esec.value=sec;
		entries.push_back(esec);	// store as entries
		actualSection=sec;
		hasSection=true;

		// push the entry
		entries.push_back(etmp);
		}
	}
return true;
}

/**
 * retrieve the position of all the present sections (start and end)
 * the first position is the GLOBAL section that has the name "*GLOBAL*" (SCF_GLOBAL_SECTION_NAME)
 * @param spos
 */
void SimpleCfgFile::findSectionsPos(vector<SectionPos_st> &spos)
{
SectionPos_st p;
int n=0;
// at least the global section is present
p.section=SCF_GLOBAL_SECTION_NAME;
p.startPos=0;
p.endPos=-1;	// to be defined
spos.push_back(p);

for(vector<entry_t>::iterator it=entries.begin();it<entries.end();it++)
	{
	if(it->isSection)
		{
		p.section=it->value;
		p.startPos=distance(entries.begin(),it);
		spos.push_back(p);
		// update the previous entry with end posiztion info
		spos[n].endPos=p.startPos-1;
		n++;
		}
	}
// update the last section
spos[n].endPos=distance(entries.begin(),entries.end())-1;
}

/**
 * get the position in the vector of the start indicator
 * @param arrayName
 * @return position, -1 not found
 */
int SimpleCfgFile::getArrayStartPos(string arrayName)
{
vector<entry_t>::iterator it;
if(arrayName.empty()) return(false);

for(vector<entry_t>::iterator it=entries.begin();it<entries.end();it++)
	{
	if(arrayName==it->value && it->isArrayStart)
		{
		return distance(entries.begin(),it);
		}
	}
return -1;
}
/**
 * get the position in the vector of the end indicator
 * @param arrayName
 * @return position, -1 not found
 */
int SimpleCfgFile::getArrayEndPos(string arrayName)
{
vector<entry_t>::iterator it;
if(arrayName.empty()) return(false);

for(vector<entry_t>::iterator it=entries.begin();it<entries.end();it++)
	{
	if(arrayName==it->value && it->isArrayEnd)
		{
		return distance(entries.begin(),it);
		}
	}
return -1;
}

/**
 * inc/dec the size of an array
 *
 * @param	ar array base name
 * @param incdec if positive add to tne array, if negative subtract to the size
 * @return new size
 */
int SimpleCfgFile::incdecArraySize(string ar,int incdec)
{
string r=getValueOf(SCF_ARRAY_SIZE_PREFIX+ar);
int val=to_number<int>(r);
val+=incdec;
updateVariable(SCF_ARRAY_SIZE_PREFIX+ar,to_string(val));
return val;
}

