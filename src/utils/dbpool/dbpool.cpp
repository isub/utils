#include "dbpool.h"

#include <string>
#include <semaphore.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>

#if ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE)
#define LOG_ERR_DESCR(str) 		{ int __errcode__ = errno; \
		char mcErrDescr[1024]; \
		if (g_pcoLog) { \
			if (0 != strerror_r (__errcode__, mcErrDescr, sizeof (mcErrDescr))) { \
				mcErrDescr[0] = '\0'; \
			} \
			g_pcoLog->WriteLog ("dbpool: %s: error: %s: code: '%d'; description: '%s'", __func__, str, __errcode__, mcErrDescr); \
    } \
  }
#else
#define LOG_ERR_DESCR(str) 	{	int __errcode__ = errno; \
		char mcErrDescr[1024]; \
		if (g_pcoLog) { \
			if (NULL == strerror_r (__errcode__, mcErrDescr, sizeof (mcErrDescr))) { \
				mcErrDescr[0] = '\0'; \
			} \
			g_pcoLog->WriteLog ("dbpool: %s: error: %s: code: '%d'; description: '%s'", __func__, str, __errcode__, mcErrDescr); \
		} \
  }
#endif

/* шаблон строки подключения к БД */
static char g_mcRadDBConnTempl[] = "%s/%s@%s";
/* указатель на объект логгера */
static CLog *g_pcoLog = NULL;
/* параметры подключения к БД */
static std::string g_strDBUser;
static std::string g_strDBPswd;
static std::string g_strDBDscr;
/* размер пула */
static int g_iPoolSize;

/* семафор для ожидания свободного указателя на объект класса для взаимодействия с БД */
sem_t g_tSem;
/* элемент пула */
struct SPoolElem {
	otl_connect m_coDBConn;
  pthread_mutex_t m_mutexLock;
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

int ConnectDB (otl_connect &p_coDBConn, std::string &p_strDBUsr, std::string &p_strDBPwd, std::string &p_strDBDescr)
{
	int iRetVal = 0;
	int iFnRes = 0;
	char mcConnStr[1024];

	/* формируем строку подключения */
	snprintf (
		mcConnStr,
		sizeof(mcConnStr),
		g_mcRadDBConnTempl,
		p_strDBUsr.c_str(),
		p_strDBPwd.c_str(),
		p_strDBDescr.c_str());
	try {
		p_coDBConn.rlogon (mcConnStr);
		p_coDBConn.auto_commit_off ();
		if (g_pcoLog) {
			g_pcoLog->WriteLog ("dbpool: %s: DB connected successfully", __func__);
		}
	} catch (otl_exception &coOtlExc) {
		if (g_pcoLog) {
      g_pcoLog->WriteLog( "dbpool: %s: error: code: '%d'; description: '%s'; user name: %s; description: %s", __func__, coOtlExc.code, coOtlExc.msg, p_strDBUsr.c_str(), p_strDBDescr.c_str() );
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

    iFnRes = ConnectDB( p_coDBConn, g_strDBUser, g_strDBPswd, g_strDBDscr );
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

  std::string strDBDummyReq = "select to_char(sysdate, 'yyyy') from dual";

	/* проверяем работоспособность подключения на простейшем запросе */
	try {
		char mcTime[128];
		otl_stream coStream (1, strDBDummyReq.c_str (), p_coDBConn);
		coStream >> mcTime;
	} catch (otl_exception &coExc) {
		/* если запрос выполнился с ошибкой */
		if (g_pcoLog) {
			g_pcoLog->WriteLog ("dbpool: %s: error: connection test failed: error: code: '%d'; description: '%s'", __FUNCTION__, coExc.code, coExc.msg );
		}
		iRetVal = -3;
	}

	return iRetVal;
}

int db_pool_init ( CLog *p_pcoLog, std::string &p_strDBUsr, std::string &p_strDBPwd, std::string &p_strDBDscr, int p_iPoolSize )
{
	/* копируем указатель на объект класса логгера */
	if (p_pcoLog) {
		g_pcoLog = p_pcoLog;
	} else {
		return -1;
	}
  /* копируем параметры подключения к БД */
  g_strDBUser = p_strDBUsr;
  g_strDBPswd = p_strDBPwd;
  g_strDBDscr = p_strDBDscr;
  g_iPoolSize = p_iPoolSize;

	int iRetVal = 0;
	int iFnRes;

	if (0 == g_iPoolSize) {
		g_iPoolSize = 1;
	}

	/* выделяем память для пула */
	g_pmsoPool = new SPoolElem [g_iPoolSize];
	for (int iInd = 0; iInd < g_iPoolSize; ++ iInd) {
    iFnRes = ConnectDB( g_pmsoPool[ iInd ].m_coDBConn, g_strDBUser, g_strDBPswd, g_strDBDscr );
    /* инициализация мьютекса */
    iFnRes = pthread_mutex_init( &g_pmsoPool[iInd].m_mutexLock, NULL );
		if (iFnRes) {
			break;
		}
	}

  if (iFnRes) {
		db_pool_deinit ();
		return -2;
  }

	/* проверяем результат инициализации массива */
	if (iFnRes) {
		db_pool_deinit ();
		return -3;
	}

	/* инициализируем семафор */
	iFnRes = sem_init (&g_tSem, 0, g_iPoolSize);
	if (iFnRes) {
		db_pool_deinit ();
		return -4;
	}

	return iRetVal;
}

void db_pool_deinit ()
{
	/* освобождаем ресурсы, занятые семафором */
	sem_destroy (&g_tSem);

	/* освобождаем массив пула */
	if (g_pmsoPool) {
		for (int iInd = 0; iInd < g_iPoolSize; ++ iInd) {
			DisconnectDB (g_pmsoPool[iInd].m_coDBConn);
      pthread_mutex_unlock( &g_pmsoPool[iInd].m_mutexLock );
      pthread_mutex_destroy( &g_pmsoPool[iInd].m_mutexLock );
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
	soTimeSpec.tv_sec = soTimeVal.tv_sec + 5;
	soTimeSpec.tv_nsec = soTimeVal.tv_usec * 1000;

	/* ждем освобождения объекта для взаимодействия с БД */
	iFnRes = sem_timedwait (&g_tSem, &soTimeSpec);
	/* если во время ожидания возникла ошибка */
	if (iFnRes) {
		LOG_ERR_DESCR( "sem_timedwait error accurred" );
		return NULL;
	}

	/* запрашиваем текущее время */
	iFnRes = gettimeofday (&soTimeVal, NULL);
	if (iFnRes) {
		return NULL;
	}

	/* ищем свободный элемент */
	for (int iInd = 0; iInd < g_iPoolSize; ++ iInd) {
		/* если наши незанятый элемент */
		if (0 == pthread_mutex_trylock( &g_pmsoPool[iInd].m_mutexLock ) ) {
			pcoRetVal = & (g_pmsoPool[iInd].m_coDBConn);
      break;
		}
	}
	/* на всякий случай проверим значение указателя */
	if (NULL == pcoRetVal) {
		if (g_pcoLog) {
			g_pcoLog->WriteLog ("dbpool: %s: error: unexpected error occurred: there is no free db connections but semaphore has allowed to pass through it", __func__);
		}
	}

	return pcoRetVal;
}

int db_pool_release (otl_connect *p_pcoDBConn)
{
	int iRetVal = 0;
	int iFnRes;

	int iInd = 0;
	/* ищем соответствующий элемент */
	for (; iInd < g_iPoolSize; ++ iInd) {
		/* если наши искомый элемент */
		if (&(g_pmsoPool[iInd].m_coDBConn) == p_pcoDBConn) {
      pthread_mutex_unlock( &g_pmsoPool[iInd].m_mutexLock );
			break;
		}
	}
	/* на всякий случай проверим значение */
	if (g_iPoolSize == iInd) {
		if (g_pcoLog) {
			g_pcoLog->WriteLog ("dbpool: %s: error: unexpected error occurred: db coonector not found", __func__);
		}
	}

	/* увеличиваем счетчик семафора */
	iFnRes = sem_post (&g_tSem);
	/* если во время ожидания возникла ошибка */
	if (iFnRes) {
		LOG_ERR_DESCR ("sem_post error accurred");
	}

	return iRetVal;
}
