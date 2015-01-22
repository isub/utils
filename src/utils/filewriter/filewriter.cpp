#define FILE_WRITER_EXPORT
#include "FileWriter.h"

#include <stdio.h>

#ifdef _WIN32
#	include <Windows.h>
#else
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <fcntl.h>
#	include <string.h>
#	include <errno.h>
#	include <unistd.h>
#	define INVALID_HANDLE_VALUE -1
#	define BYTE unsigned char
#	define DWORD unsigned int
#	define MAX_PATH 256
#endif

int CFileWriter::Init (size_t p_stBufSize)
{
	m_stBufSize = p_stBufSize;
	m_pucBuf = new BYTE[m_stBufSize];
#ifdef _DEBUG
	memset (m_pucBuf, 0, m_stBufSize);
#endif

	return 0;
}

int CFileWriter::Init ()
{
	return CFileWriter::Init (0x10000);
}

int CFileWriter::CreateOutputFile (const char *p_pszFileName)
{
	int iRetVal = 0;

	// Если имя файла не задано, присваиваем значение по умолчанию
	if (NULL == p_pszFileName) {
		return -1;
	}
	if (INVALID_HANDLE_VALUE != m_hFile) {
		return -1;
	}

	DWORD dwLastError;

#ifdef _WIN32
	m_hFile = CreateFileA (p_pszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	if (INVALID_HANDLE_VALUE == m_hFile) {
		iRetVal = GetLastError();
#else
	m_hFile = open (p_pszFileName, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (INVALID_HANDLE_VALUE == m_hFile) {
		iRetVal = errno;
#endif
	}

	return iRetVal;
}

int CFileWriter::WriteData(
	const unsigned char *p_bpData,
	size_t p_stDataSize)
{
	if (INVALID_HANDLE_VALUE == m_hFile) {
		return -1;
	}
	if (NULL == m_pucBuf) {
		return -1;
	}

	size_t stBytesToCopy = 0;
	size_t stDataReadInd = 0;

	while (stDataReadInd < p_stDataSize) {
		if (m_stCurPos == m_stBufSize) {
			if (Finalise ()) {
				return -1;
			}
		}
		stBytesToCopy =
			m_stBufSize - m_stCurPos > p_stDataSize - stDataReadInd
			? p_stDataSize - stDataReadInd
			: m_stBufSize - m_stCurPos;
		if (stBytesToCopy) {
			memcpy (&(m_pucBuf[m_stCurPos]), &(p_bpData[stDataReadInd]), stBytesToCopy);
			stDataReadInd += stBytesToCopy;
			m_stCurPos += stBytesToCopy;
		}
	}

	return 0;
}

int CFileWriter::Finalise()
{
	DWORD dwBytesWritten;

	if (INVALID_HANDLE_VALUE == m_hFile) {
		return -1;
	}
	if (m_stCurPos) {
#ifdef _WIN32
		if (! WriteFile (m_hFile, m_pucBuf, (DWORD)m_stCurPos, &dwBytesWritten, NULL)) {
			return -1;
		}
		if (dwBytesWritten != (size_t) m_stCurPos) {
			return -1;
		}
#else
		if (m_stCurPos != write (m_hFile, m_pucBuf, m_stCurPos)) {
			return -1;
		}
#endif
		Flush ();
		m_stCurPos = 0;
	}

	return 0;
}

CFileWriter::CFileWriter(void)
{
	m_pucBuf = NULL;
	m_stBufSize = 0;
	m_stCurPos = 0;
	m_hFile = INVALID_HANDLE_VALUE;
}

CFileWriter::~CFileWriter(void)
{
	if (m_pucBuf) {
		delete[] m_pucBuf;
		m_pucBuf = NULL;
	}
	m_stBufSize = 0;
	m_stCurPos = 0;
	if (INVALID_HANDLE_VALUE != m_hFile) {
#ifdef _WIN32
		CloseHandle (m_hFile);
#else
		close (m_hFile);
#endif
		m_hFile = INVALID_HANDLE_VALUE;
	}
}

void CFileWriter::Flush ()
{
#ifdef _WIN32
	FlushFileBuffers (m_hFile);
#else
	fsync (m_hFile);
#endif
}
