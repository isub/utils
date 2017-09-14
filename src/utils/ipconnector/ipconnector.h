#ifdef	WIN32
#	ifdef	IPCONNECTOR_IMPORT
#		define	IPCONNECTOR_SPEC	__declspec(dllimport)
#	else
#		define	IPCONNECTOR_SPEC __declspec(dllexport)
#	endif
#else
#	define	IPCONNECTOR_SPEC
#	include <netinet/in.h>
#endif

class IPCONNECTOR_SPEC CIPConnector {
public:
	CIPConnector();
	CIPConnector( unsigned int p_uiReqTimeout ); /* в конструкторе таймаут задается в секундах для обратной совместимости */
	~CIPConnector();
public:
  /* функция SetTimeout задает таймаут в миллисекундах */
  void SetTimeout( unsigned int p_uiMilliSec ) { m_uiRequestTimeout = p_uiMilliSec; }
	int Connect (const char *p_pszHostName, unsigned short p_usPort, int p_iProtoType = IPPROTO_TCP);
	int Send (const char *p_mcBuf, int p_iLen);
	int Recv (char *p_mcBuf, int p_iBufSize);
	void DisConnect ();
	/* статус 0 - объект класса создан
	   статус 1 - объект класса инициализирован (для Windows дополнительно означает, что библиотека WSA загружена)
	   статус 2 - соединение установлено */
	int GetStatus () { return m_iStatus; }
private:
	/* статус объекта */
	int m_iStatus;
	/* время максимального ожидания операций с сокетом в миллисекундах */
	unsigned int m_uiRequestTimeout;
#ifdef WIN32
	SOCKET
#else
	int
#endif
	m_sockSock;
private:
	void init( unsigned int p_uiReqTimeout );
};
