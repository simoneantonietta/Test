/**----------------------------------------------------------------------------
 * PROJECT: WUtilities
 * PURPOSE:
 * conversion of base of arbitrary strings length
 * C ans C++ version
 *-----------------------------------------------------------------------------  
 * CREATION: Jun 25, 2014
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

#ifndef BASECONV_H_
#define BASECONV_H_

#ifdef __cplusplus
#include <string>
#include <iostream>
#include <algorithm>
using namespace std;
string ConvertBase(const string & number, int oldBase, int newBase);
#else
#include <string.h>
void ConvertBase(const char* number, int oldBase, int newBase, char *result);
#endif


//-----------------------------------------------
#endif /* BASECONV_H_ */
