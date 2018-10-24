/*
 *-----------------------------------------------------------------------------
 * PROJECT: gpsnif
 * PURPOSE: see module SendFrame.h file
 *-----------------------------------------------------------------------------
 */

#include "TXHFrame.h"
#include "common/utils/Utils.h"
#include "comm/SerCommTh.h"
#include "comm/SktCliCommTh.h"
#include "comm/SktSrvCommTh.h"
#include "comm/UDPCommTh.h"

extern Trace *dbg;
extern struct globals_st gd;
extern SerCommTh *serComm;
extern SktCliCommTh *sktCliComm;
extern SktSrvCommTh *sktSrvComm;
extern UDPCommTh *udpSrvComm;
extern CommTh *comm;

/**
 * ctor
 */
TXHFrame::TXHFrame()
{
}

/**
 * dtor
 */
TXHFrame::~TXHFrame()
{
}

/**
 * setup if serial o socket
 */
void TXHFrame::setCommInterface(CommTh** comm)
{
if(gd.useSerial)
	{
	this->commif=static_cast<CommTh*>(serComm);
	}
#ifdef USE_UDP_PROTOCOL
else if(gd.useSocketServer)
	{
	this->commif=static_cast<CommTh*>(udpSrvComm);
	}
#else
else if(gd.useSocketClient)
	{
	this->commif=static_cast<CommTh*>(sktCliComm);
	}
else if(gd.useSocketServer)
	{
	this->commif=static_cast<CommTh*>(sktSrvComm);
	}
#endif
*comm=this->commif;
}


/**
 * parse the string that describe a frame to be sent
 * @param fr frame in the format described here:
 * src.dst.cmd.data
 * where
 * typ type of frame [c|r|a]
 * src source ID, start with x for hex [0..255]
 * dst destination ID, start with x for hex [0..255]
 * cmd command, start with x for hex [0..255]
 * data x[...]d[...]s[string]; you MUST use [] to fill data
 * i.e.:
 * r.1.x12.x21.x[12fd].d[123]
 * r.s.d.3
 * r#3
 * R.3
 * the last two takes id from the gui
 * @param frameBuff frame to be sent
 * @return ok parse success
 */
bool TXHFrame::parseDescFrame(string &sfr)
{
char tmp[10];
bool ret=true;
string _sfr=sfr;
vector<string> tok;

int cbase=10;
int buff_ndx=0;
enum basenum_e {base_dec,base_hex,base_str} base=base_hex;

trim(_sfr);
if(_sfr.empty()) return 0;	// empty line
if(_sfr[0]=='#') return 0;	// comment
size_t pos=_sfr.find_first_of('#');
if(pos!=string::npos)	// has an inline comment
	{
	_sfr.resize(pos);
	}

Split(_sfr,tok,".[]");
type=tok[0][0];
switch(type)
	{
	case 'C': case 'c': f.hdr=HPROT_HDR_COMMAND; break;
	case 'R': case 'r': f.hdr=HPROT_HDR_REQUEST; break;
	case 'A': case 'a': f.hdr=HPROT_HDR_ANSWER; break;
	default:
		ret &= false;
		if(gd.useSerial)
			{
			commif->txResult.setError("bad frame type",0);
			}
		break;
	}

bool guidata=false;
if(isupper(tok[0][0])) guidata=true;
if(tok[0].length()>1)
	{
	if(tok[0][1]=='+') guidata=true;
	}

if(guidata || tok[1][0]=='s')
	{
	int num=gd.myID;
	if(num>=0)
		{
		f.srcID=(uint8_t)num;
		}
	else
		{
		ret &= false;
		commif->txResult.setError("bad source value",0);
		}
	}
else
	{
	if(tok[1][0]=='x')
		{
		tok[1].erase(0,1);
		cbase=16;
		}
	else
		{
		cbase=10;
		}
	f.srcID=(uint8_t)strtol(tok[1].c_str(),0,cbase);
	}

if(guidata || tok[2][0]=='d')
	{
	int num=gd.defaultDest;
	if(num>=0)
		{
		f.dstID=(uint8_t)num;
		}
	else
		{
		ret &= false;
		commif->txResult.setError("bad destination value",0);
		}
	}
else
	{
	if(tok[2][0]=='x')
		{
		tok[2].erase(0,1);
		cbase=16;
		}
	else
		{
		cbase=10;
		}
	f.dstID=(uint8_t)strtol(tok[2].c_str(),0,cbase);
	}

// handle following tokens
int ntok=0;
(guidata) ? (ntok=1) : (ntok=3);

// command
if(tok[ntok][0]=='x')
	{
	tok[ntok].erase(0,1);
	cbase=16;
	}
else
	{
	cbase=10;
	}
f.cmd=(uint8_t)strtol(tok[ntok].c_str(),0,cbase);

(guidata) ? (ntok=2) : (ntok=4);
if(tok.size()>ntok)	// has data
	{
	for(unsigned int i=ntok;i<tok.size();i++)
		{
		if(tok[i]=="i")
			{
			base=base_dec;
			continue;
			}
		else if(tok[i]=="x" || tok[i]=="h")
			{
			base=base_hex;
			continue;
			}
		else if(tok[i]=="s" || tok[i]=="t")
			{
			base=base_str;
			continue;
			}

		switch(base)
			{
			case base_dec:
				dbuff[buff_ndx++]=(uint8_t)atoi(tok[i].c_str());
				break;
			case base_hex:
				dbuff[buff_ndx++]=(uint8_t)strtol(tok[i].c_str(),0,16);
				break;
			case base_str:
				strcpy((char*)&dbuff[buff_ndx],tok[i].c_str());
				buff_ndx+=tok[i].length();
				break;
			}
		}
	}
f.len=buff_ndx;
f.data=dbuff;

return ret;
}

#if 1
/**
 *
 * @return
 */
bool TXHFrame::transmitFrame()
{
bool ret=true;
if(gd.connected)
	{
	commif->setMyId(f.srcID);
	switch(type)
		{
		case 'C':
		case 'c':
			commif->sendCommand(f.dstID, f.cmd, f.data, f.len);
			usleep(200000);
			break;
		case 'A':
		case 'a':
			commif->sendAnswer(f.dstID, f.cmd, f.data, f.len, f.num);
			usleep(200000);
			break;
		case 'R':
		case 'r':
//			//start(NULL);	// start the listener thread
//			//txResult.setStatus(Result::is_waiting);
//			if(gd.useSerial)
//				{
//				serComm->startThread(NULL,'t');
//				serComm->txResult.setStatus(Result::is_waiting);
//				serComm->sendRequest(f.dstID, f.cmd, f.data, f.len, 3);
//				// wait answer in the thread
//				}
//			else
//				{
//				commif->startThread(NULL,'t');
//				commif->txResult.setStatus(Result::is_waiting);
//				commif->sendRequest(f.dstID, f.cmd, f.data, f.len, 3);
//				}
			commif->startThread(NULL,'t');
			commif->txResult.setStatus(Result::is_waiting);
			commif->sendRequest(f.dstID, f.cmd, f.data, f.len, 3);
			break;
		}
	}
else
	{
	dbg->trace(DBG_ERROR, "cannot send frame, device disconnected?");
	}
return ret;
}
#endif

/**
 * is waiting an answer
 * @return
 */
bool TXHFrame::isWaiting()
{
return commif->txResult.isWaiting();
}

/**
 * if result is an error
 * @return
 */
bool TXHFrame::isError()
{
return commif->txResult.isError();
}

/**
 * if is ok
 * @return
 */
bool TXHFrame::isOk()
{
return commif->txResult.isOK();
}

/**
 * get the error message, if any
 * @return
 */
string TXHFrame::getErrorMessage()
{
return (commif->txResult.isError()) ? ((string)commif->txResult.message) : "no error";
}

/**
 * reset the result variable
 */
void TXHFrame::resetResult()
{
commif->txResult.reset();
}

