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
#include <unordered_set>

extern CLog *g_pcoLog;

static std::map<std::string,SStat*> g_mapStat;
static pthread_t                    g_tThread;
static pthread_mutex_t              g_mutexThread;
static pthread_rwlock_t             g_rwlockStat;

void * stat_output (void *p_pArg);

struct SStat {
  std::string m_strObjName;  /* имя объекта статистики */
  uint64_t m_ui64Count;      /* количество выполнений */
  uint64_t m_ui64CountPrec;  /* количество выполнений - предыдущее значение */
  timeval m_soTmMin;         /* минимальная продолжительность выполнения */
  timeval m_soTmMax;         /* максимальная продолжительность выполнения */
  timeval m_soTmTotal;       /* суммарная продожительность выполнения */
  timeval m_soTmLast;        /* последнее значение времени выполнения */
  timeval m_soTmLastTotal;   /* суммарная продожительность выполнения с момента последнего отчета */
  pthread_mutex_t m_mutexStat;
  bool m_bInitialized;
  bool m_bFirst;             /* фиксируется первое значение */
  SStat *m_psoNext;          /* указатель на следующий объект ветви */
  /* конструктор */
  SStat(const char *p_pszObjName);
  /* деструктор */
  ~SStat();
};

/* инициализация модуля статистики */
int stat_init()
{
  int iRetVal = 0;

  pthread_rwlock_init( &g_rwlockStat, NULL );
  pthread_mutex_init( &g_mutexThread, NULL );
  pthread_mutex_lock( &g_mutexThread );
  pthread_create( &g_tThread, NULL, stat_output, NULL );

  return iRetVal;
}

/* деинициализация модуля статистики */
void stat_fin()
{
  pthread_mutex_unlock( &g_mutexThread );
  pthread_join( g_tThread, NULL );
  pthread_rwlock_wrlock( &g_rwlockStat );
  std::map<std::string,SStat*>::iterator iter;
  iter = g_mapStat.begin();
  /* обходим все ветки */
  for (; iter != g_mapStat.end(); ++iter) {
    /* обходим все объекты ветки */
    delete iter->second;
  }
  pthread_rwlock_unlock( &g_rwlockStat );
  pthread_rwlock_destroy( &g_rwlockStat );
}

static std::unordered_set<void(*)(char**)> g_usetCbFn;

void stat_register_cb( void ( p_pCbFn )( char **p_ppszString ) )
{
  g_usetCbFn.insert( p_pCbFn );
}

SStat * stat_get_branch (const char *p_pszObjName)
{
  std::map<std::string,SStat*>::iterator iter;
  std::string strObjName = p_pszObjName;
  SStat *psoStat;

  /* ищем соответствующую ветку */
  pthread_rwlock_rdlock( &g_rwlockStat );
  iter = g_mapStat.find( strObjName );
  pthread_rwlock_unlock( &g_rwlockStat );
  if ( iter == g_mapStat.end() ) {
    std::pair<std::map<std::string,SStat*>::iterator,bool> pairInsertResult;
    /* создаем новый объект */
    psoStat = new SStat(p_pszObjName);
    pthread_rwlock_wrlock( &g_rwlockStat );
    pairInsertResult = g_mapStat.insert( std::make_pair( strObjName, psoStat ) );
    if( pairInsertResult.second ) {
      /* если за время разблокировки никто не успел создать такую же ветку */
    } else {
      delete psoStat;
      psoStat = pairInsertResult.first->second;
    }
    pthread_rwlock_unlock( &g_rwlockStat );
  } else {
    /* используем найденный */
    psoStat = iter->second;
  }

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
      UTL_LOG_D( *g_pcoLog, "CTimeMeasurer::GetDifference: error: result code: %d", iFnRes );
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
  if (p_pcoTM) {
    if (psoWanted->m_bFirst) {
      psoWanted->m_bFirst = false;
      psoWanted->m_soTmMin = tvDif;
      psoWanted->m_soTmMax = tvDif;
    } else {
      p_pcoTM->GetMin(&psoWanted->m_soTmMin, &tvDif);
      p_pcoTM->GetMax(&psoWanted->m_soTmMax, &tvDif);
    }
    p_pcoTM->Add(&psoWanted->m_soTmTotal, &tvDif);
    p_pcoTM->Add(&psoWanted->m_soTmLastTotal, &tvDif);
    psoWanted->m_soTmLast = tvDif;
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
  memset(&m_soTmLast, 0, sizeof(m_soTmLast));
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

static inline double stat_avg_duration(const timeval &p_soTimeVal, uint64_t p_uiCount)
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

void * stat_output( void * )
{
  int iFnRes;
  timespec soTmSpec;
  SStat *psoTmp;
  std::string strMsg;
  char mcBuf[0x1000], mcMin[64], mcMax[64], mcTotal[64], mcLast[64];
  CTimeMeasurer coTM;

  clock_gettime (CLOCK_REALTIME_COARSE, &soTmSpec);
  soTmSpec.tv_sec += 60;

  while( ETIMEDOUT == pthread_mutex_timedlock( &g_mutexThread, &soTmSpec ) ) {
    /**/
    pthread_rwlock_rdlock( &g_rwlockStat );
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
    for( std::map<std::string,SStat*>::iterator iter = g_mapStat.begin(); iter != g_mapStat.end(); ++iter ) {
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
    pthread_rwlock_unlock( &g_rwlockStat );

    std::unordered_set<void(*)(char**)>::iterator iterCbFn = g_usetCbFn.begin();
    char *pszString = NULL;

    for( ; iterCbFn != g_usetCbFn.end(); ++iterCbFn ) {
      (*iterCbFn)(&pszString);
      if( NULL != pszString ) {
        strMsg += "\r\n";
        strMsg += pszString;
        free( pszString );
        pszString = NULL;
      }
    }

    g_pcoLog->WriteLog (strMsg.c_str());
    /**/
    clock_gettime( CLOCK_REALTIME_COARSE, &soTmSpec );
    soTmSpec.tv_sec += 60;
  }

  pthread_exit (NULL);
}
