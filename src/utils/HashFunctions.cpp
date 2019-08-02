/*
 *-----------------------------------------------------------------------------
 * PROJECT: WUtilities
 * PURPOSE: see module GeneralHashFunctions.h file
 *-----------------------------------------------------------------------------
 */
#include "HashFunctions.h"

hashValue_t RSHash(const std::string& str)
{
   hashValue_t b    = 378551;
   hashValue_t a    = 63689;
   hashValue_t hash = 0;

   for(std::size_t i = 0; i < str.length(); i++)
   {
      hash = hash * a + str[i];
      a    = a * b;
   }

   return hash;
}
/* End Of RS Hash Function */


hashValue_t JSHash(const std::string& str)
{
   hashValue_t hash = 1315423911;

   for(std::size_t i = 0; i < str.length(); i++)
   {
      hash ^= ((hash << 5) + str[i] + (hash >> 2));
   }

   return hash;
}
/* End Of JS Hash Function */


hashValue_t PJWHash(const std::string& str)
{
   hashValue_t BitsInUnsignedInt = (hashValue_t)(sizeof(hashValue_t) * 8);
   hashValue_t ThreeQuarters     = (hashValue_t)((BitsInUnsignedInt  * 3) / 4);
   hashValue_t OneEighth         = (hashValue_t)(BitsInUnsignedInt / 8);
   hashValue_t HighBits          = (hashValue_t)(0xFFFFFFFF) << (BitsInUnsignedInt - OneEighth);
   hashValue_t hash              = 0;
   hashValue_t test              = 0;

   for(std::size_t i = 0; i < str.length(); i++)
   {
      hash = (hash << OneEighth) + str[i];

      if((test = hash & HighBits)  != 0)
      {
         hash = (( hash ^ (test >> ThreeQuarters)) & (~HighBits));
      }
   }

   return hash;
}
/* End Of  P. J. Weinberger Hash Function */


hashValue_t ELFHash(const std::string& str)
{
   hashValue_t hash = 0;
   hashValue_t x    = 0;

   for(std::size_t i = 0; i < str.length(); i++)
   {
      hash = (hash << 4) + str[i];
      if((x = hash & 0xF0000000L) != 0)
      {
         hash ^= (x >> 24);
      }
      hash &= ~x;
   }

   return hash;
}
/* End Of ELF Hash Function */


hashValue_t BKDRHash(const std::string& str)
{
   hashValue_t seed = 131; // 31 131 1313 13131 131313 etc..
   hashValue_t hash = 0;

   for(std::size_t i = 0; i < str.length(); i++)
   {
      hash = (hash * seed) + str[i];
   }

   return hash;
}
/* End Of BKDR Hash Function */


hashValue_t SDBMHash(const std::string& str)
{
   hashValue_t hash = 0;

   for(std::size_t i = 0; i < str.length(); i++)
   {
      hash = str[i] + (hash << 6) + (hash << 16) - hash;
   }

   return hash;
}
/* End Of SDBM Hash Function */


hashValue_t DJBHash(const std::string& str)
{
   hashValue_t hash = 5381;

   for(std::size_t i = 0; i < str.length(); i++)
   {
      hash = ((hash << 5) + hash) + str[i];
   }

   return hash;
}
/* End Of DJB Hash Function */


hashValue_t DEKHash(const std::string& str)
{
   hashValue_t hash = static_cast<hashValue_t>(str.length());

   for(std::size_t i = 0; i < str.length(); i++)
   {
      hash = ((hash << 5) ^ (hash >> 27)) ^ str[i];
   }

   return hash;
}
/* End Of DEK Hash Function */


hashValue_t BPHash(const std::string& str)
{
   hashValue_t hash = 0;
   for(std::size_t i = 0; i < str.length(); i++)
   {
      hash = hash << 7 ^ str[i];
   }

   return hash;
}
/* End Of BP Hash Function */


hashValue_t FNVHash(const std::string& str)
{
   const hashValue_t fnv_prime = 0x811C9DC5;
   hashValue_t hash = 0;
   for(std::size_t i = 0; i < str.length(); i++)
   {
      hash *= fnv_prime;
      hash ^= str[i];
   }

   return hash;
}
/* End Of FNV Hash Function */


hashValue_t APHash(const std::string& str)
{
   hashValue_t hash = 0xAAAAAAAA;

   for(std::size_t i = 0; i < str.length(); i++)
   {
      hash ^= ((i & 1) == 0) ? (  (hash <<  7) ^ str[i] * (hash >> 3)) :
                               (~((hash << 11) + (str[i] ^ (hash >> 5))));
   }

   return hash;
}
/* End Of AP Hash Function */

//-----------------------------------------------------------------------------
// BYTE ORIENTED
//-----------------------------------------------------------------------------

hashValue_t RSHash(char* str, unsigned int len)
{
   hashValue_t b    = 378551;
   hashValue_t a    = 63689;
   hashValue_t hash = 0;
   hashValue_t i    = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash = hash * a + (*str);
      a    = a * b;
   }

   return hash;
}
/* End Of RS Hash Function */


hashValue_t JSHash(char* str, unsigned int len)
{
   hashValue_t hash = 1315423911;
   hashValue_t i    = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash ^= ((hash << 5) + (*str) + (hash >> 2));
   }

   return hash;
}
/* End Of JS Hash Function */


hashValue_t PJWHash(char* str, unsigned int len)
{
   const hashValue_t BitsInUnsignedInt = (hashValue_t)(sizeof(hashValue_t) * 8);
   const hashValue_t ThreeQuarters     = (hashValue_t)((BitsInUnsignedInt  * 3) / 4);
   const hashValue_t OneEighth         = (hashValue_t)(BitsInUnsignedInt / 8);
   const hashValue_t HighBits          = (hashValue_t)(0xFFFFFFFF) << (BitsInUnsignedInt - OneEighth);
   hashValue_t hash              = 0;
   hashValue_t test              = 0;
   hashValue_t i                 = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash = (hash << OneEighth) + (*str);

      if((test = hash & HighBits)  != 0)
      {
         hash = (( hash ^ (test >> ThreeQuarters)) & (~HighBits));
      }
   }

   return hash;
}
/* End Of  P. J. Weinberger Hash Function */


hashValue_t ELFHash(char* str, unsigned int len)
{
   hashValue_t hash = 0;
   hashValue_t x    = 0;
   hashValue_t i    = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash = (hash << 4) + (*str);
      if((x = hash & 0xF0000000L) != 0)
      {
         hash ^= (x >> 24);
      }
      hash &= ~x;
   }

   return hash;
}
/* End Of ELF Hash Function */


hashValue_t BKDRHash(char* str, unsigned int len)
{
   hashValue_t seed = 131; /* 31 131 1313 13131 131313 etc.. */
   hashValue_t hash = 0;
   hashValue_t i    = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash = (hash * seed) + (*str);
   }

   return hash;
}
/* End Of BKDR Hash Function */


hashValue_t SDBMHash(char* str, unsigned int len)
{
   hashValue_t hash = 0;
   hashValue_t i    = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash = (*str) + (hash << 6) + (hash << 16) - hash;
   }

   return hash;
}
/* End Of SDBM Hash Function */


hashValue_t DJBHash(char* str, unsigned int len)
{
   hashValue_t hash = 5381;
   hashValue_t i    = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash = ((hash << 5) + hash) + (*str);
   }

   return hash;
}
/* End Of DJB Hash Function */


hashValue_t DEKHash(char* str, unsigned int len)
{
   hashValue_t hash = len;
   hashValue_t i    = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash = ((hash << 5) ^ (hash >> 27)) ^ (*str);
   }
   return hash;
}
/* End Of DEK Hash Function */


hashValue_t BPHash(char* str, unsigned int len)
{
   hashValue_t hash = 0;
   hashValue_t i    = 0;
   for(i = 0; i < len; str++, i++)
   {
      hash = hash << 7 ^ (*str);
   }

   return hash;
}
/* End Of BP Hash Function */


hashValue_t FNVHash(char* str, unsigned int len)
{
   const hashValue_t fnv_prime = 0x811C9DC5;
   hashValue_t hash      = 0;
   hashValue_t i         = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash *= fnv_prime;
      hash ^= (*str);
   }

   return hash;
}
/* End Of FNV Hash Function */


hashValue_t APHash(char* str, unsigned int len)
{
   hashValue_t hash = 0xAAAAAAAA;
   hashValue_t i    = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash ^= ((i & 1) == 0) ? (  (hash <<  7) ^ (*str) * (hash >> 3)) :
                               (~((hash << 11) + ((*str) ^ (hash >> 5))));
   }

   return hash;
}
/* End Of AP Hash Function */
