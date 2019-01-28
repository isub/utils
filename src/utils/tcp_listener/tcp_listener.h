#ifndef __TCP_LISTENER_H__
#define __TCP_LISTENER_H__

#include <netinet/ip.h>

struct STCPListener;

#ifdef _WIN32
  #define INET_ADDRSTRLEN 16
#endif

struct SAcceptedSock {
  int m_iAcceptedSock;                  /* сокет для взаимодействия с пиром */
  char m_mcIPAddress[INET_ADDRSTRLEN];  /* ip-адрес пира */
  in_port_t m_usPort;                   /* номер порта пира (в порядке следования байтов хоста) */
};

#ifdef __cplusplus
extern "C" {
#endif

struct STCPListener * tcp_listener_init (
  const char *p_pszIpAddr,              /* локальный ip-адрес, принимающий подключения */
  in_port_t p_tPort,                    /* номер локального порта, принимающего подключения */
  int p_iQueueLen,                      /* максимальный размер очереди подключений */
  int p_iMaxThreads,                    /* максимальное количество программных потоков, обслуживающих подключения */
  int (*p_fnCBAddr)(const struct SAcceptedSock *p_psoAcceptedSocket),
  int *p_iErrLine);

void tcp_listener_fini (struct STCPListener *p_psoIPListener);

#ifdef __cplusplus
}
#endif

#endif /* __TCP_LISTENER_H__ */
