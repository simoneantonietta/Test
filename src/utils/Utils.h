/*
 * PROJECT:
 *
 * FILENAME: Utils.h
 *
 * PURPOSE:
 * Various usefull routine
 *
 *
 * LICENSE: please refer to LICENSE.TXT
 * CREATED: 29-10-08
 * AUTHOR: 	Luca Mini
 *
 * REVISION:
 * 22/06/10		added boolean handling in to_number
 * 01/02/16		added itoa replacement
 * 03/10/16   bugfix in txtFile2String
 */


#ifndef _UTILS_H
#define _UTILS_H

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdarg.h>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <sys/time.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
//#include <stdint.h>

using namespace std;


#define YES							1
#define OK							1
#define NO							0
#define KO							0

// some useful datatypes
typedef unsigned char byte;
typedef unsigned short int word;



/*
ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
	C strings (asciiz and ansii)
ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
*/
char* StrTrim(char *str);
const char *itoa_buf(char *buf, size_t len, int num);
int strsubncmp(const char *str1, int start1, const char *str2, int start2, int size);
void hexdump(void *ptr, int buflen);
void hexdumpStr(string &strout,void *ptr, int buflen);
std::string hexdumpStr2(void* x, unsigned long len, unsigned int w);
char * printableBuffer(char *dst,unsigned char *src, int len);
char* Hex2AsciiHex(char *dst,unsigned char *src, int len, bool sep, char sepChar);
int AsciiHex2Hex(unsigned char *dst,char *src, int len);
bool txtFile2String(string fname, string &dst);
int Hex2Int(string &v);
bool IsIntNumber(char *str);
bool IsFloatNumber(char *str);
bool IsDoubleNumber(char *str);
bool to_bool(const string &str);

bool IsIntNumber(const std::string& s);
bool IsFloatNumber(const std::string& s);
bool IsDoubleNumber(const std::string& s);
string& triml(string& s);
string& trimr(string& s);
string& trim(string& s);
std::string strFormat(const std::string fmt, ...);
void Split(const string& str,								// input string
           vector<string>& tokens,					// output vector
           const string& delimiters=" "			// delim
           );
void Split2(vector<string> &tokens, char *str, const char *delimiters);

bool strSetFields(string values,const string sep, string format, ...);
string& alignFields(string &sdst,int pos1, ...);
string getDelimitedStr(string src,string left, string right);
string& toUpperStr(string & s);
string& toLowerStr(string & s);
string& strReplace(string &s, string to_subst, string new_str);
string& strReplaceAll(string &s, string to_subst, string new_str);
string &strRemoveChar(string &s,char c);
string extractBetween(const string& text, const string& delimStart, const string& delimStop);
bool startsWith(const string& text, const string& token);

double RoundTo(double numb, double gran);
unsigned char getNibble(unsigned char value,char which);

void profStartTime_us(unsigned int &t1us);
unsigned int profGetDeltaTime_us(unsigned int &t1us);

void WriteTodmesg(const char* p_message, ...);

/*
ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
 files
ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
*/
bool fileExists(string filename);
long int filelength(FILE *fp);
string filenameGetDir(const string& fnp, bool slash=false);
string filenameGetFilename(const string& fnp);
string filenameGetExt(const string& fnp, bool dot=false);
bool fileList(const char* path, vector<string> &files, string filter,bool ignoreHidden);
bool saveLogFile(const string logfname, char* buff, size_t size);
bool saveLogFile(const string logfname, const char *buff);
bool saveLogFile(const string logfname, const string &buff);
void saveStrVec2File(string fname,vector<string> &v);
/*
ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
	string again (template)
ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
*/
/**
 * Convert anything to a C-string, starting with a user-supplied string ("note")
 * @param note string that appears before
 * @param i data to be printed after the "note"
 */
template<typename T> const char* makestr(char* note, const T& i)
{
std::ostringstream os;
os << note << i;
os << std::ends;
return os.str().c_str();
}

/**
 * convert a number to a string
 * @param value value to convert
 * @return a string containing the decimal value
 */
template <typename T>
string to_string(const T& value)
{
stringstream oss;
oss << value;
return oss.str();
}
/**
 * convert a string containing a valid number into a number
 * Note: is a function template so you must specify the number type
 * @param str string containing the number to be converted in a real number
 * @return the number represented in the string
 */
template <typename T>
T to_number(const string &str)
{
stringstream iss(str);
T i;
iss>>i;
return i;
}

/**
 * round a number to best integer in this manner:
 * if decimals >=0.5 then round to ceil, else to floor
 * NOTE: this implementation is for the various integer type
 * @param numb floating point number to round
 * @return value
 */
template <typename T>
T Round(double numb)
{
double q=floor(numb);
double r=numb-q;
if(r>=0.5) q+=1.0;
return((T)(q));
}

/**
 * check if a value is in the range specified
 * @param value value to check
 * @param Rmin mimimum value of the range
 * @param Rmax maximum value of the range
 * @param extremes tell which extreme is inclusive. Can be:
 *                 'N' -> left (Rmin) exclusive; right (Rmax) exclusive (both exlusive) ()
 *                 'L' -> left (Rmin) inclusive; right (Rmax) exclusive [)
 *                 'R' -> left (Rmin) exclusive; right (Rmax) inclusive (]
 *                 'B' -> left (Rmin) inclusive; right (Rmax) inclusive (both inclusive) []
 */
template <typename T>
bool isInRange(T value, T Rmin, T Rmax, char extremes)
{
if(extremes=='N')
	return((Rmin<value && value<Rmax) ? (true) : (false));
if(extremes=='L')
	return((Rmin<=value && value<Rmax) ? (true) : (false));
if(extremes=='R')
	return((Rmin<value && value<=Rmax) ? (true) : (false));
if(extremes=='B')
	return((Rmin<=value && value<=Rmax) ? (true) : (false));

return(false);		// must not arrive here!
}

/**
 * check if a value is in the symmetrical tolerance specified as %
 * @param value value to check
 * @param nom nominal value
 * @param tolerance %
 */
template <typename T>
bool isInToll(T value, T nom, float perc)
{
if(perc>=0.0 && perc <=100.0)
	{
	T max=nom*(1+perc/100);
	T min=nom*(1-perc/100);
	if((value <= max) && (value >= min)) return true;
	}
return false;		// must not arrive here!
}

/**
 * to move items in a vector
 * @param start index of the start block to be moved
 * @param length lenght of the block to be moved
 * @param dst destinatio index of the block
 * @param v vector where data are
 */
template<class T>
void moveVectorRange(size_t start, size_t length, size_t dst, std::vector<T> & v)
{
  const size_t final_dst = dst > start ? dst - length : dst;

  std::vector<T> tmp(v.begin() + start, v.begin() + start + length);
  v.erase(v.begin() + start, v.begin() + start + length);
  v.insert(v.begin() + final_dst, tmp.begin(), tmp.end());
}

/**
 * convert a number (decimal) into a binary string
 * @param n number to be converted
 * @param sz sued to align to a certain length; 0 (default) means no alignement
 * @return binary string
 */
template<class T>
string dec2binString(T n, int sz=0)
{
const size_t size = sizeof(T) * 8;
char result[size];
string res;

T index = size;
do
	{
	result[--index] = '0' + (n & 1);
	}
while (n >>= 1);
res=std::string(result + index, result + size);
if(sz>0)
	{
	int nz=sz-res.length();
	if(nz>0)
		{
		res=std::string('0',nz)+res;
		}
	}
return res;
}

//=============================================================================
// some useful MACROS !!!
//=============================================================================

#define WORD_MSB(val) ((unsigned char)((val & 0xFF00)>>8))
#define WORD_LSB(val) ((unsigned char)(val & 0x00FF))

#define _S 		(string)
//#define FOR_EACH(itname,vector_type,vector_name) for(vector<vector_type>::iterator itname=vector_name.begin(); itname<vector_name.end(); itname++)
//#define FOR_EACH_REV(itname,vector_type,vector_name) for(vector<vector_type>::reverse_iterator itname=vector_name.rbegin(); itname<vector_name.rend(); ++itname)
#define FOR_EACH(itname,vector_type,vector_name) for(std::vector<vector_type>::iterator itname=vector_name.begin(); itname<vector_name.end(); itname++)
#define FOR_EACH_REV(itname,vector_type,vector_name) for(std::vector<vector_type>::reverse_iterator itname=vector_name.rbegin(); itname<vector_name.rend(); ++itname)
#define GET_NDXFROMITERATOR(vec,iter) (distance(vec.begin(),iter))
#define GET_ITERATORFROMNDX(vec,ndx) (vec.begin()+ndx)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define WAIT_KEYPRESS(msg) 		cout << msg << endl; getc(stdin)
//=============================================================================
#endif // UTILS


