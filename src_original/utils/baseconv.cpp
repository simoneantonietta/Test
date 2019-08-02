//============================================================================
// Name        : baseconv.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#ifdef __cplusplus
//=============================================================================
// C++ version
//=============================================================================

#include "baseconv.h"

#define MIN_BASE 			2
#define MAX_BASE 			36

static bool tablesInitialised = false;
static char tblIntToChar[MAX_BASE] =
	{
	0
	};
static int tblCharToInt[256] =
	{
	0
	};

/**
 * initialize a table
 */
void InitTables()
{
if (tablesInitialised) return;

for (int i = 0; i < 10; i++)
	{
	tblIntToChar[i] = '0' + i;
	tblCharToInt[tblIntToChar[i]] = i;
	}

for (int i = 0; i < 26; i++)
	{
	tblIntToChar[i + 10] = 'a' + i;
	tblCharToInt['a' + i] = i + 10;
	tblCharToInt['A' + i] = i + 10;
	}

tablesInitialised = true;
}

/**
 * long add function
 */
string Add(const string& a, const string& b, int base = 10)
{
InitTables();
if (base > MAX_BASE || base < MIN_BASE) return "";

// Reserve storage for the result.
string result;
result.reserve(1 + std::max(a.size(), b.size()));

// Column positions and carry flag.
int apos = a.size();
int bpos = b.size();
int carry = 0;

// Do long arithmetic.
while (carry > 0 || apos > 0 || bpos > 0)
	{
	if (apos > 0) carry += tblCharToInt[(unsigned char) a[--apos]];
	if (bpos > 0) carry += tblCharToInt[(unsigned char) b[--bpos]];
	result.push_back(tblIntToChar[carry % base]);
	carry /= base;
	}

// The result string is backwards.  Reverse and return it.
reverse(result.begin(), result.end());
return result;
}

/**
 * Converts a single value to some base, intended for single-digit conversions.
 */
string AsBase(int number, int base)
{
InitTables();
if (number <= 0) return "0";
string result;
while (number > 0)
	{
	result += tblIntToChar[number % base];
	number /= base;
	}
reverse(result.begin(), result.end());
return result;
}

/**
 * Converts a number from one base to another.
 */
string ConvertBase(const string & number, int oldBase, int newBase)
{
InitTables();
string result;

for (unsigned digit = 0; digit < number.size(); digit++)
	{
	int value = tblCharToInt[(unsigned char) number[digit]];
	if (result.empty())
		{
		result = AsBase(value, newBase);
		}
	else
		{
		string temp = result;
		for (int i = 1; i < oldBase; i++)
			{
			temp = Add(result, temp, newBase);
			}
		result = Add(temp, AsBase(value, newBase), newBase);
		}
	}

return result;
}

#if 0
int main(void)
{
string num="ffffffffffffffffffff";
cout << "conversion: " << ConvertBase(num,16,10) << endl;
}
#endif

//=============================================================================
#else
//=============================================================================
// C version
//=============================================================================

#include "../WUtilities/baseconv.h"

#define MIN_BASE						2
#define MAX_BASE						36
#define CNV_MAX_DATA_LEN		32

static int tablesInitialised = 0;
static char tblIntToChar[MAX_BASE] =
	{
	0
	};
static int tblCharToInt[256] =
	{
	0
	};

/**
 * reverse string
 * @param str
 * @return
 */
char *strrev(char *str)
{
char *p1, *p2;

if (!str || !*str) return str;
for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
	{
	*p1 ^= *p2;
	*p2 ^= *p1;
	*p1 ^= *p2;
	}
return str;
}


/**
 * initialize a table
 */
void InitTables()
{
int i;
if (tablesInitialised) return;

for (i = 0; i < 10; i++)
	{
	tblIntToChar[i] = '0' + i;
	tblCharToInt[tblIntToChar[i]] = i;
	}

for (i = 0; i < 26; i++)
	{
	tblIntToChar[i + 10] = 'a' + i;
	tblCharToInt['a' + i] = i + 10;
	tblCharToInt['A' + i] = i + 10;
	}

tablesInitialised = 1;
}

/**
 * long add function
 */
void Add(const char* a, const char* b, int base, char *result)
{
int i=0;
InitTables();
if (base > MAX_BASE || base < MIN_BASE)
	{
	*result=0;
	return;
	}

// Column positions and carry flag.
int apos = strlen(a);
int bpos = strlen(b);
int carry = 0;

// Do long arithmetic.
while (carry > 0 || apos > 0 || bpos > 0)
	{
	if (apos > 0) carry += tblCharToInt[(unsigned char) a[--apos]];
	if (bpos > 0) carry += tblCharToInt[(unsigned char) b[--bpos]];
	result[i++]=tblIntToChar[carry % base];
	carry /= base;
	}
result[i]=0;	// terminate the string
// The result string is backwards.  Reverse and return it.
strrev(result);
}

/**
 * Converts a single value to some base, intended for single-digit conversions.
 */
void AsBase(int number, int base, char *result)
{
int i=0;
InitTables();
if (number <= 0)
	{
	*result=0;
	return;
	}
while (number > 0)
	{
	result[i++] = tblIntToChar[number % base];
	number /= base;
	}
result[i]=0;	// terminate the string
// The result string is backwards.  Reverse and return it.
strrev(result);
}

/**
 * Converts a number from one base to another.
 */
void ConvertBase(const char* number, int oldBase, int newBase, char *result)
{
int i;
unsigned digit;
char temp1[CNV_MAX_DATA_LEN],temp2[CNV_MAX_DATA_LEN];

InitTables();

for (digit = 0; digit < strlen(number); digit++)
	{
	int value = tblCharToInt[(unsigned char) number[digit]];
	if(strlen(result)==0)
		{
		AsBase(value, newBase,result);
		}
	else
		{
		strcpy(temp1,result);
		for (i = 1; i < oldBase; i++)
			{
			Add(result, temp1, newBase,temp2);
			strcpy(temp1,temp2);
			}
		AsBase(value, newBase, temp2);
		Add(temp1,temp2 , newBase,result);
		}
	}
}

#if 0
int main(void)
{
char num[]="ffffffffffffffffffff";
char c[32];
ConvertBase(num,16,10,c);
printf("conversion: %s\n", c);
return 0;
}
#endif
//=============================================================================

#endif
