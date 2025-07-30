#include "SKFormat.h"
#include "SKClients.h"
#include "SKServers.h"
#include "Utility.h"
#include <iostream>
#include <cstdio>
#include <vector>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <fcntl.h>
#include <glib.h>

void InitialGlobal();
void ReadServersConfigFile(string& strConfigFileName, struct TSKServersConfig& struConfig, string& strListenPort);
void ReadShmConfigFile(string& strConfigFileName, struct TSKShmConfig& struConfig);
bool ShmTimer();

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0,
    unsigned char* pMessage1 = NULL, int nMessage1Length = 0,
    unsigned char* pMessage2 = NULL, int nMessage2Length = 0);
extern void FprintfStderrLog(const string& strMsg);

CSKServers* g_pServers = NULL;
CSKClients* g_pClients = NULL;

// Loop Setting 5x12 = 60(1min)
int nMainLoopSec = 5;
int nMainLoopTimes = 12;
int nMainLoopTimes2 = 6;
//max timeout Order Number per 30s
int nOrderTimeOutMaxTimes = 30;    //每30秒內，timeout的委託單子數，如果超過這個數，則記錄在serverError
// Shared Memory
int nServersShmfd = -1;
string strServersShmName = "/SKServers-shm";
int nClientsShmfd = -1;
string strClientsShmName = "/SKClients-shm";

// Config File Name
//string strClientsConfigName = "SKClients.ini";
string strServersConfigName = "SKServers.ini";
string strShmConfigName = "SKShm.ini";

int Atoi(char* ptr, int n)
{
    int i, num;
    int sum = 0;

    for (i = 0; i < n; i++)
    {
        if (*(ptr + i) == '\0') // if '\0' -> end of string so break
            break;

        num = *(ptr + i) - '0';
        if (num < 0 || num > 9) // if any byte error -> return -1
            return -1;

        sum *= 10;
        sum += num;
    }

    return sum;
}

int main(int argc, char *argv[])
{
    // ------------------------------ Check Program Singleton ------------------------------
    /* open & shm_open mode
    * S_IRWXU  00700 user (file owner) has rwx permission
    * S_IRUSR  00400 user has r permission
    * S_IWUSR  00200 user has w permission
    * S_IXUSR  00100 user has x permission

    * S_IRWXG  00070 group has rwx permission
    * S_IRGRP  00040 group has r permission
    * S_IWGRP  00020 group has w permission
    * S_IXGRP  00010 group has x permission

    * S_IRWXO  00007 others have rwx permission
    * S_IROTH  00004 others have r permission
    * S_IWOTH  00002 others have w permission
    * S_IXOTH  00001 others have x permission
    */
    int nLockFile = open("SingletonCheck", O_CREAT | O_RDWR, 0666);
    int nRc = flock(nLockFile, LOCK_EX | LOCK_NB);
    if (nRc != 0)
    {
        FprintfStderrLog("Program is runing or permission denied, byebye.");
        exit(1);
    }
    else
    {
        FprintfStderrLog("Program Start");
    }

    // ------------------------------ Signal ------------------------------
    signal(SIGPIPE, SIG_IGN);

    // ------------------------------ Initialize and Check Singleton ------------------------------
    InitialGlobal();

    CHttpRequest::GlobalInit();

   // g_pServers = CSKServers::GetInstance();
    g_pServers = new CSKServers();
    if (g_pServers == NULL)
    {
        FprintfStderrLog("GET_SERVERS_ERROR");
        exit(1);
    }
  // g_pClients = CSKClients::GetInstance();
    g_pClients = new CSKClients();
    if (g_pClients == NULL)
    {
        FprintfStderrLog("GET_CLIENTS_ERROR");
        exit(1);
    }

    g_strLocalIp = CUtility::GetServerIP();
    FprintfStderrLog("Local IP is : " + g_strLocalIp);

    // ------------------------------ Read Servers/Clients/Shm Config File ------------------------------
    TSKServersConfig tSKServersConfig = {};
    TSKShmConfig tSKShmConfig = {};
    string strListenPort = "";

    ReadServersConfigFile(strServersConfigName, tSKServersConfig, strListenPort);
    ReadShmConfigFile(strShmConfigName, tSKShmConfig);

    // ------------------------------ Servers shared memory ------------------------------
    nServersShmfd = shm_open(strServersShmName.c_str(), O_CREAT | O_RDWR | O_EXCL, 0777);
    if (nServersShmfd < 0) // Shm 存在
    {
        nServersShmfd = shm_open(strServersShmName.c_str(), O_RDWR, 0777);
        if (nServersShmfd < 0)
        {
            FprintfStderrLog("Open SKServers-shm Error!");
            exit(1);
        }
        FprintfStderrLog("Open SKServers-shm Success!");
    }
    else
    {
        FprintfStderrLog("Create SKServers-shm Success!");
    }
    ftruncate(nServersShmfd, sizeof(TSKServersShm)); // 設定 Shm 大小

    // Servers-shm map
    g_tSKServersShm = (TSKServersShm *)mmap(NULL, sizeof(TSKServersShm), PROT_READ | PROT_WRITE, MAP_SHARED, nServersShmfd, 0);
    if (g_tSKServersShm == NULL || g_tSKServersShm == MAP_FAILED)
    {
        FprintfStderrLog("Get SKServers-shm Pointer Error!");
        close(nServersShmfd);
        exit(1);
    }
    FprintfStderrLog("Get SKServers-shm Pointer Success!");

    // 清空SHM
    memset(g_tSKServersShm, 0, sizeof(TSKServersShm));

    // 初始化
    memcpy(g_tSKServersShm->caLogMode, tSKShmConfig.strLogMode.c_str(), tSKShmConfig.strLogMode.length());
    memcpy(g_tSKServersShm->caIntraday, tSKShmConfig.strIntraday.c_str(), tSKShmConfig.strIntraday.length());
    memcpy(g_tSKServersShm->caIntradayStart, tSKShmConfig.strIntradayStart.c_str(), tSKShmConfig.strIntradayStart.length());
    memcpy(g_tSKServersShm->caIntradayEnd, tSKShmConfig.strIntradayEnd.c_str(), tSKShmConfig.strIntradayEnd.length());
    memcpy(g_tSKServersShm->caDailyReConnMin, tSKShmConfig.strDailyReConnMin.c_str(), tSKShmConfig.strDailyReConnMin.length());
    memcpy(g_tSKServersShm->caDailyReConnHour, tSKShmConfig.strDailyReConnHour.c_str(), tSKShmConfig.strDailyReConnHour.length());
    memcpy(g_tSKServersShm->caReConn, tSKShmConfig.strReConn.c_str(), tSKShmConfig.strReConn.length());
    memcpy(g_tSKServersShm->caHttpMode, tSKShmConfig.strHttpMode.c_str(), tSKShmConfig.strHttpMode.length());
    memcpy(g_tSKServersShm->caLoggerQID, tSKShmConfig.strLoggerQID.c_str(), tSKShmConfig.strLoggerQID.length());
    memcpy(g_tSKServersShm->caLoginQID, tSKShmConfig.strLoginQID.c_str(), tSKShmConfig.strLoginQID.length());
    memcpy(g_tSKServersShm->caSessionQID, tSKShmConfig.strSessionQID.c_str(), tSKShmConfig.strSessionQID.length());
    memcpy(g_tSKServersShm->caTSOrderQID, tSKShmConfig.strTSOrderQID.c_str(), tSKShmConfig.strTSOrderQID.length());
    memcpy(g_tSKServersShm->caTSAlterQID, tSKShmConfig.strTSAlterQID.c_str(), tSKShmConfig.strTSAlterQID.length());
    memcpy(g_tSKServersShm->caTFOrderQID, tSKShmConfig.strTFOrderQID.c_str(), tSKShmConfig.strTFOrderQID.length());
    memcpy(g_tSKServersShm->caTFAlterQID, tSKShmConfig.strTFAlterQID.c_str(), tSKShmConfig.strTFAlterQID.length());
    memcpy(g_tSKServersShm->caOFOrderQID, tSKShmConfig.strOFOrderQID.c_str(), tSKShmConfig.strOFOrderQID.length());
    memcpy(g_tSKServersShm->caFTSOrderQID, tSKShmConfig.strFTSOrderQID.c_str(), tSKShmConfig.strFTSOrderQID.length());
    memcpy(g_tSKServersShm->caFTSAlterQID, tSKShmConfig.strFTSAlterQID.c_str(), tSKShmConfig.strFTSAlterQID.length());
    memcpy(g_tSKServersShm->caOSOrderQID, tSKShmConfig.strOSOrderQID.c_str(), tSKShmConfig.strOSOrderQID.length());
    memcpy(g_tSKServersShm->caOSBOrderQID, tSKShmConfig.strOSBOrderQID.c_str(), tSKShmConfig.strOSBOrderQID.length());
    memcpy(g_tSKServersShm->caPreTSAlterQID, tSKShmConfig.strPreTSAlterQID.c_str(), tSKShmConfig.strPreTSAlterQID.length());
    memcpy(g_tSKServersShm->caPreTFAlterQID, tSKShmConfig.strPreTFAlterQID.c_str(), tSKShmConfig.strPreTFAlterQID.length());
    memcpy(g_tSKServersShm->caPreOFOrderQID, tSKShmConfig.strPreOFOrderQID.c_str(), tSKShmConfig.strPreOFOrderQID.length());
    memcpy(g_tSKServersShm->caPreOSOrderQID, tSKShmConfig.strPreOSOrderQID.c_str(), tSKShmConfig.strPreOSOrderQID.length());
    memcpy(g_tSKServersShm->caCertExpireD, tSKServersConfig.strCrtExpireDay.c_str(), tSKServersConfig.strCrtExpireDay.length());
    memcpy(g_tSKServersShm->caServerRun, "Y", 1);

    if (0 == system("pgrep SKShmAP > /dev/null"))
    {
        //A process having name PROCESS is running.
        memcpy(g_tSKServersShm->caShmAPRun, "Y", 1);
    }
    else
    {
        memcpy(g_tSKServersShm->caShmAPRun, "N", 1);
    }


    // ------------------------------ Client shared memory ------------------------------
    nClientsShmfd = shm_open(strClientsShmName.c_str(), O_CREAT | O_RDWR | O_EXCL, 0777);
    if (nClientsShmfd < 0) // Shm 存在
    {
        nClientsShmfd = shm_open(strClientsShmName.c_str(), O_RDWR, 0777);
        if (nClientsShmfd < 0)
        {
            FprintfStderrLog("Open SKClients-shm Error!");
            exit(1);
        }
        FprintfStderrLog("Open SKClients-shm Success!");
    }
    else
    {
        FprintfStderrLog("Create SKClients-shm Success!");
    }
    ftruncate(nClientsShmfd, sizeof(TSKClientShm) * g_nClientShmSize); // 設定 Shm 大小

    // Client-shm map
    g_tSKClientsShm = (TSKClientShm*)mmap(NULL, sizeof(TSKClientShm) * g_nClientShmSize, PROT_READ | PROT_WRITE, MAP_SHARED, nClientsShmfd, 0);
    if (g_tSKClientsShm == NULL || g_tSKClientsShm == MAP_FAILED)
    {
        FprintfStderrLog("Get SKClients-shm Pointer Error!");
        close(nClientsShmfd);
        exit(1);
    }
    FprintfStderrLog("Get SKClients-shm Pointer Success!");

    // 清空SHM
    memset(g_tSKClientsShm, 0, sizeof(TSKClientShm) * g_nClientShmSize);

    for (int nIdx = 0; nIdx < g_nClientShmSize; nIdx++)
    {
        // Index設定
        memcpy((g_tSKClientsShm + nIdx)->caIndex, to_string(nIdx).c_str(), to_string(nIdx).size());

        // 新增閒置Index
        g_pClients->InsertToShmQueue(nIdx);
    }

    /*if (bClientShmNew) // 新增Shm && 設定變數
    {
        for (int nIdx = 0; nIdx < nClientShmSize; nIdx++)
        {
            memset(g_tSKClientShm + nIdx, 0, sizeof(TSKClientShm));

            memcpy((g_tSKClientShm + nIdx)->caIndex, to_string(nIdx).c_str(), to_string(nIdx).size());
            memcpy((g_tSKClientShm + nIdx)->caNowConnNum, "0", sizeof("0"));
            
            // 初次新增, 全部都可用
            g_pClients->InsertToShmQueue(nIdx);
        }
    }
    else // Shm 存在, 初始部分變數
    {
        for (int nIdx = 0; nIdx < nClientShmSize; nIdx++)
        {
            memcpy((g_tSKClientShm + nIdx)->caIndex, to_string(nIdx).c_str(), to_string(nIdx).size()); //Index設定

            memset((g_tSKClientShm + nIdx)->caNowConnNum, 0, sizeof((g_tSKClientShm + nIdx)->caNowConnNum));
            memcpy((g_tSKClientShm + nIdx)->caNowConnNum, "0", sizeof("0")); //當前連線數: 0

            if(string((g_tSKClientShm + nIdx)->caLoginID) != "") // 有LoginID, 代表在使用中
                g_pClients->InsertToShmMap((g_tSKClientShm + nIdx)->caLoginID, nIdx);
            else // 沒有LoginID, 閒置中
                g_pClients->InsertToShmQueue(nIdx);
        }
    }
    */

    // ----------------如果資料庫中有此台server未紀錄已斷線的連線，則將他們都設為已斷線-------------------
    string strConnToZero = "";
    try
    {
        string strPostJson = "{\"function\":\"ToZeroByIP\",\"ServerIP\":\"" + g_strLocalIp + "\"}";
        CHttpRequest hRequest;

        for (int i = 0; i < 3; i++)
        {
            stringstream ssResponse;
            hRequest.Post(tSKServersConfig.strConnToZeroByIPURL, strPostJson, ssResponse, "");
            FprintfStderrLog("ConnToZeroByIP: " + ssResponse.str());
            strConnToZero = ssResponse.str();
            if (strConnToZero.find("000") != std::string::npos)
                break;
        }
    }
    catch (...)
    {
        FprintfStderrLog("ConnToZeroByIP: error");
    }

    // ------------------------------ Set Servers/Clients ------------------------------
    g_pServers->SetConfig(tSKServersConfig);
    g_pServers->StartServer();
    sleep(1);
    g_pClients->SetConfig(strListenPort, g_nClientShmSize);

    if (strConnToZero.find("000") == std::string::npos)
    {
        if (strConnToZero.find("403") != std::string::npos && strConnToZero.find("3:") != std::string::npos) //code:403,datas:3 可連線數有被歸0，但紀錄斷線時間失敗->不用記在告警file
        g_pServers->EnQueueLogger(e_ltServerData, "ConnToZeroByIP fail! : " + strConnToZero, false);
        else
        g_pServers->EnQueueLogger(e_ltServerError, "ConnToZeroByIP fail! : " + strConnToZero, false);
    }
    else
        g_pServers->EnQueueLogger(e_ltServerData, "ConnToZeroByIP: " + strConnToZero, false);
   

    // Start!!
    g_pServers->EnQueueLogger(e_ltServerInfo, "Server PID : " + to_string(::getpid()), false);
    g_pServers->EnQueueLogger(e_ltServerInfo, g_strProgram + "_" + g_strVersion, false);

    g_nHttpMode = Atoi(g_tSKServersShm->caHttpMode, sizeof(g_tSKServersShm->caHttpMode));

    // ------------------------------ loop check Client status, Servers, Shared memory ------------------------------
    int nLoopTimes = 0;
    while (1)
    {
        sleep(nMainLoopSec);

        nLoopTimes++;

        // false = Shm讀到斷線重連
        if (ShmTimer() == false)
        {
            break;
        }

        if (g_bTimeOutOrderNumPer30s >= nOrderTimeOutMaxTimes)
        {
            string strLogMsg = "Order TimeOut " + to_string(g_bTimeOutOrderNumPer30s) + " times within these 30 seconds";
            g_pServers->EnQueueLogger(e_ltServerError, strLogMsg, false);
        }

        // Every 30 second
        if (nLoopTimes == nMainLoopTimes2)
        {
            g_b_mtxTimeOutOrderNumPer30s.lock();
            g_bTimeOutOrderNumPer30s = 0;
            g_b_mtxTimeOutOrderNumPer30s.unlock();
        }

        // Every 1 Minutes
        if (nLoopTimes == nMainLoopTimes)
        {
            //g_pServers->Timer();
            g_pClients->CheckClientVector();
            nLoopTimes = 0;
        }
    }

    /*delete g_pClients;
    delete g_pServers;*/
    while (1) //Logger要寫完、在queue裡的單子要處理完才可以關閉程式
    {
        if (g_pServers->IsAllQueueEmpty() == true)
        {
            break;
        }
        else
        {
            sleep(1);
        }
    }
    delete g_pClients;

    while (1) //Logger要寫完、在queue裡的單子要處理完才可以關閉程式
    {
        if (g_pServers->IsAllQueueEmpty() == true)
        {
            sleep(3);
            memcpy(g_tSKServersShm->caLoggerQNum, "0", sizeof(g_tSKServersShm->caLoggerQNum));
            memcpy(g_tSKServersShm->caLoginQNum, "0", sizeof(g_tSKServersShm->caLoginQNum));
            memcpy(g_tSKServersShm->caSessionQNum, "0", sizeof(g_tSKServersShm->caSessionQNum));
            memcpy(g_tSKServersShm->caTSOrderQNum, "0", sizeof(g_tSKServersShm->caTSOrderQNum));
            memcpy(g_tSKServersShm->caTSAlterQNum, "0", sizeof(g_tSKServersShm->caTSAlterQNum));
            memcpy(g_tSKServersShm->caTFOrderQNum, "0", sizeof(g_tSKServersShm->caTFOrderQNum));
            memcpy(g_tSKServersShm->caTFAlterQNum, "0", sizeof(g_tSKServersShm->caTFAlterQNum));
            memcpy(g_tSKServersShm->caOFOrderQNum, "0", sizeof(g_tSKServersShm->caOFOrderQNum));
            memcpy(g_tSKServersShm->caFTSOrderQNum, "0", sizeof(g_tSKServersShm->caFTSOrderQNum));
            memcpy(g_tSKServersShm->caFTSAlterQNum, "0", sizeof(g_tSKServersShm->caFTSAlterQNum));
            memcpy(g_tSKServersShm->caOSOrderQNum, "0", sizeof(g_tSKServersShm->caOSOrderQNum));
            memcpy(g_tSKServersShm->caOSBOrderQNum, "0", sizeof(g_tSKServersShm->caOSBOrderQNum));
            memcpy(g_tSKServersShm->caPreTSAlterQNum, "0", sizeof(g_tSKServersShm->caPreTSAlterQNum));
            memcpy(g_tSKServersShm->caPreTFAlterQNum, "0", sizeof(g_tSKServersShm->caPreTFAlterQNum));
            memcpy(g_tSKServersShm->caPreOFOrderQNum, "0", sizeof(g_tSKServersShm->caPreOFOrderQNum));
            memcpy(g_tSKServersShm->caPreOSOrderQNum, "0", sizeof(g_tSKServersShm->caPreOSOrderQNum));
            break;
        }
        else
        {
            sleep(1);
        }
    }
    delete g_pServers;

    memcpy(g_tSKServersShm->caServerRun, "N", 1);

    memcpy(g_tSKServersShm->caConnCounts, "0", sizeof(g_tSKServersShm->caConnCounts));
    
    // 關閉 Shmfd, 其他資源自行釋放

    close(nServersShmfd);
    close(nClientsShmfd);

    //shm_unlink(strServersShmName.c_str());
    //shm_unlink(strClientsShmName.c_str());

    return 0;
}

void ReadServersConfigFile(string& strConfigFileName, struct TSKServersConfig& tskConfig, string& strListenPort)
{
    GKeyFile* keyfile;
    GKeyFileFlags flags;
    GError* error = NULL;

    keyfile = g_key_file_new();
    flags = GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);

    assert(g_key_file_load_from_file(keyfile, strConfigFileName.c_str(), flags, &error));

    //Other
    if (g_key_file_get_string(keyfile, "ThreadCount", "GWConnThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini GWConnThreadCount NULL");
        exit(1);
    }
    tskConfig.nGWConnThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "GWConnThreadCount", NULL);
    if (tskConfig.nGWConnThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini GWConnThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "LoggerThreadCountLC", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini LoggerThreadCountLC NULL");
        exit(1);
    }
    tskConfig.nLoggerThreadCountLC = g_key_file_get_integer(keyfile, "ThreadCount", "LoggerThreadCountLC", NULL);
    if (tskConfig.nLoggerThreadCountLC == 0)
    {
        FprintfStderrLog("SKServers.ini LoggerThreadCountLC error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "LoggerThreadCountGW", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini LoggerThreadCountGW NULL");
        exit(1);
    }
    tskConfig.nLoggerThreadCountGW = g_key_file_get_integer(keyfile, "ThreadCount", "LoggerThreadCountGW", NULL);
    if (tskConfig.nLoggerThreadCountGW == 0)
    {
        FprintfStderrLog("SKServers.ini LoggerThreadCountGW error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "LoginThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini LoginThreadCount NULL");
        exit(1);
    }
    tskConfig.nLoginThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "LoginThreadCount", NULL);
    if (tskConfig.nLoginThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini LoginThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "SessionThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini SessionThreadCount NULL");
        exit(1);
    }
    tskConfig.nSessionThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "SessionThreadCount", NULL);
    if (tskConfig.nSessionThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini SessionThreadCount error");
        exit(1);
    }
    //TSOrder
    if (g_key_file_get_string(keyfile, "ThreadCount", "TSOrderMinThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TSOrderMinThreadCount NULL");
        exit(1);
    }
    tskConfig.nTSOrderMinThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "TSOrderMinThreadCount", NULL);
    if (tskConfig.nTSOrderMinThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini TSOrderMinThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "TSOrderMaxThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TSOrderMaxThreadCount NULL");
        exit(1);
    }
    tskConfig.nTSOrderMaxThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "TSOrderMaxThreadCount", NULL);
    if (tskConfig.nTSOrderMaxThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini TSOrderMaxThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "TSOrderIdleLessToNewThread", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TSOrderIdleLessToNewThread NULL");
        exit(1);
    }
    tskConfig.nTSOrderIdleLessToNewThread = g_key_file_get_integer(keyfile, "ThreadCount", "TSOrderIdleLessToNewThread", NULL);
    if (tskConfig.nTSOrderIdleLessToNewThread == 0)
    {
        FprintfStderrLog("SKServers.ini TSOrderIdleLessToNewThread error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "TSOrderThreadSleepUS", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TSOrderThreadSleepUS NULL");
        exit(1);
    }
    tskConfig.nTSOrderThreadSleepUS = g_key_file_get_integer(keyfile, "ThreadCount", "TSOrderThreadSleepUS", NULL);
    if (tskConfig.nTSOrderThreadSleepUS == 0)
    {
        FprintfStderrLog("SKServers.ini TSOrderThreadSleepUS error");
        exit(1);
    }
    //TSAlter
    if (g_key_file_get_string(keyfile, "ThreadCount", "TSAlterMinThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TSAlterMinThreadCount NULL");
        exit(1);
    }
    tskConfig.nTSAlterMinThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "TSAlterMinThreadCount", NULL);
    if (tskConfig.nTSAlterMinThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini TSAlterMinThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "TSAlterMaxThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TSAlterMaxThreadCount NULL");
        exit(1);
    }
    tskConfig.nTSAlterMaxThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "TSAlterMaxThreadCount", NULL);
    if (tskConfig.nTSAlterMaxThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini TSAlterMaxThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "TSAlterIdleLessToNewThread", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TSAlterIdleLessToNewThread NULL");
        exit(1);
    }
    tskConfig.nTSAlterIdleLessToNewThread = g_key_file_get_integer(keyfile, "ThreadCount", "TSAlterIdleLessToNewThread", NULL);
    if (tskConfig.nTSAlterIdleLessToNewThread == 0)
    {
        FprintfStderrLog("SKServers.ini TSAlterIdleLessToNewThread error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "TSAlterThreadSleepUS", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TSAlterThreadSleepUS NULL");
        exit(1);
    }
    tskConfig.nTSAlterThreadSleepUS = g_key_file_get_integer(keyfile, "ThreadCount", "TSAlterThreadSleepUS", NULL);
    if (tskConfig.nTSAlterThreadSleepUS == 0)
    {
        FprintfStderrLog("SKServers.ini TSAlterThreadSleepUS error");
        exit(1);
    }
    //TFOrder
    if (g_key_file_get_string(keyfile, "ThreadCount", "TFOrderMinThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TFOrderMinThreadCount NULL");
        exit(1);
    }
    tskConfig.nTFOrderMinThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "TFOrderMinThreadCount", NULL);
    if (tskConfig.nTFOrderMinThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini TFOrderMinThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "TFOrderMaxThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TFOrderMaxThreadCount NULL");
        exit(1);
    }
    tskConfig.nTFOrderMaxThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "TFOrderMaxThreadCount", NULL);
    if (tskConfig.nTFOrderMaxThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini TFOrderMaxThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "TFOrderIdleLessToNewThread", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TFOrderIdleLessToNewThread NULL");
        exit(1);
    }
    tskConfig.nTFOrderIdleLessToNewThread = g_key_file_get_integer(keyfile, "ThreadCount", "TFOrderIdleLessToNewThread", NULL);
    if (tskConfig.nTFOrderIdleLessToNewThread == 0)
    {
        FprintfStderrLog("SKServers.ini TFOrderIdleLessToNewThread error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "TFOrderThreadSleepUS", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TFOrderThreadSleepUS NULL");
        exit(1);
    }
    tskConfig.nTFOrderThreadSleepUS = g_key_file_get_integer(keyfile, "ThreadCount", "TFOrderThreadSleepUS", NULL);
    if (tskConfig.nTFOrderThreadSleepUS == 0)
    {
        FprintfStderrLog("SKServers.ini TFOrderThreadSleepUS error");
        exit(1);
    }
    //TFAlter
    if (g_key_file_get_string(keyfile, "ThreadCount", "TFAlterMinThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TFAlterMinThreadCount NULL");
        exit(1);
    }
    tskConfig.nTFAlterMinThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "TFAlterMinThreadCount", NULL);
    if (tskConfig.nTFAlterMinThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini TFAlterMinThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "TFAlterMaxThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TFAlterMaxThreadCount NULL");
        exit(1);
    }
    tskConfig.nTFAlterMaxThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "TFAlterMaxThreadCount", NULL);
    if (tskConfig.nTFAlterMaxThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini TFAlterMaxThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "TFAlterIdleLessToNewThread", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TFAlterIdleLessToNewThread NULL");
        exit(1);
    }
    tskConfig.nTFAlterIdleLessToNewThread = g_key_file_get_integer(keyfile, "ThreadCount", "TFAlterIdleLessToNewThread", NULL);
    if (tskConfig.nTFAlterIdleLessToNewThread == 0)
    {
        FprintfStderrLog("SKServers.ini TFAlterIdleLessToNewThread error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "TFAlterThreadSleepUS", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TFAlterThreadSleepUS NULL");
        exit(1);
    }
    tskConfig.nTFAlterThreadSleepUS = g_key_file_get_integer(keyfile, "ThreadCount", "TFAlterThreadSleepUS", NULL);
    if (tskConfig.nTFAlterThreadSleepUS == 0)
    {
        FprintfStderrLog("SKServers.ini TFAlterThreadSleepUS error");
        exit(1);
    }
    //OFOrder
    if (g_key_file_get_string(keyfile, "ThreadCount", "OFOrderMinThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OFOrderMinThreadCount NULL");
        exit(1);
    }
    tskConfig.nOFOrderMinThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "OFOrderMinThreadCount", NULL);
    if (tskConfig.nOFOrderMinThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini OFOrderMinThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "OFOrderMaxThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OFOrderMaxThreadCount NULL");
        exit(1);
    }
    tskConfig.nOFOrderMaxThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "OFOrderMaxThreadCount", NULL);
    if (tskConfig.nOFOrderMaxThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini OFOrderMaxThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "OFOrderIdleLessToNewThread", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OFOrderIdleLessToNewThread NULL");
        exit(1);
    }
    tskConfig.nOFOrderIdleLessToNewThread = g_key_file_get_integer(keyfile, "ThreadCount", "OFOrderIdleLessToNewThread", NULL);
    if (tskConfig.nOFOrderIdleLessToNewThread == 0)
    {
        FprintfStderrLog("SKServers.ini OFOrderIdleLessToNewThread error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "OFOrderThreadSleepUS", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OFOrderThreadSleepUS NULL");
        exit(1);
    }
    tskConfig.nOFOrderThreadSleepUS = g_key_file_get_integer(keyfile, "ThreadCount", "OFOrderThreadSleepUS", NULL);
    if (tskConfig.nOFOrderThreadSleepUS == 0)
    {
        FprintfStderrLog("SKServers.ini OFOrderThreadSleepUS error");
        exit(1);
    }
    //OSOrder
    if (g_key_file_get_string(keyfile, "ThreadCount", "OSOrderMinThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OSOrderMinThreadCount NULL");
        exit(1);
    }
    tskConfig.nOSOrderMinThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "OSOrderMinThreadCount", NULL);
    if (tskConfig.nOSOrderMinThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini OSOrderMinThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "OSOrderMaxThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OSOrderMaxThreadCount NULL");
        exit(1);
    }
    tskConfig.nOSOrderMaxThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "OSOrderMaxThreadCount", NULL);
    if (tskConfig.nOSOrderMaxThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini OSOrderMaxThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "OSOrderIdleLessToNewThread", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OSOrderIdleLessToNewThread NULL");
        exit(1);
    }
    tskConfig.nOSOrderIdleLessToNewThread = g_key_file_get_integer(keyfile, "ThreadCount", "OSOrderIdleLessToNewThread", NULL);
    if (tskConfig.nOSOrderIdleLessToNewThread == 0)
    {
        FprintfStderrLog("SKServers.ini OSOrderIdleLessToNewThread error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "OSOrderThreadSleepUS", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OSOrderThreadSleepUS NULL");
        exit(1);
    }
    tskConfig.nOSOrderThreadSleepUS = g_key_file_get_integer(keyfile, "ThreadCount", "OSOrderThreadSleepUS", NULL);
    if (tskConfig.nOSOrderThreadSleepUS == 0)
    {
        FprintfStderrLog("SKServers.ini OSOrderThreadSleepUS error");
        exit(1);
    }
    //OSBOrder
    if (g_key_file_get_string(keyfile, "ThreadCount", "OSBOrderMinThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OSBOrderMinThreadCount NULL");
        exit(1);
    }
    tskConfig.nOSBOrderMinThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "OSBOrderMinThreadCount", NULL);
    if (tskConfig.nOSBOrderMinThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini OSBOrderMinThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "OSBOrderMaxThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OSBOrderMaxThreadCount NULL");
        exit(1);
    }
    tskConfig.nOSBOrderMaxThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "OSBOrderMaxThreadCount", NULL);
    if (tskConfig.nOSBOrderMaxThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini OSBOrderMaxThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "OSBOrderIdleLessToNewThread", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OSBOrderIdleLessToNewThread NULL");
        exit(1);
    }
    tskConfig.nOSBOrderIdleLessToNewThread = g_key_file_get_integer(keyfile, "ThreadCount", "OSBOrderIdleLessToNewThread", NULL);
    if (tskConfig.nOSBOrderIdleLessToNewThread == 0)
    {
        FprintfStderrLog("SKServers.ini OSBOrderIdleLessToNewThread error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "OSBOrderThreadSleepUS", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OSBOrderThreadSleepUS NULL");
        exit(1);
    }
    tskConfig.nOSBOrderThreadSleepUS = g_key_file_get_integer(keyfile, "ThreadCount", "OSBOrderThreadSleepUS", NULL);
    if (tskConfig.nOSBOrderThreadSleepUS == 0)
    {
        FprintfStderrLog("SKServers.ini OSBOrderThreadSleepUS error");
        exit(1);
    }
     //PreTSAlter
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreTSAlterMinThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreTSAlterMinThreadCount NULL");
        exit(1);
    }
    tskConfig.nPreTSAlterMinThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "PreTSAlterMinThreadCount", NULL);
    if (tskConfig.nPreTSAlterMinThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini PreTSAlterMinThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreTSAlterMaxThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreTSAlterMaxThreadCount NULL");
        exit(1);
    }
    tskConfig.nPreTSAlterMaxThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "PreTSAlterMaxThreadCount", NULL);
    if (tskConfig.nPreTSAlterMaxThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini PreTSAlterMaxThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreTSAlterIdleLessToNewThread", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreTSAlterIdleLessToNewThread NULL");
        exit(1);
    }
    tskConfig.nPreTSAlterIdleLessToNewThread = g_key_file_get_integer(keyfile, "ThreadCount", "PreTSAlterIdleLessToNewThread", NULL);
    if (tskConfig.nPreTSAlterIdleLessToNewThread == 0)
    {
        FprintfStderrLog("SKServers.ini PreTSAlterIdleLessToNewThread error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreTSAlterThreadSleepUS", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreTSAlterThreadSleepUS NULL");
        exit(1);
    }
    tskConfig.nPreTSAlterThreadSleepUS = g_key_file_get_integer(keyfile, "ThreadCount", "PreTSAlterThreadSleepUS", NULL);
    if (tskConfig.nPreTSAlterThreadSleepUS == 0)
    {
        FprintfStderrLog("SKServers.ini PreTSAlterThreadSleepUS error");
        exit(1);
    }
    //PreTFAlter
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreTFAlterMinThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreTFAlterMinThreadCount NULL");
        exit(1);
    }
    tskConfig.nPreTFAlterMinThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "PreTFAlterMinThreadCount", NULL);
    if (tskConfig.nPreTFAlterMinThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini PreTFAlterMinThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreTFAlterMaxThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreTFAlterMaxThreadCount NULL");
        exit(1);
    }
    tskConfig.nPreTFAlterMaxThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "PreTFAlterMaxThreadCount", NULL);
    if (tskConfig.nPreTFAlterMaxThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini PreTFAlterMaxThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreTFAlterIdleLessToNewThread", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreTFAlterIdleLessToNewThread NULL");
        exit(1);
    }
    tskConfig.nPreTFAlterIdleLessToNewThread = g_key_file_get_integer(keyfile, "ThreadCount", "PreTFAlterIdleLessToNewThread", NULL);
    if (tskConfig.nPreTFAlterIdleLessToNewThread == 0)
    {
        FprintfStderrLog("SKServers.ini PreTFAlterIdleLessToNewThread error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreTFAlterThreadSleepUS", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreTFAlterThreadSleepUS NULL");
        exit(1);
    }
    tskConfig.nPreTFAlterThreadSleepUS = g_key_file_get_integer(keyfile, "ThreadCount", "PreTFAlterThreadSleepUS", NULL);
    if (tskConfig.nPreTFAlterThreadSleepUS == 0)
    {
        FprintfStderrLog("SKServers.ini PreTFAlterThreadSleepUS error");
        exit(1);
    }
    //PreOFOrder
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreOFOrderMinThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreOFOrderMinThreadCount NULL");
        exit(1);
    }
    tskConfig.nPreOFOrderMinThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "PreOFOrderMinThreadCount", NULL);
    if (tskConfig.nPreOFOrderMinThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini PreOFOrderMinThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreOFOrderMaxThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreOFOrderMaxThreadCount NULL");
        exit(1);
    }
    tskConfig.nPreOFOrderMaxThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "PreOFOrderMaxThreadCount", NULL);
    if (tskConfig.nPreOFOrderMaxThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini PreOFOrderMaxThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreOFOrderIdleLessToNewThread", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreOFOrderIdleLessToNewThread NULL");
        exit(1);
    }
    tskConfig.nPreOFOrderIdleLessToNewThread = g_key_file_get_integer(keyfile, "ThreadCount", "PreOFOrderIdleLessToNewThread", NULL);
    if (tskConfig.nPreOFOrderIdleLessToNewThread == 0)
    {
        FprintfStderrLog("SKServers.ini PreOFOrderIdleLessToNewThread error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreOFOrderThreadSleepUS", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreOFOrderThreadSleepUS NULL");
        exit(1);
    }
    tskConfig.nPreOFOrderThreadSleepUS = g_key_file_get_integer(keyfile, "ThreadCount", "PreOFOrderThreadSleepUS", NULL);
    if (tskConfig.nPreOFOrderThreadSleepUS == 0)
    {
        FprintfStderrLog("SKServers.ini PreOFOrderThreadSleepUS error");
        exit(1);
    }
    //PreOSOrder
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreOSOrderMinThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreOSOrderMinThreadCount NULL");
        exit(1);
    }
    tskConfig.nPreOSOrderMinThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "PreOSOrderMinThreadCount", NULL);
    if (tskConfig.nPreOSOrderMinThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini PreOSOrderMinThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreOSOrderMaxThreadCount", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreOSOrderMaxThreadCount NULL");
        exit(1);
    }
    tskConfig.nPreOSOrderMaxThreadCount = g_key_file_get_integer(keyfile, "ThreadCount", "PreOSOrderMaxThreadCount", NULL);
    if (tskConfig.nPreOSOrderMaxThreadCount == 0)
    {
        FprintfStderrLog("SKServers.ini PreOSOrderMaxThreadCount error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreOSOrderIdleLessToNewThread", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreOSOrderIdleLessToNewThread NULL");
        exit(1);
    }
    tskConfig.nPreOSOrderIdleLessToNewThread = g_key_file_get_integer(keyfile, "ThreadCount", "PreOSOrderIdleLessToNewThread", NULL);
    if (tskConfig.nPreOSOrderIdleLessToNewThread == 0)
    {
        FprintfStderrLog("SKServers.ini PreOSOrderIdleLessToNewThread error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ThreadCount", "PreOSOrderThreadSleepUS", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreOSOrderThreadSleepUS NULL");
        exit(1);
    }
    tskConfig.nPreOSOrderThreadSleepUS = g_key_file_get_integer(keyfile, "ThreadCount", "PreOSOrderThreadSleepUS", NULL);
    if (tskConfig.nPreOSOrderThreadSleepUS == 0)
    {
        FprintfStderrLog("SKServers.ini PreOSOrderThreadSleepUS error");
        exit(1);
    }

    //URL
    if (g_key_file_get_string(keyfile, "GWServer", "LoggerURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini LoggerURL NULL");
        exit(1);
    }
    tskConfig.strLoggerURL = g_key_file_get_string(keyfile, "GWServer", "LoggerURL", NULL);
    if (strlen(tskConfig.strLoggerURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini LoggerURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "LoginURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini LoginURL NULL");
        exit(1);
    }
    tskConfig.strLoginURL = g_key_file_get_string(keyfile, "GWServer", "LoginURL", NULL);
    if (strlen(tskConfig.strLoginURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini LoginURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "LoginSalesURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini LoginSalesURL NULL");
        exit(1);
    }
    tskConfig.strLoginSalesURL = g_key_file_get_string(keyfile, "GWServer", "LoginSalesURL", NULL);
    if (strlen(tskConfig.strLoginSalesURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini LoginSalesURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "SessionURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini SessionURL NULL");
        exit(1);
    }
    tskConfig.strSessionURL = g_key_file_get_string(keyfile, "GWServer", "SessionURL", NULL);
    if (strlen(tskConfig.strSessionURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini SessionURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "TSOrderURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TSOrderURL NULL");
        exit(1);
    }
    tskConfig.strTSOrderURL = g_key_file_get_string(keyfile, "GWServer", "TSOrderURL", NULL);
    if (strlen(tskConfig.strTSOrderURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini TSOrderURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "TSAlterURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TSAlterURL NULL");
        exit(1);
    }
    tskConfig.strTSAlterURL = g_key_file_get_string(keyfile, "GWServer", "TSAlterURL", NULL);
    if (strlen(tskConfig.strTSAlterURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini TSAlterURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "TFOrderURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TFOrderURL NULL");
        exit(1);
    }
    tskConfig.strTFOrderURL = g_key_file_get_string(keyfile, "GWServer", "TFOrderURL", NULL);
    if (strlen(tskConfig.strTFOrderURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini TFOrderURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "TFAlterURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini TFAlterURL NULL");
        exit(1);
    }
    tskConfig.strTFAlterURL = g_key_file_get_string(keyfile, "GWServer", "TFAlterURL", NULL);
    if (strlen(tskConfig.strTFAlterURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini TFAlterURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "OFOrderURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OFOrderURL NULL");
        exit(1);
    }
    tskConfig.strOFOrderURL = g_key_file_get_string(keyfile, "GWServer", "OFOrderURL", NULL);
    if (strlen(tskConfig.strOFOrderURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini OFOrderURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "OSOrderURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OSOrderURL NULL");
        exit(1);
    }
    tskConfig.strOSOrderURL = g_key_file_get_string(keyfile, "GWServer", "OSOrderURL", NULL);
    if (strlen(tskConfig.strOSOrderURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini OSOrderURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "OSBOrderURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini OSBOrderURL NULL");
        exit(1);
    }
    tskConfig.strOSBOrderURL = g_key_file_get_string(keyfile, "GWServer", "OSBOrderURL", NULL);
    if (strlen(tskConfig.strOSBOrderURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini OSBOrderURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "ConnURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini ConnURL NULL");
        exit(1);
    }
    tskConfig.strConnURL = g_key_file_get_string(keyfile, "GWServer", "ConnURL", NULL);
    if (strlen(tskConfig.strConnURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini ConnURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "PreTSAlterURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreTSAlterURL NULL");
        exit(1);
    }
    tskConfig.strPreTSAlterURL = g_key_file_get_string(keyfile, "GWServer", "PreTSAlterURL", NULL);
    if (strlen(tskConfig.strPreTSAlterURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini PreTSAlterURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "PreTFAlterURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreTFAlterURL NULL");
        exit(1);
    }
    tskConfig.strPreTFAlterURL = g_key_file_get_string(keyfile, "GWServer", "PreTFAlterURL", NULL);
    if (strlen(tskConfig.strPreTFAlterURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini PreTFAlterURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "PreOFOrderURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreOFOrderURL NULL");
        exit(1);
    }
    tskConfig.strPreOFOrderURL = g_key_file_get_string(keyfile, "GWServer", "PreOFOrderURL", NULL);
    if (strlen(tskConfig.strPreOFOrderURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini PreOFOrderURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "PreOSOrderURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini PreOSOrderURL NULL");
        exit(1);
    }
    tskConfig.strPreOSOrderURL = g_key_file_get_string(keyfile, "GWServer", "PreOSOrderURL", NULL);
    if (strlen(tskConfig.strPreOSOrderURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini PreOSOrderURL empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "GWServer", "ConnToZeroByIPURL", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini ConnToZeroByIPURL NULL");
        exit(1);
    }
    tskConfig.strConnToZeroByIPURL = g_key_file_get_string(keyfile, "GWServer", "ConnToZeroByIPURL", NULL);
    if (strlen(tskConfig.strConnToZeroByIPURL.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini ConnToZeroByIPURL empty");
        exit(1);
    }
    //Cert File Path
    if (g_key_file_get_string(keyfile, "ServerCert", "Key", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini Key NULL");
        exit(1);
    }
    g_strServerKey = g_key_file_get_string(keyfile, "ServerCert", "Key", NULL);
    if (strlen(g_strServerKey.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini Key empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "ServerCert", "Crt", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini Crt NULL");
        exit(1);
    }
    g_strServerCrt = g_key_file_get_string(keyfile, "ServerCert", "Crt", NULL);
    if (strlen(g_strServerCrt.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini Crt empty");
        exit(1);
    }
    //g_strServerChainCrt = g_key_file_get_string(keyfile, "ServerCert", "ChainCrt", NULL);
    //ListenPort
    if (g_key_file_get_string(keyfile, "PROXY", "ListenPort", NULL) == NULL)
    {
        FprintfStderrLog("SKServers.ini ListenPort NULL");
        exit(1);
    }
    strListenPort = g_key_file_get_string(keyfile, "PROXY", "ListenPort", NULL);
    if (strlen(strListenPort.c_str()) == 0)
    {
        FprintfStderrLog("SKServers.ini ListenPort empty");
        exit(1);
    }

    FILE* fp;
    char output[51];
    string strExpireD;
    string strCommand = "/bin/openssl x509 -enddate -noout -in " + g_strServerCrt;  //20231110
   // strCommand.append(" -dates | grep \"notAfter\"");

    fp = popen(strCommand.c_str(), "r");
    if (fp == NULL)
        exit(1);
    //printf("Cert Expire Day is ");
    while (fgets(output, 50, fp) != NULL)
    {
        strExpireD = output;
    }
    pclose(fp);

    int index = strExpireD.find("=")+1;
    strExpireD = strExpireD.substr(index);

    tskConfig.strCrtExpireDay = strExpireD;
    if (strlen(tskConfig.strCrtExpireDay.c_str()) == 0)
    {
        FprintfStderrLog("Crt Expire Day empty");
        exit(1);
    }
   
    FprintfStderrLog("--------------------Proxy Server Running Variable--------------------------");
    FprintfStderrLog("Cert Expire Day is " + tskConfig.strCrtExpireDay);

    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("GWConn thread count is " + to_string(tskConfig.nGWConnThreadCount));
    FprintfStderrLog("Logger Local thread count is " + to_string(tskConfig.nLoggerThreadCountLC));
    FprintfStderrLog("Logger GW thread count is " + to_string(tskConfig.nLoggerThreadCountGW));
    FprintfStderrLog("Login thread count is " + to_string(tskConfig.nLoginThreadCount));
    FprintfStderrLog("Session thread count is " + to_string(tskConfig.nSessionThreadCount));
    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("TSOrder min thread count is " + to_string(tskConfig.nTSOrderMinThreadCount));
    FprintfStderrLog("TSOrder max thread count is " + to_string(tskConfig.nTSOrderMaxThreadCount));
    FprintfStderrLog("TSOrder idle threads less than " + to_string(tskConfig.nTSOrderIdleLessToNewThread) + " will new threads");
    FprintfStderrLog("TSOrder thread sleep timer(us) " + to_string(tskConfig.nTSOrderThreadSleepUS));
    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("TSAlter min thread count is " + to_string(tskConfig.nTSAlterMinThreadCount));
    FprintfStderrLog("TSAlter max thread count is " + to_string(tskConfig.nTSAlterMaxThreadCount));
    FprintfStderrLog("TSAlter idle threads less than " + to_string(tskConfig.nTSAlterIdleLessToNewThread) + " will new threads");
    FprintfStderrLog("TSAlter thread sleep timer(us) " + to_string(tskConfig.nTSAlterThreadSleepUS));
    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("TFOrder min thread count is " + to_string(tskConfig.nTFOrderMinThreadCount));
    FprintfStderrLog("TFOrder max thread count is " + to_string(tskConfig.nTFOrderMaxThreadCount));
    FprintfStderrLog("TFOrder idle threads less than " + to_string(tskConfig.nTFOrderIdleLessToNewThread) + " will new threads");
    FprintfStderrLog("TFOrder thread sleep timer(us) " + to_string(tskConfig.nTFOrderThreadSleepUS));
    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("TFAlter min thread count is " + to_string(tskConfig.nTFAlterMinThreadCount));
    FprintfStderrLog("TFAlter max thread count is " + to_string(tskConfig.nTFAlterMaxThreadCount));
    FprintfStderrLog("TFAlter idle threads less than " + to_string(tskConfig.nTFAlterIdleLessToNewThread) + " will new threads");
    FprintfStderrLog("TFAlter thread sleep timer(us) " + to_string(tskConfig.nTFAlterThreadSleepUS));
    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("OFOrder min thread count is " + to_string(tskConfig.nOFOrderMinThreadCount));
    FprintfStderrLog("OFOrder max thread count is " + to_string(tskConfig.nOFOrderMaxThreadCount));
    FprintfStderrLog("OFOrder idle threads less than " + to_string(tskConfig.nOFOrderIdleLessToNewThread) + " will new threads");
    FprintfStderrLog("OFOrder thread sleep timer(us) " + to_string(tskConfig.nOFOrderThreadSleepUS));
    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("OSOrder min thread count is " + to_string(tskConfig.nOSOrderMinThreadCount));
    FprintfStderrLog("OSOrder max thread count is " + to_string(tskConfig.nOSOrderMaxThreadCount));
    FprintfStderrLog("OSOrder idle threads less than " + to_string(tskConfig.nOSOrderIdleLessToNewThread) + " will new threads");
    FprintfStderrLog("OSOrder thread sleep timer(us) " + to_string(tskConfig.nOSOrderThreadSleepUS));
    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("OSBOrder min thread count is " + to_string(tskConfig.nOSBOrderMinThreadCount));
    FprintfStderrLog("OSBOrder max thread count is " + to_string(tskConfig.nOSBOrderMaxThreadCount));
    FprintfStderrLog("OSBOrder idle threads less than " + to_string(tskConfig.nOSBOrderIdleLessToNewThread) + " will new threads");
    FprintfStderrLog("OSBOrder thread sleep timer(us) " + to_string(tskConfig.nOSBOrderThreadSleepUS));
    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("PreTSAlter min thread count is " + to_string(tskConfig.nPreTSAlterMinThreadCount));
    FprintfStderrLog("PreTSAlter max thread count is " + to_string(tskConfig.nPreTSAlterMaxThreadCount));
    FprintfStderrLog("PreTSAlter idle threads less than " + to_string(tskConfig.nPreTSAlterIdleLessToNewThread) + " will new threads");
    FprintfStderrLog("PreTSAlter thread sleep timer(us) " + to_string(tskConfig.nPreTSAlterThreadSleepUS));
    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("PreTFAlter min thread count is " + to_string(tskConfig.nPreTFAlterMinThreadCount));
    FprintfStderrLog("PreTFAlter max thread count is " + to_string(tskConfig.nPreTFAlterMaxThreadCount));
    FprintfStderrLog("PreTFAlter idle threads less than " + to_string(tskConfig.nPreTFAlterIdleLessToNewThread) + " will new threads");
    FprintfStderrLog("PreTFAlter thread sleep timer(us) " + to_string(tskConfig.nPreTFAlterThreadSleepUS));
    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("PreOFOrder min thread count is " + to_string(tskConfig.nPreOFOrderMinThreadCount));
    FprintfStderrLog("PreOFOrder max thread count is " + to_string(tskConfig.nPreOFOrderMaxThreadCount));
    FprintfStderrLog("PreOFOrder idle threads less than " + to_string(tskConfig.nPreOFOrderIdleLessToNewThread) + " will new threads");
    FprintfStderrLog("PreOFOrder thread sleep timer(us) " + to_string(tskConfig.nPreOFOrderThreadSleepUS));
    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("PreOSOrder min thread count is " + to_string(tskConfig.nPreOSOrderMinThreadCount));
    FprintfStderrLog("PreOSOrder max thread count is " + to_string(tskConfig.nPreOSOrderMaxThreadCount));
    FprintfStderrLog("PreOSOrder idle threads less than " + to_string(tskConfig.nPreOSOrderIdleLessToNewThread) + " will new threads");
    FprintfStderrLog("PreOSOrder thread sleep timer(us) " + to_string(tskConfig.nPreOSOrderThreadSleepUS));
    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("LoggerURL is " + tskConfig.strLoggerURL);
    FprintfStderrLog("LoginURL is " + tskConfig.strLoginURL);
    FprintfStderrLog("LoginSalesURL is " + tskConfig.strLoginSalesURL);
    FprintfStderrLog("SessionURL is " + tskConfig.strSessionURL);
    FprintfStderrLog("TSOrderURL is " + tskConfig.strTSOrderURL);
    FprintfStderrLog("TSAlterURL is " + tskConfig.strTSAlterURL);
    FprintfStderrLog("TFOrderURL is " + tskConfig.strTFOrderURL);
    FprintfStderrLog("TFAlterURL is " + tskConfig.strTFAlterURL);
    FprintfStderrLog("OFOrderURL is " + tskConfig.strOFOrderURL);
    FprintfStderrLog("OSOrderURL is " + tskConfig.strOSOrderURL);
    FprintfStderrLog("OSBOrderURL is " + tskConfig.strOSBOrderURL);
    FprintfStderrLog("PreTSAlterURL is " + tskConfig.strPreTSAlterURL);
    FprintfStderrLog("PreTFAlterURL is " + tskConfig.strPreTFAlterURL);
    FprintfStderrLog("PreOFOrderURL is " + tskConfig.strPreOFOrderURL);
    FprintfStderrLog("PreOSOrderURL is " + tskConfig.strPreOSOrderURL);
    FprintfStderrLog("ConnToZeroByIPURL is " + tskConfig.strConnToZeroByIPURL);
    FprintfStderrLog("ConnURL is " + tskConfig.strConnURL);
    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("ServerKey File Path " + g_strServerKey);
    FprintfStderrLog("ServerCrt File Path " + g_strServerCrt);
    //FprintfStderrLog("ServerChainCrt File Path " + g_strServerChainCrt);
    FprintfStderrLog("--------------------------------------------------");
    FprintfStderrLog("Listen Port is " + strListenPort);
    FprintfStderrLog("--------------------------------------------------");

}

void ReadShmConfigFile(string& strConfigFileName, struct TSKShmConfig& tskConfig)
{
    GKeyFile* keyfile;
    GKeyFileFlags flags;
    GError* error = NULL;

    keyfile = g_key_file_new();
    flags = GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);

    assert(g_key_file_load_from_file(keyfile, strConfigFileName.c_str(), flags, &error));

    if (g_key_file_get_string(keyfile, "SKServersShm", "LogMode", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini LogMode NULL");
        exit(1);
    }
    tskConfig.strLogMode = g_key_file_get_string(keyfile, "SKServersShm", "LogMode", NULL);
    if (strlen(tskConfig.strLogMode.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini LogMode empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "Intraday", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini Intraday NULL");
        exit(1);
    }
    tskConfig.strIntraday = g_key_file_get_string(keyfile, "SKServersShm", "Intraday", NULL);
    if (strlen(tskConfig.strIntraday.c_str()) == 0||(tskConfig.strIntraday!="1"&& tskConfig.strIntraday != "0"))
    {
        FprintfStderrLog("SKShm.ini Intraday ERROR");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "IntradayStart", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini IntradayStart NULL");
        exit(1);
    }
    tskConfig.strIntradayStart = g_key_file_get_string(keyfile, "SKServersShm", "IntradayStart", NULL);
    if (strlen(tskConfig.strIntradayStart.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini IntradayStart empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "IntradayEnd", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini IntradayEnd NULL");
        exit(1);
    }
    tskConfig.strIntradayEnd = g_key_file_get_string(keyfile, "SKServersShm", "IntradayEnd", NULL);
    if (strlen(tskConfig.strIntradayEnd.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini IntradayEnd empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "DailyReConnMin", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini DailyReConnMin NULL");
        exit(1);
    }
    tskConfig.strDailyReConnMin = g_key_file_get_string(keyfile, "SKServersShm", "DailyReConnMin", NULL);
    if (strlen(tskConfig.strDailyReConnMin.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini DailyReConnMin empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "DailyReConnHour", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini DailyReConnHour NULL");
        exit(1);
    }
    tskConfig.strDailyReConnHour = g_key_file_get_string(keyfile, "SKServersShm", "DailyReConnHour", NULL);
    if (strlen(tskConfig.strDailyReConnHour.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini DailyReConnHour empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "ReConn", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini ReConn NULL");
        exit(1);
    }
    tskConfig.strReConn = g_key_file_get_string(keyfile, "SKServersShm", "ReConn", NULL);
    if (strlen(tskConfig.strReConn.c_str()) == 0 || tskConfig.strReConn != "N")
    {
        FprintfStderrLog("SKShm.ini ReConn ERROR");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "HttpMode", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini HttpMode NULL");
        exit(1);
    }
    tskConfig.strHttpMode = g_key_file_get_string(keyfile, "SKServersShm", "HttpMode", NULL);
    if (strlen(tskConfig.strHttpMode.c_str()) == 0 || (tskConfig.strHttpMode!="0" && tskConfig.strHttpMode != "1"))
    {
        FprintfStderrLog("SKShm.ini HttpMode error");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "LoggerQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini LoggerQID NULL");
        exit(1);
    }
    tskConfig.strLoggerQID = g_key_file_get_string(keyfile, "SKServersShm", "LoggerQID", NULL);
    if (strlen(tskConfig.strLoggerQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini LoggerQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "LoginQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini LoginQID NULL");
        exit(1);
    }
    tskConfig.strLoginQID = g_key_file_get_string(keyfile, "SKServersShm", "LoginQID", NULL);
    if (strlen(tskConfig.strLoginQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini LoginQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "SessionQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini SessionQID NULL");
        exit(1);
    }
    tskConfig.strSessionQID = g_key_file_get_string(keyfile, "SKServersShm", "SessionQID", NULL);
    if (strlen(tskConfig.strSessionQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini SessionQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "TSOrderQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini TSOrderQID NULL");
        exit(1);
    }
    tskConfig.strTSOrderQID = g_key_file_get_string(keyfile, "SKServersShm", "TSOrderQID", NULL);
    if (strlen(tskConfig.strTSOrderQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini TSOrderQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "TSAlterQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini TSAlterQID NULL");
        exit(1);
    }
    tskConfig.strTSAlterQID = g_key_file_get_string(keyfile, "SKServersShm", "TSAlterQID", NULL);
    if (strlen(tskConfig.strTSAlterQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini TSAlterQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "TFOrderQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini TFOrderQID NULL");
        exit(1);
    }
    tskConfig.strTFOrderQID = g_key_file_get_string(keyfile, "SKServersShm", "TFOrderQID", NULL);
    if (strlen(tskConfig.strTFOrderQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini TFOrderQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "TFAlterQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini TFAlterQID NULL");
        exit(1);
    }
    tskConfig.strTFAlterQID = g_key_file_get_string(keyfile, "SKServersShm", "TFAlterQID", NULL);
    if (strlen(tskConfig.strTFAlterQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini TFAlterQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "OFOrderQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini OFOrderQID NULL");
        exit(1);
    }
    tskConfig.strOFOrderQID = g_key_file_get_string(keyfile, "SKServersShm", "OFOrderQID", NULL);
    if (strlen(tskConfig.strOFOrderQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini OFOrderQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "FTSOrderQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini FTSOrderQID NULL");
        exit(1);
    }
    tskConfig.strFTSOrderQID = g_key_file_get_string(keyfile, "SKServersShm", "FTSOrderQID", NULL);
    if (strlen(tskConfig.strFTSOrderQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini FTSOrderQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "FTSAlterQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini FTSAlterQID NULL");
        exit(1);
    }
    tskConfig.strFTSAlterQID = g_key_file_get_string(keyfile, "SKServersShm", "FTSAlterQID", NULL);
    if (strlen(tskConfig.strFTSAlterQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini FTSAlterQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "OSOrderQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini OSOrderQID NULL");
        exit(1);
    }
    tskConfig.strOSOrderQID = g_key_file_get_string(keyfile, "SKServersShm", "OSOrderQID", NULL);
    if (strlen(tskConfig.strOSOrderQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini OSOrderQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "OSBOrderQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini OSBOrderQID NULL");
        exit(1);
    }
    tskConfig.strOSBOrderQID = g_key_file_get_string(keyfile, "SKServersShm", "OSBOrderQID", NULL);
    if (strlen(tskConfig.strOSBOrderQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini OSBOrderQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "PreTSAlterQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini PreTSAlterQID NULL");
        exit(1);
    }
    tskConfig.strPreTSAlterQID = g_key_file_get_string(keyfile, "SKServersShm", "PreTSAlterQID", NULL);
    if (strlen(tskConfig.strPreTSAlterQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini PreTSAlterQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "PreTFAlterQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini PreTFAlterQID NULL");
        exit(1);
    }
    tskConfig.strPreTFAlterQID = g_key_file_get_string(keyfile, "SKServersShm", "PreTFAlterQID", NULL);
    if (strlen(tskConfig.strPreTFAlterQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini PreTFAlterQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "PreOFOrderQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini PreOFOrderQID NULL");
        exit(1);
    }
    tskConfig.strPreOFOrderQID = g_key_file_get_string(keyfile, "SKServersShm", "PreOFOrderQID", NULL);
    if (strlen(tskConfig.strPreOFOrderQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini PreOFOrderQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKServersShm", "PreOSOrderQID", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini PreOSOrderQID NULL");
        exit(1);
    }
    tskConfig.strPreOSOrderQID = g_key_file_get_string(keyfile, "SKServersShm", "PreOSOrderQID", NULL);
    if (strlen(tskConfig.strPreOSOrderQID.c_str()) == 0)
    {
        FprintfStderrLog("SKShm.ini PreOSOrderQID empty");
        exit(1);
    }
    if (g_key_file_get_string(keyfile, "SKClientsShm", "ClientShmSize", NULL) == NULL)
    {
        FprintfStderrLog("SKShm.ini ClientShmSize NULL");
        exit(1);
    }
    g_nClientShmSize = g_key_file_get_integer(keyfile, "SKClientsShm", "ClientShmSize", NULL);
    if (g_nClientShmSize == 0)
    {
        FprintfStderrLog("SKShm.ini ClientShmSize ERROR");
        exit(1);
    }
   

    FprintfStderrLog("--------------------------------------------------");
    //FprintfStderrLog("LogMode is " + tskConfig.strLogMode);
    FprintfStderrLog("Intraday is " + tskConfig.strIntraday);
    FprintfStderrLog("IntradayStart is " + tskConfig.strIntradayStart);
    FprintfStderrLog("IntradayEnd is " + tskConfig.strIntradayEnd);
    FprintfStderrLog("DailyReConnMin is " + tskConfig.strDailyReConnMin);
    FprintfStderrLog("DailyReConnHour is " + tskConfig.strDailyReConnHour);
    FprintfStderrLog("ReConn is " + tskConfig.strReConn);
    FprintfStderrLog("HttpMode is " + tskConfig.strHttpMode);
    FprintfStderrLog("LoggerQID is " + tskConfig.strLoggerQID);
    FprintfStderrLog("LoginQID is " + tskConfig.strLoginQID);
    FprintfStderrLog("SessionQID is " + tskConfig.strSessionQID);
    FprintfStderrLog("TSOrderQID is " + tskConfig.strTSOrderQID);
    FprintfStderrLog("TSAlterQID is " + tskConfig.strTSAlterQID);
    FprintfStderrLog("TFOrderQID is " + tskConfig.strTFOrderQID);
    FprintfStderrLog("TFAlterQID is " + tskConfig.strTFAlterQID);
    FprintfStderrLog("OFOrderQID is " + tskConfig.strOFOrderQID);
    FprintfStderrLog("FTSOrderQID is " + tskConfig.strFTSOrderQID);
    FprintfStderrLog("FTSAlterQID is " + tskConfig.strFTSAlterQID);
    FprintfStderrLog("OSOrderQID is " + tskConfig.strOSOrderQID);
    FprintfStderrLog("OSBOrderQID is " + tskConfig.strOSBOrderQID);
    FprintfStderrLog("PreTSAlterQID is " + tskConfig.strPreTSAlterQID);
    FprintfStderrLog("PreTFAlterQID is " + tskConfig.strPreTFAlterQID);
    FprintfStderrLog("PreOFOrderQID is " + tskConfig.strPreOFOrderQID);
    FprintfStderrLog("PreOSOrderQID is " + tskConfig.strPreOSOrderQID);
    FprintfStderrLog("ClientShm Size is " + to_string(g_nClientShmSize));
    FprintfStderrLog("--------------------------------------------------");
}

bool ShmTimer()
{
    // 斷線重連通知
    if (string(g_tSKServersShm->caReConn) == "Y")
    {
        g_pClients->ShutdownServer();
        g_pClients->ClientReConn(string(g_tSKServersShm->caReConnType)/*, string(g_tSKServersShm->caReConnIP), string(g_tSKServersShm->caReConnPort)*/);
        sleep(10);  //確保所有即將斷線通知都有發送給前端
        return false;
    }

    // 更改Log模式
    int nLogMode = Atoi(g_tSKServersShm->caLogMode, sizeof(g_tSKServersShm->caLogMode));
    if (nLogMode == 0 || nLogMode == 1)
    {
        g_nLogMode = nLogMode;
    }

    // 更新Shm狀態
    g_pServers->ShmTimer();
    g_pClients->ShmTimer();

    // 紀錄執行完成
    g_pServers->EnQueueLogger(e_ltServerInfo, "ShmTimer Success!", false);

    return true;
}