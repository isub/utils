#include <stdio.h>

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

timeval & operator -= (timeval &p_soLeft, timeval &p_soRight)
{
	p_soLeft.tv_sec -= p_soRight.tv_sec;
	if (p_soLeft.tv_usec < p_soRight.tv_usec) {
		p_soLeft.tv_usec += 1000000 - p_soRight.tv_usec;
		--p_soLeft.tv_sec;
	} else {
		p_soLeft.tv_usec -= p_soRight.tv_usec;
	}
	return p_soLeft;
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

	do {
		if (! m_bInitialized) {
			iRetVal = -1;
			break;
		}

		timeval soTV;
		int iStrLen;

		iRetVal = Set (&soTV);
		if (iRetVal) {
			break;
		}
		soTV -= m_soTV;
		if (p_psoTV) {
			*p_psoTV = soTV;
		}
		if (p_pszString) {
			if (soTV.tv_sec) {
#ifdef WIN32
				iStrLen = _snprintf_s (p_pszString, p_stMaxLen, _TRUNCATE, "%u,%03u sec", soTV.tv_sec, soTV.tv_usec / 1000);
#else
				iStrLen = snprintf (p_pszString, p_stMaxLen - 1, "%u,%03u sec", soTV.tv_sec, soTV.tv_usec / 1000);
				if (0 < iStrLen) {
					if (iStrLen > p_stMaxLen - 1) {
						p_pszString[p_stMaxLen - 1] = '\0';
					} else {
						p_pszString[iStrLen] = '\0';
					}
				} else {
					iRetVal = -1;
					*p_pszString = '\0';
				}
#endif
			} else if (soTV.tv_usec / 1000) {
#ifdef WIN32
				_snprintf_s (p_pszString, p_stMaxLen, _TRUNCATE, "%u,%03u mlsec", soTV.tv_usec / 1000, soTV.tv_usec % 1000);
#else
				iStrLen = snprintf (p_pszString, p_stMaxLen, "%u,%03u mlsec", soTV.tv_usec / 1000, soTV.tv_usec % 1000);
				if (0 < iStrLen) {
					if (iStrLen > p_stMaxLen - 1) {
						p_pszString[p_stMaxLen - 1] = '\0';
					} else {
						p_pszString[iStrLen] = '\0';
					}
				} else {
					iRetVal = -1;
					*p_pszString = '\0';
				}
#endif
			} else {
#ifdef WIN32
				_snprintf_s (p_pszString, p_stMaxLen, _TRUNCATE, "%u mcsec", soTV.tv_usec);
#else
				iStrLen = snprintf (p_pszString, p_stMaxLen, "%u mcsec", soTV.tv_usec);
				if (0 < iStrLen) {
					if (iStrLen > p_stMaxLen - 1) {
						p_pszString[p_stMaxLen - 1] = '\0';
					} else {
						p_pszString[iStrLen] = '\0';
					}
				} else {
					iRetVal = -1;
					*p_pszString = '\0';
				}
#endif
			}
		}
	} while (0);

	if (iRetVal && p_pszString) {
		*p_pszString = '\0';
	}

	return iRetVal;
}
