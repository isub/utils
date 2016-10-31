# utils
auxiliary utilites

example of using CPSPacket interface:

#include <stdio.h>
#include "utils/pspacket/pspacket.h"

int main(int argc, char *argv[])
{
  int iBufSize;
  int iAttrNum;
  char mcBuf[0x100];
  CPSPacket coPSPack;
  SPSRequest *psoReq;
  char *pmcParsed;

  if (argc >= 2) {
    iBufSize = atoi (argv[1]);
  } else {
    iBufSize = 0x1000;
  }
  if (argc >= 3) {
    iAttrNum = atoi (argv[2]);
  } else {
    iAttrNum = 5;
  }

  pmcParsed = reinterpret_cast<char*>(malloc (iBufSize));

  psoReq = reinterpret_cast<SPSRequest*>(mcBuf);
  coPSPack.Init (psoReq, sizeof(mcBuf), 0x10, 0x1010);
  if (--iAttrNum >= 0)
    coPSPack.AddAttr (psoReq, sizeof(mcBuf), 0x1234, "0123456789", 10, 0);
  if (--iAttrNum >= 0)
    coPSPack.AddAttr (psoReq, sizeof(mcBuf), 0x1234, "\x0\x1\x2\x3\x4\x5\x6\x7\x8\x9", 10, 0);
  if (--iAttrNum >= 0)
    coPSPack.AddAttr (psoReq, sizeof(mcBuf), 0x2345, "1234567890", 10, 0);
  if (--iAttrNum >= 0)
    coPSPack.AddAttr (psoReq, sizeof(mcBuf), 0x2345, "\x1\x2\x3\x4\x5\x6\x7\x8\x9\xa", 10, 0);
  if (--iAttrNum >= 0)
    coPSPack.AddAttr (psoReq, sizeof(mcBuf), 0x3456, NULL, 0, 0);

  /* проверка текстовой версии */
  coPSPack.Parse (psoReq, sizeof(mcBuf), pmcParsed, iBufSize);
  puts (pmcParsed);

  if (pmcParsed) {
    free (pmcParsed);
  }

  puts ("\n\n");

  /* проверка по атрибутам */
  int iFnRes;
  std::multimap<__uint16_t,SPSReqAttrParsed> mmapAttrList;
  std::multimap<__uint16_t,SPSReqAttrParsed>::iterator iter;
  __uint32_t ui32ReqNum;
  __uint16_t ui16ReqType;
  __uint16_t ui16PackLen;

  iFnRes = coPSPack.Parse (psoReq, sizeof(mcBuf), ui32ReqNum, ui16ReqType, ui16PackLen, mmapAttrList);
  if (iFnRes) {
    return 0;
  }

  printf ("packet number: %u; request type: %#x; packet length: %u\n", ui32ReqNum, ui16ReqType, ui16PackLen);
  for (iter = mmapAttrList.begin(); iter != mmapAttrList.end(); ++iter) {
    printf ("attr type: %#x; data length: %u; data pointer: %p\n", iter->first, iter->second.m_usDataLen, iter->second.m_pvData);
  }

  coPSPack.EraseAttrList (mmapAttrList);

  return 0;
}
