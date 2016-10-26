#include <netinet/ip.h>

struct SIPListener;

struct SAcceptedSock {
  int m_iAcceptedSock;                  /* сокет для взаимодействия с пиром */
  char m_mcIPAddress[INET_ADDRSTRLEN];  /* ip-адрес пира */
  in_port_t m_usPort;                   /* номер порта пира (в порядке следования байтов хоста) */
};

struct SIPListener * iplistener_init (
  const char *p_pszIpAddr,              /* локальный ip-адрес, принимающий подключения */
  in_port_t p_tPort,                    /* номер локального порта, принимающего подключения */
  int p_iQueueLen,                      /* максимальный размер очереди подключений */
  int p_iMaxThreads,                    /* максимальное количество программных потоков, обслуживающих подключения */
  int (*p_fnCBAddr)(const struct SAcceptedSock *p_psoAcceptedSocket));

void iplistener_fini (struct SIPListener *p_psoIPListener);
