#ifndef _LOG_H_
#define _LOG_H_

#ifdef	WIN32
#include <Windows.h>
#	ifdef	LOG_EXPORT
#		define	LOG_SPEC __declspec(dllexport)
#	else
#		define	LOG_SPEC	__declspec(dllimport)
#	endif
#	define __STRIPPED_FILE__ __FILE__
#else
#	define	LOG_SPEC
#	include <pthread.h>
#	include <unistd.h>
#	ifndef __STRIPPED_FILE__
#		include <libgen.h>
		static char * file_bname = NULL;
		static char * file_bname_init(char * full) { file_bname = basename(full); return file_bname; }
#		define __STRIPPED_FILE__	(file_bname ?: file_bname_init((char *)__FILE__))
#	endif
#endif
#ifdef DEBUG
#define UTL_LOG(logger,status, format,args...)	(logger).WriteLog(status ": %s@%s[%u]: " format, __FUNCTION__, __STRIPPED_FILE__, __LINE__, ## args)
#else
#define UTL_LOG(logger,status, format,args...)	(logger).WriteLog(status ": " format, ## args)
#endif
#define UTL_LOG_E(logger,format,args...)	UTL_LOG(logger,"error",format,##args)
#define UTL_LOG_N(logger,format,args...)	UTL_LOG(logger,"noti",format,##args)
#ifdef DEBUG
#define UTL_LOG_D(logger,format,args...)	UTL_LOG(logger,"debug",format,##args)
#else
#define UTL_LOG_D
#endif
#define UTL_LOG_W(logger,format,args...)	UTL_LOG(logger,"warning",format,##args)
#define UTL_LOG_F(logger,format,args...)	UTL_LOG(logger,"fatal",format,##args)

class LOG_SPEC CLog {
public:
	int Init (const char *p_pcszLogFileMask, char *p_pszEndOfLine = NULL);
	void Flush ();

#ifdef WIN32
#else
	void SetUGIds (gid_t p_idUId, gid_t p_idGId);
#endif
	void WriteLog (const char *p_pszMsg, ...);
	void Dump (const char *p_pszTitle, const char *p_pszMessage);
	void Dump (const char *p_pszMessage);
public:
	CLog ();
	~CLog ();
private:
	int GetLogFileName (char *p_pszLogFileName, size_t p_stMaxSize);
	bool CheckLogFileName (const char *p_pcszLogFileName);
	int OpenLogFile (const char *p_pcszLogFileName);
#ifdef WIN32
	friend DWORD WINAPI ReCreateFileProc (void *p_pParam);
#else
	friend void * ReCreateFileProc (void *p_pParam);
#endif
private:
#ifdef WIN32
	HANDLE m_iLogFileFD;
	HANDLE m_hReCreateFile;
	double m_dCap;
#else
	int m_iLogFileFD;
	pthread_t m_hReCreateFile;
	gid_t m_idUId;
	gid_t m_idGId;
	/* mutex для проверки соответствия имени файла заданной маске */
	pthread_mutex_t m_tMutex;
	/* интервал проверки имени файла в секундах */
	unsigned int m_uiWaitInterval;
#endif
	int m_iInit;
	char *m_pszLogFileMask;
	char *m_pszLogFileName;
	char *m_pszEndOfLine;
};

#endif	// _LOG_H_
