#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "webformparam.h"

#ifndef WIN32
#define sprintf_s snprintf
#define strcpy_s strcpy
#define strncpy_s strncpy
#endif

void DecodeParam (char *p_pszParam);

SParamList * ParseBuffer(
	const unsigned char *p_pucBuf,
	size_t p_stLen)
{
	SParamList *psoParamList = NULL;
	SParamList *psoPLTmp;
	unsigned char *pmucBuf = NULL;

	pmucBuf = new unsigned char[p_stLen + 1];
	memcpy(
		pmucBuf,
		p_pucBuf,
		p_stLen);
	pmucBuf[p_stLen] = '\0';

	char *pszParamName = (char*)pmucBuf;
	char *pszParamVal;

	while (pszParamName && pszParamName < (char*)pmucBuf + p_stLen) {
		// ищем разделитель имени параметра и его значения
		pszParamVal = strstr (pszParamName, "=");
		if (NULL == pszParamVal) {
			break;
		}
		*pszParamVal = '\0';
		++pszParamVal;
		// создаем новый элемент списка параметров
		if (NULL == psoParamList) {
			psoParamList = new SParamList;
			psoPLTmp = psoParamList;
		}
		else {
			psoPLTmp->m_psoNext = new SParamList;
			psoPLTmp = psoPLTmp->m_psoNext;
		}
		psoPLTmp->m_mcParamName[0] = '\0';
		psoPLTmp->m_mcParamValue[0] = '\0';
		psoPLTmp->m_psoNext = NULL;
		// копируем имя параметра
		strcpy_s (
		psoPLTmp->m_mcParamName,
#ifdef WIN32
		sizeof(psoPLTmp->m_mcParamName),
#endif
		pszParamName);
		DecodeParam (psoPLTmp->m_mcParamName);
		pszParamName = strstr (pszParamVal, "&");
		if (pszParamName) {
			*pszParamName = '\0';
			++pszParamName;
		}
		// копируем значение параметра
		strcpy_s (psoPLTmp->m_mcParamValue,
#ifdef WIN32
		sizeof(psoPLTmp->m_mcParamValue),
#endif
		pszParamVal);
		DecodeParam (psoPLTmp->m_mcParamValue);
	}

	if (pmucBuf) {
		delete [] pmucBuf;
		pmucBuf = NULL;
	}

	return psoParamList;
}

unsigned char * EncodeParam (SParamList *p_psoParamList)
{
	SParamList *psoTmpParamList;
	unsigned char
		*pmucBuf,
		*pmucRetVal;
	size_t
		stPos,
		stStrLen,
		stBufSize;
	char *pszTmpPtr;

	stBufSize = 0x10000;
	pmucBuf = new unsigned char [stBufSize];
	psoTmpParamList = p_psoParamList;
	stPos = 0;
	while (psoTmpParamList) {
		pszTmpPtr = psoTmpParamList->m_mcParamName;
		stStrLen = strlen (pszTmpPtr);
		for (size_t stInd = 0; stInd < stStrLen; ++stInd) {
			if (isalnum ((unsigned char)(pszTmpPtr[stInd]))) {
				pmucBuf[stPos++] = pszTmpPtr[stInd];
			}
			else {
				stPos += sprintf_s(
					(char*)&(pmucBuf[stPos]),
					stBufSize - stPos,
					"%%%0X",
					(unsigned char)(pszTmpPtr[stInd]));
			}
		}
		pmucBuf[stPos++] = '=';
		pszTmpPtr = psoTmpParamList->m_mcParamValue;
		stStrLen = strlen (pszTmpPtr);
		for (size_t stInd = 0; stInd < stStrLen; ++stInd) {
			if (isalnum ((unsigned char)(pszTmpPtr[stInd]))) {
				pmucBuf[stPos++] = pszTmpPtr[stInd];
			}
			else {
				stPos += sprintf_s(
					(char*)&(pmucBuf[stPos]),
					stBufSize - stPos,
					"%%%0X",
					(unsigned char)(pszTmpPtr[stInd]));
			}
		}
		psoTmpParamList = psoTmpParamList->m_psoNext;
		if (psoTmpParamList) {
			pmucBuf[stPos++] = '&';
		}
	}
	pmucBuf[stPos++] = '\0';
	pmucRetVal = new unsigned char [stPos];
	memcpy(
		pmucRetVal,
		pmucBuf,
		stPos);
	if (pmucBuf) {
		delete [] pmucBuf;
		pmucBuf =	NULL;
	}

	return pmucRetVal;
}

void DecodeParam (char *p_pszParam)
{
	size_t stStrLen, stReadInd, stWriteInd;
	char mcEncodeSeq[8], *pszEndPtr, cEncodedSymb;

	stStrLen = strlen (p_pszParam);
	stReadInd = 0;
	stWriteInd = 0;
	while (stReadInd < stStrLen) {
		if ('%' == p_pszParam[stReadInd]) {
			++stReadInd;
			strncpy_s (mcEncodeSeq, &(p_pszParam[stReadInd]), 2);
			cEncodedSymb = (char)strtol (mcEncodeSeq, &pszEndPtr, 16);
			p_pszParam[stWriteInd] = cEncodedSymb;
			++stReadInd;
		}
		else if ('+' == p_pszParam[stReadInd]) {
			p_pszParam[stWriteInd] = ' ';
		}
		else {
			p_pszParam[stWriteInd] = p_pszParam[stReadInd];
		}
		++stReadInd;
		++stWriteInd;
	}

	p_pszParam[stWriteInd] = '\0';
}
