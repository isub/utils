#ifndef _MYSMTP_H_
#define _MYSMTP_H_

#ifdef	WIN32
#include <Windows.h>
#	ifdef	MYSMTP_EXPORT
#		define	MYSMTP_SPEC	__declspec(dllexport)
#	else
#		define	MYSMTP_SPEC __declspec(dllimport)
#	endif
#else
#	define	MYSMTP_SPEC
#endif

#include <string>
#include <vector>

class MYSMTP_SPEC CMySMTP {
public:
	int SendMail (
		const char *p_pcszSMTPServer,
		const unsigned short p_usPort,
		const char *p_pcszFrom,
		std::vector<std::string> *p_pvectRecList,
		std::vector<std::string> *p_pvectCopyList,
		std::vector<std::string> *p_pvectBlindCopyList,
		const char *p_pcszSubject,
		const char *p_pcszMessage,
		std::vector<std::string> *p_pvectAttachList,
		std::string *p_pstrReport);
	int VerifyRecipient (
		const char *p_pcszSMTPServer,
		const unsigned short p_usPort,
		const char *p_pcszRecipient,
		std::string *p_pstrReport);
public:
	CMySMTP(void);
	~CMySMTP(void);
private:
	int ConnectToServer (SOCKET *p_psockSock, const char *p_pcszHostName, unsigned short p_usPort);
	int SendMsg (SOCKET p_sockSock, const char *p_pcszMsg, int p_iMsgLen);
	int OutputFile (SOCKET p_sockSock, const char *p_pcszFileName);
	int GetResp (SOCKET p_sockSock, char *p_pmcBuf, int *p_piBufSize);
	int CheckResp (const char *p_pcszMsg, const char *p_pcszResp, int p_iRespLen);
private:
	BOOL m_bInitialized;
};

#endif	// _MYSMTP_H_
