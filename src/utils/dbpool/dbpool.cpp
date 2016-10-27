#include "dbpool.h"

#include <string>
#include <semaphore.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>

#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
#define LOG_ERR_DESCR(str) 		iFnRes = errno; \
		char mcErrDescr[1024]; \
		if (g_pcoLog) { \
			if (0 != strerror_r (iFnRes, mcErrDescr, sizeof (mcErrDescr))) { \
				mcErrDescr[0] = '\0'; \
			} \
			g_pcoLog->WriteLog ("dbpool: %s: error: %s: code: '%d'; description: '%s'", __func__, str, iFnRes, mcErrDescr); \
		}
#else
#define LOG_ERR_DESCR(str) 		iFnRes = errno; \
		char mcErrDescr[1024]; \
		if (g_pcoLog) { \
			if (NULL == strerror_r (iFnRes, mcErrDescr, sizeof (mcErrDescr))) { \
				mcErrDescr[0] = '\0'; \
			} \
			g_pcoLog->WriteLog ("dbpool: %s: error: %s: code: '%d'; description: '%s'", __func__, str, iFnRes, mcErrDescr); \
		}
#endif

/* шаблон строки подключения к БД */
static char g_mcRadDBConnTempl[] = "%s/%s@%s";
/* указатель на объект логгера */
static CLog *g_pcoLog = NULL;
/* указатель на объект конфигурации */
static CConfig *g_pcoConf = NULL;

/* семафор для ожидания свободного указателя на объект класса для взаимодействия с БД */
sem_t g_tSem;
/* размер пула */
static int g_iPoolSize;
/* элемент пула */
struct SPoolElem {
	otl_connect m_coDBConn;
	bool m_bIsBusy;
	SPoolElem () { m_bIsBusy = false; }
	~SPoolElem () { m_bIsBusy = true; }
};
/* массив пула */
static SPoolElem *g_pmsoPool = NULL;
/* мьютекс для изменения статусов элементов пула */
static pthread_mutex_t g_tMutex;

void DisconnectDB (otl_connect &p_coDBConn)
{
	if (p_coDBConn.connected) {
		p_coDBConn.cancel ();
		p_coDBConn.logoff ();
	}
}

int ConnectDB (otl_connect &p_coDBConn)
{
	/* проверка параметров */
	if (NULL == g_pcoConf) {
		if (g_pcoLog) {
			g_pcoLog->WriteLog ("%s: dbpool: error: configuration not defined", __func__);
		}
		return -1;
	}

	int iRetVal = 0;
	int iFnRes = 0;
	char mcConnStr[1024];

	std::string
		strDBUser,
		strDBPswd,
		strDBDescr;
	const char *pcszConfParam = NULL;

	/* запрашиваем имя пользователя БД из конфигурации */
	pcszConfParam = "db_user";
	iFnRes = g_pcoConf->GetParamValue (pcszConfParam, strDBUser);
	if (iFnRes || 0 == strDBUser.length ()) {
		if (g_pcoLog) {
			g_pcoLog->WriteLog ("dbpool: %s: error: configuration parameter '%s' not defined", __func__, pcszConfParam);
      return -1;
		}
	}

	/* запрашиваем пароль пользователя БД из конфигурации */
	pcszConfParam = "db_pswd";
	iFnRes = g_pcoConf->GetParamValue (pcszConfParam, strDBPswd);
	if (iFnRes || 0 == strDBPswd.length ()) {
		if (g_pcoLog) {
			g_pcoLog->WriteLog ("dbpool: %s: error: configuration parameter '%s' not defined", __func__, pcszConfParam);
      return -1;
		}
	}

	/* запрашиваем дескриптор БД из конфигурации */
	pcszConfParam = "db_descr";
	iFnRes = g_pcoConf->GetParamValue (pcszConfParam, strDBDescr);
	if (iFnRes || 0 == strDBDescr.length ()) {
		if (g_pcoLog) {
			g_pcoLog->WriteLog ("dbpool: %s: error: configuration parameter '%s' not defined", __func__, pcszConfParam);
      return -1;
		}
	}

	/* формируем строку подключения */
	snprintf (
		mcConnStr,
		sizeof(mcConnStr),
		g_mcRadDBConnTempl,
		strDBUser.c_str(),
		strDBPswd.c_str(),
		strDBDescr.c_str());
	try {
		p_coDBConn.rlogon (mcConnStr);
		p_coDBConn.auto_commit_off ();
		if (g_pcoLog) {
			g_pcoLog->WriteLog ("dbpool: %s: DB connected successfully", __func__);
		}
	} catch (otl_exception &coOtlExc) {
		if (g_pcoLog) {
			g_pcoLog->WriteLog ("dbpool: %s: error: code: '%d'; description: '%s'", __func__, coOtlExc.code, coOtlExc.msg);
		}
		iRetVal = coOtlExc.code;
	}

	return iRetVal;
}

int db_pool_reconnect (otl_connect &p_coDBConn)
{
	int iRetVal = 0;
	int iFnRes;

	do {
		if (p_coDBConn.connected) {
			DisconnectDB (p_coDBConn);
		}
		if (g_pcoLog) {
			g_pcoLog->WriteLog ("dbpool: %s: info: trying to reconnect to DB", __func__);
		}
		iFnRes = ConnectDB (p_coDBConn);
		if (iFnRes) {
			/* подключиться к БД не удалось */
			if (g_pcoLog) {
				g_pcoLog->WriteLog ("dbpool: %s: error: reconnect failed", __func__);
			}
			iRetVal = -200001;
			break;
		}
	} while (0);

	return iRetVal;
}

int db_pool_check (otl_connect &p_coDBConn)
{
	int iRetVal = 0;
	int iFnRes;

	/* объект класса не подключен к БД */
	if (! p_coDBConn.connected) {
		return -2;
	}

	std::string strDBDummyReq;

	/* запрашиваем текст проверочного запроса из конфигурации */
	if (g_pcoConf) {
		iFnRes = g_pcoConf->GetParamValue ("db_dummy_req", strDBDummyReq);
	} else {
		iFnRes = -1;
	}
	if (iFnRes || 0 == strDBDummyReq.length ()) {
		strDBDummyReq = "select to_char(sysdate, 'yyyy') from dual";
	}

	/* проверяем работоспособность подключения на простейшем запросе */
	try {
		char mcTime[128];
		otl_stream coStream (1, strDBDummyReq.c_str (), p_coDBConn);
		coStream >> mcTime;
	} catch (otl_exception &coExc) {
		/* если запрос выполнился с ошибкой */
		if (g_pcoLog) {
			g_pcoLog->WriteLog ("dbpool: %s: error: connection test failed: error: code: '%s'; description: '%s'", __func__, coExc.code, coExc.msg);
		}
		iRetVal = -3;
	}

	return iRetVal;
}

int db_pool_init (CLog *p_pcoLog, CConfig *p_pcoConf)
{
	/* копируем указатель на объект класса логгера */
	if (p_pcoLog) {
		g_pcoLog = p_pcoLog;
	} else {
		return -1;
	}
	/* копируем указатель на объект класса конфигурации */
	if (p_pcoConf) {
		g_pcoConf = p_pcoConf;
	} else {
		return -1;
	}

	int iRetVal = 0;
	int iFnRes;

	/* запрашиваем в кофигурации размер пула */
	std::string strDBPoolSize;
	const char *pcszConfParam = "db_pool_size";
	iFnRes = g_pcoConf->GetParamValue (pcszConfParam, strDBPoolSize);
	if (iFnRes || 0 == strDBPoolSize.length ()) {
		g_iPoolSize = 1;
		g_pcoLog->WriteLog ("dbpool: %s: info: configuration parameter '%s' not defined, set pool size value to '%d'", __func__, pcszConfParam, g_iPoolSize);
	} else {
		g_iPoolSize = atoi (strDBPoolSize.c_str ());
	}

	/* выделяем память для пула */
	g_pmsoPool = new SPoolElem [g_iPoolSize];
	for (int iInd = 0; iInd < g_iPoolSize; ++ iInd) {
		iFnRes = ConnectDB (g_pmsoPool[iInd].m_coDBConn);
		if (iFnRes) {
			break;
		}
	}

	/* проверяем результат инициализации массива */
	if (iFnRes) {
		db_pool_deinit ();
		return -2;
	}

	/* инициализируем семафор */
	iFnRes = sem_init (&g_tSem, 0, g_iPoolSize);
	if (iFnRes) {
		db_pool_deinit ();
		return -3;
	}

	/* инициализация мьютекса */
	iFnRes = pthread_mutex_init (&g_tMutex, NULL);
	if (iFnRes) {
		db_pool_deinit ();
		return -4;
	}
	/* на всякий случай освобождаем мьютекс, при первом использовании он нам нужен свободным */
	pthread_mutex_unlock (&g_tMutex);

	return iRetVal;
}

void db_pool_deinit ()
{
	/* освобождаем ресурсы, занятые семафором */
	sem_destroy (&g_tSem);

	/* освобождаем ресурсы, занятые мьютексом */
	pthread_mutex_destroy (&g_tMutex);

	/* освобождаем массив пула */
	if (g_pmsoPool) {
		for (int iInd = 0; iInd < g_iPoolSize; ++ iInd) {
			DisconnectDB (g_pmsoPool[iInd].m_coDBConn);
		}
		delete [] g_pmsoPool;
		g_pmsoPool = NULL;
	}
}

otl_connect * db_pool_get ()
{
	otl_connect * pcoRetVal = NULL;

	int iFnRes;
	timespec soTimeSpec;
	timeval soTimeVal;

	/* запрашиваем текущее время */
	iFnRes = gettimeofday (&soTimeVal, NULL);
	if (iFnRes) {
		return NULL;
	}

	/* вычисляем время до которого мы будем ждать освобождения семафора */
	soTimeSpec.tv_sec = soTimeVal.tv_sec + 1;
	soTimeSpec.tv_nsec = soTimeVal.tv_usec * 1000;

	/* ждем освобождения объекта для взаимодействия с БД */
	iFnRes = sem_timedwait (&g_tSem, &soTimeSpec);
	/* если во время ожидания возникла ошибка */
	if (iFnRes) {
		LOG_ERR_DESCR ("sem_timedwait error accurred");
		return NULL;
	}

	/* запрашиваем текущее время */
	iFnRes = gettimeofday (&soTimeVal, NULL);
	if (iFnRes) {
		return NULL;
	}

	/* вычисляем время до которого мы будем ждать освобождения мьютекса */
	soTimeSpec.tv_sec = soTimeVal.tv_sec + 1;
	soTimeSpec.tv_nsec = soTimeVal.tv_usec * 1000;

	/* входим в критическую секцию */
	iFnRes = pthread_mutex_timedlock (&g_tMutex, &soTimeSpec);
	/* если во время ожидания возникла ошибка */
	if (iFnRes) {
		LOG_ERR_DESCR ("pthread_mutex_timedlock error accurred");
		/* возвращаем семафор */
		sem_post (&g_tSem);
		return NULL;
	}

	/* ищем свободный элемент */
	for (int iInd = 0; iInd < g_iPoolSize; ++ iInd) {
		/* если наши незанятый элемент */
		if (! g_pmsoPool[iInd].m_bIsBusy) {
			g_pmsoPool[iInd].m_bIsBusy = true;
			pcoRetVal = & (g_pmsoPool[iInd].m_coDBConn);
		}
	}
	/* на всякий случай проверим значение указателя */
	if (NULL == pcoRetVal) {
		if (g_pcoLog) {
			g_pcoLog->WriteLog ("dbpool: %s: error: unexpected error occurred: there is no free db connections but semaphore has allowed to pass through it", __func__);
		}
	}

	/* освобождаем мьютекс */
	pthread_mutex_unlock (&g_tMutex);

	return pcoRetVal;
}

int db_pool_release (otl_connect *p_pcoDBConn)
{
	int iRetVal = 0;
	int iFnRes;
	timespec soTimeSpec;
	timeval soTimeVal;

	/* запрашиваем текущее время */
	iFnRes = gettimeofday (&soTimeVal, NULL);
	if (iFnRes) {
    /* в случае ошибки выполнения функции gettimeofday будет нулевое ожидание */
    soTimeSpec.tv_sec = 0;
    soTimeSpec.tv_nsec = 0;
	} else {
    /* вычисляем время до которого мы будем ждать освобождения мьютекса */
    soTimeSpec.tv_sec = soTimeVal.tv_sec + 1;
    soTimeSpec.tv_nsec = soTimeVal.tv_usec * 1000;
  }

	/* входим в критическую секцию */
	iFnRes = pthread_mutex_timedlock (&g_tMutex, &soTimeSpec);
	/* если во время ожидания возникла ошибка */
	if (iFnRes) {
		LOG_ERR_DESCR ("pthread_mutex_timedlock error accurred");
		return -1;
	}

	int iInd = 0;
	/* ищем соответствующий элемент */
	for (; iInd < g_iPoolSize; ++ iInd) {
		/* если наши искомый элемент */
		if (&(g_pmsoPool[iInd].m_coDBConn) == p_pcoDBConn) {
			break;
		}
	}
	/* на всякий случай проверим значение */
	if (g_iPoolSize == iInd) {
		if (g_pcoLog) {
			g_pcoLog->WriteLog ("dbpool: %s: error: unexpected error occurred: db coonector not found", __func__);
		}
	} else {
		g_pmsoPool[iInd].m_bIsBusy = false;
	}

	/* освобождаем мьютекс */
	iFnRes = pthread_mutex_unlock (&g_tMutex);
	/* если во время ожидания возникла ошибка */
	if (iFnRes) {
		LOG_ERR_DESCR ("pthread_mutex_unlock error accurred");
	}

	/* увеличиваем счетчик семафора */
	iFnRes = sem_post (&g_tSem);
	/* если во время ожидания возникла ошибка */
	if (iFnRes) {
		LOG_ERR_DESCR ("sem_post error accurred");
	}

	return iRetVal;
}
