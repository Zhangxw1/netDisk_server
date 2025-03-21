#include"protocol.h"

//给协议申请动态空间
PDU *mkPDU(uint uiMsgLen){
    uint uiPDULen=sizeof(PDU)+uiMsgLen;
    PDU *pdu=(PDU *)malloc(uiPDULen);
    if(NULL==pdu){
        exit(EXIT_FAILURE);
    }
    memset(pdu,0,uiPDULen);
    pdu->uiPDULen=uiPDULen;
    pdu->uiMsgLen=uiMsgLen;
    return pdu;
}
