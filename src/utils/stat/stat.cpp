#include "stat.h"
#include "utils/log/log.h"
#include "utils/timemeasurer/timemeasurer.h"

#include <map>
#include <string>
#include <new>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

extern CLog *g_pcoLog;
std::map<std::string,SStat*> *g_pmapStat = NULL;
pthread_t *g_ptThread = NULL;
pthread_mutex_t *g_pmutexThread = NULL;
pthread_mutex_t *g_pmutexStat = NULL;
bool *g_bStopThread;

void * stat_output (void *p_pArg);

#define MYDELETE(a) if (NULL != a) { delete a; a = NULL; }

struct SStat {
	std::string m_strObjName;	/* имя объекта статистики */
	uint64_t m_ui64Count;			/* количество выполнений */
	uint64_t m_ui64CountPrec; /* количество выполнений - предыдущее значение */
	timeval m_soTmMin;				/* минимальная продолжительность выполнения */
	timeval m_soTmMax;				/* максимальная продолжительность выполнения */
	timeval m_soTmTotal;			/* суммарная продожительность выполнения */
  timeval m_soTmLastTotal;	/* суммарная продожительность выполнения с момента последнего отчета */
  pthread_mutex_t m_mutexStat;
	bool m_bInitialized;
	bool m_bFirst;					/* фиксируется первое значение */
	SStat *m_psoNext;				/* указатель на следующий объект ветви */
	/* конструктор */
	SStat(const char *p_pszObjName);
	/* деструктор */
	~SStat();
};

/* инициализация модуля статистики */
int stat_init()
{
	int iRetVal = 0;

	if (g_pmapStat) {
		UTL_LOG_D (*g_pcoLog, "abnormal pointer value: 'g_pmapStat'");
	}
	if (g_pmutexStat) {
		UTL_LOG_D (*g_pcoLog, "abnormal pointer value: 'g_pmutexStat'");
	}
	try {
		g_pmapStat = new std::map<std::string,SStat*>;
		g_pmutexStat = new pthread_mutex_t;
		g_pmutexThread = new pthread_mutex_t;
		g_ptThread = new pthread_t;
		g_bStopThread = new bool;
		*g_bStopThread = false;
	} catch (std::bad_alloc &ba) {
		iRetVal = ENOMEM;
		MYDELETE (g_pmapStat);
		MYDELETE (g_pmutexStat);
		MYDELETE (g_pmutexThread);
		MYDELETE (g_ptThread);
		MYDELETE (g_bStopThread);
		UTL_LOG_F (*g_pcoLog, "bad alloc: %s", ba.what());
	}

	pthread_mutex_init (g_pmutexStat, NULL);
	pthread_mutex_init (g_pmutexThread, NULL);
	pthread_mutex_lock (g_pmutexThread);
	pthread_create (g_ptThread, NULL, stat_output, g_bStopThread);

	return iRetVal;
}

/* деинициализация модуля статистики */
int stat_fin()
{
	int iRetVal = 0;

	if (g_ptThread) {
		if (g_bStopThread) {
			*g_bStopThread = true;
		}
		if (g_pmutexThread) {
			pthread_mutex_unlock (g_pmutexThread);
		}
		pthread_join (*g_ptThread, NULL);
		MYDELETE (g_ptThread);
	}
  if( NULL != g_pmutexStat ) {
    iRetVal = pthread_mutex_lock (g_pmutexStat);
    if (g_pmapStat) {
      std::map<std::string,SStat*>::iterator iter;
      iter = g_pmapStat->begin();
      /* обходим все ветки */
      for (; iter != g_pmapStat->end(); ++iter) {
        /* обходим все объекты ветки */
        delete iter->second;
      }
      MYDELETE (g_pmapStat);
    }
    iRetVal = pthread_mutex_unlock (g_pmutexStat);
  	iRetVal = pthread_mutex_destroy (g_pmutexStat);
  }

  MYDELETE (g_pmutexStat);
	MYDELETE (g_pmutexThread);
	MYDELETE (g_bStopThread);

	return iRetVal;
}

SStat * stat_get_branch (const char *p_pszObjName)
{
	if (NULL == g_pmapStat) {
		UTL_LOG_D (*g_pcoLog, "'g_pmapStat' contains NULL value");
		return NULL;
	}

	std::map<std::string,SStat*>::iterator iter;
	std::string strObjName = p_pszObjName;
	SStat *psoStat;

	/* ищем соответствующую ветку */
	pthread_mutex_lock (g_pmutexStat);
	iter = g_pmapStat->find(strObjName);
	if (iter == g_pmapStat->end()) {
		/* создаем новый объект */
		psoStat = new SStat(p_pszObjName);
		g_pmapStat->insert(std::make_pair(strObjName, psoStat));
	} else {
		/* используем найденный */
		psoStat = iter->second;
	}
	pthread_mutex_unlock (g_pmutexStat);

	return psoStat;
}

void stat_measure (SStat *p_psoStat, const char *p_pszObjName, CTimeMeasurer *p_pcoTM)
{
	/* проверка параметров */
	if (NULL != p_psoStat) {
  } else {
		UTL_LOG_D (*g_pcoLog, "it has got NULL pointer 'p_psoStat'");
		return;
	}
	if (NULL != p_pszObjName) {
  } else {
		UTL_LOG_D (*g_pcoLog, "it has got NULL pointer 'p_pszObjName'");
		return;
	}

	SStat *psoLast = p_psoStat;
	SStat *psoWanted = NULL;
	timeval tvDif;

	/* фиксируем продолжительность исполнения */
  if (p_pcoTM) {
    int iFnRes = p_pcoTM->GetDifference(&tvDif, NULL, 0);
    if (0 == iFnRes) {
    } else {
      UTL_LOG_D("CTimeMeasurer::GetDifference: error: result code: %d", iFnRes);
      return;
    }
  }

	pthread_mutex_lock(&p_psoStat->m_mutexStat);
	/* ищем соответствующий объект */
	while (psoLast) {
		/* объект найден */
		if (0 == psoLast->m_strObjName.compare(p_pszObjName)) {
			psoWanted = psoLast;
			break;
		}
		/* дошли до конца списка, но не нашли необходимый объект */
		if (NULL == psoLast->m_psoNext) {
			psoWanted = new SStat (p_pszObjName);
			psoLast->m_psoNext = psoWanted;
			break;
		}
		psoLast = psoLast->m_psoNext;
	}
	pthread_mutex_unlock(&p_psoStat->m_mutexStat);

	pthread_mutex_lock(&psoWanted->m_mutexStat);
	psoWanted->m_ui64Count++;
	if (psoWanted->m_bFirst) {
    if (p_pcoTM) {
      psoWanted->m_bFirst = false;
      psoWanted->m_soTmMin = tvDif;
      psoWanted->m_soTmMax = tvDif;
    }
	} else {
    if (p_pcoTM) {
      p_pcoTM->GetMin(&psoWanted->m_soTmMin, &tvDif);
      p_pcoTM->GetMax(&psoWanted->m_soTmMax, &tvDif);
    }
	}
  if (p_pcoTM) {
    p_pcoTM->Add(&psoWanted->m_soTmTotal, &tvDif);
    p_pcoTM->Add(&psoWanted->m_soTmLastTotal, &tvDif);
  }
	pthread_mutex_unlock(&psoWanted->m_mutexStat);
}

SStat::SStat(const char *p_pszObjName)
{
	m_bInitialized = false;
	m_bFirst = true;
	m_strObjName = p_pszObjName;
	m_ui64Count = 0ULL;
	m_ui64CountPrec = 0ULL;
	memset (&m_soTmMin, 0, sizeof (m_soTmMin));
	memset(&m_soTmMax, 0, sizeof(m_soTmMax));
	memset(&m_soTmTotal, 0, sizeof(m_soTmTotal));
	memset(&m_soTmLastTotal, 0, sizeof(m_soTmLastTotal));
	m_psoNext = NULL;
	if (0 == pthread_mutex_init (&m_mutexStat, NULL))
		m_bInitialized = true;
}

SStat::~SStat()
{
	if (m_psoNext)
		delete m_psoNext;
	pthread_mutex_destroy (&m_mutexStat);
}

static double stat_avg_duration(const timeval &p_soTimeVal, uint64_t p_uiCount)
{
  if (static_cast<uint64_t>(0) != p_uiCount) {
    double dfRetVal;
    dfRetVal = p_soTimeVal.tv_sec;
    dfRetVal *= 1000000;
    dfRetVal += p_soTimeVal.tv_usec;
    dfRetVal /= p_uiCount;
    dfRetVal /= 1000000;
    return dfRetVal;
  } else {
    return static_cast<double>(0);
  }
}

void * stat_output (void *p_pArg)
{
  int iFnRes;
	bool *bStop = (bool*)p_pArg;
	timespec soTmSpec;
	SStat *psoTmp;
	std::string strMsg;
	char mcBuf[0x1000], mcMin[64], mcMax[64], mcTotal[64], mcLast[64];
	CTimeMeasurer coTM;

	clock_gettime (CLOCK_REALTIME_COARSE, &soTmSpec);
	soTmSpec.tv_sec += 60;

	while (*bStop == false) {
		pthread_mutex_timedlock (g_pmutexThread, &soTmSpec);
		/**/
		pthread_mutex_lock (g_pmutexStat);
		strMsg.clear();
    iFnRes = snprintf (mcBuf, sizeof (mcBuf), "\r\n               branch name               | total count |  diff  |  min. value  |    max. value    |  total duration  |  last duration  | tot. avg | last tot.avg |" );
    if (0 < iFnRes) {
      if (sizeof(mcBuf) > static_cast<size_t>(iFnRes)) {
      } else {
        mcBuf[sizeof(mcBuf)-1] = '\0';
      }
      strMsg = mcBuf; 
    } else {
      continue;
    }
		for (std::map<std::string,SStat*>::iterator iter = g_pmapStat->begin(); iter != g_pmapStat->end(); ++iter) {
			strMsg += "\r\n";
			psoTmp = iter->second;
			for (; psoTmp; psoTmp = psoTmp->m_psoNext) {
				pthread_mutex_lock (&psoTmp->m_mutexStat);
				coTM.ToString (&psoTmp->m_soTmMin, mcMin, sizeof(mcMin));
				coTM.ToString (&psoTmp->m_soTmMax, mcMax, sizeof(mcMax));
				coTM.ToString (&psoTmp->m_soTmTotal, mcTotal, sizeof(mcTotal));
				coTM.ToString (&psoTmp->m_soTmLastTotal, mcLast, sizeof(mcLast));
				iFnRes = snprintf (mcBuf, sizeof (mcBuf), "%40s | %11llu | %6llu | %12s | %16s | %16s | %15s | %0.6f |     %0.6f |\r\n",
					psoTmp->m_strObjName.c_str (),
					psoTmp->m_ui64Count,
					psoTmp->m_ui64Count - psoTmp->m_ui64CountPrec,
					mcMin, mcMax, mcTotal, mcLast,
          stat_avg_duration(psoTmp->m_soTmTotal, psoTmp->m_ui64Count),
          stat_avg_duration(psoTmp->m_soTmLastTotal, psoTmp->m_ui64Count - psoTmp->m_ui64CountPrec));
				psoTmp->m_ui64CountPrec = psoTmp->m_ui64Count;
        memset(&psoTmp->m_soTmLastTotal, 0, sizeof(psoTmp->m_soTmLastTotal));
        if (0 < iFnRes) {
          if (sizeof(mcBuf) > static_cast<size_t>(iFnRes)) {
          } else {
            mcBuf[sizeof(mcBuf)-1] = '\0';
          }
          strMsg += mcBuf;
        } else {
          continue;
        }
				pthread_mutex_unlock (&psoTmp->m_mutexStat);
			}
		}
		pthread_mutex_unlock (g_pmutexStat);
		g_pcoLog->WriteLog (strMsg.c_str());
		/**/
		if (*bStop) {
			break;
		}
		clock_gettime (CLOCK_REALTIME_COARSE, &soTmSpec);
		soTmSpec.tv_sec += 60;
	}

	pthread_exit (NULL);
}
