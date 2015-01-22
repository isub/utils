struct SParamList {
	char m_mcParamName[128];
	char m_mcParamValue[1024];
	SParamList *m_psoNext;
};

SParamList * ParseBuffer(
	const unsigned char *p_pucBuf,
	size_t p_stLen);

unsigned char * EncodeParam (SParamList *p_psoParamList);
