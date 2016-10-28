#include <map>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#  include <Winsock2.h>
#  include "utils/ps_common.h"
#  pragma  comment(lib, "ws2_32.lib")
#else
#  include <arpa/inet.h>
#  include "utils/ps_common.h"
#endif

#include "pspacket.h"

int CPSPacket::Init (SPSRequest *p_psoBuf, size_t p_stBufSize, __uint32_t p_ui32ReqNum, __uint16_t p_ui16ReqType) {
  int iRetVal = 0;

  do {
    /* проверяем достаточно ли размера буфера для заголовка пакета */
    if (p_stBufSize < sizeof(SPSRequest)) { iRetVal = -1; break; }
    p_psoBuf->m_uiReqNum = htonl (p_ui32ReqNum);
    p_psoBuf->m_usReqType = htons (p_ui16ReqType);
    p_psoBuf->m_usPackLen = htons (sizeof(SPSRequest));
  } while (0);

  return iRetVal;
}

int CPSPacket::SetReqNum (SPSRequest *p_psoBuf, size_t p_stBufSize, __uint32_t p_ui32ReqNum, int p_iValidate) {
  int iRetVal = 0;

  do {
    if (p_iValidate) { iRetVal = Validate (p_psoBuf, p_stBufSize); }
    if (iRetVal) { break; }
    p_psoBuf->m_uiReqNum = htonl (p_ui32ReqNum);
  } while (0);

  return iRetVal;
}

int CPSPacket::SetReqType (SPSRequest *p_psoBuf, size_t p_stBufSize, __uint16_t p_ui16ReqType, int p_iValidate) {
  int iRetVal = 0;

  do {
    if (p_iValidate) { iRetVal = Validate (p_psoBuf, p_stBufSize); }
    if (iRetVal) { break; }
    p_psoBuf->m_usReqType = htons (p_ui16ReqType);
  } while (0);

  return iRetVal;
}

int CPSPacket::AddAttr (SPSRequest *p_psoBuf, size_t p_stBufSize, __uint16_t p_ui16Type, const void *p_pValue, __uint16_t p_ui16ValueLen, int p_iValidate) {
  int iRetVal = 0;
  SPSReqAttr *psoTmp;
  __uint16_t
    ui16PackLen,
    ui16AttrLen;

  do {
    /* валидация пакета */
    if (p_iValidate) { iRetVal = Validate (p_psoBuf, p_stBufSize); }
    if (iRetVal) { break; }
    /* проверка размера буфера */
    ui16PackLen = ntohs (p_psoBuf->m_usPackLen);
    ui16AttrLen = p_ui16ValueLen + sizeof(SPSReqAttr);
    if ((size_t)(ui16PackLen + ui16AttrLen) > p_stBufSize) { iRetVal = -1; break; }
    psoTmp = (SPSReqAttr*)((char*)p_psoBuf + ui16PackLen);
    psoTmp->m_usAttrType = htons (p_ui16Type);
    psoTmp->m_usAttrLen = htons (ui16AttrLen);
    memcpy ((char*)psoTmp + sizeof(SPSReqAttr), p_pValue, p_ui16ValueLen);
    ui16PackLen += ui16AttrLen;
    p_psoBuf->m_usPackLen = htons (ui16PackLen);
    iRetVal = ui16PackLen;
  } while (0);

  return iRetVal;
}

int CPSPacket::Validate (const SPSRequest *p_psoBuf, size_t p_stBufSize) {
  int iRetVal = 0;
  SPSReqAttr *psoTmp;
  __uint16_t
    ui16TotalLen,
    ui16PackLen,
    ui16AttrLen;

  do {
    /* проверяем достаточен ли размер буфера для заголовка пакета */
    if (p_stBufSize < sizeof(SPSRequest)) { iRetVal = -1; break; }
    /* инициализируем суммарную длину начальным значением */
    ui16TotalLen = sizeof(SPSRequest);
    /* определяем длину пакета по заголовку */
    ui16PackLen = ntohs (p_psoBuf->m_usPackLen);
    /* инициализируем указатель на атрибут начальным значением */
    psoTmp = (SPSReqAttr*)((char*)p_psoBuf + sizeof(SPSRequest));
    /* обходим все атрибуты */
    while (((char*)p_psoBuf + ui16PackLen) > ((char*)psoTmp)) {
      /* определяем длину атрибута */
      ui16AttrLen = ntohs (psoTmp->m_usAttrLen);
      /* увеличиваем суммарную длину пакета */
      ui16TotalLen += ui16AttrLen;
      /* проверяем умещается ли пакет в буфере */
      if (ui16TotalLen > p_stBufSize) { iRetVal = -1; break; }
      /* определяем указатель на следующий атрибут */
      psoTmp = (SPSReqAttr*)((char*)psoTmp + ui16AttrLen);
      /* если суммарная длина больше длины пакета нет смысла проверять дальше */
      if (ui16TotalLen > ui16PackLen) { iRetVal = -1; break; }
    }
    /* если найдена ошибка завершаем проверку */
    if (iRetVal) { break; }
    /* сличаем длину пакета с суммой длин атрибутов */
    if (ui16TotalLen != ui16PackLen) { iRetVal = -1; break; }
  } while (0);

  return iRetVal;
}

void CPSPacket::EraseAttrList (std::multimap<__uint16_t,SPSReqAttr*> &p_mmapAttrList) {
  SPSReqAttr *psoTmp;
  std::multimap<__uint16_t,SPSReqAttr*>::iterator iterList;

  for (iterList = p_mmapAttrList.begin(); iterList != p_mmapAttrList.end(); ++iterList) {
    psoTmp = iterList->second;
    if (psoTmp) {
      free (psoTmp);
    }
  }
  p_mmapAttrList.clear ();
}

void CPSPacket::EraseAttrList (std::multimap<__uint16_t,SPSReqAttrParsed*> &p_mmapAttrList) {
  SPSReqAttrParsed *psoTmp;
  std::multimap<__uint16_t,SPSReqAttrParsed*>::iterator iterList;

  for (iterList = p_mmapAttrList.begin(); iterList != p_mmapAttrList.end(); ++iterList) {
    psoTmp = iterList->second;
    if (psoTmp) {
      if (NULL != psoTmp->m_pvData) {
        free (psoTmp->m_pvData);
      }
      if (NULL != psoTmp) {
        free (psoTmp);
      }
    }
  }
  p_mmapAttrList.clear ();
}

int CPSPacket::Parse (
  const SPSRequest *p_psoBuf,
  size_t p_stBufSize,
  std::multimap<__uint16_t,SPSReqAttr*> &p_pumapAttrList,
  int p_iValidate)
{
  int iRetVal = 0;
  SPSReqAttr *psoTmp;
  SPSReqAttr *psoPSReqAttr;
  __uint16_t
    ui16PackLen,
    ui16AttrLen;

  EraseAttrList (p_pumapAttrList);

  do {
    /* валидация пакета */
    if (p_iValidate) { iRetVal = Validate (p_psoBuf, p_stBufSize); }
    if (iRetVal) { break; }
    /* определяем длину пакета */
    ui16PackLen = ntohs (p_psoBuf->m_usPackLen);
    /* инициализируем указатель на атрибут начальным значением */
    psoTmp = (SPSReqAttr*)((char*)p_psoBuf + sizeof(SPSRequest));
    /* обходим все атрибуты */
    while (((char*)p_psoBuf + ui16PackLen) > ((char*)psoTmp)) {
      /* определяем длину атрибута */
      ui16AttrLen = ntohs (psoTmp->m_usAttrLen);
      /* добавляем атрибут в список */
      psoPSReqAttr = (SPSReqAttr*) malloc (ui16AttrLen);
      memcpy (psoPSReqAttr, psoTmp, ui16AttrLen);
      p_pumapAttrList.insert (std::make_pair (ntohs (psoTmp->m_usAttrType), psoPSReqAttr));
      /* определяем указатель на следующий атрибут */
      psoTmp = (SPSReqAttr*)((char*)psoTmp + ui16AttrLen);
    }
  } while (0);

  return iRetVal;
}

int CPSPacket::Parse (
  const SPSRequest *p_psoBuf, size_t p_stBufSize, __uint32_t &p_ui32ReqNum,
  __uint16_t &p_ui16ReqType,
  __uint16_t &p_ui16PackLen,
  std::multimap<__uint16_t,SPSReqAttr*> &p_pumapAttrList, int p_iValidate)
{
    int iRetVal = 0;

    /* валидация пакета */
    if (p_iValidate) { iRetVal = Validate (p_psoBuf, p_stBufSize); }
    if (iRetVal) { return iRetVal; }
    p_ui32ReqNum=ntohl (p_psoBuf->m_uiReqNum);
    p_ui16ReqType=ntohs (p_psoBuf->m_usReqType);
    p_ui16PackLen=ntohs (p_psoBuf->m_usPackLen);

    return Parse(p_psoBuf,p_stBufSize,p_pumapAttrList,0);
}

int CPSPacket::Parse (
  const SPSRequest *p_psoBuf,
  size_t p_stBufSize,
  std::multimap<__uint16_t,SPSReqAttrParsed*> &p_pumapAttrList,
  int p_iValidate)
{
  int iRetVal = 0;
  SPSReqAttr *psoTmp;
  SPSReqAttrParsed *psoPSReqAttr;
  __uint16_t
    ui16PackLen,
    ui16AttrLen;

  EraseAttrList (p_pumapAttrList);

  do {
    /* валидация пакета */
    if (p_iValidate) {
      iRetVal = Validate (p_psoBuf, p_stBufSize);
    }
    if (iRetVal) {
      break;
    }
    /* определяем длину пакета */
    ui16PackLen = ntohs (p_psoBuf->m_usPackLen);
    /* инициализируем указатель на атрибут начальным значением */
    psoTmp = (SPSReqAttr*)((char*)p_psoBuf + sizeof(SPSRequest));
    /* обходим все атрибуты */
    while (((char*)p_psoBuf + ui16PackLen) > ((char*)psoTmp)) {
      /* определяем длину атрибута */
      ui16AttrLen = ntohs (psoTmp->m_usAttrLen) - sizeof(*psoTmp);
      /* добавляем атрибут в список */
      psoPSReqAttr = (SPSReqAttrParsed*) malloc (sizeof(*psoPSReqAttr));
      if (0 < ui16AttrLen) {
        psoPSReqAttr->m_pvData = (SPSReqAttr*) malloc (ui16AttrLen);
        memcpy (psoPSReqAttr->m_pvData, ((char*)psoTmp) + sizeof(*psoTmp), ui16AttrLen);
      } else {
        psoPSReqAttr->m_pvData = NULL;
      }
      psoPSReqAttr->m_usDataLen = ui16AttrLen;
      psoPSReqAttr->m_usAttrType = ntohs (psoTmp->m_usAttrType);
      p_pumapAttrList.insert (std::make_pair (psoPSReqAttr->m_usAttrType, psoPSReqAttr));
      /* определяем указатель на следующий атрибут */
      psoTmp = (SPSReqAttr*)((char*)psoTmp + ui16AttrLen);
    }
  } while (0);

  return iRetVal;
}

int CPSPacket::Parse (
  const SPSRequest *p_psoBuf,
  size_t p_stBufSize,
  __uint32_t &p_ui32ReqNum,
  __uint16_t &p_ui16ReqType,
  __uint16_t &p_ui16PackLen,
  std::multimap<__uint16_t,SPSReqAttrParsed*> &p_pumapAttrList,
  int p_iValidate)
{
    int iRetVal = 0;

    /* валидация пакета */
    if (p_iValidate) { iRetVal = Validate (p_psoBuf, p_stBufSize); }
    if (iRetVal) { return iRetVal; }
    p_ui32ReqNum=ntohl (p_psoBuf->m_uiReqNum);
    p_ui16ReqType=ntohs (p_psoBuf->m_usReqType);
    p_ui16PackLen=ntohs (p_psoBuf->m_usPackLen);

    return Parse(p_psoBuf,p_stBufSize,p_pumapAttrList,0);
}

int CPSPacket::Parse (
  const SPSRequest *p_psoBuf,
  size_t p_stBufSize,
  char *p_pmcOutBuf,
  size_t p_stOutBufSize)
{
  int iRetVal = 0;
  int iFnRes, iStrLen;
  char mcString[0x1000];
  size_t stWrtInd, stStrLen;
  std::multimap<__uint16_t,SPSReqAttrParsed*> mapAttrList;

  do {
    /* разбор атрибутов пакета */
    iRetVal = Parse (p_psoBuf, p_stBufSize, mapAttrList);
    if (iRetVal) { break; }
    /* формируем заголовок пакета */
#ifdef WIN32
    iFnRes = _snprintf (
#else
    iFnRes = snprintf (
#endif
      mcString,
      sizeof (mcString),
      "request number: 0x%08x; type: 0x%04x; length: %u;",
      ntohl (p_psoBuf->m_uiReqNum),
      ntohs (p_psoBuf->m_usReqType),
      ntohs (p_psoBuf->m_usPackLen));
    if (0 < iFnRes) {
      if (sizeof (mcString) > static_cast<size_t>(iFnRes)) {
      } else {
        iFnRes = sizeof (mcString) - 1;
      }
    } else {
      iRetVal = -1;
      break;
    }
    mcString[iFnRes] = '\0';
    stWrtInd = 0;
    stStrLen = p_stOutBufSize > static_cast<size_t>(iFnRes) ? iFnRes : p_stOutBufSize - 1;
    memcpy (p_pmcOutBuf, mcString, stStrLen);
    p_pmcOutBuf[stStrLen] = '\0';
    stWrtInd = stStrLen;
    /* обходим все атрибуты */
    bool bStop = false;
    for (std::multimap<__uint16_t,SPSReqAttrParsed*>::iterator iter = mapAttrList.begin(); iter != mapAttrList.end (); ++iter) {
      /* формируем заголовок атрибута */
#ifdef WIN32
      iFnRes = _snprintf (
#else
      iFnRes = snprintf (
#endif
        mcString,
        sizeof (mcString),
        " attribute code: 0x%04x; data length: %u; value: ",
        iter->second->m_usAttrType,
        iter->second->m_usDataLen);
      /* если произошла ошика завершаем формирование атрибутов */
      if (0 < iFnRes) {
        if (static_cast<size_t>(iFnRes) >= sizeof (mcString)) {
          iFnRes = sizeof (mcString) - 1;
        }
      } else {
        iRetVal = -1;
        break;
      }
      mcString[iFnRes] = '\0';
      /* выводим значение атрибута по байтам */
      for (size_t i = 0; i < iter->second->m_usDataLen && static_cast<size_t>(iFnRes) < sizeof(mcString) - 1; ++i) {
        if (0x20 <= reinterpret_cast<char*>(iter->second->m_pvData)[i] && 0x7F > reinterpret_cast<char*>(iter->second->m_pvData)[i]) {
          mcString[iFnRes] = reinterpret_cast<char*>(iter->second->m_pvData)[i];
          ++iFnRes;
        } else {
#ifdef WIN32
          iStrLen = _snprintf (
#else
          iStrLen = snprintf (
#endif
            &mcString[iFnRes],
            sizeof (mcString) - iFnRes,
            "\\x%02x",
            reinterpret_cast<unsigned char*>(iter->second->m_pvData)[i]);
          /* если при выводе очередного байта возникла ошибка прекращаем обработку атрибута */
          if (0 < iStrLen) {
            if (sizeof (mcString) - iFnRes > static_cast<size_t> (iStrLen)) {
            } else {
              iStrLen = sizeof (mcString) - 1 - iFnRes;
            }
          } else {
            bStop =  true;
            break;
          }
          iFnRes += iStrLen;
        }
        /* досрочно завершаем вывод атрибута */
        if (bStop) {
          break;
        }
      }
      mcString[iFnRes] = '\0';
      if (iRetVal) {
        break;
      } else {
        stStrLen = p_stOutBufSize - stWrtInd > static_cast<size_t>(iFnRes) ? iFnRes : p_stOutBufSize - stWrtInd - 1;
        memcpy (&p_pmcOutBuf[stWrtInd], mcString, stStrLen);
        stWrtInd += stStrLen;
        p_pmcOutBuf[stWrtInd] = '\0';
      }
      /* если формирование атрибута прервано завершаем вывод всего пакета */
      if (bStop) {
        break;
      }
    }
    if (0 == iRetVal) {
      iRetVal = static_cast<int>(stWrtInd);
    }
  } while (0);

  EraseAttrList (mapAttrList);

  return iRetVal;
}
