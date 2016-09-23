#include <string.h>
#include <errno.h>
#ifdef WIN32
#	include <Winsock2.h>
#	define	in_addr_t	ULONG
#	pragma	comment(lib, "ws2_32.lib")
#else
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <arpa/inet.h>
#	include <netdb.h>
#	include <unistd.h>
#	include <poll.h>
#	define INVALID_SOCKET	(-1)
#	define SOCKET_ERROR (-1)
#endif

#include "ipconnector.h"

void CIPConnector::init (int p_iReqTimeout)
{
	m_iStatus = 0;
	m_sockSock = INVALID_SOCKET;
#ifdef	WIN32
	WSAData soWSAData;
	if (S_OK == WSAStartup (MAKEWORD(2, 2), &soWSAData)) {
		m_iStatus = 1;
	}
#else
	m_iStatus = 1;
#endif
	m_iRequestTimeout = p_iReqTimeout;
}

CIPConnector::CIPConnector ()
{
	init (10);
}

CIPConnector::CIPConnector (int p_iReqTimeout)
{
	init (p_iReqTimeout);
}

CIPConnector::~CIPConnector ()
{
	/* если соединение установлено */
	if (2 == m_iStatus) {
		DisConnect ();
		m_iStatus = 1;
	}
#ifdef	WIN32
	if (1 < m_iStatus) {
		WSACleanup ();
	}
#endif
	m_iStatus = 0;
}

int CIPConnector::Connect (const char *p_pszHostName, unsigned short p_usPort, int p_iProtoType)
{
	int iRetVal = 0;
	sockaddr_in soSockAddr;
	hostent *psoHostEnt;

	do {
		/* проверяем инициализирован ли экземпляр класса */
		if (! m_iStatus) {
			iRetVal = -1;
			break;
		}
		DisConnect ();

		/* создаем сокет */
		switch (p_iProtoType) {
			case IPPROTO_TCP:
				m_sockSock = socket (AF_INET, SOCK_STREAM, p_iProtoType);
				break;
			case IPPROTO_UDP:
				m_sockSock = socket (AF_INET, SOCK_DGRAM, p_iProtoType);
				break;
			default:
				iRetVal = -2;
				break;
		}
		if (iRetVal)
			break;

		memset (&soSockAddr, 0, sizeof(soSockAddr));

		/* определ€ем ip-адрес хоста */
		psoHostEnt = gethostbyname (p_pszHostName);
		if (psoHostEnt) {
			soSockAddr.sin_addr.s_addr = *((in_addr_t*)(psoHostEnt->h_addr_list[0]));
			soSockAddr.sin_family = psoHostEnt->h_addrtype;
		} else {
			soSockAddr.sin_addr.s_addr = inet_addr (p_pszHostName);
			soSockAddr.sin_family = AF_INET;
		}

		/* адрес не удалось распознать */
		if (INADDR_NONE == soSockAddr.sin_addr.s_addr) {
			iRetVal = -3;
			break;
		}

		soSockAddr.sin_port = htons (p_usPort);
		soSockAddr.sin_family = AF_INET;
		iRetVal = connect (m_sockSock, (sockaddr*)&soSockAddr, sizeof(soSockAddr));
		if (SOCKET_ERROR == iRetVal) {
			iRetVal = -4;
			break;
		}
		m_iStatus = 2;
	} while (0);

	return iRetVal;
}

int CIPConnector::Send (const char *p_mcBuf, int p_iLen)
{
	int iRetVal = 0;

	do {
		int iFnRes;
		pollfd soPollFD;
		int iSent;

		/* провер€ем, было ли выполнено подключение */
		if (2 != m_iStatus) {
			iRetVal = -1;
			break;
		}

		memset (&soPollFD, 0, sizeof (soPollFD));
		soPollFD.fd = m_sockSock;
		soPollFD.events = POLLOUT;
#ifdef WIN32
		iFnRes = WSAPoll (&soPollFD, 1, m_iRequestTimeout * 1000);
#else
		iFnRes = poll (&soPollFD, 1, m_iRequestTimeout * 1000);
#endif

		if (1 != iFnRes) {
			iRetVal = -1;
			break;
		}

		iSent = 0;

		while (iSent < p_iLen) {
			iFnRes = send (m_sockSock, p_mcBuf + iSent, p_iLen - iSent, 0);
			if (SOCKET_ERROR == iFnRes) {
				/* в случае вознекновени€ ошибки */
				iRetVal = errno;
				switch (iRetVal) {
					/* если соединение с процессом потер€но */
					case EPIPE:
					/* если соединение разорвано */
					case ECONNRESET:
						DisConnect ();
						break;
					default:
						break;
				}
				break;
			}
			iSent += iFnRes;
		}
	} while (0);

	return iRetVal;
}

int CIPConnector::Recv (char *p_mcBuf, int p_iBufSize)
{
	int iRetVal = 0;

	do {
		int iFnRes;
		pollfd soPollFD;

		/* провер€ем, было ли выполнено подключение */
		if (2 != m_iStatus) {
			iRetVal = -1;
			break;
		}

		memset (&soPollFD, 0, sizeof (soPollFD));
		soPollFD.fd = m_sockSock;
		soPollFD.events = POLLIN;
#ifdef WIN32
		iFnRes = WSAPoll (&soPollFD, 1, m_iRequestTimeout * 1000);
#else
		iFnRes = poll (&soPollFD, 1, m_iRequestTimeout * 1000);
#endif

		if (1 != iFnRes) {
			iRetVal = -1;
			break;
		}

		iFnRes = recv (m_sockSock, p_mcBuf, p_iBufSize, 0);
		/* если соединение разорвано */
		if (0 == iFnRes) {
			DisConnect ();
			iRetVal = 0;
			break;
		}
		if (SOCKET_ERROR == iFnRes) {
			iRetVal = -2;
			break;
		}
		iRetVal = iFnRes;
	} while (0);

	return iRetVal;
}

void CIPConnector::DisConnect () {
	if (INVALID_SOCKET != m_sockSock) {
		m_iStatus = 1;
#ifdef WIN32
		closesocket (m_sockSock);
#else
		close (m_sockSock);
#endif
		m_sockSock = INVALID_SOCKET;
	}
}
