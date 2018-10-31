/*
 *-----------------------------------------------------------------------------
 * PROJECT: hpsnif
 * PURPOSE: see module HandleFrame.h file
 *-----------------------------------------------------------------------------
 */

#include "RXHFrame.h"
#include "common/prot6/hprot.h"
#include "common/prot6/hprot_stdcmd.h"
#include "common/utils/Utils.h"
#include "common/utils/Trace.h"

#define TO_UCHAR(x) ((int)x & 0xFF)

extern Trace *dbg;
extern struct globals_st gd;

/**
 * ctor
 */
RXHFrame::RXHFrame()
{
parseEnable=false;
parse_stdcmd_f=false;
hideCrc_f=false;
enableLog=false;
onlyDebugMessages=false;
}

/**
 * dtor
 */
RXHFrame::~RXHFrame()
{
}

///**
// * setup if serial o socket
// */
//void RXHFrame::setCommInterface()
//{
//if(gd.serial_flag)
//	{
//	comm=static_cast<SerCommTh*>(serComm);
//	}
//else
//	{
//	comm=static_cast<SktCommTh*>(sktComm);
//	}
//}

/**
 * pretty printer
 * @param frmdir frame direction can be 't' or 'T' for TX and r or R for RX; '-' is not considered
 * @param buff
 */
void RXHFrame::printFrame(char frmdir, uint8_t * buff, int size,int fr_error)
{
#if 0
//char dir[4];
char tmp[100];
char hex[HPROT_BUFFER_SIZE*2];

char startColor[20]=TXT_COLOR_DEFAULT;
string strdmp;

if(!penable) return;
if(size==0) return;

/*
 * check if need to exe the script
 * it uses the same logic of the filter
 */
for(int i=0;i<WP_MAX_SCRIPTS;i++)
	{
	wp->execute(i,buff,size);
	}

// calculates filter for print log
if(getfilterEnable())
	{
	if(!ffil.calcFilter(buff,size)) return;
	}

// log is for filtered version
if(!logToFile(buff,size))
	{
	dbg->trace(DBG_ERROR,_S __FUNCTION__ + ": log error");
	}

//dir[0]=0;
str[0]=0;

if(timestamp_f)
	{
	strcat(str,getTimestampMS().c_str());
	strcat(str,"]");
	}

bool isDebug=false;
bool isBroadcast=false;
bool isMappedMsg=false;
bool isVarMsg=false;
char ftype='U';

if(parseEnable)
	{
	// HDR|SRC|DST|NUM|CMD|LEN|data|CRC8

	// check destination kind
	switch(TO_UCHAR(buff[2]))
	{
		case HPROT_DEBUG_ID:
			isDebug=true;
			break;
		case HPROT_BROADCAST_ID:
			isBroadcast=true;
			break;

		default:
			break;
	}

	// header
	if(fr_error == pc_error_crc || fr_error == pc_error_cmd)
		{
		strcpy(startColor,TXT_COLOR_ERROR);
		}
	else if(isDebug)
		{
		strcpy(startColor,TXT_COLOR_DEBUG);
		}
	else if(isBroadcast)
		{
		strcpy(startColor,TXT_COLOR_BROADCAST);
		}
	else
		{
		switch(buff[0])
			{
			case HPROT_HDR_COMMAND:
				strcpy(startColor,TXT_COLOR_COMMAND);
				ftype='C';
				break;
			case HPROT_HDR_REQUEST:
				strcpy(startColor,TXT_COLOR_REQUEST);
				ftype='R';
				break;
			case HPROT_HDR_ANSWER:
				strcpy(startColor,TXT_COLOR_ANSWER);
				ftype='A';
				break;
			}
		}
	sprintf(tmp,"%s %c ",startColor,ftype);
	strcat(str,tmp);
	// source
	sprintf(tmp,"%03d->",TO_UCHAR(buff[1]));
	strcat(str,tmp);
	// destination
	if(isDebug)
		{
		strcat(str,"DBG");
		}
	else if(isBroadcast)
		{
		strcat(str,"ALL");
		}
	else
		{
		if(buff[2]==HPROT_INVALID_ID)
			{
			sprintf(tmp,"%s%03d",TXT_COLOR_ERROR,TO_UCHAR(buff[2]));
			}
		else
			{
			sprintf(tmp,"%03d",TO_UCHAR(buff[2]));
			}
		strcat(str,tmp);
		}
	// number
	sprintf(tmp," (N:%3d)",buff[3]);
	strcat(str,tmp);
	// command
	if(parse_stdcmd_f)
		{
		switch((int)buff[4] & 0xFF)
			{
			case HPROT_CMD_NONE:
				sprintf(tmp," %sC:<bad cmd (0)>",TXT_COLOR_ERROR);
				strcat(str,tmp);
				break;
			case HPROT_CMD_CHKLNK:
				sprintf(tmp," C:cklnk");
				strcat(str,tmp);
				break;
			case HPROT_CMD_ACK:
				sprintf(tmp," C:ack  ");
				strcat(str,tmp);
				break;
			case HPROT_CMD_NACK:
				sprintf(tmp," C:nack ");
				strcat(str,tmp);
				break;
			case HPROT_CMD_DEBUG_MSG:
				sprintf(tmp," C:dbg  ");
				strcat(str,tmp);
				break;
			case HPROT_CMD_DEBUG_MAPPED_MSG:
				isMappedMsg=true;
				break;
			case HPROT_CMD_DEBUG_VALUES_MSG:
				isVarMsg=true;
				break;
			default:
				sprintf(tmp," C:%02X   ",TO_UCHAR(buff[4]));
				strcat(str,tmp);
				break;
			}
		}
	else
		{
		sprintf(tmp," C:%02X",TO_UCHAR(buff[4]));
		strcat(str,tmp);
		}
	// length
	if(!isDebug)
		{
		sprintf(tmp," L:%*d",3,buff[5]);
		strcat(str,tmp);
		}

	// data
	int len=buff[5];
	if(len>0)
		{
		strcat(str," ");
		// data is for debug purpose
		if(isDebug)
			{
			if(isMappedMsg)
				{
				// get the number (16bit) and retrieve the mapped string
				uint16_t num=*(uint16_t*)&buff[6];
				sprintf(tmp,"(%04X) ",(int)num);
				strcat(str, tmp);
				if(messageMap.count(num)>0)	// if the message is mapped
					{
					strcat(str, messageMap[num].c_str());
					}
				}
			else if(isVarMsg)
				{
				strcpy(strVar,"%%v");
				strcat(strVar,parseDbgVarFrame(&buff[6],len).c_str());
				}
			else
				{
				// print as is because is a simple string
				buff[6+len]=0;
				strcat(str,(char *)&buff[6]);
				}
			}
		// real data
		else
			{
			if(len>dumpTrigger)
				{
				hexdumpStr(strdmp,&buff[6],len);
				for(unsigned long int c=0;c<strlen((const char*)strdmp.c_str());c++)
					{
					// search for special chars
					if(strdmp[c]=='$')
						{
						strdmp[c]='.';
						}
					}
				strcat(str,"\r\n");
				strcat(str,strdmp.c_str());
				}
			else
				{
				Hex2AsciiHex(hex,(unsigned char*)&buff[6],len,false);
				strcat(str,hex);
				}
			}
		}
	// crc
	switch(fr_error)
		{
		case pc_error_crc:
			stat_crcError++;
			dbg->trace(DBG_DEBUG,"found protocol error (CRC)");
			sprintf(tmp,"protocol error: CRC [%u]",stat_crcError);
			guiMain->setStatusBarInfo('E',tmp);
			sprintf(tmp,TXT_COLOR_CRC " CRC?[%02X]",TO_UCHAR((buff[6]+len)));
			strcat(str,tmp);
			break;
		case pc_error_cmd:
			stat_cmdError++;
			dbg->trace(DBG_DEBUG,"found protocol error (CMD)");
			sprintf(tmp,"protocol error: CMD [%u]",stat_cmdError);
			guiMain->setStatusBarInfo('E',tmp);
			strcat(str," <CMD unk>");
			break;

		default:
			if(!hideCrc_f)
				{
				if(!isDebug)
					{
					sprintf(tmp,TXT_COLOR_CRC " [%02X]",TO_UCHAR((buff[6]+len)));
					strcat(str,tmp);
					}
				}
			break;
		}
	strcat(str,TXT_COLOR_DEFAULT);
	strcat(str,"\n");
	}
else
	{
	// todo not parsed version
	}
//------------------
// output!
if(isVarMsg)
	{
	guiMain->printLog(strVar);
	}
else
	{
	if(onlyDebugMessages)
		{
		if(isDebug) guiMain->printLog(str);
		}
	else
		{
		guiMain->printLog(str);
		}
	}
#endif
}

/**
 * print a frame into a queue (binary)
 * @param dir
 * @param buff
 * @param size
 */
void RXHFrame::putQueue(char dir,uint8_t *buff, int size, int fr_error)
{
qdata_t q;

if(!penable) return;
if(size <= HPROT_BUFFER_SIZE)
	{
	q.dir=dir;
	q.size=size;
	q.fr_error=fr_error;
	memcpy(q.data,buff,size);

	if(qframes.size()<ABSOLUTE_MAX_QUEUE) qframes.push(q);
	}
else
	{
	dbg->trace(DBG_ERROR,"frame length error [%d]",size);
	}
}

/**
 * get data frame form the queue
 * @param q queue elemente
 */
void RXHFrame::getQueue(qdata_t *q)
{
*q=qframes.front();
qframes.pop();
}

/**
 * if enabled, log data to file
 * @param buff
 * @param size
 * @param txtflag to indicates that buff contai a string and size will be ignored (default =false)
 * @return true: ok
 */
bool RXHFrame::logToFile(uint8_t *buff, int size, bool txt)
{
if(enableLog)
	{
	if(txt)
		{
		return saveLogFile(logFilename,(const char*)buff);
		}
	else
		{
		char hex[HPROT_BUFFER_SIZE*2+20];
		Hex2AsciiHex(hex,(unsigned char*)buff,size,false,0);
		return saveLogFile(logFilename,hex);
		}
	}
return true;
}

/**
 * load the message map
 * @param fname
 * @return true:ok
 */
bool RXHFrame::loadDbgMessageMap(string fname)
{
bool ret=true;
ifstream f;
string line;
int nline=0;
vector<string> toks;

messageMap.clear();
f.open(fname.c_str());
if(f.is_open())
	{
	// read the file
	while (!f.eof())
		{
		toks.clear();
		nline++;
		getline(f, line);
		trim(line);
		if(line.empty()) continue;
		if(line[0]=='#') continue;
		Split(line,toks,":");
		if(toks.size()==2)
			{
			int num=to_number<int>(trim(toks[0]));
			messageMap[num]=trim(toks[1]);
			}
		else
			{
			dbg->trace(DBG_ERROR,"message map file error at line %d",nline);
			ret=false;
			}
		}
	}
else
	{
	dbg->trace(DBG_WARNING,"no message map file found: " + fname);
	ret=false;
	}
dbg->trace(DBG_NOTIFY,"loaded %d debug messages",messageMap.size());
return ret;
}

/**
 * load the variable map
 * @param fname
 * @return true:ok
 */
bool RXHFrame::loadDbgVariableMap(string fname)
{
bool ret=true;
ifstream f;
string line;
int nline=0;
vector<string> toks;

variableMap.clear();
f.open(fname.c_str());
if(f.is_open())
	{
	// read the file
	while (!f.eof())
		{
		toks.clear();
		nline++;
		getline(f, line);
		trim(line);
		if(line.empty()) continue;
		if(line[0]=='#') continue;
		Split(line,toks,":");
		if(toks.size()==2)
			{
			int num=to_number<int>(trim(toks[0]));
			variableMap[num]=trim(toks[1]);
			}
		else
			{
			dbg->trace(DBG_ERROR,"variable map file error at line %d",nline);
			ret=false;
			}
		}
	}
else
	{
	dbg->trace(DBG_WARNING,"no variable map file found: " + fname);
	ret=false;
	}
dbg->trace(DBG_NOTIFY,"loaded %d debug variables",messageMap.size());
return ret;
}

//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------

/**
 * return the timestamp string in the format: YMD:HMS
 *
 * @return timestamp string
 */
string RXHFrame::getTimestamp()
{
time_t rawtime;
struct tm * timeinfo;
char buffer [80];

time ( &rawtime );
timeinfo = localtime ( &rawtime );

strftime (buffer,80,"%y%m%d:%H%M%S",timeinfo);

string ret(buffer);
return(ret);
}

/**
 * return the timepstamp in ms
 */
string RXHFrame::getTimestampMS(bool initFlag)
{
struct timeval te;
char tmp[20];
gettimeofday(&te, NULL);  // get current time
long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;  // calculate milliseconds
if(initFlag)
	milliseconds_init=milliseconds;
//printf("milliseconds: %lld\n", milliseconds);
sprintf(tmp,"%010lld",milliseconds-milliseconds_init);
string ret(tmp);
return ret;
}

/**
 * parse a databuffer for the debug variables and compose the string to ve visualized
 * @param databuf binary data buffer in the format VarndxTypeValue...
 * @param size databuf size in bytes
 * @return string with variables values in the format 'varname=value'
 */
string RXHFrame::parseDbgVarFrame(uint8_t *databuf, int size)
{
bool res=true;
int ndx=0;
string varstr="";
char _tmp[50];
int vndx;
char type;
value_t val;

while(ndx<size)
	{
	// variable
	vndx=databuf[ndx++];
	if(variableMap.count(vndx)>0)	// variable mapped
		{
		varstr += variableMap[vndx]+"=";
		}
	else	// variable not mapped
		{
		sprintf(_tmp,"(%02X)=",vndx);
		varstr += _tmp;
		}

	// type
	type=databuf[ndx++];
	switch(type)
		{
		case 'b':
			val.sbValue=(int8_t)databuf[ndx];
			sprintf(_tmp,"%i",val.sbValue);
			ndx++;
			break;
		case 'B':
			val.ubValue=(uint8_t)databuf[ndx];
			sprintf(_tmp,"%u",val.ubValue);
			ndx++;
			break;
		case 'w':
			val.swValue=*(int16_t*)&databuf[ndx];
			sprintf(_tmp,"%i",val.swValue);
			ndx += sizeof(int16_t);
			break;
		case 'W':
			val.uwValue=*(uint16_t*)&databuf[ndx];
			sprintf(_tmp,"%u",val.uwValue);
			ndx += sizeof(uint16_t);
			break;
		case 'i':
			val.siValue=*(int32_t*)&databuf[ndx];
			sprintf(_tmp,"%i",val.siValue);
			ndx += sizeof(int32_t);
			break;
		case 'I':
			val.uiValue=*(uint32_t*)&databuf[ndx];
			sprintf(_tmp,"%u",val.uiValue);
			ndx += sizeof(uint32_t);
			break;
		case 'f':
			val.fValue=*(float*)&databuf[ndx];
			sprintf(_tmp,"%f",val.fValue);
			ndx += sizeof(float);
			break;
		case 'd':
			val.dValue=*(double*)&databuf[ndx];
			sprintf(_tmp,"%f",val.dValue);
			ndx += sizeof(double);
			break;

		default:
			dbg->trace(DBG_ERROR,"invalid type specifier for variable (%d)", vndx);
			res=false;
			break;
		}
	// write data
	if(res)
		{
		varstr += _S _tmp + ";";
		}
	else
		{
		varstr = "ERROR: invalid frame format";
		break;
		}
	}
return varstr;
}

