#define FILE_READER_EXPORT
#include "filereader.h"

#include <string>
#include <errno.h>

#ifdef _WIN32
#	include <io.h>	/* "_sopen_s" declaration */
#else
#	define ERROR_INVALID_PARAMETER EINVAL
#	define ERROR_OUTOFMEMORY ENOMEM
#	define _close close
#	define _read read
#endif
#include <fcntl.h>	/* "O_RDONLY | O_BINARY | O_SEQUENTIAL" definitions */
#include <sys/stat.h>	/* _S_IREAD */
#include <stdlib.h>

#include <string.h>
#ifdef _WIN32
#	include <curl/curl.h>
#else
#	include <curl/curl.h>
#endif

#ifdef WIN32
	/* функции и типы данных для работы с библиотекой CURL ********************/
	typedef CURLcode (*typeCURLGlobalInit)(long);
	typedef void (*typeCURLGlobalCleanup)(void);
	typedef CURL* (*typeCURLEasyInit)(void);
	typedef void (*typeCURLEasyCleanup)(CURL*);
	typedef CURLcode (*typeCURLEasySetopt)(CURL*, CURLoption, ...);
	typedef CURLcode (*typeCURLEasyPerform)(CURL*);

	typeCURLGlobalInit		fn_curl_global_init = NULL;
	typeCURLEasyInit		fn_curl_easy_init = NULL;
	typeCURLEasySetopt		fn_curl_easy_setopt = NULL;
	typeCURLEasyPerform		fn_curl_easy_perform = NULL;
	typeCURLEasyCleanup		fn_curl_easy_cleanup = NULL;
	typeCURLGlobalCleanup	fn_curl_global_cleanup = NULL;
	/*****************************************************************************/
#	define	close _close
#	define strdup _strdup
	/* функция для чтения данный с диска **************************************/
	DWORD WINAPI FS_LoadFile (void *p_pcoThis);
#else
#	define fn_curl_global_init		curl_global_init
#	define fn_curl_easy_init		curl_easy_init
#	define fn_curl_easy_setopt		curl_easy_setopt
#	define fn_curl_easy_perform	curl_easy_perform
#	define fn_curl_easy_cleanup	curl_easy_cleanup
#	define fn_curl_global_cleanup	curl_global_cleanup
	void * CURL_LoadFile (void *p_pcoThis);
	void * FS_LoadFile (void *p_pcoThis);
#endif

int CFileReader::OpenDataFile (
		const char *p_pszType,
#ifdef _WIN32
		SFileInfo *p_psoCURLLib,
#endif
		SFileInfo &p_soFileInfo,
		std::string *p_pstrHost,
		std::string *p_pstrUserName,
		std::string *p_pstrPassword)
{
	int iRetVal = 0;
	int iFnRes;

#ifdef WIN32
	m_hReadStarted = CreateEvent (NULL, TRUE, FALSE, NULL);
	if (NULL == m_hReadStarted) {
		iRetVal = GetLastError ();
		return iRetVal;
	}
#endif

	/* запоминаем информацию о файле */
	m_soFileInfo = p_soFileInfo;

	/* запоминаем тип */
	m_strType = p_pszType;

	if (p_pstrHost) {
		m_pszHost = strdup (p_pstrHost->c_str ());
	}
	if (p_pstrUserName) {
		m_pszUserName = strdup (p_pstrUserName->c_str ());
	}
	if (p_pstrPassword) {
		m_pszPassword = strdup (p_pstrPassword->c_str ());
	}

	/* формируем полное имя файла */
	std::string strFileName;
	if (m_soFileInfo.m_strDir.length ()) {
		strFileName = m_soFileInfo.m_strDir;
		if (strFileName[strFileName.length () - 1] != '/'
			&& strFileName[strFileName.length () - 1] != '\\') {
			strFileName += '/';
		}
	}
	strFileName += m_soFileInfo.m_strTitle;

	do {
		/* производим подготовительные для работы с файлом */
		if (0 == strcmp (p_pszType, "file")) {       /* открываем файл */
			/* чтение файлов без буферизации */
#ifdef _WIN32
			m_hWriteThread = CreateThread (NULL, 0, FS_LoadFile, this, 0, NULL);
			if (NULL == m_hWriteThread) {
				iRetVal = GetLastError ();
				m_hWriteThread = (HANDLE) -1;
				break;
			}
#else
			iFnRes = pthread_create (&m_hWriteThread, NULL, FS_LoadFile, this);
			if (iFnRes) {
				iRetVal = iFnRes;
				m_hWriteThread = (pthread_t) -1;
				break;
			}
#endif
		} else if (0 == strcmp (p_pszType, "ftp")
				|| 0 == strcmp (p_pszType, "sftp")
				|| 0 == strcmp (p_pszType, "ftps")) { /* загрузка файла с использованием библиотеки CURL */
			if (NULL == p_psoCURLLib) {
				return EINVAL;
			}
			/* чтение данных с буферизацией */
			/* создаем поток записи данных в буфер */
#ifdef _WIN32
			iFnRes = CURL_Init (*p_psoCURLLib);
			if (iFnRes) {
				iRetVal = iFnRes;
				break;
			}
			m_hWriteThread = CreateThread (NULL, 0, CURL_LoadFile, this, 0, NULL);
			if (NULL == m_hWriteThread) {
				iRetVal = GetLastError ();
				m_hWriteThread = (HANDLE) -1;
				break;
			}
#else
			iFnRes = pthread_create (&m_hWriteThread, NULL, CURL_LoadFile, this);
			if (iFnRes) {
				iRetVal = iFnRes;
				m_hWriteThread = (pthread_t) -1;
				break;
			}
#endif
		} else {
			iRetVal = -1;
		}
	} while (0);

	/* ждем когда начнется чтение данных */
#ifdef WIN32
	if (0 == iRetVal) {
		WaitForSingleObject (m_hReadStarted, INFINITE);
	}
	if (NULL != m_hReadStarted) {
		CloseHandle (m_hReadStarted);
		m_hReadStarted = NULL;
	}
#endif

	return iRetVal;
}

int CFileReader::ReadData (unsigned char *p_pucData, int &p_iDataSize)
{
	int iRetVal = 0;

#ifdef _WIN32
	EnterCriticalSection (&m_soCSBuf);
#else
	pthread_mutex_lock (&m_tMutex);
#endif

	do {
		/* на всякий случай проверяем значение параметров */
		if (0 == p_iDataSize) {
			iRetVal = 0;
			break;
		}
		if (NULL == p_pucData) {
			p_iDataSize = 0;
			iRetVal = ERROR_INVALID_PARAMETER;
			break;
		}
		/* если запись в буфер завершена и он пуст */
		if (IsWritingBufCompl () && IsBufferEmpty ()) {
			p_iDataSize = 0;
			iRetVal = -1;
			break;
		}
		/* определяем объем доступных данных */
		p_iDataSize = m_stFileSize - m_stReadPointer > p_iDataSize ? p_iDataSize : m_stFileSize - m_stReadPointer;
		/* если нечего копировать */
		if (0 == p_iDataSize) {
			break;
		}
		/* копируем запрошенный блок данных */
		memcpy (p_pucData, &m_pmucBuf[m_stReadPointer], p_iDataSize);
		m_stReadPointer += p_iDataSize;
	} while (0);

#ifdef _WIN32
	LeaveCriticalSection (&m_soCSBuf);
#else
	pthread_mutex_unlock (&m_tMutex);
#endif

	return iRetVal;
}

int CFileReader::CloseDataFile ()
{
	int iRetVal = 0;

	if (-1 != m_hFile) {
		_close (m_hFile);
		m_hFile = -1;
	} else {
		iRetVal = -1;
	}
#ifdef _WIN32
	if ((HANDLE) -1 != m_hWriteThread) {
		DWORD dwExitCode;
		/* сигнализируем потоку о необходимости прекратить запись в буфер */
		m_iCancelWritingBuf = 1;
		WaitForSingleObject (m_hWriteThread, INFINITE);
		GetExitCodeThread (m_hWriteThread, &dwExitCode);
		iRetVal = (int) dwExitCode;
		CloseHandle (m_hWriteThread);
		m_hWriteThread = (HANDLE) -1;
		if (m_hCURLLib) {
			FreeLibrary (m_hCURLLib);
			m_hCURLLib = NULL;
		}
#else
	if ((pthread_t) -1 != m_hWriteThread) {
		m_iCancelWritingBuf = 1;
		pthread_join (m_hWriteThread, NULL);
		m_hWriteThread = (pthread_t) -1;
#endif
	}
	m_stReadPointer = 0;
	m_stFileSize = 0;
	m_iIsWritingBufCompl = 0;
	m_soFileInfo.m_stFileSize = 0;
	m_soFileInfo.m_strDir = "";
	m_soFileInfo.m_strTitle = "";
	if (m_pszHost) {
		free (m_pszHost);
		m_pszHost = NULL;
	}
	if (m_pszUserName) {
		free (m_pszUserName);
		m_pszUserName = NULL;
	}
	if (m_pszPassword) {
		free (m_pszPassword);
		m_pszPassword = NULL;
	}
	m_iCancelWritingBuf = 0;

	return iRetVal;
}

int CFileReader::IsWritingBufCompl ()
{
	return m_iIsWritingBufCompl;
}

int CFileReader::IsBufferEmpty ()
{
	if (m_stFileSize == m_stReadPointer) {
		return 1;
	} else {
		return 0;
	}
}

int CFileReader::AllocateMemBlock (size_t p_stSize)
{
	int iRetVal = 0;

#ifdef _WIN32
			EnterCriticalSection (&m_soCSBuf);
#else
			pthread_mutex_lock (&m_tMutex);
#endif
	do {
		if (p_stSize > m_stBufSize) {
			/* запрашиваем у системы новый блок памяти */
			unsigned char *pmucTmp = reinterpret_cast<unsigned char*> (malloc (p_stSize));
			if (NULL == pmucTmp) {
				iRetVal = ERROR_OUTOFMEMORY;
				break;
			};
			/* копируем прежнее содержимое в новый буфер, если в нем что-то есть */
			if (m_pmucBuf) {
				memcpy (pmucTmp, m_pmucBuf, m_stBufSize);
				/* освобождаем старый буфер */
				free (m_pmucBuf);
			}
			m_pmucBuf = pmucTmp;
			m_stBufSize = p_stSize;
		}
	} while (0);
#ifdef _WIN32
			LeaveCriticalSection (&m_soCSBuf);
#else
			pthread_mutex_unlock (&m_tMutex);
#endif

	return iRetVal;
}

#ifdef _WIN32
int CFileReader::CURL_Init (SFileInfo &p_soCURLLibInfo)
{
	int iRetVal = 0;

	do {
		SetDllDirectoryA (p_soCURLLibInfo.m_strDir.c_str ());

		/* загружаем библиотеку */
		if (NULL == m_hCURLLib) {
			m_hCURLLib = LoadLibraryA (p_soCURLLibInfo.m_strTitle.c_str ());
			if (NULL == m_hCURLLib) {
				iRetVal = GetLastError ();
				break;
			}
		}

		/* загружаем необходимые функции из библиотеки */
		if (NULL == fn_curl_global_init) {
			fn_curl_global_init = reinterpret_cast<typeCURLGlobalInit>(GetProcAddress (m_hCURLLib, "curl_global_init"));
			if (NULL == fn_curl_global_init) {
				iRetVal = GetLastError ();
				break;
			}
		}
		if (NULL == fn_curl_easy_init) {
			fn_curl_easy_init = reinterpret_cast<typeCURLEasyInit> (GetProcAddress (m_hCURLLib, "curl_easy_init"));
			if (NULL == fn_curl_easy_init) {
				iRetVal = GetLastError ();
				break;
			}
		}
		if (NULL == fn_curl_easy_setopt) {
			fn_curl_easy_setopt = reinterpret_cast<typeCURLEasySetopt> (GetProcAddress (m_hCURLLib, "curl_easy_setopt"));
			if (NULL == fn_curl_easy_setopt) {
				iRetVal = GetLastError ();
				break;
			}
		}
		if (NULL == fn_curl_easy_perform) {
			fn_curl_easy_perform = reinterpret_cast<typeCURLEasyPerform> (GetProcAddress (m_hCURLLib, "curl_easy_perform"));
			if (NULL == fn_curl_easy_perform) {
				iRetVal = GetLastError ();
				break;
			}
		}
		if (NULL == fn_curl_easy_cleanup) {
			fn_curl_easy_cleanup = reinterpret_cast<typeCURLEasyCleanup> (GetProcAddress (m_hCURLLib, "curl_easy_cleanup"));
			if (NULL == fn_curl_easy_cleanup) {
				iRetVal = GetLastError ();
				break;
			}
		}
		if (NULL == fn_curl_global_cleanup) {
			fn_curl_global_cleanup = reinterpret_cast<typeCURLGlobalCleanup> (GetProcAddress (m_hCURLLib, "curl_global_cleanup"));
			if (NULL == fn_curl_global_cleanup) {
				iRetVal = GetLastError ();
				break;
			}
		}

	} while (0);

	/* если возникла ошибка освобождаем библиотеку */
	if (iRetVal) {
		if (m_hCURLLib) {
			FreeLibrary (m_hCURLLib);
			m_hCURLLib = NULL;
		}
	}

	return iRetVal;
}
#endif

size_t CURL_Write (void *p_pvSrc, size_t p_stSize, size_t p_stCount, CFileReader *p_pcoFileReader)
{
	size_t stSize = p_stSize * p_stCount;

	if (p_pcoFileReader->m_iCancelWritingBuf) {
		return 0;
	}

	if (p_pcoFileReader->AllocateMemBlock (p_pcoFileReader->m_stFileSize + stSize)) {
		stSize = 0;
	} else {
#ifdef _WIN32
		EnterCriticalSection (&p_pcoFileReader->m_soCSBuf);
#else
		pthread_mutex_lock (&(p_pcoFileReader->m_tMutex));
#endif
		memcpy (
			&p_pcoFileReader->m_pmucBuf[p_pcoFileReader->m_stFileSize],
			p_pvSrc,
			stSize);
#ifdef _WIN32
		LeaveCriticalSection (&p_pcoFileReader->m_soCSBuf);
#else
		pthread_mutex_unlock (&(p_pcoFileReader->m_tMutex));
#endif
		p_pcoFileReader->m_stFileSize += stSize;
	}

	return stSize;
}

#ifdef _WIN32
DWORD WINAPI CURL_LoadFile (void *p_pcoThis)
#else
void * CURL_LoadFile (void *p_pcoThis)
#endif
{
	CFileReader *pcoThis = reinterpret_cast<CFileReader*> (p_pcoThis);
	int iRetVal = 0;
	int iFnRes;
	CURLcode curlRes;
	int iGlobalInit = 0;
	CURL *pvCurl = NULL;
	std::string strURL;

	do {
		/* инициализация библиотеки CURL */
		curlRes = fn_curl_global_init (CURL_GLOBAL_DEFAULT);
		if (CURLE_OK != curlRes) {
			iRetVal = curlRes;
			break;
		}
		iGlobalInit = 1;

		/* инициализация экземпляра CURL */
		pvCurl = fn_curl_easy_init ();
		if (NULL == pvCurl) {
			iRetVal = -1;
			break;
		}

		/* user name */
		if (pcoThis->m_pszUserName) {
			curlRes = fn_curl_easy_setopt (pvCurl, CURLOPT_USERNAME, pcoThis->m_pszUserName);
			if (curlRes) {
				iRetVal = curlRes;
				break;
			}
		}

		/* user password */
		if (pcoThis->m_pszPassword) {
			curlRes = fn_curl_easy_setopt (pvCurl, CURLOPT_PASSWORD, pcoThis->m_pszPassword);
			if (curlRes) {
				iRetVal = curlRes;
				break;
			}
		}

		/* set write function */
		curlRes = fn_curl_easy_setopt (pvCurl, CURLOPT_WRITEFUNCTION, CURL_Write);
		if (curlRes) {
			iRetVal = curlRes;
			break;
		}

		/* set write data */
		curlRes = fn_curl_easy_setopt (pvCurl, CURLOPT_WRITEDATA, pcoThis);
		if (curlRes) {
			iRetVal = curlRes;
			break;
		}

		/* URL */
		strURL = pcoThis->m_strType;
		strURL += "://";
		strURL += pcoThis->m_pszHost;
		strURL += '/';
		strURL += pcoThis->m_soFileInfo.m_strDir;
		if (strURL[strURL.length () - 1] != '/') {
			strURL += '/';
		}
		strURL += pcoThis->m_soFileInfo.m_strTitle;
		curlRes = fn_curl_easy_setopt (pvCurl, CURLOPT_URL, strURL.c_str ());
		if (curlRes) {
			iRetVal = curlRes;
			break;
		}

		/* попробуем выделить память для чтения файла заранее */
		if (pcoThis->m_soFileInfo.m_stFileSize != -1 && pcoThis->m_soFileInfo.m_stFileSize > 0) {
			iFnRes = pcoThis->AllocateMemBlock (pcoThis->m_soFileInfo.m_stFileSize);
			if (iFnRes) {
				iRetVal = iFnRes;
				break;
			}
		}

		/* если чтение отменено */
		if (pcoThis->m_iCancelWritingBuf) {
			break;
		}

		/* execute */
		curlRes = fn_curl_easy_perform (pvCurl);
		/* запись в буфер завершена */
		pcoThis->m_iIsWritingBufCompl = 1;

		/* если при выполнении запроса произошла ошибка */
		if (curlRes) {
			iRetVal = curlRes;
			break;
		}

	} while (0);

	if (pvCurl) {
		fn_curl_easy_cleanup (pvCurl);
		pvCurl = NULL;
	}
	if (iGlobalInit) {
		fn_curl_global_cleanup ();
		iGlobalInit = 0;
	}

#ifdef _WIN32
	ExitThread (iRetVal);
#else
	pthread_exit (0);
#endif
}

CFileReader::CFileReader(void)
{
#ifdef WIN32
	InitializeCriticalSection (&m_soCSBuf);
	m_hWriteThread = (HANDLE) -1;
	m_hCURLLib = NULL;
	m_hReadStarted = NULL;
#else
	pthread_mutex_init (&m_tMutex, NULL);
	m_hWriteThread = (pthread_t) -1;
#endif
	m_stBufSize = 0;
	m_pmucBuf = NULL;
	AllocateMemBlock (65536);
	m_hFile = -1;
	m_stFileSize = 0;
	m_stReadPointer = 0;
	m_iIsWritingBufCompl = 0;
	m_iCancelWritingBuf = 0;
	m_pszHost = NULL;
	m_pszUserName = NULL;
	m_pszPassword = NULL;
}

CFileReader::~CFileReader(void)
{
	if (-1 != m_hFile) {
		close (m_hFile);
		m_hFile = -1;
	}
#ifdef _WIN32
	if ((HANDLE) -1 != m_hWriteThread) {
		m_iCancelWritingBuf = 1;
		WaitForSingleObject (m_hWriteThread, INFINITE);
		m_hWriteThread = (HANDLE) -1;
#else
	if ((pthread_t) -1 != m_hWriteThread) {
		m_iCancelWritingBuf = 1;
		pthread_join (m_hWriteThread, NULL);
		m_hWriteThread = (pthread_t) -1;
#endif
	}
#ifdef _WIN32
	EnterCriticalSection (&m_soCSBuf);
#else
	pthread_mutex_lock (&m_tMutex);
#endif
	if (m_pmucBuf) {
		free (m_pmucBuf);
		m_pmucBuf = NULL;
	}
#ifdef _WIN32
	LeaveCriticalSection (&m_soCSBuf);
#else
	pthread_mutex_unlock (&m_tMutex);
#endif
	m_stBufSize = 0;
	m_stFileSize = 0;
	m_stReadPointer = 0;
#ifdef WIN32
	DeleteCriticalSection (&m_soCSBuf);
	if (NULL != m_hReadStarted) {
		CloseHandle (m_hReadStarted);
		m_hReadStarted = NULL;
	}
#else
	pthread_mutex_destroy (&m_tMutex);
#endif
	if (m_pszHost) {
		free (m_pszHost);
		m_pszHost = NULL;
	}
	if (m_pszUserName) {
		free (m_pszUserName);
		m_pszUserName = NULL;
	}
	if (m_pszPassword) {
		free (m_pszPassword);
		m_pszPassword = NULL;
	}
}

/*****************************************************************************/
#ifdef _WIN32
DWORD WINAPI FS_LoadFile (void *p_pcoThis)
#else
void * FS_LoadFile (void *p_pcoThis)
#endif
{
	CFileReader *pcoThis = reinterpret_cast<CFileReader*> (p_pcoThis);
	int iRetVal = 0;
	int iFnRes;
	std::string strFileName;
	unsigned char mucBuf[0x10000];

	do {
		if (pcoThis->m_iCancelWritingBuf) {
			iRetVal = -1;
			break;
		}
		/* проверка содержимого дескриптора */
		if (-1 != pcoThis->m_hFile) {
			iRetVal = -1;
			break;
		}
		/* формируем полное имя файла */
		if (pcoThis->m_soFileInfo.m_strDir.length()) {
			strFileName = pcoThis->m_soFileInfo.m_strDir;
			if (strFileName[strFileName.length () - 1] != '/'
#ifdef WIN32
				&& strFileName[strFileName.length () - 1] != '\\') {
				strFileName += '\\';
#else
				) {
				strFileName += '/';
#endif
			}
		}
		strFileName += pcoThis->m_soFileInfo.m_strTitle;
		/* открываем дескриптор файла */
#ifdef WIN32
		iFnRes = _sopen_s (&(pcoThis->m_hFile), strFileName.c_str(), O_RDONLY | O_BINARY | O_SEQUENTIAL, _SH_DENYWR, _S_IREAD);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
#else
		pcoThis->m_hFile = open (strFileName.c_str(), O_RDONLY);
		if (-1 == pcoThis->m_hFile) {
			iRetVal = errno;
			break;
		}
#endif
		/* если размер файла задан, сразу запрашиваем блок памяти */
		if (-1 != pcoThis->m_soFileInfo.m_stFileSize) {
			pcoThis->AllocateMemBlock (pcoThis->m_soFileInfo.m_stFileSize);
		}
		/* считываем файл поблочно, чтобы конвертор мог сразу начать обработку данных */
		while (0 < (iFnRes = _read (pcoThis->m_hFile, mucBuf, sizeof(mucBuf)))) {
#ifdef WIN32
			if (NULL != pcoThis->m_hReadStarted) {
				SetEvent (pcoThis->m_hReadStarted);
			}
#endif
			/* если размер файла не задан, дозапрашиваем блок памяти для очередного блока данных */
			if (-1 == pcoThis->m_soFileInfo.m_stFileSize) {
				iFnRes = pcoThis->AllocateMemBlock (pcoThis->m_soFileInfo.m_stFileSize + sizeof (mucBuf));
				if (iFnRes) {
					iRetVal = iFnRes;
					break;
				}
			}
			memcpy (
				&pcoThis->m_pmucBuf[pcoThis->m_stFileSize],
				mucBuf,
				iFnRes);
			pcoThis->m_stFileSize += iFnRes;
		}
		if (iRetVal) {
			break;
		}
	} while (0);

	/* запись в буфер завершена */
	pcoThis->m_iIsWritingBufCompl = 1;

	if (NULL != pcoThis->m_hReadStarted) {
		SetEvent (pcoThis->m_hReadStarted);
	}

#ifdef _WIN32
	ExitThread (iRetVal);
#else
	pthread_exit (0);
#endif
}
