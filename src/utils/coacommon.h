#ifndef _COACOMMON_H_
#define _COACOMMON_H_

#define	NAS_ID		"NASId"
#define	NAS_IP		"NASIp"
#define	NAS_PORT	"NASPort"
#define USER_NAME	"UserName"
#define USER_PSWD	"UserPswd"
#define SESSION_ID	"SessionId"
#define	ACCNT_INF	"AccountInfo"
#define	COMMAND		"Command"
#define	CMD_ACCNT_LOGON		"AccountLogon"
#define	CMD_ACCNT_LOGOFF	"AccountLogoff"
#define CMD_SESSION_QUERY	"SessionQuery"
#define CMD_SRV_ACTIVATE	"ServiceActivate"
#define CMD_SRV_DEACTIVATE	"ServiceDeActivate"
#define CMD_OPTION_FORCE	"Force"
#define CMD_ERX_ACTIVATE	"ERX-Service-Activate"
#define CMD_ERX_DEACTIVATE	"ERX-Service-Deactivate"
#define CMD_BNG_ACTIVATE	"BNG-Service-Activate"
#define CMD_BNG_DEACTIVATE	"BNG-Service-Deactivate"

#define	CMD_VAL_SEP			"="
#define	CMD_PARAM_SEP		"\n"

/*	IRBiS-PS request types
 *	0x00xx
 */
#define	MONIT_REQ	(unsigned short)0x0000
#define	MONIT_RSP	(unsigned short)0x0001
#define	ADMIN_REQ	(unsigned short)0x0002
#define	ADMIN_RSP	(unsigned short)0x0003

/*	IRBiS-PS request types
 *	0x03xx
 */
#define	COMMAND_REQ	(unsigned short)0x0302
#define	COMMAND_RSP	(unsigned short)0x0303

/*	IRBiS-PS common attribute types
 *	0x00xx
 */
#define	PS_RESULT		(unsigned short)0x0000
#define	PS_DESCR		(unsigned short)0x0001
#define	PS_SUBSCR		(unsigned short)0x0002
#define	PS_LASTOK		(unsigned short)0x0005
#define	PS_LASTER		(unsigned short)0x0006
#define	PS_STATUS		(unsigned short)0x0007
#define	PS_MDATTR		(unsigned short)0x0008
#define	PS_ADMCMD		(unsigned short)0x0010

/*	IRBiS-PS CoA module attribute types
 *	0x03xx
 */
#define	PS_NASIP		(unsigned short)0x0300
#define	PS_NASPORT	(unsigned short)0x0301
#define PS_USERNAME	(unsigned short)0x0302
#define	PS_USERPSWD	(unsigned short)0x0303
#define PS_SESSID		(unsigned short)0x0304
#define	PS_ACCINFO	(unsigned short)0x0305
#define PS_COMMAND	(unsigned short)0x0306
#define PS_FRAMEDIP	(unsigned short)0x0307
#define PS_SESSTATUS	(unsigned short)0x0308
#define PS_NASPORTID	(unsigned short)0x0309

#pragma pack(push,1)
/*	Атрибут запроса в формате IRBiS-PS
 */
struct SPSReqAttr {
	unsigned short m_usAttrType;
	unsigned short m_usAttrLen;
};

/*	Запрос в формате IRBiS-PS
 */
struct SPSRequest {
	unsigned int m_uiReqNum;
	unsigned short m_usReqType;
	unsigned short m_usPackLen;
};
#pragma pack(pop)

#endif /*_COACOMMON_H_*/
