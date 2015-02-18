#include <string>
#include <map>
#include <vector>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#	define	strerror_rs(a, b, c)	strerror_s(b,c,a)
#else
#	define	strerror_rs(a,b,c)	strerror_r(a,b,c)
#endif

#include "config.h"

int CConfig::LoadConf (const char *p_pcszFileName, int p_iCollectEmptyParam)
{
	int iRetVal = 0;
	FILE *psoFile = 0;
	char mcParam[0x1000];
	char *pcValue;
	char *pcEOL, *pcEOLAlt;

	do {
		/* открываем файл для чтения */
#ifdef	WIN32
		if (0 != fopen_s (&psoFile, p_pcszFileName, "r")) {
#else
		psoFile = fopen (p_pcszFileName, "r");
		if (0 == psoFile) {
#endif
			iRetVal = errno;
			if (2 <= m_iDebug) {
				strerror_rs (iRetVal, mcParam, sizeof(mcParam));
				printf ("CConfig::LoadConf: can not open file %s; Error code: %d: %s\n", p_pcszFileName, iRetVal, mcParam);
			}
			 break;
		}
		if (2 <= m_iDebug) {
			printf ("CConfig::LoadConf: Load configuration file %s\n", p_pcszFileName);
		}
		while (! feof(psoFile)) {
			/* читаем очередную строку из файла, если чтение не выполнено, проверяем наличие ошибки */
			if (0 == fgets (mcParam, sizeof(mcParam), psoFile)) {
				iRetVal = errno;
				if (iRetVal) {
					break;
				}
				continue;
			}
			/* Ищем и затираем, символы перехода на новую строку */
			pcEOL = strstr (mcParam, "\r");
			pcEOLAlt = strstr (mcParam, "\n");
			if (pcEOL) {
				*pcEOL = '\0';
			}
			if (pcEOLAlt) {
				*pcEOLAlt = '\0';
			}
			/* если строка пустая переходим к следующей итеррации */
			if (0 == strlen (mcParam)) {
				continue;
			}
			/* ищем разделитель значения */
			pcValue = strstr (mcParam, "=");
			/* если разделитель не найден переходим к следующей итерации */
			if (0 == pcValue) {
				if (2 <= m_iDebug) {
					printf ("CConfig::LoadConf: parameter: %s: devider not found\n", mcParam);
				}
				continue;
			}
			/* Затираем разделитель и переводим указатель на значение параметра */
			*pcValue = '\0';
			++pcValue;
			/* Если значение не задано переходим к следующей итерации */
			if (0 == strlen(pcValue) && 0 == p_iCollectEmptyParam) {
				if (2 <= m_iDebug) {
					printf ("CConfig::LoadConf: parameter: %s: zero value length\n", mcParam);
				}
				continue;
			}
			/* Добавляем параметры в список */
			if (2 <= m_iDebug) {
				printf ("CConfig::LoadConf: parameter: %s; value: %s\n", mcParam, pcValue);
			}
			m_mmapConf.insert (std::make_pair (std::string(mcParam), std::string(pcValue)));
		}
	} while(0);

	if (0 != psoFile) {
		fclose (psoFile);
	}

	return iRetVal;
}

int CConfig::GetParamValue (const char *p_pcszName, std::vector<std::string> &p_vectValueList) {
	int iRetVal = 0;
	std::multimap<std::string,std::string>::iterator iterValueList;

	iterValueList = m_mmapConf.find (p_pcszName);
	while (iterValueList != m_mmapConf.end() && 0 == iterValueList->first.compare(p_pcszName)) {
		p_vectValueList.push_back (iterValueList->second);
		++iterValueList;
	}

	if (0 == p_vectValueList.size ()) {
		iRetVal = -1;
	}

	return iRetVal;
}

int CConfig::GetParamValue (const char *p_pcszName, std::string &p_strValue) {
	int iRetVal = 0;
	std::multimap<std::string,std::string>::iterator iterValueList;

	iterValueList = m_mmapConf.find (p_pcszName);
	if (iterValueList != m_mmapConf.end() && 0 == iterValueList->first.compare(p_pcszName)) {
		p_strValue = iterValueList->second;
	} else {
		iRetVal = -1;
	}

	return iRetVal;
}

int CConfig::SetParamValue (const char *p_pcszName, std::string &p_strValue)
{
	std::multimap<std::string,std::string>::iterator iterValueList;

	iterValueList = m_mmapConf.insert (std::make_pair (p_pcszName, p_strValue));

	if (iterValueList == m_mmapConf.end ()) {
		return -1;
	} else {
		return 0;
	}
}

CConfig::CConfig (int p_iDebugLevel) {
	m_iDebug = p_iDebugLevel;
}

void CConfig::SetDebugLevel (int p_iDebugLevel) {
	m_iDebug = p_iDebugLevel;
}
