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
#endif

class TM_SPEC CTimeMeasurer {
public:
	CTimeMeasurer ();
	~CTimeMeasurer ();
public:
	int Set (timeval *p_psoTV = NULL);
	int GetDifference (timeval *p_psoTV, char *p_pszString, size_t p_stMaxLen);
private:
	timeval m_soTV;
	bool m_bInitialized;
};
