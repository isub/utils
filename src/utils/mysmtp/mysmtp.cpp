#include <WinSock2.h>
#include <string>
#include <vector>
#include "base64/base64.h"

#define MYSMTP_EXPORT
#include "MySMTP.h"

#pragma comment(lib, "Ws2_32.lib")

#define BOUNDARY "012345678909876543210"

CMySMTP::CMySMTP(void) {
	int iFnRes;
	WSADATA soWSAData;

	iFnRes = WSAStartup (0x0200 | 0x0002, &soWSAData);
	if (iFnRes) {
		m_bInitialized = FALSE;
	} else {
		m_bInitialized = TRUE;
	}
}

CMySMTP::~CMySMTP(void) {
	WSACleanup ();
	m_bInitialized = FALSE;
}

int CMySMTP::SendMail(
	const char *p_pcszSMTPServer,
	const unsigned short p_usPort,
	const char *p_pcszFrom,
	std::vector<std::string> *p_pvectRecList,
	std::vector<std::string> *p_pvectCopyList,
	std::vector<std::string> *p_pvectBlindCopyList,
	const char *p_pcszSubject,
	const char *p_pcszMessage,
	std::vector<std::string> *p_pvectAttachList,
	std::string *p_pstrReport)
{
	if (! m_bInitialized) {
		return -1;
	}

	int iRetVal = 0;

	int iFnRes;
	SOCKET sockSMTP = INVALID_SOCKET;

	do {
		if (NULL == p_pcszSMTPServer) {
			iRetVal = -1;
			break;
		}
		iFnRes = ConnectToServer (&sockSMTP, p_pcszSMTPServer, p_usPort == 0 ? 25 : p_usPort);
		if (iFnRes) {
			if (p_pstrReport) {
				*p_pstrReport += "Can't connect to server \"";
				*p_pstrReport += p_pcszSMTPServer;
				*p_pstrReport += "\";";
			}
			iRetVal = iFnRes;
			break;
		}
		char mcReadBuf[0x10000];
		int iDataSize;
		std::string strMsg;
		iDataSize = sizeof(mcReadBuf)/sizeof(*mcReadBuf);
		iFnRes = GetResp (sockSMTP, mcReadBuf, &iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		mcReadBuf[iDataSize] = '\0';
		iFnRes = CheckResp (NULL, mcReadBuf, iFnRes);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		strMsg = "EHLO ";
		char mcHostName[256];
		iFnRes = gethostname (mcHostName, sizeof(mcHostName)/sizeof(*mcHostName));
		if (0 == iFnRes) {
			strMsg += mcHostName;
		}
		strMsg += "\r\n";
		iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		iDataSize = sizeof(mcReadBuf)/sizeof(*mcReadBuf);
		iFnRes = GetResp (sockSMTP, mcReadBuf, &iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		mcReadBuf[iDataSize] = '\0';
		iFnRes = CheckResp (strMsg.c_str(), mcReadBuf, iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		strMsg = "MAIL FROM: <";
		if (p_pcszFrom) {
			strMsg += p_pcszFrom;
		}
		strMsg += ">\r\n";
		iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		iDataSize = sizeof(mcReadBuf)/sizeof(*mcReadBuf);
		iFnRes = GetResp (sockSMTP, mcReadBuf, &iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		mcReadBuf[iDataSize] = '\0';
		iFnRes = CheckResp (strMsg.c_str(), mcReadBuf, iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		std::vector<std::string> *mpvectList[] = {
			p_pvectRecList,
			p_pvectCopyList,
			p_pvectBlindCopyList};
		std::vector<std::string>::iterator iterList;
		int iInvalidRecCnt = 0;
		for (int iInd = 0; iInd < sizeof(mpvectList)/sizeof(void*); ++iInd) {
			if (NULL == mpvectList[iInd]) {
				continue;
			}
			iterList = mpvectList[iInd]->begin();
			while (iterList != mpvectList[iInd]->end()) {
				strMsg = "RCPT TO:<";
				strMsg += iterList->c_str();
				strMsg += ">\r\n";
				iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
				if (iFnRes) {
					iRetVal = iFnRes;
					break;
				}
				iDataSize = sizeof(mcReadBuf)/sizeof(*mcReadBuf);
				iFnRes = GetResp (sockSMTP, mcReadBuf, &iDataSize);
				if (iFnRes) {
					iRetVal = iFnRes;
					break;
				}
				mcReadBuf[iDataSize] = '\0';
				iFnRes = CheckResp (strMsg.c_str(), mcReadBuf, iDataSize);
				if (iFnRes) {
					if (p_pstrReport) {
						*p_pstrReport += "Request: ";
						*p_pstrReport += strMsg;
						*p_pstrReport += "Response: ";
						*p_pstrReport += mcReadBuf;
						*p_pstrReport += ";";
					}
					++ iInvalidRecCnt;
				}
				++iterList;
			}
		}
		if (iRetVal) {
			break;
		}
		int iRecCnt = -iInvalidRecCnt;
		if (p_pvectRecList) {
			iRecCnt += p_pvectRecList->size();
		}
		if (p_pvectCopyList) {
			iRecCnt += p_pvectCopyList->size();
		}
		if (p_pvectBlindCopyList) {
			iRecCnt += p_pvectBlindCopyList->size();
		}
		if (0 == iRecCnt) {
			iRetVal = -1;
			break;
		}
		strMsg = "DATA\r\n";
		iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		iDataSize = sizeof(mcReadBuf)/sizeof(*mcReadBuf);
		iFnRes = GetResp (sockSMTP, mcReadBuf, &iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		mcReadBuf[iDataSize] = '\0';
		iFnRes = CheckResp (strMsg.c_str(), mcReadBuf, iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		if (p_pcszFrom) {
			strMsg = "From: ";
			strMsg += p_pcszFrom;
			strMsg += "\r\n";
			iFnRes = SendMsg(
				sockSMTP,
				strMsg.c_str(),
				strMsg.length());
			if (iFnRes) {
				iRetVal = iFnRes;
				break;
			}
		}
		if (p_pvectRecList) {
			iterList = p_pvectRecList->begin();
			while (iterList != p_pvectRecList->end()) {
				strMsg = "To: ";
				strMsg += iterList->c_str();
				strMsg += "\r\n";
				iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
				if (iFnRes) {
					iRetVal = iFnRes;
					break;
				}
				++iterList;
			}
			if (iRetVal) {
				break;
			}
		}
		if (p_pvectCopyList) {
			iterList = p_pvectCopyList->begin();
			while (iterList != p_pvectCopyList->end()) {
				strMsg = "Cc: ";
				strMsg += iterList->c_str();
				strMsg += "\r\n";
				iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
				if (iFnRes) {
					iRetVal = iFnRes;
					break;
				}
				++iterList;
			}
			if (iRetVal) {
				break;
			}
		}
		if (p_pcszSubject) {
			strMsg = "Subject: ";
			strMsg += p_pcszSubject;
			strMsg += "\r\n";
			iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
			if (iFnRes) {
				iRetVal = iFnRes;
				break;
			}
		}
		strMsg = "MIME-Version: 1.0\r\n";
		iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		strMsg = "Content-Type: multipart/mixed; boundary=\""BOUNDARY"\"\r\n\r\n";
		iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		if (p_pcszMessage) {
			strMsg = "--"BOUNDARY"\r\n";
			iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
			if (iFnRes) {
				iRetVal = iFnRes;
				break;
			}
			strMsg = "Content-Type: text/plain; charset=windows-1251\r\n\r\n";
			iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
			if (iFnRes) {
				iRetVal = iFnRes;
				break;
			}
			strMsg = p_pcszMessage;
			strMsg += "\r\n";
			iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
			if (iFnRes) {
				iRetVal = iFnRes;
				break;
			}
		}
		if (p_pvectAttachList) {
			iterList = p_pvectAttachList->begin();
			while (iterList != p_pvectAttachList->end()) {
				do {
					char mcFileTitle[MAX_PATH];
					iFnRes = GetFileTitleA (iterList->c_str(), mcFileTitle, sizeof(mcFileTitle)/sizeof(*mcFileTitle));
					if (iFnRes) {
						iRetVal = iFnRes;
						break;
					}
					strMsg = "--"BOUNDARY"\r\n";
					iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
					if (iFnRes) {
						iRetVal = iFnRes;
						break;
					}
					strMsg = "Content-Type: Application/Octet-Stream; name=";
					strMsg += mcFileTitle;
					strMsg += "\r\n";
					iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
					if (iFnRes) {
						iRetVal = iFnRes;
						break;
					}
					strMsg = "Content-Transfer-Encoding: base64\r\n\r\n";
					iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
					if (iFnRes) {
						iRetVal = iFnRes;
						break;
					}
					iFnRes = OutputFile (sockSMTP, iterList->c_str());
					if (iFnRes) {
						iRetVal = iFnRes;
						break;
					}
					strMsg = "\r\n";
					iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
					if (iFnRes) {
						iRetVal = iFnRes;
						break;
					}
				} while (0);
				++iterList;
			}
			if (iRetVal) {
				break;
			}
		}
		strMsg = "--"BOUNDARY"--\r\n";
		iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		strMsg = ".\r\n";
		iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		iDataSize = sizeof(mcReadBuf)/sizeof(*mcReadBuf);
		iFnRes = GetResp (sockSMTP, mcReadBuf, &iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		mcReadBuf[iDataSize] = '\0';
		iFnRes = CheckResp (strMsg.c_str(), mcReadBuf, iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		strMsg = "QUIT\r\n";
		iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		iDataSize = sizeof(mcReadBuf)/sizeof(*mcReadBuf);
		iFnRes = GetResp (sockSMTP, mcReadBuf, &iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		mcReadBuf[iDataSize] = '\0';
		iFnRes = CheckResp (strMsg.c_str(), mcReadBuf, iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
	} while (0);

	if (INVALID_SOCKET != sockSMTP) {
		closesocket (sockSMTP);
	}

	return iRetVal;
}

int CMySMTP::VerifyRecipient (
	const char *p_pcszSMTPServer,
	const unsigned short p_usPort,
	const char *p_pcszRecipient,
	std::string *p_pstrReport) {
	if (! m_bInitialized) {
		return -1;
	}

	int iRetVal = 0;

	int iFnRes;
	SOCKET sockSMTP = INVALID_SOCKET;
	char mcReadBuf[0x10000];
	int iDataSize;
	std::string strMsg;

	do {
		if (NULL == p_pcszSMTPServer) {
			iRetVal = -1;
			break;
		}
		iFnRes = ConnectToServer (&sockSMTP, p_pcszSMTPServer, p_usPort == 0 ? 25 : p_usPort);
		if (iFnRes) {
			if (p_pstrReport) {
				*p_pstrReport += "Can't connect to server \"";
				*p_pstrReport += p_pcszSMTPServer;
				*p_pstrReport += "\";";
			}
			iRetVal = iFnRes;
			break;
		}
		iDataSize = sizeof(mcReadBuf)/sizeof(*mcReadBuf);
		iFnRes = GetResp (sockSMTP, mcReadBuf, &iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		mcReadBuf[iDataSize] = '\0';
		iFnRes = CheckResp (NULL, mcReadBuf, iFnRes);
		if (iFnRes) {
			break;
		}
		strMsg = "EHLO ";
		char mcHostName[256];
		iFnRes = gethostname (mcHostName, sizeof(mcHostName)/sizeof(*mcHostName));
		if (0 == iFnRes) {
			strMsg += mcHostName;
		}
		strMsg += "\r\n";
		iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
		if (iFnRes) {
			break;
		}
		iDataSize = sizeof(mcReadBuf)/sizeof(*mcReadBuf);
		iFnRes = GetResp (sockSMTP, mcReadBuf, &iDataSize);
		if (iFnRes) {
			break;
		}
		mcReadBuf[iDataSize] = '\0';
		iFnRes = CheckResp (strMsg.c_str(), mcReadBuf, iDataSize);
		if (iFnRes) {
			break;
		}
		// VRFY
		strMsg = "VRFY";
		if (p_pcszRecipient) {
			strMsg += " <";
			strMsg += p_pcszRecipient;
			strMsg += ">";
		};
		strMsg += "\r\n";
		iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		iDataSize = sizeof(mcReadBuf)/sizeof(*mcReadBuf);
		iFnRes = GetResp (sockSMTP, mcReadBuf, &iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		mcReadBuf[iDataSize] = '\0';
		iFnRes = CheckResp (strMsg.c_str(), mcReadBuf, iDataSize);
		if (iFnRes) {
			if (p_pstrReport) {
				*p_pstrReport = mcReadBuf;
			}
			iRetVal = iFnRes;
			break;
		}
		// QUIT
		strMsg = "QUIT\r\n";
		iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		iDataSize = sizeof(mcReadBuf)/sizeof(*mcReadBuf);
		iFnRes = GetResp (sockSMTP, mcReadBuf, &iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		mcReadBuf[iDataSize] = '\0';
		iFnRes = CheckResp (strMsg.c_str(), mcReadBuf, iDataSize);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
	} while (0);

	// если произошла ошибка
	if (iRetVal && -1 != iRetVal) {
		// RSET
		strMsg = "RSET\r\n";
		iFnRes = SendMsg (sockSMTP, strMsg.c_str(), strMsg.length());
		if (iFnRes) {
			iRetVal = iFnRes;
		} else {
			iDataSize = sizeof(mcReadBuf)/sizeof(*mcReadBuf);
			iFnRes = GetResp (sockSMTP, mcReadBuf, &iDataSize);
			if (iFnRes) {
				iRetVal = iFnRes;
			} else {
				mcReadBuf[iDataSize] = '\0';
			}
		}
	}

	if (INVALID_SOCKET != sockSMTP) {
		closesocket (sockSMTP);
	}

	return iRetVal;
}

int CMySMTP::ConnectToServer (SOCKET *p_psockSock, const char *p_pcszHostName, unsigned short p_usPort) {
	int iRetVal = 0;

	int iFnRes;

	do {
		*p_psockSock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == *p_psockSock) {
			iRetVal = WSAGetLastError ();
			break;
		}
		hostent *psoHostEnt;
		sockaddr_in sockaddrSMTP;
		sockaddrSMTP.sin_family = AF_INET;
		sockaddrSMTP.sin_port = htons (p_usPort);
		psoHostEnt = gethostbyname (p_pcszHostName);
		if (psoHostEnt) {
			int i = 0;
			while (psoHostEnt->h_addr_list[i]) {
				sockaddrSMTP.sin_addr.s_addr = *(u_long*)psoHostEnt->h_addr_list[i++];
				iFnRes = connect (*p_psockSock, (sockaddr*)&sockaddrSMTP, sizeof(sockaddrSMTP));
				if (0 == iFnRes) {
					iRetVal = 0;
					break;
				} else {
					iRetVal = WSAGetLastError();
				}
			}
		}
	} while (0);

	return iRetVal;
}

int CMySMTP::SendMsg (SOCKET p_sockSock, const char *p_pcszMsg, int p_iMsgLen) {
	int iRetVal = 0;

	int iFnRes;
	WSAPOLLFD soFD;

	do {
		int iTryCount = 5;
		int iSent = 0;
		while (--iTryCount
			&& p_iMsgLen != iSent) {
			soFD.events = POLLWRNORM;
			soFD.fd = p_sockSock;
			iFnRes = WSAPoll (&soFD, 1, 60000);
			if (1 != iFnRes || ! (soFD.revents & POLLWRNORM)) {
				switch (iFnRes) {
				case 0:
					iRetVal = ERROR_TIMEOUT;
					break;
				case SOCKET_ERROR:
					iRetVal = WSAGetLastError ();
					break;
				default:
					iRetVal = -1;
					break;
				}
				break;
			}
			iFnRes = send (p_sockSock, &(p_pcszMsg[iSent]), p_iMsgLen, 0);
			if (0 >= iFnRes) {
				switch (iFnRes) {
				case SOCKET_ERROR:
					iRetVal = WSAGetLastError ();
					break;
				case 0:
					iRetVal = ERROR_CONNECTION_ABORTED;
					break;
				default:
					iRetVal = - 1;
					break;
				}
				break;
			}
			iSent += iFnRes;
		}
		if (iRetVal) {
			break;
		}
	} while (0);

	return iRetVal;
}

int CMySMTP::OutputFile (SOCKET p_sockSock, const char *p_pcszFileName) {
	int iRetVal = 0;

	int iFnRes;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwBytesRead;
	unsigned char mucReadBuf[8190];
	char *pszBase64 = NULL;
	int iBase64Len;

	hFile = CreateFileA (p_pcszFileName, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (INVALID_HANDLE_VALUE == hFile) {
		return GetLastError();
	}
	while (1) {
	if (! ReadFile (hFile, mucReadBuf, sizeof(mucReadBuf)/sizeof(*mucReadBuf), &dwBytesRead, NULL)) {
			iRetVal = GetLastError ();
			break;
	}
	if (0 == dwBytesRead) {
		break;
	}
	iBase64Len = 8 * ((dwBytesRead + (0 == (dwBytesRead % 6) ? 0 : (6 - (dwBytesRead % 6)))) / 6);
	pszBase64 = new char [iBase64Len];
	if (NULL == pszBase64) { return iRetVal; }
		iFnRes = base64_encode (mucReadBuf, dwBytesRead, pszBase64);
		/* если в буфер ничего не записано, значит произошла обшибка */
		if (0 == iFnRes) { iRetVal = -1; break; }
		iFnRes = SendMsg (p_sockSock, pszBase64, iFnRes);
		if (pszBase64) { delete [] pszBase64; pszBase64 = NULL; }
		if (iFnRes) { iRetVal = iFnRes; break; }
	}
	if (pszBase64) { delete [] pszBase64; pszBase64 = NULL; }
	if (INVALID_HANDLE_VALUE != hFile) {
		CloseHandle (hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	return iRetVal;
}

int CMySMTP::GetResp (SOCKET p_sockSock, char *p_pmcBuf, int *p_piBufSize) {
	int iRetVal = 0;

	int iFnRes;
	WSAPOLLFD soFD;

	do {
		soFD.events = POLLRDNORM;
		soFD.fd = p_sockSock;
		iFnRes = WSAPoll (&soFD, 1, 60000);
		if (1 != iFnRes
			|| ! (soFD.revents & POLLRDNORM)) {
			switch (iFnRes) {
			case 0:
				iRetVal = ERROR_TIMEOUT;
				break;
			case SOCKET_ERROR:
				iRetVal = WSAGetLastError ();
				break;
			default:
				iRetVal = -1;
				break;
			}
			break;
		}
		iFnRes = recv (p_sockSock, p_pmcBuf, *p_piBufSize, 0);
		if (0 >= iFnRes) {
			switch (iFnRes) {
			case SOCKET_ERROR:
				iRetVal = WSAGetLastError ();
				break;
			case 0:
				iRetVal = ERROR_CONNECTION_ABORTED;
				break;
			default:
				iRetVal = - 1;
				break;
			}
			break;
		}
		*p_piBufSize = iFnRes;
	} while (0);

	return iRetVal;
}

int CMySMTP::CheckResp (const char *p_pcszMsg, const char *p_pcszResp, int p_iRespLen) {
	int iRetVal = 0;

	char *pszTmplt = NULL;
	size_t stTmplt = 0;

	// connect
	if (NULL == p_pcszMsg) {
		pszTmplt = "220";
		stTmplt = strlen (pszTmplt);
		if (0 == strncmp (p_pcszResp, pszTmplt, stTmplt)) {
				return 0;
		} else {
			return (atoi (p_pcszResp));
		}
	}
	// EHLO
	pszTmplt = "EHLO";
	stTmplt = strlen (pszTmplt);
	if (0 == strncmp (pszTmplt, p_pcszMsg, stTmplt)) {
			pszTmplt = "250";
			stTmplt = strlen (pszTmplt);
			if (0 == strncmp (p_pcszResp, pszTmplt, stTmplt)) {
					return 0;
			} else {
				return (atoi (p_pcszResp));
			}
	}
	// MAIL FROM
	pszTmplt = "MAIL FROM";
	stTmplt = strlen (pszTmplt);
	if (0 == strncmp (pszTmplt, p_pcszMsg, stTmplt)) {
			pszTmplt = "250";
			stTmplt = strlen (pszTmplt);
			if (0 == strncmp (p_pcszResp, pszTmplt, stTmplt)) {
					return 0;
			} else {
				return (atoi (p_pcszResp));
			}
	}
	// RCPT TO
	pszTmplt = "RCPT TO";
	stTmplt = strlen (pszTmplt);
	if (0 == strncmp (pszTmplt, p_pcszMsg, stTmplt)) {
			pszTmplt = "250";
			stTmplt = strlen (pszTmplt);
			if (0 == strncmp (p_pcszResp, pszTmplt, stTmplt)) {
					return 0;
			} else {
				return (atoi (p_pcszResp));
			}
	}
	// VRFY
	pszTmplt = "VRFY";
	stTmplt = strlen (pszTmplt);
	if (0 == strncmp (pszTmplt, p_pcszMsg, stTmplt)) {
			pszTmplt = "252";
			stTmplt = strlen (pszTmplt);
			if (0 == strncmp (p_pcszResp, pszTmplt, stTmplt)) {
					return 0;
			} else {
				return (atoi (p_pcszResp));
			}
	}
	// DATA
	pszTmplt = "DATA";
	stTmplt = strlen (pszTmplt);
	if (0 == strncmp (pszTmplt, p_pcszMsg, stTmplt)) {
			pszTmplt = "354";
			stTmplt = strlen (pszTmplt);
			if (0 == strncmp (p_pcszResp, pszTmplt, stTmplt)) {
					return 0;
			} else {
				return (atoi (p_pcszResp));
			}
	}
	// .\r\n
	pszTmplt = ".\r\n";
	stTmplt = strlen (pszTmplt);
	if (0 == strncmp (pszTmplt, p_pcszMsg, stTmplt)) {
			pszTmplt = "250";
			stTmplt = strlen (pszTmplt);
			if (0 == strncmp (p_pcszResp, pszTmplt, stTmplt)) {
					return 0;
			} else {
				return (atoi (p_pcszResp));
			}
	}
	// QUIT
	pszTmplt = "QUIT";
	stTmplt = strlen (pszTmplt);
	if (0 == strncmp (pszTmplt, p_pcszMsg, stTmplt)) {
			pszTmplt = "221";
			stTmplt = strlen (pszTmplt);
			if (0 == strncmp (p_pcszResp, pszTmplt, stTmplt)) {
					return 0;
			} else {
				return (atoi (p_pcszResp));
			}
	}

	return iRetVal;
}
