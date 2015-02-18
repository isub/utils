#define FILE_LIST_EXPORT
#include "filelist.h"
#ifdef _WIN32
#	include <curl.h>
#else
#	include <curl/curl.h>
#endif

#ifdef WIN32
#	include <Windows.h>
#else
#	include <sys/types.h>
#	include <dirent.h>
#	include <errno.h>
#	define	fn_curl_global_init		curl_global_init
#	define	fn_curl_easy_init		curl_easy_init
#	define	fn_curl_easy_setopt		curl_easy_setopt
#	define	fn_curl_easy_perform	curl_easy_perform
#	define	fn_curl_easy_cleanup	curl_easy_cleanup
#	define	fn_curl_global_cleanup	curl_global_cleanup
#endif

#ifdef _WIN32
	typedef CURLcode (*typeCURLGlobalInit)(long);
	typedef void (*typeCURLGlobalCleanup)(void);
	typedef CURL* (*typeCURLEasyInit)(void);
	typedef void (*typeCURLEasyCleanup)(CURL*);
	typedef CURLcode (*typeCURLEasySetopt)(CURL*, CURLoption, ...);
	typedef CURLcode (*typeCURLEasyPerform)(CURL*);

	static typeCURLGlobalInit		fn_curl_global_init;
	static typeCURLEasyInit			fn_curl_easy_init;
	static typeCURLEasySetopt		fn_curl_easy_setopt;
	static typeCURLEasyPerform		fn_curl_easy_perform;
	static typeCURLEasyCleanup		fn_curl_easy_cleanup;
	static typeCURLGlobalCleanup	fn_curl_global_cleanup;
#endif

/* прототипы функций для работы с различными типами хранилищ */
/* оболочка для работы с библиотекой CURL */
int CURL_CreateFileList (SFileListInfo &p_soFileListOpt);
/* оболочка для работы с локальными и NFS файлами */
int FS_CreateFileList (SFileListInfo &p_soFileListOpt);
/*****************************************************************************/

int CFileList::CreateFileList (SFileListInfo &p_soFileListOpt)
{
	int iRetVal = 0;

	if (0 == p_soFileListOpt.m_strFileType.compare ("file")) {
		iRetVal = FS_CreateFileList (p_soFileListOpt);
	} else if (0 == p_soFileListOpt.m_strFileType.compare ("ftp")) {
		iRetVal = CURL_CreateFileList (p_soFileListOpt);
	} else if (0 == p_soFileListOpt.m_strFileType.compare ("sftp")) {
		iRetVal = CURL_CreateFileList (p_soFileListOpt);
	} else if (0 == p_soFileListOpt.m_strFileType.compare ("ftps")) {
		iRetVal = CURL_CreateFileList (p_soFileListOpt);
	} else {
		iRetVal = -1;
	}
	return iRetVal;
}

static size_t CURL_write_it (
	char *buff,
	size_t size,
	size_t nmemb,
	SFileListInfo *p_psoFileListOpt);
void CURL_OperateFileList (
	char *p_pmcBuf,
	size_t p_stSize,
	size_t p_stNMemb,
	SFileListInfo &p_soFileListOpt);

int CURL_CreateFileList (SFileListInfo &p_soFileListOpt)
{
	int iRetVal = 0;
	int iFnRes;
	CURL *pHandle = NULL;

	do {
		/* инициализация дескриптора */
		pHandle = fn_curl_easy_init ();
		if (NULL == pHandle) {
			iRetVal = CURLE_OUT_OF_MEMORY;
			break;
		}

		/* this callback will write contents into files */ 
		iFnRes = fn_curl_easy_setopt(pHandle, CURLOPT_WRITEFUNCTION, CURL_write_it);
		if (CURLE_OK != iFnRes) {
			iRetVal = iFnRes;
			break;
		}

		/* put transfer data into callbacks */ 
		iFnRes = fn_curl_easy_setopt(pHandle, CURLOPT_WRITEDATA, &p_soFileListOpt);
		if (CURLE_OK != iFnRes) {
			iRetVal = iFnRes;
			break;
		}

		/* set an user name */
		iFnRes = fn_curl_easy_setopt (pHandle, CURLOPT_USERNAME , p_soFileListOpt.m_strUserName.c_str ());
		if (CURLE_OK != iFnRes) {
			iRetVal = iFnRes;
			break;
		}

		/* set an password */
		iFnRes = fn_curl_easy_setopt (pHandle, CURLOPT_PASSWORD , p_soFileListOpt.m_strPassword.c_str ());
		if (CURLE_OK != iFnRes) {
			iRetVal = iFnRes;
			break;
		}

		/* set an URL containing wildcard pattern (only in the last part) */
		std::string strURL;
		strURL = p_soFileListOpt.m_strFileType + "://" + p_soFileListOpt.m_strHost + '/';
		if (p_soFileListOpt.m_strPath.length ()) {
			strURL += p_soFileListOpt.m_strPath + "/";
		}
		iFnRes = fn_curl_easy_setopt (pHandle, CURLOPT_URL, strURL.c_str ());
		if (CURLE_OK != iFnRes) {
			iRetVal = iFnRes;
			break;
		}

		/* perform request */
		iFnRes = fn_curl_easy_perform (pHandle);
		if (CURLE_OK != iFnRes) {
			iRetVal = iFnRes;
			break;
		}
	} while (0);

	/* освобождаем занятые ресурсы */
	if (NULL != pHandle) {
		fn_curl_easy_cleanup (pHandle);
	}

	return iRetVal;
}

static size_t CURL_write_it (
	char *p_pmcBuf,
	size_t p_stSize,
	size_t p_stNMemb,
	SFileListInfo *p_psoFileListOpt)
{
	size_t stWritten = 0;

	CURL_OperateFileList (
		p_pmcBuf,
		p_stSize,
		p_stNMemb,
		*p_psoFileListOpt);

	stWritten = p_stNMemb * p_stSize;

	return stWritten;
}

/******************************************************************************
  Образец строки, содержащей информацию о элементе удаленной файловой системы
  drwxr-x---   6 hpiumusr   iumusers        96 Nov  1 00:00 2013
******************************************************************************/
static const char g_mcFormat [] = "%s %u %s %s %u %s %u %u:%u %s";
struct SFTPFileInfo {
	char m_mcMode[16];
	unsigned int m_uiUID;
	char m_mcFileOwner[32];
	char m_mcFileGroup[32];
	unsigned int m_uiFileSize;
	char m_mcMon[8];
	unsigned int m_uiDay;
	unsigned int m_uiHour;
	unsigned int m_uiMin;
	char m_mcFileName[256];
};

void CURL_OperateFileList (
	char *p_pmcBuf,
	size_t p_stSize,
	size_t p_stNMemb,
	SFileListInfo &p_soFileListOpt)
{
	std::string strBuf, strSubstr, strFileTitle;
	size_t stSubstrBeg = 0, stSep = 0;
	SFTPFileInfo soInfo;
	int iFnRes;

	strBuf = p_pmcBuf;
	while (static_cast<size_t> (-1) != (stSep = strBuf.find_first_of ('\n', stSep))) {
		strSubstr = strBuf.substr (stSubstrBeg, stSep - stSubstrBeg);
		stSubstrBeg = stSep + 1;
		++ stSep;

		iFnRes = sscanf (
			strSubstr.c_str (),
			g_mcFormat,
			soInfo.m_mcMode,
			&soInfo.m_uiUID,
			soInfo.m_mcFileOwner,
			soInfo.m_mcFileGroup,
			&soInfo.m_uiFileSize,
			soInfo.m_mcMon,
			&soInfo.m_uiDay,
			&soInfo.m_uiHour,
			&soInfo.m_uiMin,
			soInfo.m_mcFileName);

		if (10 != iFnRes) {
			continue;
		}
		strFileTitle = soInfo.m_mcFileName;
		switch (soInfo.m_mcMode[0]) {
		case 'd': /* если файл является директорией */
			if (p_soFileListOpt.m_iLookNestedFold) { /* если необходимо просматривать вложенные папки */
				std::string strSubDir;
				/* формируем путь к вложенной папке */
				strSubDir = p_soFileListOpt.m_strPath;
				if (strSubDir[strSubDir.length () - 1] != '/') {
					strSubDir += '/';
				}
				strSubDir += strFileTitle;
				SFileListInfo soOpt = p_soFileListOpt;
				soOpt.m_strPath = strSubDir;
				CURL_CreateFileList (soOpt);
			}
			break;
		default: /* во всех остальных случаях */
			SFileInfo soFileInfo = { strFileTitle, p_soFileListOpt.m_strPath, soInfo.m_uiFileSize };
			p_soFileListOpt.m_pmmapFileList->insert (std::make_pair (strFileTitle, soFileInfo));
			break;
		}
	}
}

/*****************************************************************************/

int FS_CreateFileList (SFileListInfo &p_soFileListOpt)
{
	int iRetVal = 0;
	int iFnRes;
	std::string strFindPath;
	std::string::size_type strstStrLen;
	std::string strFileTitle;
#ifdef WIN32
	WIN32_FIND_DATAA soFindData;
	HANDLE hFind;
#else
	DIR *psoDir = NULL;
	dirent *psoDirEnt = NULL;
#endif

	/* формируем строку поиска */
	strFindPath = p_soFileListOpt.m_strPath;
	strstStrLen = strFindPath.length ();
#ifdef WIN32
	if ('\\' != strFindPath[strstStrLen - 1]
		&& '/' != strFindPath[strstStrLen - 1]) {
		strFindPath += "\\";
	}
	strFindPath += '*';
#endif

	do {
		/* открываем дескриптор поиска */
#ifdef WIN32
		hFind = FindFirstFileA (strFindPath.c_str(), &soFindData);
		if (INVALID_HANDLE_VALUE == hFind) { iRetVal = GetLastError (); break; }
#else
		psoDir = opendir (strFindPath.c_str());
		if (NULL == psoDir) { iRetVal = errno; break; }
#endif

		/* формирование списка файлов */
#ifdef WIN32
		do {
#else
		while (NULL != (psoDirEnt = readdir (psoDir))) {
#endif
			/* обрабатываем сведения по очередному файлу */
#ifdef WIN32
			strFileTitle = soFindData.cFileName;
			/* если найденный элемент является директорией */
			if (FILE_ATTRIBUTE_DIRECTORY & soFindData.dwFileAttributes) {
#else
				strFileTitle = psoDirEnt->d_name;
				if (DT_DIR == psoDirEnt->d_type) {
#endif
				/* проверяем необходимость поиска в директориях */
				if (0 != p_soFileListOpt.m_iLookNestedFold) {
					/* проверяем не является ли текущая файл ссылкой на текущую или родительскую директорию */
					if (0 == strFileTitle.compare (".") || 0 == strFileTitle.compare ("..")) { continue; }
					std::string strSubDir;
					strSubDir = p_soFileListOpt.m_strPath;
					/* добавляем разделитель в конец имени директории в случае необходимости */
					strstStrLen = strSubDir.length ();
#ifdef WIN32
					if ('\\' != strSubDir[strstStrLen - 1] && '/' != strSubDir[strstStrLen - 1]) {
						strSubDir += "\\";
					}
#else
					if ('/' != strSubDir[strstStrLen - 1]) {
						strSubDir += "/";
					}
#endif
					strSubDir += strFileTitle;
					SFileListInfo soOpt = p_soFileListOpt;
					soOpt.m_strPath = strSubDir;
					iFnRes = FS_CreateFileList (soOpt);
				}
			} else {
				/* иначе считаем, что найденный элемент является файлом */
				std::string strFileName;
				size_t stFileSize;
				FILE *psoFile = NULL;

				strFileName = p_soFileListOpt.m_strPath;
				if (strFileName[strFileName.length () - 1] != '/'
#ifdef WIN32
					&& strFileName[strFileName.length () - 1] != '\\') {
						strFileName += '\\';
#else
					) {
						strFileName += '/';
#endif
				}
				strFileName += strFileTitle;
#ifdef WIN32
				iFnRes = fopen_s (&psoFile, strFileName.c_str (), "r");
				if (iFnRes) {
#else
				psoFile = fopen (strFileName.c_str (), "r");
				if (NULL == psoFile) {
#endif
					stFileSize = -1;
				} else {
					if (fseek (psoFile, 0, SEEK_END)) {
						stFileSize = -1;
					} else {
						stFileSize = ftell (psoFile);
					}
					fclose (psoFile);
				}
				SFileInfo soFileInfo = { strFileTitle, p_soFileListOpt.m_strPath, stFileSize};
				p_soFileListOpt.m_pmmapFileList->insert (std::make_pair (strFileTitle, soFileInfo));
			}
#ifdef WIN32
		}	while (FindNextFileA (hFind, &soFindData));
#else
		}
#endif
	} while (0);

	return iRetVal;
}

#ifdef _WIN32
int CFileList::CURL_Init (SFileInfo &p_soCURLLibInfo)
{
	int iRetVal = 0;

	do {

		SetDllDirectoryA (p_soCURLLibInfo.m_strDir.c_str ());

		/* загружаем библиотеку */
		m_hCURLLib = LoadLibraryA ("libcurl.dll");
		if (NULL == m_hCURLLib) {
			iRetVal = GetLastError ();
			break;
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

		iRetVal = fn_curl_global_init (CURL_GLOBAL_DEFAULT);

	} while (0);

	/* если возникла ошибка освобождаем библиотеку */
	if (iRetVal) {
		CURL_Cleanup ();
	}

	return iRetVal;
}

int CFileList::CURL_Cleanup ()
{
	int iRetVal = 0;

	fn_curl_global_cleanup ();

	fn_curl_global_init = NULL;
	fn_curl_easy_init = NULL;
	fn_curl_easy_setopt = NULL;
	fn_curl_easy_perform = NULL;
	fn_curl_easy_cleanup = NULL;
	fn_curl_global_cleanup = NULL;

	if (m_hCURLLib) {
		FreeLibrary (m_hCURLLib);
		m_hCURLLib = NULL;
	}

	return iRetVal;
}
#else
#endif
