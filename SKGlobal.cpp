#include "SKGlobal.h"
#include <cstring>

//const char* g_pHeartbeatLength         = "0004";
const char* g_pHeartbeatRequestMessage = "HTBT";
const char* g_pHeartbeatReplyMessage   = "HTBR";

unsigned char g_uncaHeaetbeatRequestBuf[6];
unsigned char g_uncaHeaetbeatReplyBuf[6];

int g_nLogMode = 1;
int g_nHttpMode = 1;
int g_nClientShmSize = 10000;
int g_bMaxWaitTime = 5000000; //³æ¦ìus
int g_bTimeOutOrderNumPer30s = 0;
mutex g_b_mtxTimeOutOrderNumPer30s;
bool g_bIntraday = false;

string g_strServerKey = "";
string g_strServerCrt = "";
//string g_strServerChainCrt = "";
string g_strLocalIp = "";

TSKClientShm* g_tSKClientsShm = NULL;
TSKServersShm* g_tSKServersShm = NULL;

void InitialGlobal()
{
    memset(g_uncaHeaetbeatRequestBuf, 0, sizeof(g_uncaHeaetbeatRequestBuf));
    memset(g_uncaHeaetbeatReplyBuf, 0, sizeof(g_uncaHeaetbeatReplyBuf));

    g_uncaHeaetbeatRequestBuf[0] = g_uncaHeaetbeatReplyBuf[0] = ESCAPE_BYTE;
    g_uncaHeaetbeatRequestBuf[1] = HEARTBEAT_REQ_BYTE;
    g_uncaHeaetbeatReplyBuf[1] = HEARTBEAT_REP_BYTE;

    memcpy(g_uncaHeaetbeatRequestBuf + 2, g_pHeartbeatRequestMessage, strlen(g_pHeartbeatRequestMessage));
    memcpy(g_uncaHeaetbeatReplyBuf + 2, g_pHeartbeatReplyMessage, strlen(g_pHeartbeatReplyMessage));
}