/*
 *-----------------------------------------------------------------------------
 * PROJECT: rasp_varcosv
 * PURPOSE: see module MasterInterface.h file
 *-----------------------------------------------------------------------------
 */

#include "MasterInterface.h"
#include "db/DBWrapper.h"
#include "HardwareGPIO.h"

#define MI_HPROT_REQ_TIMEOUT_DEFAULT			1000
#define NOT_RESPOND_COUNTER_MAX 10
//=============================================================================
extern DBWrapper *db;
extern UDPProtocolDebug *udpDbg;
extern HardwareGPIO *hwgpio;

int notRespondCounter;
//=============================================================================
/**
 * ctor
 */
MasterInterface::MasterInterface()
{
opened = false;
connected = false;
miStatus = mi_init;
}

/**
 * dtor
 */
MasterInterface::~MasterInterface()
{
notRespondCounter = 0;
}

//=============================================================================
/**
 * begin a communication
 * @param addr
 * @param port
 * @return
 */
bool MasterInterface::initComm(const std::string& addr, int port, int plantID)
{
commif = new UDPClient(gd.svm.plantID);

if(gd.useUdpDebug)
	{
	commif->setUDPDebugInterface(udpDbg);
	commif->setUseUDPDebug(true);
	}

if(commif->openCommunication(addr, port))
	{
	struct sockaddr_in serveraddr; /* server's addr */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	if(inet_aton(gd.svm.ip.c_str(), &serveraddr.sin_addr) < 0)
		{
		TRACE(dbg, DBG_ERROR, "invalid address");
		return false;
		}

	serveraddr.sin_port = htons((unsigned short) gd.svm.port);

//	char *inet=inet_ntoa(serveraddr.sin_addr);
//	TRACE(dbg,DBG_SVM_DEBUG,"svm ip: %s port %u",inet,ntohs(serveraddr.sin_port));

//commif->addClient(300,serveraddr,sizeof(serveraddr),commif->ip_string_2_uint32(gd.svm.ip));
	commif->setUseQueue(true);
	commif->addClient(gd.svm.svcID, serveraddr, sizeof(serveraddr), commif->ip_string_2_uint32(gd.svm.ip));

	commif->setNewKey(gd.hprotKey);
	commif->enableCrypto(gd.enableCrypto);
	commif->startThread(NULL, 'r');

	TRACE(dbg, DBG_SVM_NOTIFY, "svm socket opened");
	opened = true;
	}
else
	{
	return false;
	}
return true;
}

/**
 * main loop
 * must run in a while(1) ..
 * @return
 */
bool MasterInterface::mainLoop()
{
bool ret = true;

try
	{
	switch(miStatus)
		{
		//....................
		case mi_init:
			miStatus = mi_connect;
			break;
			//....................
		case mi_connect:
			// test the connection with a check link
			commif->setTimeout(MI_HPROT_REQ_TIMEOUT_DEFAULT);
			TRACE(dbg, DBG_SVM_NOTIFY, "svm link request");
			commif->impl_resetToken(gd.svm.svcID);
			commif->sendRequest(gd.svm.svcID, HPROT_CMD_CHKLNK, NULL, 0, true);
			while(commif->isWaiting())
				;
			if(commif->rxResult.isOK())
				{
				TRACE(dbg, DBG_SVM_NOTIFY, "svm connected!");
				tim_keepalive.startTimeout(gd.svm.keepalive_time);
				miStatus = mi_send_polling;
				}
			else
				{
				TRACE(dbg, DBG_SVM_ERROR, "svm not connected");
				tim_wait.startTimeout(SVM_CONN_ATTEMPT_WAIT_TIME);
				miStatus = mi_conn_wait;
				}
			break;
			//....................
		case mi_conn_wait:
			if(tim_wait.checkTimeout())
				{
				miStatus = mi_connect;
				}
			break;
			//....................

		case mi_send_polling:
			{
			SVMPolling_t pollingData;
			pollingData.globProfilesCRC = gd.globProfilesCRC;
			pollingData.globWeektimesCRC = gd.globWeektimesCRC;
			pollingData.globCausalsCRC = gd.globCausalsCRC;
			pollingData.badgeTimestamp = gd.mostRecentBadgeTimestamp;

			commif->sendRequest(gd.svm.svcID, HPROT_CMD_SVC_POLLING, (uint8_t*) &pollingData, sizeof(SVMPolling_t), true);

			TRACE(dbg, DBG_SVM_NOTIFY, "Sent polling to SVC");
			while(commif->isWaiting())
				;

			miStatus = mi_send_polling;

			if(commif->rxResult.isOK())
				{
				tim_keepalive.startTimeout(gd.svm.keepalive_time);
				commif->getLastRxFrameFromQueue(&rxFrame, rxFrame_payload);

				if(rxFrame.cmd == HPROT_CMD_ACK)
					{
					TRACE(dbg, DBG_SVM_NOTIFY, "got ack from SVC");
					/* got ack -> Idle state */
					miStatus = mi_idle;
					break;
					}

				miStatus = mi_send_polling;
				/*got some error -> check which one (see app_command file)*/
				switch(rxFrame_payload[0])
					{
					/**********************************************************************
					 * Profile Error
					 ***********************************************************************/
					case (svc_profile_data_error):
						TRACE(dbg, DBG_SVM_NOTIFY, "got profile error from SVC");
						miStatus = mi_send_first_part_prof_list;
						break;

					case (svc_weektime_data_error):
						/**********************************************************************
						 * Weektime  Error
						 ***********************************************************************/
						TRACE(dbg, DBG_SVM_NOTIFY, "got weektime error from SVC");
						miStatus = mi_send_wt_list;
						break;

					case (svc_causal_data_error): /* causal data error */
						/**********************************************************************
						 * Causal Error
						 ***********************************************************************/
						{
						TRACE(dbg, DBG_SVM_NOTIFY, "got causal error from SVC");

						CausalCodes_parameters_t codesArray[MAX_CAUSAL_CODES];
						if(rxFrame.len != sizeof(codesArray) + 1) break;

						memcpy(codesArray, &rxFrame.data[1], sizeof(codesArray));
						uint8_t i;
						for(i = 0;i < MAX_CAUSAL_CODES;i++)
							{
							db->setCausal(i, codesArray[i].Parameters_s.causal_Id, codesArray[i].Parameters_s.description);
							}
						db->loadCausals();
						}
						break;

					case (svc_update_badge):
						{
						/**********************************************************************
						 * Update Badge
						 ***********************************************************************/
						TRACE(dbg, DBG_SVM_NOTIFY, "got new badge from SVC");

						if(rxFrame.len == (sizeof(globals_st::badge_t) + 1))
							{
							globals_st::badge_t b;
							memcpy(&b, &rxFrame_payload[1], sizeof(globals_st::badge_t));
							db->setBadge(b.params, b.timestamp, b.printedCode, b.contact);
							}
						}
						break;

					default:
						miStatus = mi_idle;
						break;

					}
				}
			else /* Not respond */
				{
				if(notRespondCounter++ == NOT_RESPOND_COUNTER_MAX)
					{
					notRespondCounter = 0;
					miStatus = mi_connect;
					}

				commif->resetToken(gd.svm.svcID);
				}

			}
			break;

		case mi_send_first_part_prof_list:
			{
			uint8_t max_prof = MAX_PROFILES / 2;
			uint8_t crcList[MAX_PROFILES];
			crcList[0] = 0; /* start */
			crcList[1] = max_prof; /* amount */
			for(int i = 0;i < max_prof;i++)
				{
				crcList[i + 2] = gd.profiles[i].params.crc8;
				}
			commif->setTimeout(MI_HPROT_REQ_TIMEOUT_DEFAULT);
			commif->sendRequest(gd.svm.svcID, HPROT_CMD_SVC_PROFILE_CRC_LIST, (uint8_t*) &crcList, crcList[1] + 2, true);

			TRACE(dbg, DBG_SVM_NOTIFY, "Sent first part profiles crc list to SVC");
			while(commif->isWaiting())
				;

			miStatus = mi_send_polling;

			if(commif->rxResult.isOK())
				{
				commif->getLastRxFrameFromQueue(&rxFrame, rxFrame_payload);

				Profile_parameters_t *pp;
				char name[MAX_NAME_SIZE];

				if(rxFrame.cmd == HPROT_CMD_ACK)
					{
					TRACE(dbg, DBG_SVM_NOTIFY, "got ack from SVC");
					/* got ack -> Idle state */
					miStatus = mi_send_second_part_prof_list;
					break;
					}

				/* Store data */
				pp = (Profile_parameters_t *) &rxFrame_payload[0];
				/* Store name */
				memcpy(name, &rxFrame_payload[sizeof(Profile_parameters_t)], MAX_NAME_SIZE);
				db->setProfile(pp->Parameters_s.profileId, (string) name, *pp);
				db->updateProfile(pp->Parameters_s.profileId, (string) name);
				}
			}
			break;

		case mi_send_second_part_prof_list:
			{
			uint8_t max_prof = MAX_PROFILES / 2;
			uint8_t crcList[MAX_PROFILES];
			crcList[0] = max_prof; /* start */
			crcList[1] = MAX_PROFILES - max_prof; /* amount */
			for(int i = 0;i < max_prof;i++)
				{
				crcList[i + 2] = gd.profiles[i].params.crc8;
				}
			commif->setTimeout(MI_HPROT_REQ_TIMEOUT_DEFAULT);
			commif->sendRequest(gd.svm.svcID, HPROT_CMD_SVC_PROFILE_CRC_LIST, (uint8_t*) &crcList, crcList[1] + 2, true);

			TRACE(dbg, DBG_SVM_NOTIFY, "Sent second part profiles crc list to SVC");
			while(commif->isWaiting())
				;

			miStatus = mi_send_polling;

			if(commif->rxResult.isOK())
				{
				commif->getLastRxFrameFromQueue(&rxFrame, rxFrame_payload);

				Profile_parameters_t *pp;
				char name[MAX_NAME_SIZE];

				if(rxFrame.cmd == HPROT_CMD_ACK)
					{
					TRACE(dbg, DBG_SVM_NOTIFY, "got ack from SVC");
					/* got ack -> Idle state */
					miStatus = mi_send_second_part_prof_list;
					break;
					}

				/* Store data */
				pp = (Profile_parameters_t *) &rxFrame_payload[0];
				/* Store name */
				memcpy(name, &rxFrame_payload[sizeof(Profile_parameters_t)], MAX_NAME_SIZE);
				db->setProfile(pp->Parameters_s.profileId, (string) name, *pp);
				db->updateProfile(pp->Parameters_s.profileId, (string) name);
				}
			}
			break;

		case mi_send_wt_list:
			{
			uint8_t crcList[MAX_WEEKTIMES];
			for(int i = 0;i < MAX_WEEKTIMES;i++)
				{
				crcList[i] = gd.weektimes[i].params.crc8;
				}
			commif->setTimeout(MI_HPROT_REQ_TIMEOUT_DEFAULT);
			commif->sendRequest(gd.svm.svcID, HPROT_CMD_SVC_WEEKTIME_CRC_LIST, (uint8_t*) &crcList, MAX_WEEKTIMES, true);

			TRACE(dbg, DBG_SVM_NOTIFY, "Sent weektime crc list to SVC");
			while(commif->isWaiting())
				;

			miStatus = mi_send_polling;

			if(commif->rxResult.isOK())
				{
				commif->getLastRxFrame(&rxFrame, rxFrame_payload);

				Weektime_parameters_t *wp;
				char name[MAX_NAME_SIZE];

				if(rxFrame.cmd == HPROT_CMD_ACK)
					{
					TRACE(dbg, DBG_SVM_NOTIFY, "got ack from SVC");
					/* got ack -> Idle state */
					miStatus = mi_idle;
					break;
					}

				/* Store data */
				wp = (Weektime_parameters_t *) &rxFrame_payload[0];
				/* Store name */
				memcpy(name, &rxFrame_payload[sizeof(Weektime_parameters_t)], MAX_NAME_SIZE);
				db->setWeektime(wp->Parameters_s.weektimeId, (string) name, *wp);
				db->updateWeektime(wp->Parameters_s.weektimeId, (string) name);
				}
			}
			break;

			//....................
		case mi_sync_events:
			{
			Event_t ev;
			uint64_t ev_rowid;
			db->getUnsyncedEvent(ev_rowid, ev);

			commif->sendRequest(gd.svm.svcID, HPROT_CMD_SVC_NEW_EVENT, (uint8_t*) &ev, sizeof(Event_t), true);

			TRACE(dbg, DBG_SVM_NOTIFY, "Sent event to SVC");
			while(commif->isWaiting())
				;

			if(commif->rxResult.isOK())
				{
				commif->getLastRxFrame(&rxFrame, rxFrame_payload);
				if(rxFrame.cmd == HPROT_CMD_ACK)
					{
					TRACE(dbg, DBG_SVM_NOTIFY, "got ack from SVC");
					/* got ack -> Idle state */
					db->setEventSyncStatus(ev_rowid, true);
					}
				}

			}

			miStatus = mi_send_polling;
			break;

			//....................
		case mi_idle:
			// check if need a keepalive message
			if(gd.unsynckedEvent > 0)
				{
				miStatus = mi_sync_events;
				}
			else if(tim_keepalive.checkTimeout())
				{
				miStatus = mi_send_polling;
				}
			break;
		}
	}
//--------------------------------------------------
// FAILS
catch (enum mi_Faults_e fail)
	{
	switch(fail)
		{
		case mi_error_timeout:
			break;
		case mi_error_noSync:
			break;
		}
	}

/* Stateless operation */
if(gd.SVCframeQueue.empty() != true)
	{
	commif->getLastRxFrameFromQueue(&rxFrame, rxFrame_payload);
	switch(rxFrame.cmd)
		{
		case HPROT_CMD_SVC_REQ:
			if(rxFrame.hdr == HPROT_HDR_REQUEST)
				{
				switch(rxFrame_payload[0])
					{
					case (0): /* Request Area */
						{
						uint16_t id_badge = *(uint16_t*) &rxFrame_payload[1];
						uint8_t area = db->getArea(id_badge);
						if(area < 0)
							commif->sendAnswer(rxFrame.srcID, HPROT_CMD_NACK, NULL, 0, rxFrame.num);
						else
							commif->sendAnswer(rxFrame.srcID, HPROT_CMD_SVC_REQ, (uint8_t*) &area, 1, rxFrame.num);
						}
						break;

					default:
						break;

					}

				}

			break;

		case HPROT_CMD_SVC_SET:
			switch(rxFrame_payload[0])
				{
				case (svc_update_area): /* Update Area */
					{
					uint16_t id_badge = *(uint16_t*) &rxFrame_payload[1];
					uint8_t area = rxFrame_payload[3];

					time_t et;
#ifdef RASPBERRY
					et = hwgpio->getRTC_datetime();
#else
					et=time(NULL);
#endif

					bool ret_val = db->setArea(id_badge, area);
					if(ret_val == true)
						{
						Event_t _ev;
						_ev.Parameters_s.eventCode = update_area;
						_ev.Parameters_s.idBadge = id_badge;
						_ev.Parameters_s.area = area;
						_ev.Parameters_s.causal_code = NULL_CAUSAL_CODE;
						_ev.Parameters_s.terminalId = rxFrame.srcID;
						_ev.Parameters_s.timestamp = et;
						_ev.Parameters_s.isSynced = false;
						db->addEvent(_ev);
						}

					if(rxFrame.hdr == HPROT_HDR_REQUEST)
						{
						if(ret_val == true)
							commif->sendAnswer(rxFrame.srcID, HPROT_CMD_ACK, NULL, 0, rxFrame.num);
						else
							commif->sendAnswer(rxFrame.srcID, HPROT_CMD_NACK, NULL, 0, rxFrame.num);
						}

					//miStatus = mi_sync_events;

					}
					break;

				case (svc_download_file):
					{
					vector<string> toks;
					char url[HPROT_BUFFER_SIZE];
					memset(url, '\0', HPROT_BUFFER_SIZE);
					memcpy(url, &rxFrame_payload[1], rxFrame.len - 1);
					string str(url);
					//string cmd = "wget --timeout=60 -P "



					Split(str, toks, "\n");
					string cmd = _S "wget --timeout=60 -O " + toks[0] + " -P " + FIRMWARE_NEW_PATH + " \"" + toks[1] + "\"";


					TRACE(dbg,DBG_SVM_DEBUG,cmd.c_str());

					FILE * netdata = popen(cmd.c_str(), "r");
					}
				break;

				default:
					break;
				}


			break;

		default:
			break;
		}
	}
return ret;
}

//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------
#if 0
//-----------------------------------------------------------------------------
// THREADS
//-----------------------------------------------------------------------------

/**
 * THREAD RX JOB
 * which is executed in the thread
 */
void MasterInterface::runRx()
	{
//while (!endApplicationStatus())
	while(1)
		{
		int n = udpsrv->datarecv((char*)internal_buf,SVM_MSG_BUFFER_SIZE);
		//int n = recvfrom(sockfd, internal_buf, UDP_BUFSIZE, 0, (struct sockaddr *) &clientaddr, (unsigned int *)&clientlen);
		if(n > 0)
			{
			TRACE(dbg,DBG_SVM_DEBUG,"some data received from %s port %d",udpsrv->get_lastClientAddr().c_str(),udpsrv->get_lastClientPort());
			printRXTX('r',(uint8_t *)internal_buf,n);
			}
		}
	startedRx=false;
	destroyThread('r');
	cout << "Communication RX tread stopped" << endl;
	}

/**
 * THREAD TX JOB
 * which is executed in the thread
 */
void MasterInterface::runTx()
	{
	startedTx=false;
	destroyThread('t');
	cout << "Communication TX tread stopped" << endl;
	}

/**
 * start the execution of the thread
 * @param arg (Uses default argument: arg = NULL)
 * @param which can be 't' for tx or 'r' for rx
 */
bool MasterInterface::startThread(void *arg,char which)
	{
	bool ret=true;
	bool *_started;
	void *_arg;
	pthread_t *_id;

	if(which=='r')
		{
		_started=&startedRx;
		_arg=this->argRx;
		_id=&_idRx;
		}
	else
		{
		_started=&startedTx;
		_arg=this->argTx;
		_id=&_idTx;
		}

	if (!*_started)
		{
		*_started = true;
		_arg = arg;
		/*
		 * Since pthread_create is a C library function, the 3rd
		 * argument is a global function that will be executed by
		 * the thread. In C++, we emulate the global function using
		 * the static member function that is called exec. The 4th
		 * argument is the actual argument passed to the function
		 * exec. Here we use this pointer, which is an instance of
		 * the Thread class.
		 */
		if(which=='r')
			{
			if ((ret = pthread_create(_id, NULL, &execRx, this)) != 0)
				{
				cout << strerror(ret) << endl;
				//throw "Error";
				ret=false;
				}
			}
		else
			{
			if ((ret = pthread_create(_id, NULL, &execTx, this)) != 0)
				{
				cout << strerror(ret) << endl;
				//throw "Error";
				ret=false;
				}
			}
		}
	return ret;
	}

/**
 * Allow the thread to wait for the termination status
 * @param which can be 't' for tx or 'r' for rx
 */
void MasterInterface::joinThread(char which)
	{
	if(which=='r')
		{
		if(startedRx)
			{
			pthread_join(_idRx, NULL);
			}
		}
	else
		{
		if(startedTx)
			{
			pthread_join(_idTx, NULL);
			}
		}
	}

/**
 * destroy the thread
 * @param which can be 't' for tx or 'r' for rx
 */
void MasterInterface::destroyThread(char which)
	{
	int ret;
	pthread_t *_id;
	if(which=='r')
		{
		_id=&_idRx;
		}
	else
		{
		_id=&_idTx;
		}

	pthread_detach(*_id);
	pthread_exit(0);

	if(!noattrRx)
		{
		if((ret = pthread_attr_destroy(&_attrRx)) != 0)
			{
			cout << strerror(ret) << endl;
			throw "Error";
			}
		}
	}

/**
 * Function that is used to be executed by the thread
 * @param thr
 */
void *MasterInterface::execRx(void *thr)
	{
	reinterpret_cast<MasterInterface *>(thr)->runRx();
	return NULL;
	}

/**
 * Function that is used to be executed by the thread
 * @param thr
 */
void *MasterInterface::execTx(void *thr)
	{
	reinterpret_cast<MasterInterface *>(thr)->runTx();
	return NULL;
	}
#endif
//-----------------------------------------------------------------------------
// OTHERS
//-----------------------------------------------------------------------------

/**
 * used to debug: print all the datagrams
 * @param dir r|t
 * @param dat
 * @param len
 */
void MasterInterface::printRXTX(char dir, uint8_t *dat, int len)
{
string msg = "";
char tmp[1024];

if(dir == 't')
	{
	msg += "SVM_TX: ";
	}
else
	{
	msg += "SVM_RX: ";
	}

Hex2AsciiHex(tmp, dat, len, true, ' ');
msg += _S "data=" + tmp;

TRACE(dbg, DBG_SVM_DEBUG, msg);
}

