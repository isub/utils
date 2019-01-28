#include "tcp_listener.h"

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <poll.h>

struct STCPListener {
  int m_iSock;
  int m_iMaxThreads;
  int m_iThreadCount;
  int (*m_fnCB)(const struct SAcceptedSock *p_psoAcceptedSocket);
  volatile int m_iIsInitialized;
  volatile int m_iContinue;
  pthread_t m_tThread;
};

struct SConnectData {
  struct SAcceptedSock *m_psoAcceptedSock;
  int (*m_fnCB)(const struct SAcceptedSock *p_psoAcceptedSocket);
};

void * fn_accept_thread (void *p_pArg);
void * fn_connect_thread (void *p_pArg);

struct STCPListener * tcp_listener_init (
  const char *p_pszIpAddr,
  in_port_t p_tPort,
  int p_iQueueLen,
  int p_iMaxThreads,
  int (*p_fnCBAddr)(const struct SAcceptedSock *p_psoAcceptedSocket),
  int *p_iErrLine)
{
  struct STCPListener *psoRetVal = NULL;
  int iSock;
  int iFnRes;
  int iReuseAddr = 1;
  struct sockaddr_in soSockAddr;

  do {
    /* создаем сокет */
    iSock = socket (AF_INET, SOCK_STREAM, 0);
    if (-1 != iSock) {
    } else {
      *p_iErrLine = __LINE__;
      break;
    }
    /* задаем опции сокета */
    iFnRes = setsockopt (iSock, SOL_SOCKET, SO_REUSEADDR, &iReuseAddr, sizeof(iReuseAddr));
    if (0 == iFnRes) {
    } else {
      *p_iErrLine = __LINE__;
      break;
    }
    /* задаем режим работы сокета */
    iFnRes = fcntl (iSock, F_SETFL, O_NONBLOCK);
    if (0 == iFnRes) {
    } else {
      *p_iErrLine = __LINE__;
      break;
    }
    /* задаем локальный адрес */
    soSockAddr.sin_family = AF_INET;
    soSockAddr.sin_port = htons (p_tPort);
    if (inet_aton (p_pszIpAddr, &soSockAddr.sin_addr)) {
    } else {
      *p_iErrLine = __LINE__;
      break;
    }
    /* привязка к локальному адресу */
    iFnRes = bind (iSock, (struct sockaddr*)&soSockAddr, sizeof(soSockAddr));
    if (0 == iFnRes) {
    } else {
      *p_iErrLine = __LINE__;
      break;
    }
    /* начинаем прослушивать сокет */
    iFnRes = listen (iSock, p_iQueueLen);
    if (0 == iFnRes) {
    } else {
      *p_iErrLine = __LINE__;
      break;
    }
    /* выделяем память для структуры учета параметров */
    psoRetVal = (struct STCPListener*) malloc (sizeof(struct STCPListener));
    if (psoRetVal) {
      psoRetVal->m_iSock = iSock;
      psoRetVal->m_iMaxThreads = p_iMaxThreads;
      psoRetVal->m_iThreadCount = 0;
      psoRetVal->m_fnCB = p_fnCBAddr;
      psoRetVal->m_iContinue = 1;
      psoRetVal->m_iIsInitialized = 0;
    }
    /* запускаем поток получения подключений */
    iFnRes = pthread_create (&psoRetVal->m_tThread, NULL, fn_accept_thread, psoRetVal);
    if (0 == iFnRes) {
    } else {
      *p_iErrLine = __LINE__;
      tcp_listener_fini (psoRetVal);
      psoRetVal = NULL;
    }
  } while (0);

  return psoRetVal;
}

void tcp_listener_fini (struct STCPListener *p_psoIPListener)
{
  void *pVoid;

  if (NULL != p_psoIPListener) {
  } else {
    return;
  }

  p_psoIPListener->m_iContinue = 0;
  shutdown (p_psoIPListener->m_iSock, SHUT_WR);
  if (p_psoIPListener->m_iIsInitialized) {
    pthread_join (p_psoIPListener->m_tThread, &pVoid);
    if (NULL == pVoid) {
    } else {
      free (pVoid);
    }
  }
  close (p_psoIPListener->m_iSock);
  free (p_psoIPListener);
}

void * fn_accept_thread (void *p_pArg)
{
  struct STCPListener *psoIPListener = (struct STCPListener *)p_pArg;
  struct pollfd soPollFd;
  int iFnRes;
  struct SAcceptedSock *psoAcceptedSock;
  struct SConnectData *psoConnData;
  pthread_t tThread;
  socklen_t stAddrLen;
  struct sockaddr_in soSockAddr;

  psoIPListener->m_iIsInitialized = 1;

  /* ловим изменение состояния сокета */
  while (psoIPListener->m_iContinue) {
    soPollFd.fd = psoIPListener->m_iSock;
    soPollFd.events = POLLIN;
    soPollFd.revents = 0;
    poll (&soPollFd, 1, 500);
    switch (soPollFd.revents) {
    case POLLIN:
      psoAcceptedSock = (struct SAcceptedSock *)malloc (sizeof(struct SAcceptedSock));
      psoConnData = (struct SConnectData *)malloc(sizeof(*psoConnData));
      if (NULL == psoAcceptedSock) {
        break;
      }
      stAddrLen = sizeof (soSockAddr);
      psoAcceptedSock->m_iAcceptedSock = accept (psoIPListener->m_iSock, (struct sockaddr*)&soSockAddr, &stAddrLen);
      if (sizeof (soSockAddr) == stAddrLen) {
        if (psoAcceptedSock->m_mcIPAddress == inet_ntop (AF_INET, &soSockAddr.sin_addr, psoAcceptedSock->m_mcIPAddress, sizeof(psoAcceptedSock->m_mcIPAddress))) {
        } else {
          psoAcceptedSock->m_mcIPAddress[0] = '\0';
        }
        psoAcceptedSock->m_usPort = ntohs (soSockAddr.sin_port);
      }
      if (0 <= psoAcceptedSock->m_iAcceptedSock) {
      } else {
        free (psoAcceptedSock);
        break;
      }
      psoConnData->m_psoAcceptedSock = psoAcceptedSock;
      psoConnData->m_fnCB = psoIPListener->m_fnCB;
      iFnRes = pthread_create (&tThread, NULL, fn_connect_thread, psoConnData);
      if (0 == iFnRes) {
        pthread_detach (tThread);
      } else {
        free (psoAcceptedSock);
        free(psoConnData);
        break;
      }
      break;
    }
  }

  return NULL;
}

void * fn_connect_thread (void *p_pArg)
{
  struct SConnectData *psoConnectData = (struct SConnectData *)p_pArg;

  /* проверка параметров */
  if (NULL != psoConnectData
      && NULL != psoConnectData->m_psoAcceptedSock
      && NULL != psoConnectData->m_fnCB) {
  } else {
    return NULL;
  }

  psoConnectData->m_fnCB (psoConnectData->m_psoAcceptedSock);

  /* освобождаем занятые ресурсы */
  shutdown (psoConnectData->m_psoAcceptedSock->m_iAcceptedSock, SHUT_WR);
  close(psoConnectData->m_psoAcceptedSock->m_iAcceptedSock);
  free(psoConnectData->m_psoAcceptedSock);
  free(psoConnectData);

  return NULL;
}
