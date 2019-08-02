/*
 **************************************************************************
 *                                                                        *
 *          General Purpose Hash Function Algorithms Library              *
 *                                                                        *
 * Author: Arash Partow - 2002                                            *
 * URL: http://www.partow.net                                             *
 * URL: http://www.partow.net/programming/hashfunctions/index.html        *
 *                                                                        *
 * Copyright notice:                                                      *
 * Free use of the General Purpose Hash Function Algorithms Library is    *
 * permitted under the guidelines and in accordance with the most current *
 * version of the Common Public License.                                  *
 * http://www.opensource.org/licenses/cpl1.0.php                          *
 *                                                                        *
 **************************************************************************
*/

#ifndef INCLUDE_GENERALHASHFUNCTION_CPP_H
#define INCLUDE_GENERALHASHFUNCTION_CPP_H

#include <string>

/*
 The General Hash Functions Library has the following mix of additive and rotative general purpose string hashing algorithms. The following algorithms vary in usefulness and functionality and are mainly intended as an example for learning how hash functions operate and what they basically look like in code form.

RS Hash Function
-----------------
A simple hash function from Robert Sedgwicks Algorithms in C book. I've added some simple optimizations to the algorithm in order to speed up its hashing process.

JS Hash Function
-----------------
A bitwise hash function written by Justin Sobel

PJW Hash Function
-----------------
This hash algorithm is based on work by Peter J. Weinberger of AT&T Bell Labs. The book Compilers (Principles, Techniques and Tools) by Aho, Sethi and Ulman,
recommends the use of hash functions that employ the hashing methodology found in this particular algorithm.

ELF Hash Function
-----------------
Similar to the PJW Hash function, but tweaked for 32-bit processors. Its the hash function widely used on most UNIX systems.

BKDR Hash Function
-----------------
This hash function comes from Brian Kernighan and Dennis Ritchie's book "The C Programming Language". It is a simple hash function using a strange set of possible seeds which all constitute a pattern of 31....31...31 etc, it seems to be very similar to the DJB hash function.

SDBM Hash Function
-----------------
This is the algorithm of choice which is used in the open source SDBM project. The hash function seems to have a good over-all distribution for many different data sets. It seems to work well in situations where there is a high variance in the MSBs of the elements in a data set.

DJB Hash Function
-----------------
An algorithm produced by Professor Daniel J. Bernstein and shown first to the world on the usenet newsgroup comp.lang.c. It is one of the most efficient hash functions ever published.

DEK Hash Function
-----------------
An algorithm proposed by Donald E. Knuth in The Art Of Computer Programming Volume 3, under the topic of sorting and search chapter 6.4.

AP Hash Function
-----------------
An algorithm produced by me Arash Partow. I took ideas from all of the above hash functions making a hybrid rotative and additive hash function algorithm. There isn't any real mathematical analysis explaining why one should use this hash function instead of the others described above other than the fact that I tired to resemble the design as close as possible to a simple LFSR. An empirical result which demonstrated the distributive abilities of the hash algorithm was obtained using a hash-table with 100003 buckets, hashing The Project Gutenberg Etext of Webster's Unabridged Dictionary, the longest encountered chain length was 7, the average chain length was 2, the number of empty buckets was 4579. Below is a simple algebraic description of the AP hash function:

Algebraic Description of AP Hash Function - Copyright Arash Partow

Note: For uses where high throughput is a requirement for computing hashes using the algorithms described above, one should consider unrolling the internal loops and adjusting the hash value memory foot-print to be appropriate for the targeted architecture(s). 

//-----------------------------------------
// EXAMPLE OF USAGE
//-----------------------------------------
#include <iostream>
#include <string>
#include "GeneralHashFunctions.h"

int main(int argc, char* argv[])
{
   std::string key = "abcdefghijklmnopqrstuvwxyz1234567890";

   std::cout << "General Purpose Hash Function Algorithms Test" << std::endl;
   std::cout << "By Arash Partow - 2002        " << std::endl;
   std::cout << "Key: "                          << key           << std::endl;
   std::cout << " 1. RS-Hash Function Value:   " << RSHash(key)   << std::endl;
   std::cout << " 2. JS-Hash Function Value:   " << JSHash(key)   << std::endl;
   std::cout << " 3. PJW-Hash Function Value:  " << PJWHash(key)  << std::endl;
   std::cout << " 4. ELF-Hash Function Value:  " << ELFHash(key)  << std::endl;
   std::cout << " 5. BKDR-Hash Function Value: " << BKDRHash(key) << std::endl;
   std::cout << " 6. SDBM-Hash Function Value: " << SDBMHash(key) << std::endl;
   std::cout << " 7. DJB-Hash Function Value:  " << DJBHash(key)  << std::endl;
   std::cout << " 8. DEK-Hash Function Value:  " << DEKHash(key)  << std::endl;
   std::cout << " 9. FNV-Hash Function Value:  " << FNVHash(key)  << std::endl;
   std::cout << "10. BP-Hash Function Value:   " << BPHash(key)   << std::endl;
   std::cout << "11. AP-Hash Function Value:   " << APHash(key)   << std::endl;

   return true;
}
//-----------------------------------------
*/

// STRING ORIENTED
typedef unsigned int hashValue_t;
typedef hashValue_t (*HashFunction)(const std::string&);


hashValue_t RSHash  (const std::string& str);
hashValue_t JSHash  (const std::string& str);
hashValue_t PJWHash (const std::string& str);
hashValue_t ELFHash (const std::string& str);
hashValue_t BKDRHash(const std::string& str);
hashValue_t SDBMHash(const std::string& str);
hashValue_t DJBHash (const std::string& str);
hashValue_t DEKHash (const std::string& str);
hashValue_t BPHash  (const std::string& str);
hashValue_t FNVHash (const std::string& str);
hashValue_t APHash  (const std::string& str);

// BYTE ORIENTED
typedef unsigned int (*hash_function)(char*, unsigned int len);

hashValue_t RSHash  (char* str, hashValue_t len);
hashValue_t JSHash  (char* str, hashValue_t len);
hashValue_t PJWHash (char* str, hashValue_t len);
hashValue_t ELFHash (char* str, hashValue_t len);
hashValue_t BKDRHash(char* str, hashValue_t len);
hashValue_t SDBMHash(char* str, hashValue_t len);
hashValue_t DJBHash (char* str, hashValue_t len);
hashValue_t DEKHash (char* str, hashValue_t len);
hashValue_t BPHash  (char* str, hashValue_t len);
hashValue_t FNVHash (char* str, hashValue_t len);
hashValue_t APHash  (char* str, hashValue_t len);

#endif
