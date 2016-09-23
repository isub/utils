#ifdef WIN32
#	include	<Windows.h>
#	include <Winuser.h>
#	include <io.h>
#	define	snprintf_(a,b,c,d)	_snprintf_s(a,b,b,c,d)
#	define	vsnprintf_(a,b,c,d)	vsnprintf_s(a,b,b,c,d)
#	define	strdup	_strdup
#	define	F_OK	0
#else
#	include <sys/time.h>
#	define	snprintf_(a,b,c,d)	snprintf(a,b,c,d)
#	define	vsnprintf_(a,b,c,d)	vsnprintf(a,b,c,d)
#	define	INVALID_HANDLE_VALUE	-1
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <cstdio>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <string>

#define LOG_EXPORT
#include "log.h"

#ifdef WIN32
DWORD WINAPI
#else
void *
#endif
ReCreateFileProc (void *p_pParam);
int GetTimeStamp (char *p_pszBuf, size_t p_stSize);

void Flush
#ifdef WIN32
	(HANDLE p_iFile);
#else
	(int p_iFile);
#endif

int CLog::Init (const char *p_pcszLogFileMask, char *p_pszEndOfLine)
{
	int iRetVal = 0;
	char mcLogFileName[1024];

	if (INVALID_HANDLE_VALUE != m_iLogFileFD) {
#ifdef _WIN32
		CloseHandle (m_iLogFileFD);
#else
		close (m_iLogFileFD);
#endif
		m_iLogFileFD = INVALID_HANDLE_VALUE;
	}

	do {
		m_pszLogFileMask = strdup (p_pcszLogFileMask);
		iRetVal = GetLogFileName (mcLogFileName, sizeof(mcLogFileName));
		if (iRetVal) {
			break;
		}
		iRetVal = OpenLogFile (mcLogFileName);
		if (iRetVal) {
			break;
		}
		if (p_pszEndOfLine) {
			m_pszEndOfLine = strdup (p_pszEndOfLine);
		} else {
#ifdef	_WIN32
			m_pszEndOfLine = strdup ("\r\n");
#else
			m_pszEndOfLine = strdup ("\n");
#endif
		}
		m_iInit = 1;
#ifdef WIN32
		m_hReCreateFile = CreateThread (NULL, 0, ReCreateFileProc, this, 0, NULL);
		if (NULL == m_hReCreateFile) { iRetVal = -1; break; }
#else
		iRetVal = pthread_create (&m_hReCreateFile, 0, ReCreateFileProc, this);
		if (iRetVal) { break; }
#endif
	} while (0);

	return iRetVal;
}

void CLog::Flush ()
{
	if (INVALID_HANDLE_VALUE != m_iLogFileFD) {
		::Flush (m_iLogFileFD);
	}
}

#ifdef WIN32
void Flush (HANDLE p_iFile)
{
	FlushFileBuffers (p_iFile);
}
#else
void Flush (int p_iFile)
{
	fsync (p_iFile);
}
#endif

#ifdef WIN32
#else
void CLog::SetUGIds (gid_t p_idUId, gid_t p_idGId)
{
	m_idUId = p_idUId;
	m_idGId = p_idGId;
	fchown (m_iLogFileFD, m_idUId, m_idGId);
}
#endif

void CLog::WriteLog (const char *p_pszMsg, ...)
{
	int iStrLen;
	int iFnRes;
	char mcBuf[0x10000];
	va_list vaList;

	/* формируем временную метку */
	iStrLen = GetTimeStamp (mcBuf, sizeof (mcBuf));
	va_start (vaList, p_pszMsg);
	iFnRes = vsnprintf_ (mcBuf + iStrLen, sizeof(mcBuf) - 1 - iStrLen, p_pszMsg, vaList);
	va_end (vaList);
	/* если буфер успешно заполнен */
	if (0 < iFnRes) {
		/* если строка не уместилась в буфере */
		if (static_cast<size_t>(iFnRes) > sizeof(mcBuf) - 1 - iStrLen) {
			iFnRes = sizeof(mcBuf) - 1 - iStrLen;
		}
		iStrLen += iFnRes;
	}
	/* дополняем строку последовательность перевода строки */
	if (m_pszEndOfLine) {
		iFnRes = snprintf_ (mcBuf + iStrLen, sizeof(mcBuf) - 1 - iStrLen, "%s", m_pszEndOfLine);
	}
	/* если буфер успешно заполнен */
	if (0 < iFnRes) {
		/* если строка не уместилась в буфере */
		if (static_cast<size_t>(iFnRes) > sizeof(mcBuf) - 1 - iStrLen) {
			iFnRes = sizeof(mcBuf) - 1 - iStrLen;
		}
		iStrLen += iFnRes;
	}
	if (INVALID_HANDLE_VALUE != m_iLogFileFD) {
#ifdef WIN32
		DWORD dwWritten;
		WriteFile (m_iLogFileFD, mcBuf, iStrLen, &dwWritten, NULL);
#else
		write (m_iLogFileFD, mcBuf, iStrLen);
#endif
	} else {
		puts (mcBuf);
	}
}

void CLog::Dump (const char *p_pszTitle, const char *p_pszMessage)
{
	size_t stStrLen;
	char mcBuf[0x10000];

	stStrLen = GetTimeStamp (mcBuf, sizeof(mcBuf));
#ifdef WIN32
	stStrLen += _snprintf_s (mcBuf + stStrLen, sizeof(mcBuf) - stStrLen, sizeof(mcBuf) - stStrLen, "%s: %s", p_pszTitle, p_pszMessage);
#else
	stStrLen += snprintf (mcBuf + stStrLen, sizeof(mcBuf) - stStrLen, "%s: %s", p_pszTitle, p_pszMessage);
#endif
	stStrLen += snprintf_ (mcBuf + stStrLen, sizeof(mcBuf) - stStrLen, "%s", m_pszEndOfLine);

	if (INVALID_HANDLE_VALUE != m_iLogFileFD) {
#ifdef WIN32
		DWORD dwWritten;
		WriteFile (m_iLogFileFD, mcBuf, stStrLen, &dwWritten, NULL);
#else
		write (m_iLogFileFD, mcBuf, stStrLen);
#endif
	} else {
		puts (mcBuf);
	}
}

void CLog::Dump (const char *p_pszMessage)
{
	int iStrLen;
	int iFnRes;
	char mcBuf[0x10000];

	/* формируем временную метку */
	iStrLen = GetTimeStamp (mcBuf, sizeof (mcBuf));
	iFnRes = snprintf_ (mcBuf + iStrLen, sizeof(mcBuf) - 1 - iStrLen, "%s", p_pszMessage);
	/* если буфер успешно заполнен */
	if (0 < iFnRes) {
		/* если строка не уместилась в буфере */
		if (static_cast<size_t>(iFnRes) > sizeof(mcBuf) - 1 - iStrLen) {
			iFnRes = sizeof(mcBuf) - 1 - iStrLen;
		}
		iStrLen += iFnRes;
	}
	/* дополняем строку последовательность перевода строки */
	if (m_pszEndOfLine) {
		iFnRes = snprintf_ (mcBuf + iStrLen, sizeof(mcBuf) - 1 - iStrLen, "%s", m_pszEndOfLine);
	}
	/* если буфер успешно заполнен */
	if (0 < iFnRes) {
		/* если строка не уместилась в буфере */
		if (static_cast<size_t>(iFnRes) > sizeof(mcBuf) - 1 - iStrLen) {
			iFnRes = sizeof(mcBuf) - 1 - iStrLen;
		}
		iStrLen += iFnRes;
	}
	if (INVALID_HANDLE_VALUE != m_iLogFileFD) {
#ifdef WIN32
		DWORD dwWritten;
		WriteFile (m_iLogFileFD, mcBuf, iStrLen, &dwWritten, NULL);
#else
		write (m_iLogFileFD, mcBuf, iStrLen);
#endif
	} else {
		puts (mcBuf);
	}
}

int CLog::GetLogFileName (char *p_pszLogFileName, size_t p_stMaxSize)
{
	int iRetVal = 0;

	do {
		time_t tmtTime;
		tm soTm;
		size_t stFileNameLen;

		if ((time_t)-1 == time (&tmtTime)) {
			iRetVal = -1;
			break;
		}
#ifdef	WIN32
		if (0 != localtime_s (&soTm, &tmtTime)) {
#else
		if (&soTm != localtime_r (&tmtTime, &soTm)) {
#endif
			iRetVal = -1;
			break;
		}
		stFileNameLen = strftime (
			p_pszLogFileName,
			p_stMaxSize,
			m_pszLogFileMask,
			&soTm);
		if (0 == stFileNameLen) {
			iRetVal = -1;
		}
	} while (0);

	return iRetVal;
}

bool CLog::CheckLogFileName (const char *p_pszLogFileName)
{
	if (0 == access (p_pszLogFileName, F_OK)) {
		return true;
	} else {
    return false;
  }
}

int CLog::OpenLogFile (const char *p_pcszLogFileName)
{
	int iRetVal = 0;
#ifdef WIN32
	HANDLE
#else
	int
#endif
		iNewFile, iOldFile;

	do {
#ifdef	WIN32
		iNewFile = CreateFileA (p_pcszLogFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL);
		if (INVALID_HANDLE_VALUE == iNewFile) {
			iRetVal = GetLastError ();
			break;
		}
		SetFilePointer (iNewFile, 0, NULL, FILE_END);
		iOldFile = m_iLogFileFD;
		m_iLogFileFD = iNewFile;
		if (INVALID_HANDLE_VALUE != iOldFile) {
			::Flush (iOldFile);
			CloseHandle (iOldFile);
		}
#else
		iNewFile = open (p_pcszLogFileName, O_WRONLY | O_APPEND | O_CREAT, 0644);
		if (INVALID_HANDLE_VALUE == iNewFile) {
			iRetVal = -1;
			break;
		}
		if (m_idUId != (gid_t)(-1) || m_idGId != (gid_t)(-1)) {
			fchown (iNewFile, m_idUId, m_idGId);
		}
		iOldFile = m_iLogFileFD;
		m_iLogFileFD = iNewFile;
		if (INVALID_HANDLE_VALUE != iOldFile) {
			::Flush (iOldFile);
			close (iOldFile);
		}
#endif
		m_pszLogFileName = strdup (p_pcszLogFileName);
	} while (0);

	return iRetVal;
}

int GetTimeStamp (char *p_pszBuf, size_t p_stSize)
{
	int iRetVal;

#ifdef	WIN32
	SYSTEMTIME soLocalTime;

	GetLocalTime (&soLocalTime);
	iRetVal = sprintf_s (
		p_pszBuf, p_stSize,
		"%04u.%02u.%02u %02u:%02u:%02u,%03u : ",
		soLocalTime.wYear, soLocalTime.wMonth, soLocalTime.wDay,
		soLocalTime.wHour, soLocalTime.wMinute, soLocalTime.wSecond,
		soLocalTime.wMilliseconds);
#else
	timeval soTV;
	time_t timeTime;
	tm soTm;

	gettimeofday (&soTV, NULL);
	timeTime = soTV.tv_sec;
	localtime_r (&timeTime, &soTm);
	soTm.tm_year += 1900;
	++ soTm.tm_mon;
	iRetVal = snprintf (
		p_pszBuf,
		p_stSize - 1,
		"%04u.%02u.%02u %02u:%02u:%02u,%06u : ",
		soTm.tm_year, soTm.tm_mon, soTm.tm_mday,
		soTm.tm_hour, soTm.tm_min, soTm.tm_sec,
		static_cast<unsigned int>(soTV.tv_usec));
#endif

	if (0 < iRetVal) {
		if (static_cast<size_t>(iRetVal) > p_stSize - 1) {
			iRetVal = p_stSize - 1;
		}
	} else {
		iRetVal = 0;
		*p_pszBuf = '\0';
	}

	return iRetVal;
}

CLog::CLog () {
#ifdef WIN32
	m_hReCreateFile = NULL;
#else
	m_idUId = (gid_t)-1;
	m_idGId = (gid_t)-1;
	m_hReCreateFile = 0;
	/* инициализируем мьютекс для ожидания момента проверки соответствия имени файла заданной маске */
	pthread_mutex_init (&m_tMutex, NULL);
	/* при создании мьютекс должен находиться в открытом состоянии, поэтому мы блокируем его */
	pthread_mutex_trylock (&m_tMutex);
	/* по умолчанию интервал ожидания 1 минута */
	m_uiWaitInterval = 60;
#endif
	m_pszLogFileMask = NULL;
	m_pszLogFileName = NULL;
	m_pszEndOfLine = NULL;
	m_iInit = 0;
	m_iLogFileFD = INVALID_HANDLE_VALUE;
}

CLog::~CLog ()
{
	m_iInit = 0;
	if (m_hReCreateFile) {
#ifdef WIN32
		WaitForSingleObject (m_hReCreateFile, 5000);
		m_hReCreateFile = NULL;
#else
		/* снимаем блокировку мьютекса */
		pthread_mutex_unlock (&m_tMutex);
		pthread_join (m_hReCreateFile, NULL);
		pthread_mutex_destroy (&m_tMutex);
		m_hReCreateFile = 0;
#endif
	}
	if (INVALID_HANDLE_VALUE != m_iLogFileFD) {
		/* если дескриптор содержит допустимое значение */
		/* дозаписываем содержимое буферов на диск */
		Flush ();
		/* закрыаем файл */
#ifdef WIN32
		CloseHandle (m_iLogFileFD);
#else
		close (m_iLogFileFD);
#endif
		/* значение дескриптора теперь неактуально */
		m_iLogFileFD = INVALID_HANDLE_VALUE;
	}
	/* освобождаем занятые ресурсы */
	if (m_pszLogFileMask) {
		free (m_pszLogFileMask);
		m_pszLogFileMask = NULL;
	}
	if (m_pszLogFileName) {
		free (m_pszLogFileName);
		m_pszLogFileName = NULL;
	}
	if (m_pszEndOfLine) {
		free (m_pszEndOfLine);
		m_pszEndOfLine = NULL;
	}
}

#ifdef WIN32
DWORD WINAPI
#else
void *
#endif
ReCreateFileProc (void *p_pParam)
{
	CLog * pcoLog = (CLog*)p_pParam;
#ifdef WIN32
	SYSTEMTIME soSysTime;
#else
	timespec soTimeSpec = { 0, 0 };
	timeval soTimeVal = { 0, 0 };
#endif

	do {
		if (NULL == pcoLog) {
			break;
		}

    char mcLogFileName[1024];

		while (pcoLog->m_iInit) {
      if (0 == pcoLog->GetLogFileName (mcLogFileName, sizeof(mcLogFileName))) {
        if (! pcoLog->CheckLogFileName (mcLogFileName)) {
          pcoLog->OpenLogFile (mcLogFileName);
        }
      }
#ifdef WIN32
			GetSystemTime (&soSysTime);
			Sleep (1);
#else
			/* ждем нужный момент */
			/* получаем текущее время */
			gettimeofday (&soTimeVal, NULL);
			/* задаем время срабатывания мьютекса */
			soTimeSpec.tv_sec = soTimeVal.tv_sec + pcoLog->m_uiWaitInterval;
			/* выравниваем секунды по краям интервала */
			soTimeSpec.tv_sec /= pcoLog->m_uiWaitInterval;
			soTimeSpec.tv_sec *= pcoLog->m_uiWaitInterval;
			/* запускаем ожидание */
			pthread_mutex_timedlock (&pcoLog->m_tMutex, &soTimeSpec);
#endif
		}
	} while (0);

#ifdef WIN32
	ExitThread (0);
#else
	pthread_exit (NULL);
#endif
}
