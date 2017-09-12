#ifndef _COACOMMON_H_
#define _COACOMMON_H_

/*	IRBiS-PS request types
 *	0x00xx
 */
#define MONIT_REQ    (unsigned short)0x0000
#define MONIT_RSP    (unsigned short)0x0001
#define ADMIN_REQ    (unsigned short)0x0002
#define ADMIN_RSP    (unsigned short)0x0003
#define ARE_Y_ALIVE  (unsigned short)0x0004
#define I_AM_ALIVE   (unsigned short)0x0005

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
#define GW_REQUEST_ID   (unsigned short)0x0011

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

/* IRBiS-PS SS7 gateway request types */
#define	SS7GW_IMSI_REQ  (unsigned short)0x0602 /* request-IMSI */
#define	SS7GW_IMSI_RESP (unsigned short)0x0603 /* response-IMSI */
#define	RS_TRIP_REQ     (unsigned short)0x0104 /* request-triplet */
#define	RS_TRIP_RESP    (unsigned short)0x0105 /* response-triplet */

/* IRBiS-PS SS7 gateway attribute types */
#define	SS7GW_IMSI      (unsigned short)0x0602 /* IMSI */
#define	SS7GW_TRIP_NUM  (unsigned short)0x0603 /* Number of triplets requested */
#define	RS_RAND1        (unsigned short)0x0103 /* RAND1 */
#define	RS_SRES1        (unsigned short)0x0104 /* SRES1 */
#define	RS_KC1          (unsigned short)0x0105 /* KC1 */
#define	RS_RAND2        (unsigned short)0x0106 /* RAND2 */
#define	RS_SRES2        (unsigned short)0x0107 /* SRES2 */
#define	RS_KC2          (unsigned short)0x0108 /* KC2 */
#define	RS_RAND3        (unsigned short)0x0109 /* RAND3 */
#define	RS_SRES3        (unsigned short)0x010a /* SRES3 */
#define	RS_KC3          (unsigned short)0x010b /* KC3 */
#define	RS_RAND4        (unsigned short)0x010c /* RAND4 */
#define	RS_SRES4        (unsigned short)0x010d /* SRES4 */
#define	RS_KC4          (unsigned short)0x010e /* KC4 */
#define	RS_RAND5        (unsigned short)0x010f /* RAND5 */
#define	RS_SRES5        (unsigned short)0x0110 /* SRES5 */
#define	RS_KC5          (unsigned short)0x0111 /* KC5 */

#define	SS7GW_QUINTUPLET_REQ  (unsigned short)0x0702
#define	SS7GW_QUINTUPLET_CONF (unsigned short)0x0703
#define	SS7GW_QUINTUPLET_RESP (unsigned short)0x0704

#define ATTR_RAND       (unsigned short)0x0201
#define ATTR_AUTN       (unsigned short)0x0202
#define ATTR_XRES       (unsigned short)0x0203
#define ATTR_CK         (unsigned short)0x0204
#define ATTR_IK         (unsigned short)0x0205

#define	ATTR_REQUESTED_VECTORS  (unsigned short)0x0206
#define	ATTR_RECEIVED_VECTORS   (unsigned short)0x0207

/*	IRBiS-PS PCRF module request types
 *	0x10xx
 */
#define PCRF_CMD_INSERT_SESSION 0x1000  /* insert/update session info in session cache */
#define PCRF_CMD_REMOVE_SESSION 0x1001  /* remove session info from session cache */
#define PCRF_CMD_INSERT_SESSRUL 0x1002  /* insert session rule in session rule cache */
#define PCRF_CMD_REMOVE_SESSRUL 0x1003  /* remove session rule from session rule cache */
#define PCRF_CMD_SESS_USAGE     0x1004  /* send session-usage request */

/*	IRBiS-PS PCRF module attribute types
 *	0x10xx
 */
/* attribute and value pair */
/* irbis-ps attribute payload format */
/* 0-15      | 15-31  | 32-47   | 48-63      | 64-*        */
/* vendor id | avp id | padding | avp length | avp payload */
#define PCRF_ATTR_AVP       0x1000
#define PCRF_ATTR_CGI       0x1001
#define PCRF_ATTR_ECGI      0x1002
#define PCRF_ATTR_IMEI      0x1003
#define PCRF_ATTR_IMSI      0x1004
#define PCRF_ATTR_PSES      0x1005
#define PCRF_ATTR_IPCANTYPE 0x1006
#define PCRF_ATTR_RULNM     0x1007
#define PCRF_ATTR_RATTYPE   0x1008

/*  EIR request types
 *  0x11xx
 */
#define EIR_CHECKIMEI_REQ                     0x1100
#define EIR_CHECKIMEI_RESP                    0x1101
#define EIR_IDENTITYCHK_REQ                   0x1102
#define EIR_IDENTITYCHK_RESP                  0x1103

/*  EIR attribute types
 *  0x11xx
 */
#define EIR_IMEI                              PCRF_ATTR_IMEI
#define EIR_IMEISV                            0x1100
#define EIR_IMSI                              PCRF_ATTR_IMSI
#define EIR_EQUIPMENT_STATUS                  0x1101
#define EIR_SV                                0x1102

/* EIR result code */                         
#define EIR_DIAMETER_ERROR_EQUIPMENT_UNKNOWN  5422
#define EIR_SUCCESS                           0

/* Equipment-Status */                        
#define EIR_EQUIPMENT_STATUS_WHITELISTED      0
#define EIR_EQUIPMENT_STATUS_BLACKLISTED      1
#define EIR_EQUIPMENT_STATUS_GREYLISTED       2

#pragma pack(push,1)

/*	Атрибут запроса в формате IRBiS-PS */
struct SPSReqAttr {
	unsigned short m_usAttrType;
	unsigned short m_usAttrLen;
};

/*	Запрос в формате IRBiS-PS */
struct SPSRequest {
	unsigned int m_uiReqNum;
	unsigned short m_usReqType;
	unsigned short m_usPackLen;
};

#pragma pack(pop)

/* структура для хранения атрибутов */
struct SPSAttrList {
	struct SPSReqAttr m_soPackAttr;
	unsigned char *m_pmucData;
	struct SPSAttrList *m_psoNext;
};

/* структура для хранения пакета */
struct SPSPackHolder {
	struct SPSRequest m_soPackHdr;
	struct SPSAttrList *m_psoAttrList;
};


#endif /*_COACOMMON_H_*/
