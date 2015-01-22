#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <string>
#include <vector>
#include <map>

#ifdef	WIN32
#	ifdef	CONFIG_IMPORT
#		define	CONFIG_SPEC	__declspec(dllimport)
#	else
#		define	CONFIG_SPEC __declspec(dllexport)
#	endif
#else
#	define	CONFIG_SPEC
#endif

class CONFIG_SPEC CConfig {
public:
	int LoadConf (const char *p_pcszFileName, int p_iCollectEmptyParam = 0);
	int GetParamValue (const char *p_pcszName, std::vector<std::string> &p_vectValueList);
	int GetParamValue (const char *p_pcszName, std::string &p_strValue);
	void SetDebugLevel (int p_iDebugLevel);
public:
	CConfig (int p_iDebugLevel = 0);
private:
	std::multimap<std::string,std::string> m_mmapConf;
	int m_iDebug;
};

#endif // _CONFIG_H_
