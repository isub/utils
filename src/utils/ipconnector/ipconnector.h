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
	CIPConnector ();
	CIPConnector (int p_iReqTimeout);
	~CIPConnector ();
public:
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
	/* время максимально допустимого ожидания операций с сокетом в секундах.
	   задается в конструкторе CIPConnector (int p_iReqTimeout). по умолчанию = 5 сек */
	int m_iRequestTimeout;
#ifdef WIN32
	SOCKET
#else
	int
#endif
	m_sockSock;
private:
	void init (int p_iReqTimeout);
};
