/**----------------------------------------------------------------------------
 * PROJECT:
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

#define APPLICATION_NAME				"varcosv-saetcom"

#define VERSION_MAJOR						"00"
#define VERSION_MINOR						"01"
#define VERSION_MAIN            VERSION_MAJOR "." VERSION_MINOR
#define VERSION_BUILDNUMBER     "0280"
#define VERSION                 VERSION_MAIN " build " VERSION_BUILDNUMBER
#define VERSION_NOSPACE         VERSION_MAIN "build" VERSION_BUILDNUMBER

#define NUM_VERSION_MAJOR				(uint8_t)atoi(VERSION_MAJOR)
#define NUM_VERSION_MINOR				(uint8_t)atoi(VERSION_MINOR)
#define NUM_VERSION_BUILD				(uint16_t)atoi(VERSION_BUILDNUMBER)

#define VER_MAJ_INT atoi(VERSION_MAJOR)
#define VER_MIN_INT atoi(VERSION_MINOR)

/* MACRO to write version into the code */
#define SET_FW_VERSION()				const volatile char __fwversion__[]="fw_version[" VERSION_MAJOR "." VERSION_MINOR "]"
#define SET_FW_NAME()						const volatile char __fwname__[]="fw_name[" APPLICATION_NAME "]"
//-----------------------------------------------
#endif /* VERSION_H_ */
