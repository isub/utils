#ifndef _TIMEMEASURER_H_
#define _TIMEMEASURER_H_

#ifdef WIN32
#	include <time.h>
#	include <Winsock2.h>
#	ifdef	TM_IMPORT
#		define	TM_SPEC	__declspec(dllimport)
#	else
#		define	TM_SPEC __declspec(dllexport)
#	endif
#else
#	include <sys/time.h>
#	define	TM_SPEC
#	include <stddef.h>
#endif

class TM_SPEC CTimeMeasurer {
public:
	CTimeMeasurer ();
	~CTimeMeasurer ();
public:
	int Set (timeval *p_psoTV = NULL);
	int GetDifference (timeval *p_psoTV, char *p_pszString, size_t p_stMaxLen);
	/* минимальное значение записывается в p_psoA */
	void GetMin (timeval *p_psoA, const timeval *p_psoB);
	/* максимальное значение записывается в p_psoA */
	void GetMax (timeval *p_psoA, const timeval *p_psoB);
	/* сумма записывается в p_psoA */
	void Add (timeval *p_psoA, const timeval *p_psoB);
	/* текстовое представление */
	void ToString (timeval *p_psoTv, char *p_pszString, size_t p_stMaxLen);
private:
	timeval m_soTV;
	bool m_bInitialized;
};

#endif /* _TIMEMEASURER_H_ */
