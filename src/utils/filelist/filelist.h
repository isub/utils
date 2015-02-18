#ifndef _FILE_LIST_H
#define _FILE_LIST_H

#include "utils/filereader/filereader.h" /* SFileInfo */

#include <string>
#include <map>

#ifdef WIN32
#	ifdef	FILE_LIST_EXPORT
#		define	FILE_LIST_SPEC __declspec(dllexport)
#	else
#		define	FILE_LIST_SPEC __declspec(dllimport)
#	endif
#else
#	define	FILE_LIST_SPEC
#endif

struct FILE_LIST_SPEC SFileListInfo {
	std::string m_strFileType;		/* тип файлов исходных данных "file", "ftp", "sftp", "ftps" */
	std::string m_strHost;			/* имя хоста-хранилища исходных данных */
	std::string m_strUserName;		/* имя пользователя для доступа к хосту-хранилищу исходных данных */
	std::string m_strPassword;		/* пароль пользователя для доступа к хосту-хранилищу исходных данных */
	std::string m_strPath;			/* папка, в которой производится поиск файлов */
	std::multimap<std::string, SFileInfo> *m_pmmapFileList; /* контейнер для хранения списка файлов */
	int m_iLookNestedFold;			/* флаг поиска во вложенных папках: 0 - не искать файлы во вложенных папках */
};

class FILE_LIST_SPEC CFileList {
public:
	/* создает список файлов по заданной директории 
	   возвращает 0 в случае успеха */
	int CreateFileList (SFileListInfo &p_soFileListOpt);
#ifdef _WIN32
	CFileList () { m_hCURLLib = NULL; }
	int CURL_Init (SFileInfo &p_soCURLLibInfo);
	int CURL_Cleanup ();
private:
	HMODULE m_hCURLLib;
#endif
};

#endif /* _FILE_LIST_H */
