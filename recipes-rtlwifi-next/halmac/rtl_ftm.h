/*
 * rtw_ftm.h
 *
 *  Created on: 26 jun 2017
 */

#ifndef INCLUDE_RTW_FTM_H_
#define INCLUDE_RTW_FTM_H_

#define GET_HAL_FTM(__pAdapter)     (&((__pAdapter)->hal_ftm))

#define SET_EXT_CAPABILITY_ELE_TIMING_MEAS(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE((_pEleStart)+2, 7, 1, _val)

#define SET_EXT_CAPABILITY_ELE_FTM_RESPONDER(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE((_pEleStart)+8, 6, 1, _val)
#define GET_EXT_CAPABILITY_ELE_FTM_RESPONDER(_pEleStart)				LE_BITS_TO_1BYTE((_pEleStart)+8, 6, 1)

#define SET_EXT_CAPABILITY_ELE_FTM_INITIATOR(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE((_pEleStart)+8, 7, 1, _val)
#define GET_EXT_CAPABILITY_ELE_FTM_INITIATOR(_pEleStart)				LE_BITS_TO_1BYTE((_pEleStart)+8, 7, 1)

struct halmac_adapter;

typedef struct _OCTET_STRING {
	uint8_t      *Octet;
	uint16_t      Length;
} OCTET_STRING, *POCTET_STRING;

typedef enum _FTM_REQUEST_STATE
{
  FTM_REQUEST_STATE_INIT = 0x0,
  FTM_REQUEST_STATE_SEARCH_CANDIDATE_LIST = 0x1,
  FTM_REQUEST_STATE_FLUSH_ALL_LISTS = 0x2,
  FTM_REQUEST_STATE_SCAN = 0x3,
  FTM_REQUEST_STATE_SCAN_COMPLETE = 0x4,
  FTM_REQUEST_STATE_DL_RSVD_PAGE = 0x5,
  FTM_REQUEST_STATE_SET_H2C_CMD = 0x6,
  FTM_REQUEST_STATE_WAITING_C2H_CMD = 0x7,
  FTM_REQUEST_STATE_GET_C2H_CMD = 0x8,
  FTM_REQUEST_STATE_START_FTM_SESSION = 0x9,
  FTM_REQUEST_STATE_WAIT_FTM_SESSION_END = 0xA,
  FTM_REQUEST_STATE_FTM_SESSION_END = 0xB,
  FTM_REQUEST_STATE_COMPLETE = 0xC,
  FTM_REQUEST_STATE_INIT_11K = 0xD,
  FTM_REQUEST_STATE_MAX = 0xE,
} FTM_REQUEST_STATE;

typedef enum _FTM_RESP_STATE
{
  FTM_RESP_STATE_NONE = 0x0,
  FTM_RESP_STATE_ON_INIT_FTM_REQ = 0x1,
  FTM_RESP_STATE_INIT_FTM_REQ_RECV = 0x2,
  FTM_RESP_STATE_INIT_FTM_SEND = 0x3,
  FTM_RESP_STATE_MESUREMENT_EXC_STARTED = 0x4,
  FTM_RESP_STATE_ON_FTM_REQ = 0x5,
  FTM_RESP_STATE_BURST_PERIOD_STARTED = 0x6,
  FTM_RESP_STATE_BURST_PERIOD_END = 0x7,
  FTM_RESP_STATE_MESUREMENT_EXC_END = 0x8,
  FTM_RESP_STATE_MAX = 0x9,
} FTM_RESP_STATE;

typedef enum _FTM_SESSION_STATE
{
  FTM_SESSION_STATE_INIT = 0x0,
  FTM_SESSION_STATE_NEGOTIATION = 0x1,
  FTM_SESSION_STATE_NEGOTIATION_WAIT_IFTM = 0x2,
  FTM_SESSION_STATE_NEGOTIATION_GET_IFTM = 0x3,
  FTM_SESSION_STATE_NEGOTIATION_WAIT_IFTM_TIMEOUT = 0x4,
  FTM_SESSION_STATE_MEASUREMENT = 0x5,
  FTM_SESSION_STATE_MEASUREMENT_DONE = 0x6,
  FTM_SESSION_STATE_MEASUREMENT_ASAP1 = 0x7,
  FTM_SESSION_STATE_MEASUREMENT_ASAP1_DONE = 0x8,
  FTM_SESSION_STATE_DONE = 0x9,
  FTM_SESSION_STATE_MAX = 0xA,
} FTM_SESSION_STATE;

typedef enum _FTM_ROLE
{
  FTM_NONE = 0x0,
  FTM_INITIATOR = 0x1,
  FTM_RESPONDER = 0x2,
} FTM_ROLE;

/* 1022 */
typedef enum _FTM_PARAM_FORMATBW
{
  FTM_PARAM_FORMATBW_HT_20 = 0x9,
  FTM_PARAM_FORMATBW_VHT_20 = 0xA,
  FTM_PARAM_FORMATBW_HT_40 = 0xB,
  FTM_PARAM_FORMATBW_VHT_40 = 0xC,
  FTM_PARAM_FORMATBW_VHT_80 = 0xD,
  FTM_PARAM_FORMATBW_MAX = 0xE,
} FTM_PARAM_FORMATBW;

typedef enum _FTM_REQ_BURST_STATE
{
  FTM_REQ_BURST_STATE_IDLE = 0x0,
  FTM_REQ_BURST_STATE_START = 0x1,
  FTM_REQ_BURST_STATE_RECV_FTM = 0x2,
  FTM_REQ_BURST_STATE_DONE = 0x3,
  FTM_REQ_BURST_STATE_MAX = 0x4,
} FTM_REQ_BURST_STATE;

/* 910 */
typedef enum _FTM_REQUEST_ID
{
  FTM_REQUEST_ID_UNINIT = 0x0,
  FTM_REQUEST_ID_SCAN_REQUEST = 0x1,
  FTM_REQUEST_ID_FLUSH_ALL_LIST = 0x2,
  FTM_REQUEST_ID_START_RESPONDER_ROLE = 0x3,
  FTM_REQUEST_ID_START_REQUESTER_ROLE_WITH_TARGET_LIST_BY_USER = 0x4,
  FTM_REQUEST_ID_START_REQUESTER_ROLE_WITHOUT_TARGET_LIST_BY_USER = 0x5,
  FTM_REQUEST_ID_START_REQUESTER_ROLE_WITH_TARGET_LIST_BY_RM_REQ = 0x6,
  FTM_REQUEST_ID_START_REQUESTER_ROLE_WITHOUT_TARGET_LIST_BY_RM_REQ = 0x7,
  FTM_CMD_MAX = 0x8,
} FTM_REQUEST_ID;

typedef enum _FTM_BURST_STATE
{
  FTM_BURST_STATE_START = 0x0,
  FTM_BURST_STATE_SEND_FTM = 0x1,
  FTM_BURST_STATE_TIMEOUT = 0x2,
  FTM_BURST_STATE_MAX = 0x3,
} FTM_BURST_STATE;

/* 945 */
typedef enum _ACTION_TYPE
{
  ACTION_TYPE_INVALID = 0x0,
  ACTION_TYPE_MULTICHANNEL_SWITCH = 0x1,
  ACTION_TYPE_CUSTOMIZED_SCAN = 0x2,
  ACTION_TYPE_P2P_POWERSAVE = 0x3,
  ACTION_TYPE_FTM = 0x4,
  ACTION_TYPE_GAS_REQ = 0x5,
  ACTION_TYPE_GAS_RSP = 0x6,
  ACTION_TYPE_NAN_5G_SYNC = 0x7,
  ACTION_TYPE_NAN_DW_START = 0x8,
  ACTION_TYPE_NAN_DW_END = 0x9,
  ACTION_TYPE_NAN_SDF_START = 0xA,
  ACTION_TYPE_ALL = 0xB,
  ACTION_TYPE_MAX = 0xC,
} ACTION_TYPE;
/* 3106 */
typedef struct _ACTION_TIMER_ITEM
{
  //_RT_LIST_ENTRY List;
  char ActionOwner;
  ACTION_TYPE ActionType;
  uint64_t usTimeout;
  void *pContext;
  void (*CallbackFunc)(struct _ACTION_TIMER_ITEM *const );
} ACTION_TIMER_ITEM;

typedef struct _RT_DOT11_IE
{
  char Id;
  OCTET_STRING Content;
} RT_DOT11_IE;

/* 2179 */
typedef struct _RT_FTM_RESP_PARAM
{
  unsigned int NextBurstStartTime;
  unsigned int BurstTimeout;
  uint16_t BurstPeriod;
  uint16_t BurstsNumber;
  uint16_t BurstCount;
  char MinDeltaFTM;
  char FTMsPerBurst;
  char FTMSendCountPerBurst;
  uint64_t CurBurstStartTsf;
  uint64_t NextBurstStartTsf;
  uint64_t BurstTimeoutTsf;
  uint64_t NextFTMSendTsf;
  unsigned int BurstState;
} RT_FTM_RESP_PARAM;

/* 2524 */
typedef struct _RT_FTM_SCHEDULE_PARA
{
  char StatusIndication;
  char BurstExpoNum;
  char BurstTimeout;
  uint16_t PartialTSFStartOffset;
  char MinDeltaFTM;
  char ASAPCapable;
  char ASAP;
  char FTMNumPerBust;
  FTM_PARAM_FORMATBW FTMFormatAndBW;
  uint16_t BurstPeriod;
  uint16_t PartialTSFdiscardBit;
  unsigned int TSFOffset;
} RT_FTM_SCHEDULE_PARA;

/* 2525 */
typedef struct _RT_FTM_REQ_STA
{
  char bUsed;
  char MacAddr[6];
  char DialogToken;
  char FollowUpDialogToken;
  char TOD_t1[6];
  char TOA_t4[6];
  uint16_t TODError;
  uint16_t TOAError;
  RT_FTM_SCHEDULE_PARA FTMParaInfo;
} RT_FTM_REQ_STA;

/* 2653 */
typedef struct _RT_FTM_BURST_ENTITY
{
  uint64_t CurBurstStartTsf;
  uint64_t CurBurstDurationTsf;
  uint64_t CurBurstPeriodTsf;
  uint16_t BurstCnt;
  unsigned int recvFTMnum;
  char lastDialogToken;
  FTM_REQ_BURST_STATE BurstState;
} RT_FTM_BURST_ENTITY;

typedef struct _RT_FTM_SELECT_RESP
{
  char bValid;
  char b11k;
  char bCIVICCap;
  char bLCICap;
  char bFTMdone;
  char MacId;
  char MacAddr[6];
  RT_FTM_SCHEDULE_PARA FTMParaInfo;
  char ChannelNum;
  //TODO: PHY bitmask type. Find proper usage.
  //WIRELESS_MODE wirelessmode;
  char Bandwidth;
  char ExtChnlOffsetOf40MHz;
  char ExtChnlOffsetOf80MHz;
  RT_FTM_SCHEDULE_PARA targetFTMParaInfo;
  char dialogToken;
  RT_FTM_SCHEDULE_PARA finalFTMParaInfo;
  RT_FTM_BURST_ENTITY burstEntity;
  OCTET_STRING iFTMRFrame;
  char iFTMRFrameBuf[256];
  OCTET_STRING FTMReqInfo;
  char FTMReqInfoBuf[32];
  u32  t4List[16];
  u32  t3List[16];
  u8   bcList[16];

} RT_FTM_SELECT_RESP;

/* 3260 */
typedef struct _RT_FTM_RANGE_RPT
{
  char RangeEntryCnt;
  char RangeEntry[150];
  OCTET_STRING osRangeEntry;
  char ErrorEntryCnt;
  char ErrorEntry[150];
  OCTET_STRING osErrorEntry;
} RT_FTM_RANGE_RPT;

/* 3492 */
typedef struct _RT_TARGET_RESP_LIST
{
  char bValid[3];
  char NumOfResponder;
  char MacAddr[4][6];
  char chnl[4];
  unsigned int timestamp[3];
  unsigned int range[3];
  unsigned int t4[3];
  unsigned int t3[3];
} RT_TARGET_RESP_LIST;

typedef struct _RT_TARGET_11K_RESP
{
	char bValid;
	char MacAddr[ETH_ALEN];
	char chnl;
	unsigned int timestamp;
	unsigned int range;
	unsigned int t4;
	unsigned int t3;
} RT_TARGET_11K_RESP;

/* 3627 */
typedef struct _RT_FTM_CAND_RESP
{
  char bValid;
  char bCIVICCap;
  char bLCICap;
  char bRMCap;
  char MacAddr[6];
  char MacId;
  char ChannelNum;
  //TODO: PHY bitmask type. Find proper usage.
  //WIRELESS_MODE wirelessmode;
  char Bandwidth;
  char ExtChnlOffsetOf40MHz;
  char ExtChnlOffsetOf80MHz;
  char TSFValueNow[8];
  unsigned int TSFOffset;
} RT_FTM_CAND_RESP;

/* 3628 */
typedef struct _RT_FTM_INFO
{
  spinlock_t lock;
  unsigned int preThreadStateFlag;
  void* FTMpreparingThread;
  struct	semaphore FTMpreparingEvent;
  struct	semaphore FTMpreparingTerminate;
  unsigned int sessionThreadStateFlag;
  void* FTMSessionThread;
  struct	semaphore FTMSessionEvent;
  struct	semaphore FTMSessionTerminate;
  struct timer_list FTMSessionTimer;
  uint64_t curMicroTime;
  unsigned int ftmTimeoutMs;
  char bSupportFTM;
  char bNeedLCI;
  char bNeedCIVICLocation;
  char bInitiator;
  char bResponder;
  char bRMEnalbe;
  FTM_ROLE Role;
  char bNeedToDLFTMpkts;
  char FTMReqFrameStartLoc;
  char FTMReqInfoStartLoc;
  char FTMFrameStartLoc;
  RT_FTM_CAND_RESP FTMCandicateRespList[64];
  RT_FTM_SELECT_RESP FTMSelectedRespList[3];
  RT_TARGET_11K_RESP FTM11kResp;
  RT_FTM_SELECT_RESP *pFTMActiveSelectedResp;
  char FTMCandidateRspNum;
  char FTMSelectRspNum;
  RT_TARGET_RESP_LIST FTMTargetList;
  char RespondingState;
  RT_FTM_REQ_STA FTMReqSTAEntry[1];
  RT_FTM_RESP_PARAM FTMRespondingParam[1];
  OCTET_STRING FTMFrame;
  char FTMFrameBuf[256];
  char bRespCapSupportASAP;
  char RespCapMinDeltaFTM;
  char FTMActionTimer;
  char FTMReqActionTimer;
  FTM_REQUEST_ID reqID;
  FTM_REQUEST_STATE reqState;
  FTM_SESSION_STATE sessionState;
  char bFTMSupportByFW;
  char bFTMParamFromFile;
  char ftmDataRate;
  char ftmBandWidth;
  char bIssueFTMScan;
  RT_FTM_RANGE_RPT ftmRangeRpt;
  char NeighborRptTargetAddr[6];
  char nrChnl;
  char nrScanTarget;
  char nrScanComplete;
  char bRepeatedMsntCapEnable;
  char bFTMRangeRptCapEnable;
  char bLCIMsntCapEnable;
  char bRMReqMsntType;
  char MaxAgeSub;
  char bFTMParaFromSigma;
  char bBurstExpoFromSigma;
  char bASAPFromSigma;
  char bFormatAndBWFromSigma;
  char bBurstTimeoutFromSigma;
  char bFTMNumPerBustFromSigma;
  char bAsk4LOCCivicFromSigma;
  char bAsk4LCIFromSigma;
  RT_FTM_SCHEDULE_PARA SigmaFTMParaInfo;
  char bRepeatedMsntCapSetFromSigma;
  char bFTMRangeRptCapSetFromSigma;
  char bLCIMsntCapSetFromSigma;
  char bRMReqMsntTypeSetFromSigma;
  char bMaxAgeSubSetFromSigma;
  unsigned hang;
  unsigned int mlmeScanInterval;
} RT_FTM_INFO;

/* Functions */
void rtw_ftm_init(struct halmac_adapter *padapter);
int rtw_ftm_run_threads(struct halmac_adapter *padapter);
int rtw_ftm_cancel_threads(struct halmac_adapter *padapter);
int rtw_ftm_preparing_thread(void* context);
int rtw_ftm_session_thread(void* context);

void ftm_send_ftm_frame(struct halmac_adapter *padapter, u8 is_initial_frame, u8 *pdev_raddr);
void construct_ftm_frame_packet(struct halmac_adapter *padapter, u8 is_initial_frame, struct xmit_frame *pmgntframe, u8 *pdev_raddr);
void construct_ftm_frame_buffer(struct halmac_adapter *padapter, u8 is_initial_frame, u8 *pbuffer, u32 *plength);
void ftm_send_ftm_request(struct halmac_adapter *padapter, u8 is_initial_frame, u8 *pdev_raddr);
void construct_ftm_req_packet(struct halmac_adapter *padapter, u8 is_initial_frame, struct xmit_frame *pmgntframe, u8 *pdev_raddr);
void construct_ftm_req_buffer(struct halmac_adapter *padapter, u8 is_initial_frame, u8 *pdev_raddr, RT_FTM_SCHEDULE_PARA *pftm_param_info, u8 *pbuffer, u32 *plength);

void ftm_process_range(struct halmac_adapter *adapter, int idx, RT_FTM_SELECT_RESP *sel_resp);
void ftm_set_EOME_status(struct halmac_adapter *padapter);
void ftm_set_timing_measure(struct halmac_adapter *padapter, u8 burst_num, u32 t1, u32 t2, u32 t3, u32 t4);

unsigned int rtw_process_public_act_ftm_req(struct halmac_adapter *padapter, u8 *pframe, uint frame_len);
unsigned int rtw_process_public_act_ftm_frame(struct halmac_adapter *padapter, u8 *pframe, uint frame_len);


int FTM_Request(struct halmac_adapter *padapter, u8 is_responder, u8 start, u8 resp_num, u8 *InformationBuffer, unsigned int InformationBufferLength);

void ftm_StartFTMRequestRoleByFW(struct halmac_adapter *pAdapter, char *pscanCnt);
void ftm_StartFTMRequestRoleByDriver(struct halmac_adapter *pAdapter, char *pscanCnt);
void FTM_FlushSelectResponderList(struct halmac_adapter *padapter);
void FTM_FlushTargetList(struct halmac_adapter *padapter);
void FTM_FlushAllList(struct halmac_adapter *padapter);
void FTM_StartRespondingProcess(struct halmac_adapter *padapter);
void FTM_AddCandidateResponderList(struct halmac_adapter *padapter, RT_FTM_CAND_RESP *pFTMCandicateResp);
void FTM_ConstructFTMReqInfo(struct halmac_adapter *padapter);
int FTM_OnFTMframe(struct halmac_adapter *pAdapter,  OCTET_STRING *posMpdu);
int FTM_OnFTMRequest(struct halmac_adapter *pAdapter, OCTET_STRING *posMpdu);
void FTM_ParsingFTMRequestOptionField(struct halmac_adapter *pAdapter, OCTET_STRING *posMpdu, unsigned int ElementStartOffset, char *bFTMParam, char *bLCIReq, char *bLocCivicReq);
void ftm_ParsingFTMOptionField(struct halmac_adapter *pAdapter, OCTET_STRING *posMpdu, unsigned int ElementStartOffset, char *bFTMParam, char *bLCIReq, char *bLocCivicReq);
int HasNextIE(OCTET_STRING *posMpdu, int Offset);
RT_DOT11_IE *AdvanceToNextIE(RT_DOT11_IE *Ie, OCTET_STRING *posMpdu, unsigned int *pOffset);
void FTM_SetRespBurstState(struct halmac_adapter *pAdapter, FTM_BURST_STATE BurstStateToSet, uint64_t Timeout);
int  ftm_Calc_TimeoutValue(struct halmac_adapter *pAdapter, OCTET_STRING *posMpdu, unsigned int ElementStartOffset, char *bFTMParam);
void FTM_MeasurementExchangeEnd(struct halmac_adapter *pAdapter);
void ftm_SetFixFTMParam(struct halmac_adapter *pAdapter);
void FTM_SelectBWandDataRate(FTM_PARAM_FORMATBW FTMFormatAndBW, char *bw, char *dr);
void ftm_DecideFTMFormatAndBWValue(struct halmac_adapter *pAdapter);
void FTM_BurstStart(struct halmac_adapter *pAdapter);
void ftm_StartFTMResponderRoleByFW(struct halmac_adapter *adapter);
void ftm_StartFTMRequestRoleByFW(struct halmac_adapter *pAdapter, char *pscanCnt);
void ftm_StartFTMResponderRoleByDriver(struct halmac_adapter *adapter);
void ftm_StartFTMRequestRoleByDriver(struct halmac_adapter *pAdapter, char *pscanCnt);

void ftm_set_peer(struct halmac_adapter *padapter, u8 idx, u8 *addr, u8 channel);
void ftm_get_peer(struct halmac_adapter *padapter, u8 idx, u8 *valid, u8 *addr, u8 *channel, u32 *ts, u32 *range, u32 *t3, u32 *t4, u32 *t);
int  ftm_start_requesting_process(struct halmac_adapter *padapter, unsigned int period);
int  ftm_start_11k_requesting_process(struct halmac_adapter *padapter, u8* addr, u8 ch, unsigned int period);
int  ftm_start_responding_process(struct halmac_adapter *padapter, unsigned int timeout);

#endif /* INCLUDE_RTW_FTM_H_ */
