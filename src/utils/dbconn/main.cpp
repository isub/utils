#include "utils/dbpool/dbpool.h"
#include "utils/log/log.h"
#include "utils/config/config.h"
#include <string.h>
#include <stdlib.h>
#include <string>

#ifdef _WIN32
#	define sleep Sleep
#endif

int main (int p_iArgC, char *p_pmszArgV[])
{
	int iFnRes;
	std::string strConfFile;

	/* ищем необходимые параметры */
	for (int i = 0; i < p_iArgC; ++i) {
		if (0 == strncmp (p_pmszArgV[i], "conf=", 5)) {
			strConfFile = &(p_pmszArgV[i][5]);
		}
	}
	/* если необходимый параметр не найден */
	if (0 == strConfFile.length ()) {
		return -1;
	}

	CLog coLog;
	CConfig coConf;

	/* загружаем конфигурацию модуля */
	iFnRes = coConf.LoadConf (strConfFile.c_str ());
	if (iFnRes) {
		return -2;
	}

	std::string strParam;
	const char *pszParamName;

	/* инициализация логера */
	pszParamName = "log_file_mask";
	iFnRes = coConf.GetParamValue (pszParamName, strParam);
	if (iFnRes) {
		return -3;
	}
	iFnRes = coLog.Init (strParam.c_str ());
	if (iFnRes) {
		return -4;
	}

	/* выбираем необходимые параметры */
	int iSleep = 0;
	int iMaxCount = 0;

	pszParamName = "sleep";
	iFnRes = coConf.GetParamValue (pszParamName, strParam);
	if (0 == iFnRes) {
		iSleep = strtol (strParam.c_str (), NULL, 10);
	}
	if (0 == iSleep) {
		/* задаем значение по умолчанию */
		iSleep = 5;
	}

	pszParamName = "max_count";
	iFnRes = coConf.GetParamValue (pszParamName, strParam);
	if (0 == iFnRes) {
		iMaxCount = strtol (strParam.c_str (), NULL, 10);
	}
	if (0 == iMaxCount) {
		/* задаем значение по умолчанию */
		iMaxCount = 120;
	}

	do {
		iFnRes = db_pool_init (&coLog, &coConf);
		if (iFnRes) {
			coLog.WriteLog ("%s: can not initialize db pool: error code: '%d'", __func__, iFnRes);
			sleep (iSleep);
		}
		db_pool_deinit ();
	} while (iFnRes && iMaxCount--);

	db_pool_deinit ();

	return 0;
}
