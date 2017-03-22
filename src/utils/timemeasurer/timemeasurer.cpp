#include <stdio.h>
#include <errno.h>

#include "timemeasurer.h"

CTimeMeasurer::CTimeMeasurer ()
{
	if (0 == Set ()) {
		m_bInitialized = true;
	} else {
		m_bInitialized = false;
	}
}

CTimeMeasurer::~CTimeMeasurer ()
{
	m_bInitialized = false;
}

timeval & operator -= (timeval &p_soLeft, const timeval &p_soRight)
{
	p_soLeft.tv_sec -= p_soRight.tv_sec;
	if (p_soLeft.tv_usec < p_soRight.tv_usec) {
		p_soLeft.tv_usec += 1000000;
    p_soLeft.tv_usec -= p_soRight.tv_usec;
		--p_soLeft.tv_sec;
	} else {
		p_soLeft.tv_usec -= p_soRight.tv_usec;
	}
	return p_soLeft;
}

timeval & operator += (timeval &p_soLeft, const timeval &p_soRight)
{
	p_soLeft.tv_sec += p_soRight.tv_sec;
	p_soLeft.tv_usec += p_soRight.tv_usec;
	if (p_soLeft.tv_usec >= 1000000) {
		p_soLeft.tv_usec -= 1000000;
    ++p_soLeft.tv_sec;
	}
	return p_soLeft;
}

bool operator < (const timeval &p_soLeft, const timeval &p_soRight)
{
	if (p_soLeft.tv_sec < p_soRight.tv_sec)
		return true;
	if (p_soLeft.tv_sec > p_soRight.tv_sec)
		return false;
	if (p_soLeft.tv_usec < p_soRight.tv_usec)
		return true;
	return false;
}

int CTimeMeasurer::Set (timeval *p_psoTV)
{
	int iRetVal;
	timeval *psoPtr;

	if (p_psoTV) {
		psoPtr = p_psoTV;
	} else {
		psoPtr = &m_soTV;
	}

#ifdef WIN32
	clock_t clockVal = clock();
	if (-1 == clockVal) {
		iRetVal = -1;
	} else {
		psoPtr->tv_sec = clockVal / CLOCKS_PER_SEC;
		psoPtr->tv_usec = clockVal % CLOCKS_PER_SEC;
		psoPtr->tv_usec *= 1000;
		iRetVal = 0;
	}
#else
	iRetVal = gettimeofday (psoPtr, NULL);
#endif

	return iRetVal;
}

int CTimeMeasurer::GetDifference (timeval *p_psoTV, char *p_pszString, size_t p_stMaxLen)
{
	int iRetVal = 0;

  if (NULL != p_psoTV || NULL != p_pszString) {
  } else {
    return EINVAL;
  }

	do {
		if (! m_bInitialized) {
			iRetVal = -1;
			break;
		}

		timeval soTV;

		iRetVal = Set (&soTV);
		if (iRetVal) {
			break;
		}
		soTV -= m_soTV;
		if (p_psoTV) {
			*p_psoTV = soTV;
		}
		if (p_pszString) {
			ToString (&soTV, p_pszString, p_stMaxLen);
		}
	} while (0);

	if (iRetVal && p_pszString) {
		*p_pszString = '\0';
	}

	return iRetVal;
}

void CTimeMeasurer::GetMin (timeval *p_psoA, const timeval *p_psoB)
{
	if (*p_psoB < *p_psoA)
		*p_psoA = *p_psoB;
}

void CTimeMeasurer::GetMax (timeval *p_psoA, const timeval *p_psoB)
{
	if (*p_psoA < *p_psoB)
		*p_psoA = *p_psoB;
}

void CTimeMeasurer::Add (timeval *p_psoA, const timeval *p_psoB)
{
	*p_psoA += *p_psoB;
}

void CTimeMeasurer::Sub (timeval *p_psoA, const timeval *p_psoB)
{
	*p_psoA -= *p_psoB;
}

void CTimeMeasurer::ToString (timeval *p_psoTv, char *p_pszString, size_t p_stMaxLen)
{
	int iStrLen;

	if (NULL != p_pszString) {
	} else {
		return;
	}

  if (NULL != p_psoTv) {
  } else {
    p_pszString[0] = '\0';
    return;
  }

  if (p_psoTv->tv_sec > 3600) {
#ifdef WIN32
    iStrLen = _snprintf_s(p_pszString, p_stMaxLen, _TRUNCATE, "%u:%02u:%02u hr ", p_psoTv->tv_sec / 3600, (p_psoTv->tv_sec % 3600) / 60, p_psoTv->tv_sec % 60);
#else
    iStrLen = snprintf(p_pszString, p_stMaxLen, "%u:%02u:%02u hr ",
      static_cast<unsigned int>(p_psoTv->tv_sec / 3600), static_cast<unsigned int>((p_psoTv->tv_sec % 3600) / 60), static_cast<unsigned int>(p_psoTv->tv_sec % 60));
    if (0 < iStrLen) {
      if (p_stMaxLen > static_cast<size_t>(iStrLen)) {
      } else {
        p_pszString[p_stMaxLen - 1] = '\0';
      }
    } else {
      *p_pszString = '\0';
    }
#endif
  } else if (p_psoTv->tv_sec > 60) {
#ifdef WIN32
    iStrLen = _snprintf_s(p_pszString, p_stMaxLen, _TRUNCATE, "%u:%02u,%03u min", p_psoTv->tv_sec / 60, p_psoTv->tv_sec % 60, p_psoTv->tv_usec / 1000);
#else
    iStrLen = snprintf(p_pszString, p_stMaxLen, "%u:%02u,%03u min",
      static_cast<unsigned int>(p_psoTv->tv_sec/60), static_cast<unsigned int>(p_psoTv->tv_sec % 60), static_cast<unsigned int>(p_psoTv->tv_usec / 1000));
    if (0 < iStrLen) {
      if (p_stMaxLen > static_cast<size_t>(iStrLen)) {
      } else {
        p_pszString[p_stMaxLen - 1] = '\0';
      }
    } else {
      *p_pszString = '\0';
    }
#endif
  } else if (p_psoTv->tv_sec != 0) {
#ifdef WIN32
		iStrLen = _snprintf_s (p_pszString, p_stMaxLen, _TRUNCATE, "%u,%03u sec", p_psoTv->tv_sec, p_psoTv->tv_usec / 1000);
#else
		iStrLen = snprintf (p_pszString, p_stMaxLen, "%u,%03u sec", static_cast<unsigned int>(p_psoTv->tv_sec), static_cast<unsigned int>(p_psoTv->tv_usec / 1000));
		if (0 < iStrLen) {
      if (p_stMaxLen > static_cast<size_t>(iStrLen)) {
      } else {
				p_pszString[p_stMaxLen - 1] = '\0';
			}
		} else {
			*p_pszString = '\0';
		}
#endif
	} else if (p_psoTv->tv_usec > 1000) {
#ifdef WIN32
		_snprintf_s (p_pszString, p_stMaxLen, _TRUNCATE, "%u,%03u mls", p_psoTv->tv_usec / 1000, p_psoTv->tv_usec % 1000);
#else
		iStrLen = snprintf (p_pszString, p_stMaxLen, "%u,%03u mls", static_cast<unsigned int>(p_psoTv->tv_usec / 1000), static_cast<unsigned int>(p_psoTv->tv_usec % 1000));
		if (0 < iStrLen) {
      if (p_stMaxLen > static_cast<size_t>(iStrLen)) {
      } else {
				p_pszString[p_stMaxLen - 1] = '\0';
			}
		} else {
			*p_pszString = '\0';
		}
#endif
	} else {
#ifdef WIN32
		_snprintf_s (p_pszString, p_stMaxLen, _TRUNCATE, "%u mcs", p_psoTv->tv_usec);
#else
		iStrLen = snprintf (p_pszString, p_stMaxLen, "%u mcs", static_cast<unsigned int>(p_psoTv->tv_usec));
		if (0 < iStrLen) {
			if (p_stMaxLen > static_cast<size_t>(iStrLen)) {
      } else {
				p_pszString[p_stMaxLen - 1] = '\0';
			}
		} else {
			*p_pszString = '\0';
		}
#endif
	}
}
