#ifndef _FILEWRITER_H_
#define _FILEWRITER_H_

#ifdef WIN32
#	include <Windows.h>
#else
#	define HANDLE int
#	include <stdlib.h>
#endif

#ifdef WIN32
#	ifdef	FILE_WRITER_EXPORT
#		define	FILE_WRITER_SPEC __declspec(dllexport)
#	else
#		define	FILE_WRITER_SPEC	__declspec(dllimport)
#	endif
#else
#	define	FILE_WRITER_SPEC
#endif

class FILE_WRITER_SPEC CFileWriter
{
public:
	/* работа с обычным файлом файловой системы */
	/* инициализация класса */
	int Init (size_t p_stBufSize);
	/* размер буфера для записи по-умолчанию 65536 */
	int Init ();
	/* создание файла для записи */
	int CreateOutputFile (const char *p_pszFileName);
	/* запись данных в файл */
	int WriteData (const unsigned char *p_pucData, size_t p_stDataSize);
	/* завершение записи в файл (дозапись данных из системного буфера) */
	int Finalise ();
	/* работа с файлами по протоколам ftp, sftp, ftps */
	int UploadFile (
		const char *p_pcszFileName,
		const char *p_pcszProto,
		const char *p_pcszHostName,
		const char *p_pcszPath,
		const char *p_pcszUserName,
		const char *p_pcszUserPassword);
public:
	CFileWriter(void);
	~CFileWriter(void);
private:
	unsigned char *m_pucBuf;
	size_t m_stBufSize;
	size_t m_stCurPos;
	HANDLE m_hFile;
	void Flush ();
};

#endif /* _FILEWRITER_H_ */
