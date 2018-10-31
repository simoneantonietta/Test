/**----------------------------------------------------------------------------
 * PROJECT: keyboard_driver
 * PURPOSE:
 * set the software version
 *
 * CREATION: Feb 2, 2012
 * Author: Luca Mini
 *
 * LICENCE: please see LICENCE.TXT file
 *
 * HISTORY (of the module):
 *-----------------------------------------------------------------------------
 * Author              | Date        | Description
 *-----------------------------------------------------------------------------
 * L. Mini               21/03/2012    added protocol debug inside (first implementation)
 *                                     bugfix jackinterface
 *
 *-----------------------------------------------------------------------------
 */

#ifndef VERSION_H_
#define VERSION_H_

#define VERSION_MAJOR						"1"
#define VERSION_MINOR						"0"
#define VERSION_MAIN            VERSION_MAJOR "." VERSION_MINOR
#define VERSION_BUILDNUMBER     "0043"
#define VERSION                 VERSION_MAIN " build " VERSION_BUILDNUMBER
#define VERSION_NOSPACE         VERSION_MAIN "build" VERSION_BUILDNUMBER

#define NUM_VERSION_MAJOR				(uint8_t)atoi(VERSION_MAJOR)
#define NUM_VERSION_MINOR				(uint8_t)atoi(VERSION_MINOR)
#define NUM_VERSION_BUILD				(uint16_t)atoi(VERSION_BUILDNUMBER)

//-----------------------------------------------
#endif /* VERSION_H_ */
