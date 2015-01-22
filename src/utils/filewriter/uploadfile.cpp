#define FILE_WRITER_EXPORT
#include "FileWriter.h"

#include <string>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#	include <share.h>
#	include <Windows.h>
#	include <curl.h>
#else
#	include <curl/curl.h>
#endif

HMODULE g_hCurlLib = NULL;

typedef CURLcode (*typeCURLGlobalInit)(long);
typedef void (*typeCURLGlobalCleanup)(void);
typedef CURL* (*typeCURLEasyInit)(void);
typedef void (*typeCURLEasyCleanup)(CURL*);
typedef CURLcode (*typeCURLEasySetopt)(CURL*, CURLoption, ...);
typedef CURLcode (*typeCURLEasyPerform)(CURL*);

typeCURLGlobalInit		fn_curl_global_init = NULL;
typeCURLEasyInit			fn_curl_easy_init = NULL;
typeCURLEasySetopt		fn_curl_easy_setopt = NULL;
typeCURLEasyPerform		fn_curl_easy_perform = NULL;
typeCURLEasyCleanup		fn_curl_easy_cleanup = NULL;
typeCURLGlobalCleanup	fn_curl_global_cleanup = NULL;

static size_t CURL_read (void *ptr, size_t size, size_t nmemb, int p_iFile);

/*****************************************************************************/
int CURL_Init ();
int CURL_CleanUp ();

int CFileWriter::UploadFile (
	const char *p_pcszFileName,
	const char *p_pcszProto,
	const char *p_pcszHostName,
	const char *p_pcszPath,
	const char *p_pcszUserName,
	const char *p_pcszUserPassword)
{
	int iRetVal = 0;
	int iFnRes;
	CURL *pCurl = NULL;
	int iFile = -1;

	do {
		struct stat soFileInfo;
		curl_off_t stFileSize;

		/* запрашиваем размер файла */
		iFnRes = stat (p_pcszFileName, &soFileInfo);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}
		stFileSize = soFileInfo.st_size;
		if (0 >= stFileSize) {
			/* отправл€ть нечего, завершаем работу */
			iRetVal = -1;
			break;
		}

		/* открываем файл дл€ чтени€ */
		iFnRes = _sopen_s (&iFile, p_pcszFileName, _O_RDONLY | _O_BINARY, _SH_DENYWR, _S_IREAD);
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}

		CURLcode curlRes;

		iFnRes = CURL_Init ();
		if (iFnRes) {
			iRetVal = iFnRes;
			break;
		}

		/* инициализаци€ экземпл€ра CURL */
		pCurl = fn_curl_easy_init ();
		if (NULL == pCurl) {
			iRetVal = ERROR_OUTOFMEMORY;
			break;
		}

		/* user name */
		if (p_pcszUserName) {
			curlRes = fn_curl_easy_setopt (pCurl, CURLOPT_USERNAME, p_pcszUserName);
			if (curlRes) {
				iRetVal = curlRes;
				break;
			}
		}

		/* user password */
		if (p_pcszUserPassword) {
			curlRes = fn_curl_easy_setopt (pCurl, CURLOPT_PASSWORD, p_pcszUserPassword);
			if (curlRes) {
				iRetVal = curlRes;
				break;
			}
		}

		/* we want to use our own read function */ 
		fn_curl_easy_setopt(pCurl, CURLOPT_READFUNCTION, CURL_read);

		/* enable uploading */ 
		fn_curl_easy_setopt(pCurl, CURLOPT_UPLOAD, 1L);

		/* specify target */ 
		std::string strUrl;
		strUrl = p_pcszProto;
		strUrl += "://";
		strUrl += p_pcszHostName;
		strUrl += "/";
		strUrl += p_pcszPath;
		fn_curl_easy_setopt(pCurl, CURLOPT_URL, strUrl.c_str ());

		/* now specify which file to upload */ 
		fn_curl_easy_setopt(pCurl, CURLOPT_READDATA, iFile);

		/* Set the size of the file to upload (optional).  If you give a *_LARGE
			 option you MUST make sure that the type of the passed-in argument is a
			 curl_off_t. If you use CURLOPT_INFILESIZE (without _LARGE) you must
			 make sure that to pass in a type 'long' argument. */
		fn_curl_easy_setopt(pCurl, CURLOPT_INFILESIZE_LARGE, stFileSize);

		/* выполн€ем запрос */
		curlRes = fn_curl_easy_perform (pCurl);
		if (curlRes) {
			iRetVal = curlRes;
			break;
		}

	} while (0);

	if (-1 != iFile) {
		_close (iFile);
		iFile = -1;
	}

	if (pCurl) {
		fn_curl_easy_cleanup (pCurl);
		pCurl = NULL;
	}

	CURL_CleanUp ();

	return iRetVal;
}


int CURL_Init ()
{
	int iRetVal = 0;

	do {

		SetDllDirectoryA ("D:/Lib/cURL");

		/* загружаем библиотеку */
		if (NULL == g_hCurlLib) {
			g_hCurlLib = LoadLibraryA ("libcurl.dll");
			if (NULL == g_hCurlLib) {
				iRetVal = GetLastError ();
				break;
			}
		}

		/* загружаем необходимые функции из библиотеки */
		if (NULL == fn_curl_global_init) {
			fn_curl_global_init = reinterpret_cast<typeCURLGlobalInit>(GetProcAddress (g_hCurlLib, "curl_global_init"));
			if (NULL == fn_curl_global_init) { iRetVal = GetLastError (); break; }
		}
		if (NULL == fn_curl_easy_init) {
			fn_curl_easy_init = reinterpret_cast<typeCURLEasyInit> (GetProcAddress (g_hCurlLib, "curl_easy_init"));
			if (NULL == fn_curl_easy_init) { iRetVal = GetLastError (); break; }
		}
		if (NULL == fn_curl_easy_setopt) {
			fn_curl_easy_setopt = reinterpret_cast<typeCURLEasySetopt> (GetProcAddress (g_hCurlLib, "curl_easy_setopt"));
			if (NULL == fn_curl_easy_setopt) { iRetVal = GetLastError (); break; }
		}
		if (NULL == fn_curl_easy_perform) {
			fn_curl_easy_perform = reinterpret_cast<typeCURLEasyPerform> (GetProcAddress (g_hCurlLib, "curl_easy_perform"));
			if (NULL == fn_curl_easy_perform) { iRetVal = GetLastError (); break; }
		}
		if (NULL == fn_curl_easy_cleanup) {
			fn_curl_easy_cleanup = reinterpret_cast<typeCURLEasyCleanup> (GetProcAddress (g_hCurlLib, "curl_easy_cleanup"));
			if (NULL == fn_curl_easy_cleanup) { iRetVal = GetLastError (); break; }
		}
		if (NULL == fn_curl_global_cleanup) {
			fn_curl_global_cleanup = reinterpret_cast<typeCURLGlobalCleanup> (GetProcAddress (g_hCurlLib, "curl_global_cleanup"));
			if (NULL == fn_curl_global_cleanup) { iRetVal = GetLastError (); break; }
		}

		iRetVal = fn_curl_global_init (CURL_GLOBAL_DEFAULT);

	} while (0);

	/* если возникла ошибка освобождаем библиотеку */
	if (iRetVal) {
		if (g_hCurlLib) {
			FreeLibrary (g_hCurlLib);
			g_hCurlLib = NULL;
		}
	}

	return iRetVal;
}

int CURL_CleanUp ()
{
	int iRetVal = 0;

	fn_curl_global_cleanup ();

	fn_curl_global_init = NULL;
	fn_curl_easy_init = NULL;
	fn_curl_easy_setopt = NULL;
	fn_curl_easy_perform = NULL;
	fn_curl_easy_cleanup = NULL;
	fn_curl_global_cleanup = NULL;

	if (g_hCurlLib) {
		FreeLibrary (g_hCurlLib);
		g_hCurlLib = NULL;
	}

	return iRetVal;
}

static size_t CURL_read (void *ptr, size_t size, size_t nmemb, int p_iFile)
{
	curl_off_t nread;
	/* in real-world cases, this would probably get this data differently
		as this fread() stuff is exactly what the library already would do
		by default internally */
	size_t retcode = _read (p_iFile, ptr, size * nmemb);

	nread = (curl_off_t) retcode;

	return retcode;
}
