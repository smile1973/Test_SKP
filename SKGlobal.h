#ifndef __SKGLOBAL_H__
#define __SKGLOBAL_H__

#include <string>
#include <mutex>
#include "SKFormat.h"

using namespace std;

/***** PACKET IDENTIFIER *****/
#define ESCAPE_BYTE                         0x1b

#define LOGIN_BYTE                          0x20
#define LOGIN_REP_BYTE                      0x21
#define SESSION_BYTE                        0x22
#define SESSION_REP_BYTE                    0x23
#define HEARTBEAT_REQ_BYTE                  0x30
#define HEARTBEAT_REP_BYTE                  0x31
#define RECONN_BYTE                         0x32

#define TSORDER_BYTE                        0x01
#define TSORDER_REP_BYTE                    0x02
#define TSALTER_BYTE                        0x03
#define TSALTER_REP_BYTE                    0x04
#define TFORDER_BYTE                        0x05
#define TFORDER_REP_BYTE                    0x06
#define TFALTER_BYTE                        0x07
#define TFALTER_REP_BYTE                    0x08
#define OFORDER_BYTE                        0x09
#define OFORDER_REP_BYTE                    0x10
#define OSORDER_BYTE                        0x11
#define OSORDER_REP_BYTE                    0x12
#define OSBORDER_BYTE                       0x13
#define OSBORDER_REP_BYTE                   0x14
#define PRE_TSALTER_BYTE                    0x15
#define PRE_TFALTER_BYTE                    0x16
#define PRE_OFORDER_BYTE                    0x17
#define PRE_OSORDER_BYTE                    0x18

/***** DATA SIZE *****/
// esc(1) + trans_no(1)
#define PACKET_HEADER_LENGTH                2
// Code(4) + Message(512) + SessionID(100) + Reserved(256)
#define LOGIN_REP_LENGTH                    872
// Code(4) + Message(512) + Reserved(256)
#define SESSION_REP_LENGTH                  772
// Code(4) + Message(512) + OrderNo(20) + SeqNo(20) + ORKEY(100) + Reserved(256)
#define ORDER_REP_LENGTH                    912
// Type(1)
#define RECONN_LENGTH                       1

/***** OTHER *****/
#define HEARTBEAT_TIME_INTERVAL             30

/***** HEARTBEAT MESSAGE *****/
extern const char* g_pHeartbeatRequestMessage;
extern const char* g_pHeartbeatReplyMessage;

extern unsigned char g_uncaHeaetbeatRequestBuf[6];
extern unsigned char g_uncaHeaetbeatReplyBuf[6];

const string g_strProgram = "SKProxy";
const string g_strVersion = "1.0.0";

const string g_strFuncType[16] =
{
    "Logger", "Login", "Session",
    "TSOrder", "TSAlter", "TFOrder", "TFAlter", "OFOrder", "FTSOrder", "FTSAlter", "OSOrder", "OSBOrder","PreTSAlter","PreTFAlter","PreOFOrder","PreOSOrder"
};

//const string g_strAcceptClientVersion[1] =    //支持的client 端 lib版本
//{
//   "1.0.0"
//};

// Error Code
const string g_strECPending = "1000";
const string g_strECUserData = "1001";
const string g_strECGWRet = "1002";
const string g_strECLogin = "1003";
const string g_strECSessionID = "1004";
const string g_strECOverConnection = "1005";
const string g_strECShm = "1006";
const string g_strECGoingToClose = "1007";
const string g_strECExceedMaxTime = "1008";
const string g_strECFatal = "9999";

// Error Message
const string g_strEMPending = "Pending Order";
const string g_strEMParseJson = "Parse Json Error";
const string g_strEMGWConnFail = "GW連線失敗";
const string g_strEMLoginMax = "登入超過3次";
const string g_strEMLoginMore = "已登入成功, 無須登入";
const string g_strEMLoginIng = "登入中, 請稍後再試";
const string g_strEMLoginFirst = "請先登入或驗證SessionID";
const string g_strEMOverConnection = "連線數已達上限";
const string g_strEMShm = "AP Shm 異常";
const string g_strEMSessionID = "SessionID Error";
const string g_strEMGoingToClose = "server即將斷線，無法接受委託";
const string g_strEMExceedMaxTime = "此單子等待超時，委託失敗";
const string g_strEMFatal = "Fatal Error";

extern int g_bMaxWaitTime;//如果委託在queue中的等待時間超過這個時間，則那張單子回傳失敗 (單位us)

// Server Cert 
extern string g_strServerKey;
extern string g_strServerCrt;
//extern string g_strServerChainCrt;
extern string g_strLocalIp;

extern int g_nLogMode; // Log Model (0=Debug; 1=Release)
extern int g_nHttpMode; //curl設定 (0=不重複使用連線 ; 1=重複使用連線+keepalive)
extern int g_nClientShmSize;
extern bool g_bIntraday;
extern int g_bTimeOutOrderNumPer30s; //每30秒內 委託超時的單子數
extern mutex g_b_mtxTimeOutOrderNumPer30s;

// Shared Memory
extern TSKServersShm* g_tSKServersShm;
extern TSKClientShm* g_tSKClientsShm;
#endif
