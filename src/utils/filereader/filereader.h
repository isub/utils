#ifndef _FILE_READER
#define _FILE_READER

#include <string>
#ifdef WIN32
#	include <Windows.h>
#endif

#ifdef WIN32
#	ifdef	FILE_READER_EXPORT
#		define	FILE_READER_SPEC __declspec(dllexport)
#	else
#		define	FILE_READER_SPEC __declspec(dllimport)
#	endif
#else
#	define	FILE_READER_SPEC
#endif

struct SFileInfo {
	std::string m_strTitle; 
	std::string m_strDir;
	size_t m_stFileSize;
};

class FILE_READER_SPEC CFileReader
{
public:
	/* открытие файла */
	int OpenDataFile (
		std::string &p_strType,			/* "file", "ftp" or "sftp" */
#ifdef _WIN32
		SFileInfo &p_soCURLLib,			/* параметры библиотеки cURL */
#endif
		SFileInfo &p_soFileInfo,		/* параметры открываемого файла */
		std::string *p_pstrHost,		/* имя хоста (ftp, sftp) */
		std::string *p_pstrUserName,	/* имя пользователя (ftp, sftp) */
		std::string *p_pstrPassword);	/* пароль пользователя (ftp, sftp) */
	/* чтение данных из буфера возвращает 0 в случае успешного чтения всех запрошенных данных */
	int ReadData (
		unsigned char *p_pucData,		/* буфер для чтения данных */
		int &p_iDataSize);				/* размер буфера, в случае успешного выполнения функции количество записанных в буфер данных */
	/* закрывает файл */
	int CloseDataFile ();
	const char * GetDir() { return m_soFileInfo.m_strDir.c_str (); }
	const char * GetFileName() { return m_soFileInfo.m_strTitle.c_str (); }
	/* завершена ли запись в буфер */
	int IsWritingBufCompl ();
	/* пуст ли буфер */
	int IsBufferEmpty ();
private:
	int AllocateMemBlock (size_t p_stSize);
#ifdef _WIN32
	friend DWORD WINAPI CURL_LoadFile (void *p_pcoThis);
	friend DWORD WINAPI FS_LoadFile (void *p_pcoThis);
#else
	friend void * CURL_LoadFile (void *p_pcoThis);
	friend void * FS_LoadFile (void *p_pcoThis);
#endif
	friend size_t CURL_Write (void *p_pvSrc, size_t p_stSize, size_t p_stCount, CFileReader *p_pcoFileReader);
public:
	CFileReader (void);
	~CFileReader (void);
private:
	int m_hFile;
	SFileInfo m_soFileInfo;
	std::string m_strType;
	unsigned char *m_pmucBuf;		/* буфер для буферизации содержимого файла */
	size_t m_stBufSize;				/* размер буфера */
	volatile size_t m_stFileSize;	/* размер буферизированного файла */
	volatile size_t m_stReadPointer;	/* позиция для чтение из буфера */
	volatile int m_iIsWritingBufCompl;	/* завершена ли запись в буфер */
#ifdef _WIN32
	int CURL_Init (SFileInfo &p_soCURLLibInfo);
	HANDLE m_hWriteThread;			/* дескриптор программного потока записи данных в буфер */
#else
	pthread_t m_hWriteThread;		/* дескриптор программного потока записи данных в буфер */
#endif
	int m_iCancelWritingBuf;		/* отменить запись в буфер */
	char *m_pszHost;				/* имя удаленного хоста-источника */
	char *m_pszUserName;			/* имя пользователя доступа к хосту-источнику */
	char *m_pszPassword;			/* пароль пользователя доступа к хосту-источнику */
#ifdef _WIN32
	CRITICAL_SECTION m_soCSBuf;		/* критическая секция для операций с буфером */
	HMODULE m_hCURLLib;				/* дескриптор динамической библиотеки */
#else
	pthread_mutex_t m_tMutex;		/* мьютекс для операций с буфером */
#endif
};

#endif /* _FILE_READER */
