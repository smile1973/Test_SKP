#ifndef __SKFORMAT_H__
#define __SKFORMAT_H__

#include <memory>
#include <string>

using namespace std;

class CSKClient;

/// <summary>
/// Log型態列表
/// </summary>
enum TLogType
{
    e_ltClientInfo,
    e_ltClientData,
    e_ltClientError,
    e_ltServerInfo,
    e_ltServerData,
    e_ltServerError, 
    e_LogTypeEnd        //!< 放在最後一個, 判斷數量
};

/// <summary>
/// Queue功能列表
/// </summary>
enum TFuncType
{
    e_ftLogger,
    e_ftLogin,
    e_ftSession,
    e_ftTSOrder,
    e_ftTSAlter,
    e_ftTFOrder,
    e_ftTFAlter,
    e_ftOFOrder,
    e_ftFTSOrder,
    e_ftFTSAlter,
    e_ftOSOrder,
    e_ftOSBOrder,
    e_ftPreTSAlter,
    e_ftPreTFAlter,
    e_ftPreOFOrder,
    e_ftPreOSOrder,
    e_FuncTypeEnd       //!< 放在最後一個, 判斷數量
};

/// <summary>
/// Decrease Queue 儲存資料
/// </summary>
struct TSKDeConnQ
{
    string strUserID;
    int nSocketID;
};

/// <summary>
/// GW Log 電文
/// </summary>
struct TSKLoggerGW
{
    string strFunction;
    string strUserID;
    string strFunc;
    string strType;
    string strUserData;
    string strCode;
    string strMessage;
    string strORKEY;
    string strSeqNo;
    string strOrderNo;
    string strReplyData;
    string strUseTime;
    string strExceptionMsg;
    string strClientIP;
    string strServerIP;
    string strInfo;
    string strStartTime;
    string strEndTime;
    string strTotalTime;
};

/// <summary>
/// Logger Queue 儲存資料(Local)
/// </summary>
struct TSKLoggerLCQ
{
    TLogType tLogType;
    string strLogMsg;
};

/// <summary>
/// Logger Queue 儲存資料(GW)
/// </summary>
struct TSKLoggerGWQ
{
    TSKLoggerGW tSKLoggerGW;
};

/// <summary>
/// 雙因子電文
/// </summary>
struct TSKLogin
{
    char caIDNO[10];            //!<身分證字號
    char caFunc[1];             //!<功能類型 7:密碼驗證(雙因子)
    char caPawdType[1];         //!<密碼類型 0:網路密碼 1:語音密碼 2:網路出金密碼 3:語音出金密碼
    char caPassword[50];        //!<登入密碼(須將其中的的特殊字元轉為"%"+"16進位表示")
    char caCustID[10];          //!<一般用戶填: "" AP、APH母帳登入填任一個子帳的 CertID
    char caTranType[2];         //!<驗章來源
    char caCertType[1];         //!<憑證種類
    char caCN[30];              //!<憑證主旨
    char caCertID[20];          //!<憑證序號
    char caSign[2750];          //!<簽章值
    char caRawData[250];        //!<明文
    char caUUID[256];           //!<裝置UUID: 密碼驗證(雙因子)時, 與憑證資訊擇一帶入
    char caSource[2];           //!<來源別
    char caVersionNum[10];      //!<版號(未來會新增)
    char caReserved[256];       //!<備用長度
};

/// <summary>
/// Login Queue 儲存資料
/// </summary>
struct TSKLoginQ
{
    shared_ptr<CSKClient> shpClient;
    TSKLogin tSKData;
    struct timeval tvStart;
};

/// <summary>
/// SessionID驗證電文
/// </summary>
struct TSKSession
{
    char caIDNO[10];            //!<身分證字號
    char caTokenType[1];        //!<Token類型 1:驗線上開戶Token 2:密碼平台Token 3:EZON Token
    char caSessionID[100];      //!<SessionID
    char caSource[2];           //!<來源別
    char caVersionNum[10];      //!<版號(未來會新增)
    char caReserved[246];       //!<備用長度
};

/// <summary>
/// SessionID Queue 儲存資料
/// </summary>
struct TSKSessionQ
{
    shared_ptr<CSKClient> shpClient;
    TSKSession tSKData;
    struct timeval tvStart;
};

/// <summary>
/// 證券委託電文
/// </summary>
struct TSKTSOrder
{
    char caSessionID[100];      //!<SessionID(驗證用)
    char caORKEY[100];          //!<委託識別值
    char caLoginID[10];         //!<[一般客戶]登入身分證字號
    char caIDNO[10];            //!<[一般客戶]身分證字號
    char caBrokerID[4];         //!<分公司代碼 ex:證券 9183 規定帶4碼(如果帶2碼, GW會幫轉4碼)
    char caACNO[7];             //!<帳號 (不足7碼，前面補0至7碼)
    char caStockID[6];          //!<股號
    char caORDERType[1];        //!<下單類別 1:現股買進 2:現股賣出 3:融資買進 4:融資賣出 5:融券買進 6:融券賣出 8:無券賣出 D:刪單 C:改量 P:改價
    char caPriceType[1];        //!<價格類別 1:市價 2:限價
    char caTimeInForce[1];      //!<委託時效 0:ROD 1:IOC 2:FOK
    char caMarketType[1];       //!<盤別 0:盤中 1:鉅額 2:零股 3:盤後交易 4:拍賣 5:標借 6:標購(市/櫃) 8:標購(市/櫃) C:盤中零股 (VIP 僅限 0、2、3)
    char caQty[8];              //!<股數 (VIP客戶為 張數 或 股數) 一般客戶{改量:帶要減的量 改價:帶0 刪單:帶原始量}
    char caPrice[15];           //!<委託價格 (VIP 客戶為 6位整數，3位小數) 一般客戶{改量:帶0 改價:帶要改的價, ""=市價 刪單:帶原始價, 也可帶""}
    char caPriceMark[1];        //!<[一般客戶]價格旗標 ' ':input price(一般，定價) '0':close price(前日收盤價) '1':bull price(漲停) '2':bear price(跌停)
    char caOrderSeq[20];        //!<[改單]交易流水號(13碼)
    char caOrderNo[20];         //!<[改單]委託書號
    char caSource[2];           //!<來源別
    char caTranType[2];         //!<[一般客戶]驗章來源
    char caRawData[250];        //!<[一般客戶]明文
    char caSign[2750];          //!<[一般客戶]簽章值
    char caCN[30];              //!<[一般客戶]憑證主旨
    char caCertID[20];          //!<[一般客戶]憑證序號
    char caCertType[1];         //!<[一般客戶]憑證種類 台網:E 中華:C(目前不使用)
    char caDeviceID[50];        //!<裝置代碼
    char caSellerNo[6];
    char caNum[8];              //張數
    char caOrderBS[1];
    char caOrderPrice[15];
    char caReserved[226];       //!<備用長度
    //char caMarketSide[1];       //!<[VIP]市場別 T:上市 O:上櫃
    //char caCategory[1];         //!<[VIP]下單類別 0:新單 1:改量 2:刪單
    //char caSide[1];             //!<[VIP下單]買賣別 1:買 2:賣
    //char caTwseOrdType[1];      //!<[VIP下單]委託類別 普通、定價“0”一般(非融資融券)“1”融資(證金)“2”融券(證金)“3”融資(自辦)“4”融券(自辦)“5”借券賣出(券5)“6”借券賣出(券6) 零股“0”一般(非融資融券)
    //char caTwseRejStaleOrd[1];  //!<[VIP]檢查TransactTime是否逾時 “Y”檢查TransactTime是否逾時，“N”不檢查TransactTime
    //char caSellerNo[4];         //!<[VIP]營業員代號
    //char caClOrdID[13];         //!<[VIP]13碼流水號 因第一碼固定為零，取後12碼作為唯一編號
    //char caSubAccount[7];       //!<[VIP]子帳帳號
};

/// <summary>
/// 證券委託 Queue 儲存資料
/// </summary>
struct TSKTSOrderQ
{
    shared_ptr<CSKClient> shpClient;
    TSKTSOrder tSKOrder;
    struct timeval tvStart;
};

/// <summary>
/// 期貨委託電文
/// </summary>
struct TSKTFOrder
{
    char caSessionID[100];      //!<SessionID(驗證用)
    char caORKEY[100];          //!<委託識別值
    char caMarketType[2];       //!<市場別 TO=國內選、TF=國內期
    char caLoginID[10];         //!<登入身分證字號
    char caIDNO[10];            //!<身分證字號
    char caBrokerID[7];         //!<期貨商代號(F020000/F020002)
    char caIBNO[3];             //!<分公司代號(3碼) EX：B30
    char caACNO[7];             //!<帳號 (不足7碼，前面補0至7碼)
    char caPreOrder[1];         //!<預約單記號 空值:盤中單 T:正常盤(預約單) E:T+1盤預約(尚未開放)
    char caORDERType[1];        //!<下單類別 0:期貨自動 1:選擇權新倉 2:選擇權自動 3:選擇權平倉 4:選擇權保證金試算 5:選擇權詢價 B:期貨平倉 C:期貨新倉
                                //!<刪改單類別 D:刪單 C:改量 P:改價 (D/C要帶數量, P要帶價格)
    char caCOMID1[7];           //!<[下單Y] 商品1
    char caCP1[1];              //!<[選擇權下單Y] 買/賣權1(選擇權) C:買權(Call) P:賣權(Put)
    char caSettleYMD1[6];       //!<[下單Y] 商品年月1 yyyymm:202008
    char caStrike1[8];          //!<[選擇權下單Y] 履約價1
    char caBS1[1];              //!<[下單Y] 買賣別1 B:BUY S:SELL
    char caCOMID2[7];           //!<[選擇權複式單/期貨價差單Y] 商品2
    char caCP2[1];              //!<[選擇權複式單Y] 買/賣權2(選擇權) C:買權(Call) P:賣權(Put)
    char caSettleYMD2[6];       //!<[選擇權複式單/期貨價差單Y] 商品年月2 yyyymm:202008
    char caStrike2[8];          //!<[選擇權複式單Y] 履約價2
    char caBS2[1];              //!<[選擇權複式單/期貨價差單Y] 買賣別2 B:BUY S:SELL
    char caQty[8];              //!<[下單/刪單/改量Y] 數量{改量:帶要減的量 改價:帶0也可帶"" 刪單:帶原始量}
    char caPriceflag[1];        //!<[下單Y]委價類別 M:市價單 L:限價單
    char caPrice[15];           //!<[下單/改價Y]委託價格 {改量:帶0,也可以帶"" 改價:帶要改的價 刪單:帶原始價, 也可帶""}
    char caCOND[1];             //!<[下單/刪改單Y]委託條件 空值:ROD 1:FOK 2:IOC 3:報價單
    char caDayTrade[1];         //!<[下單]當沖記號 空白/Y
    char caTranType[2];         //!<驗章來源
    char caRawData[250];        //!<明文
    char caSign[2750];          //!<簽章值
    char caCN[30];              //!<憑證主旨
    char caCertID[20];          //!<憑證序號
    char caCertType[1];         //!<憑證種類 台網:E 中華:C(目前不使用)
    char caSource[2];           //!<來源別
    char caOrderNo[20];         //!<[改單]委託書號
    char caOrderSeq[20];        //!<[改單]交易流水號(預約單沒有委託書號, 用這個刪)
    char caDeviceID[50];        //!<裝置代碼
    char caSellerNo[6];
    char caOrderBS[1];
    char caOrderPrice[15];
    char caCOMMODITYID[15];
    char caReserved[219];       //!<備用長度
};

/// <summary>
/// 期貨委託 Queue 儲存資料
/// </summary>
struct TSKTFOrderQ
{
    shared_ptr<CSKClient> shpClient;
    TSKTFOrder tSKOrder;
    struct timeval tvStart;
};

/// <summary>
/// 海期委託電文
/// </summary>
struct TSKOFOrder
{
    char caSessionID[100];      //!<SessionID(驗證用)
    char caORKEY[100];          //!<委託識別值
    char caMarketType[2];       //!<市場別 OO=海選 OF=海期 刪改單=""
    char caLoginID[10];         //!<登入身分證字號
    char caIDNO[10];            //!<身分證字號
    char caBrokerID[7];         //!<期貨商代號(F020000/F020002)
    char caIBNO[3];             //!<分公司代號(3碼) EX：B30
    char caACNO[7];             //!<帳號 (不足7碼，前面補0至7碼)
    char caFunCode[1];          //!<功能 1-買 2-賣 4-取消 5-減量 7-改價
    char caOexchange[7];        //!<交易所代號 CME:芝加哥 TCE:東京
    char caOpenKind[1];         //!<新平倉碼 0:新倉 1:平倉 2:自動倉
    char caDayTrade[1];         //!<當沖記號 空白:不當沖 Y:當沖(只需新倉帶入, 改價減量刪單不須帶)
    char caOrderType[1];        //!<委託單類別
                                //!<1:市價單(MKT) 2:限價單(LMT) 3:停損單(STP) 4:停損限價單(STP LIMIT) 5:收市單(MOC) 6:WITH_OR_WITHOUT
                                //!<7:LIMIT_OR_BETTER 8:LIMIT_WITH_OR_WITHOUT 9:ON_BASIS A:ON_CLOSE B:LIMIT_ON_CLOSE C:FOREX_MARKET
                                //!<D:PREVIOUSLY_QUOTED E:PREVIOUSLY_INDICATED F:FOREX_LIMIT G:FOREX_SWAP H:FOREX_PREVIOUSLY_QUOTED I:FUNARI
                                //!<J:觸發價(MIT) K:MARKET_WITH_LEFTOVER_AS_LIMIT L:PREVIOUS_FUND_VALUATION_POINT M:NEXT_FUND_VALUATION_POINT P:PEGGED
    char caCOMID1[7];           //!<商品1 ES:迷你S&P AD:澳幣
    char caSettleYMD1[6];       //!<商品年月1 yyyymm:202008
    char caStrike1[8];          //!<[海選] 履約價1
    char caCP1[1];              //!<[海選] 買/賣權1(選擇權) C:買權(Call) P:賣權(Put)
    char caBS1[1];              //!<[下單Y] 買賣別1 B:BUY S:SELL
    char caCOMID2[7];           //!<[選擇權複式單/期貨價差單Y] 商品2
    char caSettleYMD2[6];       //!<[選擇權複式單/期貨價差單Y] 商品年月2 yyyymm:202008
    char caStrike2[8];          //!<[選擇權複式單Y] 履約價2
    char caCP2[1];              //!<[選擇權複式單Y] 買/賣權2(選擇權) C:買權(Call) P:賣權(Put)
    char caBS2[1];              //!<[選擇權複式單/期貨價差單Y] 買賣別2 B:BUY S:SELL
    char caCOND[1];             //!<[下單/刪改單Y]委託條件 0:ROD 1:FOK 2:IOC
    char caQty1[8];             //!<[下單/改量Y] 委託口數(改價請帶空)
    char caQty2[8];             //!<[下單/改量Y] 委託口數(改價請帶空)複式單使用欄位
    char caOrderNo[20];         //!<[刪改單]委託書號
    char caInputKey[20];        //!<[刪改單]交易流水號(非必要值)
    char caTranType[2];         //!<驗章來源
    char caRawData[250];        //!<明文
    char caSign[2750];          //!<簽章值
    char caCN[30];              //!<憑證主旨
    char caCertID[20];          //!<憑證序號
    char caCertType[1];         //!<憑證種類 台網:E 中華:C(目前不使用)
    char caSource[2];           //!<來源別
    char caOrderPrice[15];      //!<[下單/改價Y]委託價(差) 5位小數
    char caOrderPrice_M[15];    //!<委託價(差)分子 4位小數
    char caOrderPrice_D[10];    //!<委託價(差)分母
    char caTouchPrice[15];      //!<觸發價 5位小數
    char caTouchPrice_M[15];    //!<觸發價分子 4位小數
    char caTouchPrice_D[10];    //!<觸發價分母
    char caDeviceID[50];        //!<裝置代碼
    char caSellerNo[6];         //!<營業員編號
    char caFindPrice[15];
    char caFindPrice_M[15];
    char caFindPrice_D[10];
    char caReserved[210];       //!<備用長度
};

/// <summary>
/// 海期委託 Queue 儲存資料
/// </summary>
struct TSKOFOrderQ
{
    shared_ptr<CSKClient> shpClient;
    TSKOFOrder tSKOrder;
    struct timeval tvStart;
};

/// <summary>
/// 複委託&海外債 電文
/// </summary>
struct TSKOSOrder
{
    char caSessionID[100];      //!<SessionID(驗證用)
    char caORKEY[100];          //!<委託識別值
    char caMarketType[2];       //!<市場別 OS=複委託 刪改單=""
    char caLoginID[10];         //!<登入身分證字號
    char caIDNO[10];            //!<身分證字號
    char caBrokerID[4];         //!<券商代號(4碼) EX:9182
    char caACNO[7];             //!<帳號 (不足7碼，前面補0至7碼)
    char caAccount[1];          //!<[下單]專戶別 1:外幣專戶 2:臺幣專戶
    char caOrderType[2];        //!<交易別 01:買進 02:賣出 04:取消
    char caMarket[2];           //!<市場別 HK:港股 US:美股 JP:日股 SP:新加坡 SG:新(幣)坡股 SB:海外債
    char caStockID[10];         //!<[下單]商品代號
    char caTradeType[1];        //!<庫存別(賣出美股時必填) 1:[美股]一般/定股(CITI) 2:[美股]定額(VIEWTRADE) 3:[其它股市] 一般
    char caQty[15];             //!<[下單]股數 只有賣出美股且庫存別為定額(VIEWTRADE)時 股數才能有小數位數 其餘都必須為整數
    char caPrice[15];           //!<[下單]委託價格
    char caCurrency[15];        //!<[下單]幣別 第一個幣別必需為商品幣 HKD、NTD、USD、JPY、SGD、EUR、AUD、CNY、GBP
                                //!<最多可選3種幣別 多幣別時以逗點區隔 EX:HKD, NTD, USD
                                //!<幣別先後順序有意義 請注意!
    char caOrseq[20];           //!<[刪單]委託單序號
    char caOrderNo[20];         //!<[刪單]委託單書號 預約單委託書號帶"0000"
    char caTranType[2];         //!<驗章來源
    char caCertType[1];         //!<憑證種類 台網:E 中華:C(目前不使用)
    char caCN[30];              //!<憑證主旨
    char caCertID[20];          //!<憑證序號
    char caSign[2750];          //!<簽章值
    char caRawData[250];        //!<明文
    char caDeviceID[50];        //!<裝置代碼
    char caSource[2];           //!<來源別
    char caSellerNo[6];
    char caReserved[250];       //!<備用長度
};

/// <summary>
/// 複委託 Queue 儲存資料
/// </summary>
struct TSKOSOrderQ
{
    shared_ptr<CSKClient> shpClient;
    TSKOSOrder tSKOrder;
    struct timeval tvStart;
};

/// <summary> 
/// SKServers 參數檔格式
/// </summary>
struct TSKServersConfig
{
    //Other
    int nGWConnThreadCount;
    int nLoggerThreadCountLC;
    int nLoggerThreadCountGW;
    int nLoginThreadCount;
    int nSessionThreadCount;
    //TSOrder
    int nTSOrderMinThreadCount;
    int nTSOrderMaxThreadCount;
    int nTSOrderIdleLessToNewThread;
    int nTSOrderThreadSleepUS;
    //TSAlter
    int nTSAlterMinThreadCount;
    int nTSAlterMaxThreadCount;
    int nTSAlterIdleLessToNewThread;
    int nTSAlterThreadSleepUS;
    //TFOrder
    int nTFOrderMinThreadCount;
    int nTFOrderMaxThreadCount;
    int nTFOrderIdleLessToNewThread;
    int nTFOrderThreadSleepUS;
    //TFAlter
    int nTFAlterMinThreadCount;
    int nTFAlterMaxThreadCount;
    int nTFAlterIdleLessToNewThread;
    int nTFAlterThreadSleepUS;
    //OFOrder
    int nOFOrderMinThreadCount;
    int nOFOrderMaxThreadCount;
    int nOFOrderIdleLessToNewThread;
    int nOFOrderThreadSleepUS;
    //OSOrder
    int nOSOrderMinThreadCount;
    int nOSOrderMaxThreadCount;
    int nOSOrderIdleLessToNewThread;
    int nOSOrderThreadSleepUS;
    //OSBOrder
    int nOSBOrderMinThreadCount;
    int nOSBOrderMaxThreadCount;
    int nOSBOrderIdleLessToNewThread;
    int nOSBOrderThreadSleepUS;
    //PreTSAlter
    int nPreTSAlterMinThreadCount;
    int nPreTSAlterMaxThreadCount;
    int nPreTSAlterIdleLessToNewThread;
    int nPreTSAlterThreadSleepUS;
    //PreTFAlter
    int nPreTFAlterMinThreadCount;
    int nPreTFAlterMaxThreadCount;
    int nPreTFAlterIdleLessToNewThread;
    int nPreTFAlterThreadSleepUS;
    //PreOFOrder
    int nPreOFOrderMinThreadCount;
    int nPreOFOrderMaxThreadCount;
    int nPreOFOrderIdleLessToNewThread;
    int nPreOFOrderThreadSleepUS;
    //PreOSOrder
    int nPreOSOrderMinThreadCount;
    int nPreOSOrderMaxThreadCount;
    int nPreOSOrderIdleLessToNewThread;
    int nPreOSOrderThreadSleepUS;

    //URL
    string strLoggerURL;
    string strLoginURL;
    string strLoginSalesURL;
    string strSessionURL;
    string strTSOrderURL;
    string strTSAlterURL;
    string strTFOrderURL;
    string strTFAlterURL;
    string strOFOrderURL;
    string strOSOrderURL;
    string strOSBOrderURL;
    string strPreTSAlterURL;
    string strPreTFAlterURL;
    string strPreOFOrderURL;
    string strPreOSOrderURL;
    string strConnURL;
    string strConnToZeroByIPURL;
    string strCrtExpireDay;
};

/// <summary> 
/// SKShm 參數檔格式
/// </summary>
struct TSKShmConfig
{
    string strLogMode;
    string strIntraday;
    string strIntradayStart;
    string strIntradayEnd;
    string strDailyReConnMin;
    string strDailyReConnHour;
    string strReConn;
    string strHttpMode;
    string strLoggerQID;
    string strLoginQID;
    string strSessionQID;
    string strTSOrderQID;
    string strTSAlterQID;
    string strTFOrderQID;
    string strTFAlterQID;
    string strOFOrderQID;
    string strFTSOrderQID;
    string strFTSAlterQID;
    string strOSOrderQID;
    string strOSBOrderQID;
    string strPreTSAlterQID;
    string strPreTFAlterQID;
    string strPreOFOrderQID;
    string strPreOSOrderQID;
};

/// <summary> 
/// Servers shm 格式, 紀錄總體相關資訊, 每個欄位最後都有'\0'
/// </summary>
struct TSKServersShm
{
    char caLogMode[2];          //!< Log模式 0:Debug(含大量紀錄時間, 效能較差) 1:Release(僅記錄起訖時間, 效能較優)
    char caIntraday[2];         //!< 當前是否為可增加Thread模式 0:否 1:是(預設08:00~14:00可增加)
    char caIntradayStart[3];    //!< Intraday 開始時間(單位:小時) ex: 08
    char caIntradayEnd[3];      //!< Intraday 結束時間(單位:小時) ex: 14
    char caDailyReConnMin[3];   //!< 每日即將重啟通知時間(單位:分鐘) ex: 05 禁止更動
    char caDailyReConnHour[3];  //!< 每日即將重啟通知時間(單位:小時) ex: 06
    char caConnCounts[6];       //!< 當前客戶端總連線數
    char caReConn[2];           //!< 即將斷線重連 N:否 Y:是 (讀到Y時發送即將斷線通知給所有Client) [啟動時設N]
    char caReConnType[2];       //!< 即將斷線原因 1:每日斷線重連 2:Server切轉 (請先設定好, 再將caReConnect設成Y)
    char caHttpMode[2];
    char caSPACE0[2020];        //!< 總體資訊保留位
    char caLoggerQID[3];        //!< Logger   Queue ID: 0 禁止更動
    char caLoginQID[3];         //!< Login    Queue ID: 1 禁止更動
    char caSessionQID[3];       //!< Session  Queue ID: 2 禁止更動
    char caTSOrderQID[3];       //!< TSOrder  Queue ID: 3 禁止更動
    char caTSAlterQID[3];       //!< TSAlter  Queue ID: 4 禁止更動
    char caTFOrderQID[3];       //!< TFOrder  Queue ID: 5 禁止更動
    char caTFAlterQID[3];       //!< TFAlter  Queue ID: 6 禁止更動
    char caOFOrderQID[3];       //!< OFOrder  Queue ID: 7 禁止更動
    char caFTSOrderQID[3];      //!< FTSOrder Queue ID: 8 異常時可切轉至 3
    char caFTSAlterQID[3];      //!< FTSAlter Queue ID: 9 異常時可切轉至 4
    char caOSOrderQID[3];       //!< OSOrder  Queue ID: 10 禁止更動
    char caOSBOrderQID[3];      //!< OSBOrder Queue ID: 11 禁止更動
    char caPreTSAlterQID[3];    //!< PreTSAlter  Queue ID: 12 禁止更動
    char caPreTFAlterQID[3];    //!< PreTFAlter  Queue ID: 13 禁止更動
    char caPreOFOrderQID[3];    //!< PreOFOrder  Queue ID: 14 禁止更動
    char caPreOSOrderQID[3];    //!< PreOSOrder  Queue ID: 15 禁止更動
    char caSPACE1[252];         //!< QueueID 保留位
    char caLoggerQNum[6];       //!< Logger   Queue 當前數量
    char caLoginQNum[6];        //!< Login    Queue 當前數量
    char caSessionQNum[6];      //!< Session  Queue 當前數量
    char caTSOrderQNum[6];      //!< TSOrder  Queue 當前數量
    char caTSAlterQNum[6];      //!< TSAlter  Queue 當前數量
    char caTFOrderQNum[6];      //!< TFOrder  Queue 當前數量
    char caTFAlterQNum[6];      //!< TFAlter  Queue 當前數量
    char caOFOrderQNum[6];      //!< OFOrder  Queue 當前數量
    char caFTSOrderQNum[6];     //!< FTSOrder Queue 當前數量
    char caFTSAlterQNum[6];     //!< FTSAlter Queue 當前數量
    char caOSOrderQNum[6];      //!< OSOrder  Queue 當前數量
    char caOSBOrderQNum[6];     //!< OSBOrder Queue 當前數量
    char caPreTSAlterQNum[6];   //!< PreTSAlter  Queue 當前數量
    char caPreTFAlterQNum[6];   //!< PreTFAlter  Queue 當前數量
    char caPreOFOrderQNum[6];   //!< PreOFOrder  Queue 當前數量
    char caPreOSOrderQNum[6];   //!< PreOSOrder  Queue 當前數量
    char caSPACE2[504];         //!< Queue 當前數量保留位
    char caLoggerThNum[4];      //!< Logger   Thread 當前數量
    char caLoginThNum[4];       //!< Login    Thread 當前數量
    char caSessionThNum[4];     //!< Session  Thread 當前數量
    char caTSOrderThNum[4];     //!< TSOrder  Thread 當前數量
    char caTSAlterThNum[4];     //!< TSAlter  Thread 當前數量
    char caTFOrderThNum[4];     //!< TFOrder  Thread 當前數量
    char caTFAlterThNum[4];     //!< TFAlter  Thread 當前數量
    char caOFOrderThNum[4];     //!< OFOrder  Thread 當前數量
    char caFTSOrderThNum[4];    //!< FTSOrder Thread 當前數量
    char caFTSAlterThNum[4];    //!< FTSAlter Thread 當前數量
    char caOSOrderThNum[4];     //!< OSOrder  Thread 當前數量
    char caOSBOrderThNum[4];    //!< OSBOrder Thread 當前數量
    char caPreTSAlterThNum[4];  //!< PreTSAlter  Thread 當前數量
    char caPreTFAlterThNum[4];  //!< PreTFAlter  Thread 當前數量
    char caPreOFOrderThNum[4];  //!< PreOFOrder  Thread 當前數量
    char caPreOSOrderThNum[4];  //!< PreOSOrder  Thread 當前數量
    char caSPACE3[336];         //!< Thread 當前數量保留位
    char caCertExpireD[30];     //!< 憑證到期日
    char caServerRun[2];        //!< server是否在運行
    char caShmAPRun[2];         //!< skshmap是否在運行
    char caSPACE4[714];         //!< 其它保留位
};

/// <summary> 
/// Client shm 格式, 紀錄客戶相關資訊, 每個欄位最後都有'\0'
/// </summary>
struct TSKClientShm
{
    char caIndex[7];            //!< Shm Index
    char caLoginID[11];         //!< 客戶登入ID
    char caSPACE0[706];         //!< 客戶資訊保留位
    char caLoggerQID[3];        //!< Logger   Queue ID: 0 禁止更動
    char caLoginQID[3];         //!< Login    Queue ID: 1 禁止更動
    char caSessionQID[3];       //!< Session  Queue ID: 2 禁止更動
    char caTSOrderQID[3];       //!< TSOrder  Queue ID: 3 禁止更動
    char caTSAlterQID[3];       //!< TSAlter  Queue ID: 4 禁止更動
    char caTFOrderQID[3];       //!< TFOrder  Queue ID: 5 禁止更動
    char caTFAlterQID[3];       //!< TFAlter  Queue ID: 6 禁止更動
    char caOFOrderQID[3];       //!< OFOrder  Queue ID: 7 禁止更動
    char caFTSOrderQID[3];      //!< FTSOrder Queue ID: 8 異常時可切轉至 3
    char caFTSAlterQID[3];      //!< FTSAlter Queue ID: 9 異常時可切轉至 4
    char caOSOrderQID[3];       //!< OSOrder  Queue ID: 10 禁止更動
    char caOSBOrderQID[3];      //!< OSBOrder Queue ID: 11 禁止更動
    char caPreTSAlterQID[3];       //!< PreTSAlter  Queue ID: 12 禁止更動
    char caPreTFAlterQID[3];       //!< PreTFAlter  Queue ID: 13 禁止更動
    char caPreOFOrderQID[3];       //!< PreOFOrder  Queue ID: 14 禁止更動
    char caPreOSOrderQID[3];       //!< PreOSOrder  Queue ID: 15 禁止更動
    char caSPACE1[252];         //!< QueueID 保留位
};

/// <summary>
/// Client Reply Queue 儲存資料
/// </summary>
struct TSKClientReplyQ
{
    struct timeval tvStart; //收到此封包的時刻
    struct timeval tvSendT; //送出reply封包後的時刻
    TSKLoggerGW tSKLoggerGW;
    unsigned char uncaReplyBuf[1024];
    int nLength;
    string strFuncName;
};

struct  TSKReplyData
{
    struct timeval tvStart;
    struct timeval tvSendT;
    TSKLoggerGW tSKLoggerGW;
    unsigned char uncaReplyData[914]; //!< 回傳Client訊息
};

#endif
