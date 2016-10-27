#include "utils/dbpool/dbpool.h" /* чтобы не заморачиваться с заголовками и макросами OTL */
#include "utils/config/config.h"
#include <errno.h>
#include <stdio.h>

int test_db(const char *p_pszDBName, CConfig &p_coConf, CLog &p_coLog);

int main (int argc, char *argv[])
{
	int iRetVal = 0;
	int iFnRes;
	const char *pszConf = NULL;
	CConfig coConf;
	CLog coLog;

	for (int i = 0; i < argc; ++i) {
		if (0 == strncmp("conf=", argv[i], 5)) {
			pszConf = &argv[i][5];
		}
	}

	if (NULL == pszConf) {
		LOG_E(coLog, "config file is not defined");
		return EINVAL;
	}

	do {
		/* загружаем конфигурацию приложения */
		iFnRes = coConf.LoadConf(pszConf, 0);
		if (iFnRes) {
			LOG_E(coLog, "config is not loaded: error code: '%d'", iFnRes);
			return EINVAL;
		}
		std::string strLogFileMask;
		iFnRes = coConf.GetParamValue("log_file_mask", strLogFileMask);
		if (iFnRes) {
			LOG_W(coLog, "log file mask is not defined");
		} else {
			iFnRes = coLog.Init(strLogFileMask.c_str());
			if (iFnRes)
				LOG_W(coLog, "logger is not initialized: error code: '%d'; file name '%s'", iFnRes, strLogFileMask.c_str());
		}
		/* загружаем имя текущей БД */
		std::string strFileName;
		char mcCurrentDB[32];
		FILE *psoFile = NULL;
		iFnRes = coConf.GetParamValue("current_db", strFileName);
		if (iFnRes) {
			LOG_F(coLog, "config parameter 'current_db' is not defined");
			break;
		}
		psoFile = fopen(strFileName.c_str(), "r");
		if (NULL == psoFile) {
			LOG_F(coLog, "can not open file '%s'; error code: '%d'", strFileName.c_str(), errno);
			break;
		}
		iFnRes = fread(mcCurrentDB, 1, sizeof(mcCurrentDB), psoFile);
		if (0 == iFnRes) {
			LOG_F(coLog, "can not read data from file '%s'; error code: '%d';", strFileName.c_str(), errno);
			fclose(psoFile);
			psoFile = NULL;
			break;
		}
		if (iFnRes > sizeof(mcCurrentDB) - 1) {
			fclose(psoFile);
			psoFile = NULL;
			LOG_F(coLog, "insufficient buffer size to store value");
			break;
		} else {
			mcCurrentDB[iFnRes] = '\0';
		}
		LOG_D(coLog, "current DB profile is: '%s'", mcCurrentDB);
		fclose(psoFile);
		psoFile = NULL;
		/* тестовое подключение к текущей БД */
		iFnRes = test_db(mcCurrentDB, coConf, coLog);
		/* если текущая БД в активном состоянии завершаем работу */
		if (0 == iFnRes) {
			LOG_N(coLog, "current DB '%s' is in active state", mcCurrentDB);
			break;
		} else {
			LOG_N(coLog, "current DB '%s': test result code: '%d'", mcCurrentDB, iFnRes);
		}
		/* проверяем состояние другой БД */
		if (0 == strcmp("local_db", mcCurrentDB)) {
			strcpy(mcCurrentDB, "alt_db");
		} else {
			strcpy(mcCurrentDB, "local_db");
		}
		iFnRes = test_db(mcCurrentDB, coConf, coLog);
		/* если возникла ошибка */
		if (iFnRes) {
			LOG_N(coLog, "next DB '%s': test result code: '%d'", mcCurrentDB, iFnRes);
			break;
		}
		/* если состояние другой БД активно переключаемся на нее */
		std::string strParamVal;
		const char *pszParamName;
		if (0 == strcmp(mcCurrentDB, "local_db")) {
			pszParamName = "local_sh";
		} else {
			pszParamName = "alt_sh";
		}
		iFnRes = coConf.GetParamValue(pszParamName, strParamVal);
		if (iFnRes) {
			iRetVal = iFnRes;
			LOG_F(coLog, "conf parameter '%s' not defined", pszParamName);
			break;
		}
		iFnRes = system(strParamVal.c_str());
		/* если скрипт завершился неудачно */
		if (iFnRes) {
			iRetVal = iFnRes;
			LOG_F(coLog, "system executed with error: code: '%d'; program name: '%s'", iFnRes, strParamVal.c_str());
			break;
		}
		/* пишем в лог-файл об успешном завершении операции */
		LOG_N(coLog, "settings configured to: '%s'", mcCurrentDB);
		/* сохраняем имя текущей БД */
		psoFile = fopen(strFileName.c_str(), "w");
		if (NULL == psoFile) {
			iRetVal = errno;
			LOG_F(coLog, "can not open file '%s'; error code: '%d'", strFileName.c_str(), errno);
			break;
		}
		iFnRes = fwrite(mcCurrentDB, 1, strlen(mcCurrentDB), psoFile);
		if (iFnRes != strlen(mcCurrentDB)) {
			LOG_F(coLog, "can not write name of current DB");
		}
		fclose(psoFile);
		psoFile = NULL;
	} while (0);

	return iRetVal;
}

int test_db(const char *p_pszDBName, CConfig &p_coConf, CLog &p_coLog)
{
	int iRetVal = 0;
	int iFnRes;

	do {
		std::string strDBUserName;
		std::string strDBUserPswd;
		std::string strParamVal;
		std::string strDBDescr;
		std::string strConnStr;
		std::string strDBStatus;
		const char *pszConfParam;
		CConfig coConf;

		pszConfParam = "db_user";
		iFnRes = p_coConf.GetParamValue(pszConfParam, strDBUserName);
		if (iFnRes){
			LOG_E(p_coLog, "configuration parameter '%s' not defined", pszConfParam);
			iRetVal = -1;
			break;
		}
		pszConfParam = "db_pswd";
		iFnRes = p_coConf.GetParamValue(pszConfParam, strDBUserPswd);
		if (iFnRes) {
			LOG_E(p_coLog, "configuration parameter '%s' not defined", pszConfParam);
			iRetVal = -2;
			break;
		}
		pszConfParam = p_pszDBName;
		iFnRes = p_coConf.GetParamValue(pszConfParam, strParamVal);
		if (iFnRes) {
			LOG_E(p_coLog, "configuration parameter '%s' not defined", pszConfParam);
			iRetVal = -3;
			break;
		}
		iFnRes = coConf.LoadConf(strParamVal.c_str(), 0);
		if (iFnRes) {
			LOG_E(p_coLog, "can not load configuration file '%s'", strParamVal.c_str());
			iRetVal = -4;
			break;
		}
		pszConfParam = "db_descr";
		iFnRes = coConf.GetParamValue(pszConfParam, strDBDescr);
		if (iFnRes) {
			LOG_E(p_coLog, "configuration parameter '%s' not defined", pszConfParam);
			iRetVal = -5;
			break;
		}
		strConnStr = strDBUserName;
		strConnStr += '/';
		strConnStr += strDBUserPswd;
		strConnStr += '@';
		strConnStr += strDBDescr;
		otl_connect coDBConn;
		otl_stream coStream;
		try {
			coDBConn.rlogon(strConnStr.c_str());
			coStream.open(1, "select CONTROLFILE_TYPE from V$DATABASE", coDBConn, NULL, "check DB state");
			if (coStream.eof()) {
				coStream.close();
				coDBConn.logoff();
				iRetVal = ENODATA;
				break;
			}
			coStream
				>> strDBStatus;
			coStream.close();
			coDBConn.logoff();
			LOG_D(p_coLog, "DB profile: '%s'; status: '%s'", p_pszDBName, strDBStatus.c_str());
		} catch (otl_exception &coExcept) {
			LOG_E(p_coLog, "code: '%d'; message: '%s'; query: '%s'", coExcept.code, coExcept.msg, coExcept.stm_text);
			iRetVal = coExcept.code;
			if (coStream.good())
				coStream.close();
			if (coDBConn.connected)
				coDBConn.logoff();
			iRetVal = -10;
			break;
		}
		/* если текущая БД является активной завершаем работу */
		if (0 == strDBStatus.compare("CURRENT")) {
			break;
		} else {
			iRetVal = EINVAL;
		}
	} while (0);

	return iRetVal;
}
