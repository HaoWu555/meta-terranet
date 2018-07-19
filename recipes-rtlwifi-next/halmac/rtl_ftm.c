#include <halmac_type.h>
#include <linux/kthread.h>
#include "rtl_ftm.h"

#define _RT_STATUS int
extern u8 rtl8822b_mac_verify(struct halmac_adapter *padapter);

static unsigned int construct_ie_ftm_param_content(struct halmac_adapter *padapter, RT_FTM_SCHEDULE_PARA *pFTMParamInfo,  u8 *pbuf, unsigned int len);
static void ftm_SetFixRequester(struct halmac_adapter *padapter);
static void ftm_SetFixResponder(struct halmac_adapter *padapter, char cnt);
static void ftm_Set11kResponder(struct halmac_adapter *padapter);
static void ftm_SetFixFTMReqParam(struct halmac_adapter *padapter);
//static void ftm_SetFixFTMParam(struct halmac_adapter *padapter);

#define FTM_INFO_LOCK _enter_critical(&ftm_info->lock, &irqL)
#define FTM_INFO_UNLOCK _exit_critical(&ftm_info->lock, &irqL)


#define FTM_FIXED_CHANNEL 149
#define FTM_FIXED_MODE    WIRELESS_MODE_AC_5G
#define FTM_FIXED_BW      HT_CHANNEL_WIDTH_20

RT_FTM_SELECT_RESP ftm_common_params = {
		  .bCIVICCap = 0,
		  .bLCICap   = 0,

		  .targetFTMParaInfo =
		  	  {
		  			.PartialTSFStartOffset = 10,
		  			.MinDeltaFTM           = 3,
		  			.BurstExpoNum          = 0,
		  			.BurstTimeout          = 10,
		  			.ASAPCapable           = 0,
		  			.ASAP                  = 0,
		  			.FTMNumPerBust         = 10,
		  			.FTMFormatAndBW        = FTM_PARAM_FORMATBW_HT_20,
		  			.BurstPeriod           = 0,
		  	  },
		  .ChannelNum   = FTM_FIXED_CHANNEL,
//		  .wirelessmode = FTM_FIXED_MODE,
		  .Bandwidth    = FTM_FIXED_BW,
		  .ExtChnlOffsetOf40MHz = 0,
		  .ExtChnlOffsetOf80MHz = 0,
};

// void rtw_ftm_init(struct halmac_adapter *padapter)
// {
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

// 	pr_info("!!![FTM] ---> %s !!!\n", __func__);

// 	memset(ftm_info, 0, sizeof(RT_FTM_INFO));
// 	pr_info("!!![FTM] ---> %s !!!\n", __func__);
// 	ftm_info->Role 						= FTM_NONE;
// 	ftm_info->bNeedToDLFTMpkts 			= _FALSE;
// 	ftm_info->bFTMParaFromSigma 		= _FALSE;
// 	ftm_info->bBurstTimeoutFromSigma	= _FALSE;
// 	ftm_info->FTMReqInfoStartLoc 		= 0;
// 	ftm_info->FTMSelectRspNum 			= 0;
// 	ftm_info->bSupportFTM 				= _TRUE;
// 	ftm_info->bNeedLCI 					= _TRUE;
// 	ftm_info->bNeedCIVICLocation 		= _TRUE;
// 	ftm_info->bInitiator 				= _FALSE;
// 	ftm_info->bResponder 				= _FALSE;
// 	ftm_info->bRMEnalbe 				= _TRUE;
// 	ftm_info->bRespCapSupportASAP 		= _FALSE;
// 	ftm_info->RespCapMinDeltaFTM 		= 50;
// 	ftm_info->ftmTimeoutMs 				= 1000;


// 	pr_info("!!![FTM] <--- %s !!!\n", __func__);
// }

static void rtw_ftm_session_timer_handler(void *FunctionContext)
{
	struct halmac_adapter *adapter = (struct halmac_adapter*)FunctionContext;
	RT_FTM_INFO *ftm_info = GET_HAL_FTM(adapter);
	_irqL irqL;

	pr_warn("+%s\n", __FUNCTION__);
	_enter_critical(&ftm_info->lock, &irqL);
	if (ftm_info->Role == FTM_RESPONDER)
	{
		ftm_info->reqState = FTM_REQUEST_STATE_COMPLETE;
		 _exit_critical(&ftm_info->lock, &irqL);

		ftm_info->ftmTimeoutMs = 0;
		_up_sema(&ftm_info->FTMpreparingEvent);
		return;
	}
	if (ftm_info->reqState == FTM_REQUEST_STATE_COMPLETE)
	{
		ftm_info->reqID = FTM_REQUEST_ID_START_REQUESTER_ROLE_WITH_TARGET_LIST_BY_USER;
		ftm_info->preThreadStateFlag |= 0x400;
		ftm_info->reqState = FTM_REQUEST_STATE_INIT;
		 _exit_critical(&ftm_info->lock, &irqL);

		if (ftm_info->ftmTimeoutMs > 0)
			_set_timer(&ftm_info->FTMSessionTimer, ftm_info->ftmTimeoutMs);
		_up_sema(&ftm_info->FTMpreparingEvent);
		return;

	}
	else if (ftm_info->reqState == FTM_REQUEST_STATE_FTM_SESSION_END)
	{
		_exit_critical(&ftm_info->lock, &irqL);
		pr_warn("[FTM] %s wait\n", __FUNCTION__);
	    if (ftm_info->ftmTimeoutMs > 0)
	    	_set_timer(&ftm_info->FTMSessionTimer, ftm_info->ftmTimeoutMs);
		return;
	}
	else
	{
		ftm_info->reqState = FTM_REQUEST_STATE_COMPLETE;
		//ftm_info->reqState = FTM_REQUEST_STATE_SET_H2C_CMD;
		//ftm_info->FTMSelectedRespList[(int)ftm_info->FTMSelectRspNum].bFTMdone = 0;

		ftm_info->hang = 0xE0;
		_exit_critical(&ftm_info->lock, &irqL);
		pr_warn("[FTM] %s cancel session\n", __FUNCTION__);

	    if (ftm_info->ftmTimeoutMs > 0)
	    	_set_timer(&ftm_info->FTMSessionTimer, ftm_info->ftmTimeoutMs);
	    _up_sema(&ftm_info->FTMpreparingEvent);

	}
}

int rtw_ftm_run_threads(struct halmac_adapter *padapter)
{
	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

	pr_info("!!![FTM] ---> %s !!!\n", __func__);
	ftm_info->Role						= FTM_NONE;
	ftm_info->bNeedToDLFTMpkts			= _FALSE;
	ftm_info->bFTMParaFromSigma 		= _FALSE;
	ftm_info->bBurstTimeoutFromSigma 	= _FALSE;
	ftm_info->FTMReqInfoStartLoc 		= 0;
	ftm_info->FTMSelectRspNum 			= 0;
	ftm_info->bSupportFTM 				= _TRUE;
	ftm_info->bNeedLCI 					= _TRUE;
	ftm_info->bNeedCIVICLocation 		= _TRUE;
	ftm_info->bInitiator 				= _FALSE;
	ftm_info->bResponder 				= _FALSE;
	ftm_info->bRMEnalbe 				= _TRUE;
	ftm_info->bRespCapSupportASAP 		= _FALSE;
	ftm_info->RespCapMinDeltaFTM  		= 50;
	ftm_info->ftmTimeoutMs 				= 1000;
	memset(&ftm_info->SigmaFTMParaInfo, 0, sizeof(RT_FTM_SCHEDULE_PARA));

	spin_lock_init(&ftm_info->lock);

	//if (is_primarystruct halmac_adapter(padapter))
	{
		sema_init(&ftm_info->FTMpreparingEvent, 0);
		sema_init(&ftm_info->FTMpreparingTerminate, 0);

		ftm_info->preThreadStateFlag = 0;//0x400;
		//TODO: fix thread
		//ftm_info->FTMpreparingThread = kthread_run(rtw_ftm_preparing_thread, padapter, "RTW_FTM_P_THREAD");
		if (IS_ERR(ftm_info->FTMpreparingThread)) {
			pr_err("RTW_FTM_P_THREAD run fail!\n");
			return _FAIL;
		}

		sema_init(&ftm_info->FTMSessionEvent, 0);
		sema_init(&ftm_info->FTMSessionTerminate, 0);

		//TODO: fix thread
		//ftm_info->FTMSessionThread = kthread_run(rtw_ftm_session_thread, padapter, "RTW_FTM_S_THREAD");
		if (IS_ERR(ftm_info->FTMSessionThread)) {
			pr_err("RTW_FTM_S_THREAD run fail!\n");
			return _FAIL;
		}
	}

	_init_timer(&ftm_info->FTMSessionTimer, rtw_ftm_session_timer_handler, padapter);

	pr_info("!!![FTM] <--- %s !!!\n", __func__);
	return _SUCCESS;
}

int rtw_ftm_cancel_threads(struct halmac_adapter *padapter)
{
	_irqL irqL;
	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

	pr_info("!!![FTM] ---> %s !!!\n", __func__);

	_cancel_timer_ex(&ftm_info->FTMSessionTimer);

	//if (is_primarystruct halmac_adapter(padapter))
	{
		/* Below is to termindate rx_thread... */
    	FTM_INFO_LOCK;
    	ftm_info->preThreadStateFlag |= 0x18;
    	FTM_INFO_UNLOCK;
		_up_sema(&ftm_info->FTMpreparingEvent);
		_down_sema(&ftm_info->FTMpreparingTerminate);

    	FTM_INFO_LOCK;
    	ftm_info->sessionThreadStateFlag |= 0x18;
    	FTM_INFO_UNLOCK;
		_up_sema(&ftm_info->FTMSessionEvent);
		_down_sema(&ftm_info->FTMSessionTerminate);
	}

	//_rtw_free_sema(&ftm_info->FTMpreparingEvent);
	//_rtw_free_sema(&ftm_info->FTMpreparingTerminate);

	//_rtw_free_sema(&ftm_info->FTMSessionEvent);
	//_rtw_free_sema(&ftm_info->FTMSessionTerminate);

	//_rtw_spinlock_free(&ftm_info->lock);

	pr_info("!!![FTM] <--- %s !!!\n", __func__);
	return _SUCCESS;
}

// int rtw_ftm_preparing_thread(void *context)
// {
// 	struct halmac_adapter *adapter = (struct halmac_adapter *)context;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(adapter);
// 	_irqL irqL;
// 	FTM_REQUEST_ID req_id;
// 	s32 err = _SUCCESS;
// 	u8 scan_cnt = 0;
// 	u8 EnableFTMFunc = 0;
// 	//struct mlme_priv *pmlmepriv = &adapter->mlmepriv;

// 	thread_enter("RTW_FTM_P_THREAD");

// 	pr_info("enter\n");

//     ftm_info->bFTMSupportByFW = _TRUE; //hal_chk_wl_func(adapter, WL_FUNC_FTM) ? _TRUE : _FALSE;
//     ftm_info->ftmDataRate = 8;
//     ftm_info->bFTMParamFromFile = 0;
//     //rtw_set_scan_deny(adapter, 600000);
// 	//TODO: disable scan while FTM
//     //ATOMIC_SET(&pmlmepriv->set_scan_deny, 1);
//     //ftm_info->mlmeScanInterval = pmlmepriv->auto_scan_int_ms;
//     //pmlmepriv->auto_scan_int_ms = 0;

// 	do {
// 		//rtw_hal_set_hwreg(adapter, HW_VAR_SET_PS_TIMER0_MAPPING_TSF_TIMER, 0);
// 		err = _down_sema(&ftm_info->FTMpreparingEvent);
// 		if (_FAIL == err) {
// 			pr_err("down FTMpreparingEvent fail!\n");
// 			goto exit;
// 		}



// 		if (RTL_CANNOT_RUN(adapter)) {
// 			pr_info("AdapterValidate: %d, ApiValidate: %d\n",
// 				halmac_adapter_validate(adapter), halmac_api_validate(adapter));
// 			goto exit;
// 		}

// 		FTM_INFO_LOCK;

// 	    if ((ftm_info->preThreadStateFlag & 0x18) != 0 )
// 	    {
// 	    	FTM_INFO_UNLOCK;
// 	    	FTM_INFO_LOCK;
// 	    	ftm_info->preThreadStateFlag &= 0xFFFFFFFB;
// 	    	FTM_INFO_UNLOCK;
// 	    	goto exit;
// 	    }
// 	    FTM_INFO_UNLOCK;

// 	    FTM_INFO_LOCK;
// 	    if ((ftm_info->preThreadStateFlag & 0x400) == 0 )
// 	    {
// 	    	FTM_INFO_UNLOCK;
// 	    }
// 	    else
// 	    {
// 	    	FTM_INFO_UNLOCK;
// 			FTM_INFO_LOCK;
// 			req_id = ftm_info->reqID;
// 			switch (req_id) {
// 			case FTM_REQUEST_ID_SCAN_REQUEST:
// 				FTM_INFO_UNLOCK;
// 				pr_info("!!![FTM] %s FTM_REQUEST_ID_SCAN_REQUEST !!!\n", __func__);
// 				if (ftm_info->reqState == FTM_REQUEST_STATE_COMPLETE)
// 				{
// 					ftm_info->preThreadStateFlag &= ~(0x400);
// 				}
// 				else
// 				{
// 					FTM_INFO_LOCK;
// 					ftm_info->reqState = FTM_REQUEST_STATE_COMPLETE;
// 					FTM_INFO_UNLOCK;
// 					//rtw_msleep_os(1);
// 					_up_sema(&ftm_info->FTMpreparingEvent);
// 				}
// 				break;
// 			case FTM_REQUEST_ID_START_RESPONDER_ROLE:
// 				FTM_INFO_UNLOCK;
// 				pr_info("!!![FTM] %s FTM_REQUEST_ID_START_RESPONDER_ROLE !!!\n", __func__);
// 				if (ftm_info->bFTMSupportByFW == _TRUE)
// 					ftm_StartFTMResponderRoleByFW(adapter);
// 				else
// 					ftm_StartFTMResponderRoleByDriver(adapter);
// 				break;
// 			case FTM_REQUEST_ID_START_REQUESTER_ROLE_WITH_TARGET_LIST_BY_USER:
// 			case FTM_REQUEST_ID_START_REQUESTER_ROLE_WITH_TARGET_LIST_BY_RM_REQ:
// 				FTM_INFO_UNLOCK;
// 				pr_info("!!![FTM] REQ %s FTM_REQUEST_ID_START_REQUESTER_ROLE !!!\n", __func__);
// 				if (ftm_info->bFTMSupportByFW == _TRUE)
// 					ftm_StartFTMRequestRoleByFW(adapter, &scan_cnt);
// 				else
// 					ftm_StartFTMRequestRoleByDriver(adapter, &scan_cnt);
// 				break;
// 			default:
// 				FTM_INFO_UNLOCK;
// 				break;
// 			}
// 		}

// 		flush_signals_thread();

// 	} while (err != _FAIL);

// exit:
// 	_up_sema(&ftm_info->FTMpreparingTerminate);
// 	pr_info("exit\n");
// 	thread_exit(NULL);
// 	return 0;
// }

// int rtw_ftm_session_thread(void *context)
// {
// 	struct halmac_adapter *adapter = (struct halmac_adapter *)context;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(adapter);
// 	_irqL irqL;
// 	FTM_REQUEST_ID req_id;
// 	s32 err = _SUCCESS;
// 	u8 dr;
// 	u8 bw;
// 	u8 rsp_idx = 0;
// 	uint64_t curr_tsf;

// 	thread_enter("RTW_FTM_S_THREAD");

// 	pr_info("enter\n");

// 	FTM_INFO_LOCK;
// 	ftm_info->sessionThreadStateFlag |= 4;
// 	FTM_INFO_UNLOCK;

// 	do {
// 		err = _down_sema(&ftm_info->FTMSessionEvent);
// 		if (_FAIL == err) {
// 			pr_err("down FTMpreparingEvent fail!\n");
// 			goto exit;
// 		}

// 		if (RTL_CANNOT_RUN(adapter)) {
// 			pr_info("AdapterValidate: %d, ApiValidate: %d\n",
// 				halmac_adapter_validate(adapter), halmac_api_validate(adapter));
// 			goto exit;
// 		}

// 		FTM_INFO_LOCK;
// 	    if ((ftm_info->sessionThreadStateFlag & 0x18) != 0 )
// 	    {
// 	    	FTM_INFO_UNLOCK;
// 	    	/*Terminate*/
// 	    	FTM_INFO_LOCK;
// 	    	ftm_info->sessionThreadStateFlag &= 0xFFFFFFFB;
// 	    	FTM_INFO_UNLOCK;
// 	    	goto exit;
// 	    }
// 	    FTM_INFO_UNLOCK;

// #if 0
// 	    FTM_INFO_LOCK;
// 	    if (ftm_info->sessionThreadStateFlag & 0x400)
// 	    {
// 	    	FTM_INFO_UNLOCK;

// 			switch (ftm_info->sessionState)
// 			{
// 			case FTM_SESSION_STATE_INIT:
// 				if (ftm_info->FTMTargetList.NumOfResponder > rsp_idx) {
// 					FTM_INFO_LOCK;
// 					ftm_info->sessionState = FTM_SESSION_STATE_NEGOTIATION;
// 					FTM_INFO_UNLOCK;
// 					_up_sema(&ftm_info->FTMSessionEvent);
// 					ftm_info->pFTMActiveSelectedResp = &ftm_info->FTMSelectedRespList[rsp_idx];
// 				} else {
// 					FTM_INFO_LOCK;
// 					ftm_info->sessionState = FTM_SESSION_STATE_MEASUREMENT;
// 					FTM_INFO_UNLOCK;
// 					_up_sema(&ftm_info->FTMSessionEvent);
// 					rsp_idx = 0;
// 				}
// 				break;
// 			case FTM_SESSION_STATE_NEGOTIATION:
// 				{
// 					bw = 0;
// 					dr = 12;
// 					WIRELESS_MODE w_mode;
// 					FTM_SelectBWandDataRate(ftm_info->pFTMActiveSelectedResp->FTMParaInfo.FTMFormatAndBW, &bw, &dr);
// 					ftm_info->ftmDataRate = dr;

// 					checkSC(adapter);

// 					w_mode = SetupJoinWirelessMode(adapter, 1, ftm_info->pFTMActiveSelectedResp->wirelessmode);
// 					adapter->hal_func.SetWirelessModeHandler(adapter, w_mode);
// 					adapter->hal_func.set_chnl_bw_handler(
// 						adapter,
// 						ftm_info->pFTMActiveSelectedResp->ChannelNum,
// 						bw,
// 						ftm_info->pFTMActiveSelectedResp->ExtChnlOffsetOf40MHz,
// 						0,
// 						0);

// 					checkSC(adapter);
// 					memset(&ftm_info->pFTMActiveSelectedResp->burstEntity, 0, sizeof(RT_FTM_BURST_ENTITY));
// 					checkSC(adapter);
// 					ftm_send_ftm_request(adapter, 1, ftm_info->pFTMActiveSelectedResp->MacAddr);
// 					//FTM_SendFTMRequest(adapter, 1, ftm_info->pFTMActiveSelectedResp->MacAddr, &ftm_info->pFTMActiveSelectedResp->FTMParaInfo);
// 					checkSC(adapter);
// 					rtw_hal_get_hwreg(adapter, HW_VAR_FREERUN_CNT, (char *)&curr_tsf);
// 					ftm_info->pFTMActiveSelectedResp->burstEntity.CurBurstStartTsf = curr_tsf
// 																					 + (ftm_info->pFTMActiveSelectedResp->FTMParaInfo.PartialTSFStartOffset << 10);
// 					ftm_info->pFTMActiveSelectedResp->burstEntity.CurBurstStartTsf -= ftm_info->pFTMActiveSelectedResp->FTMParaInfo.PartialTSFdiscardBit;

// 					FTM_INFO_LOCK;
// 					ftm_info->sessionState = FTM_SESSION_STATE_NEGOTIATION_WAIT_IFTM;
// 					FTM_INFO_UNLOCK;
// 					_up_sema(&ftm_info->FTMSessionEvent);
// 					FTM_SetReqFTMSessionActionTimer(adapter, 10240);
// 				}
// 		        break;
// 			case FTM_SESSION_STATE_NEGOTIATION_WAIT_IFTM:
// 				break;
// 			case FTM_SESSION_STATE_NEGOTIATION_GET_IFTM:
// 				ActionTimerFlushActionItem(adapter, ftm_info->FTMReqActionTimer, ACTION_TYPE_FTM);
// 				ftm_info->pFTMActiveSelectedResp->burstEntity.CurBurstDurationTsf =
// 						ftm_info->pFTMActiveSelectedResp->burstEntity.CurBurstStartTsf
// 								+ ftm_DurationValueTranslate(
// 										ftm_info->pFTMActiveSelectedResp->finalFTMParaInfo.BurstTimeout);
// 				ftm_info->pFTMActiveSelectedResp->burstEntity.CurBurstPeriodTsf =
// 						ftm_info->pFTMActiveSelectedResp->burstEntity.CurBurstStartTsf
// 								+ 102400
// 										* ftm_info->pFTMActiveSelectedResp->finalFTMParaInfo.BurstPeriod;

// 				if (ftm_info->pFTMActiveSelectedResp->FTMParaInfo.ASAP == 1) {
// 					FTM_INFO_LOCK;
// 					ftm_info->sessionState = FTM_SESSION_STATE_MEASUREMENT_ASAP1;
// 					FTM_INFO_UNLOCK;
// 					_up_sema(&ftm_info->FTMSessionEvent);
// 					break;
// 				}
// 				FTM_INFO_LOCK;
// 				ftm_info->sessionState = FTM_SESSION_STATE_INIT;
// 				FTM_INFO_UNLOCK;
// 				_up_sema(&ftm_info->FTMSessionEvent);
// 				++rsp_idx;
// 				break;
// 			case FTM_SESSION_STATE_NEGOTIATION_WAIT_IFTM_TIMEOUT:
// 				ftm_info->pFTMActiveSelectedResp->burstEntity.BurstState = FTM_BURST_STATE_MAX;
// 				FTM_INFO_LOCK;
// 				ftm_info->sessionState = FTM_SESSION_STATE_INIT;
// 				FTM_INFO_UNLOCK;
// 				_up_sema(&ftm_info->FTMSessionEvent);
// 				++rsp_idx;
// 				break;
// 			case FTM_SESSION_STATE_MEASUREMENT_ASAP1:
// 				FTM_INFO_LOCK;
// 				ftm_info->pFTMActiveSelectedResp->burstEntity.BurstState = FTM_BURST_STATE_TIMEOUT;
// 				FTM_INFO_UNLOCK;
// 				FTM_SetReqFTMBurstActionTimer(adapter, ftm_info->pFTMActiveSelectedResp->burstEntity.CurBurstStartTsf);
// 				break;
// 			case FTM_SESSION_STATE_MEASUREMENT_ASAP1_DONE:
// 				++rsp_idx;
// 				ftm_updateBurstEntity(adapter, &ftm_info->pFTMActiveSelectedResp->burstEntity);
// 				FTM_INFO_LOCK;
// 				ftm_info->sessionState = FTM_SESSION_STATE_INIT;
// 				FTM_INFO_UNLOCK;
// 				_up_sema(&ftm_info->FTMSessionEvent);
// 				break;
// 			case FTM_SESSION_STATE_MEASUREMENT:
// 				{
// 					int i;
// 					dr = 0;
// 					for (i = 0; i < 3; i++)
// 					{
// 						if (ftm_info->FTMSelectedRespList[i].bValid == 1)
// 						{
// 							if (ftm_info->FTMSelectedRespList[i].burstEntity.BurstState != FTM_BURST_STATE_MAX)
// 							{
// 								ftm_ptr2NearestBurstEntity(adapter);
// 								bw = 0;
// 								dr = 12;
// 								FTM_SelectBWandDataRate(
// 										ftm_info->pFTMActiveSelectedResp->FTMParaInfo.FTMFormatAndBW,
// 										&bw, &dr);
// 								ftm_info->ftmDataRate = dr;

// 								checkSC(adapter);
// 								MgntActSet_802_11_CHANNEL_AND_BANDWIDTH(adapter,
// 										ftm_info->pFTMActiveSelectedResp->ChannelNum,
// 										bw,
// 										ftm_info->pFTMActiveSelectedResp->ExtChnlOffsetOf40MHz,
// 										0, 0);
// 								checkSC(adapter);
// 								FTM_INFO_LOCK;
// 								ftm_info->pFTMActiveSelectedResp->burstEntity.BurstState =
// 										FTM_BURST_STATE_SEND_FTM;
// 								FTM_INFO_UNLOCK;
// 								FTM_SetReqFTMBurstActionTimer(adapter,
// 										ftm_info->pFTMActiveSelectedResp->burstEntity.CurBurstStartTsf);
// 								break;
// 							}
// 						}
// 					}
// 					if (dr == 12)
// 						break;
// 					FTM_INFO_LOCK;
// 					ftm_info->sessionState = FTM_SESSION_STATE_DONE;
// 					FTM_INFO_UNLOCK;
// 					_up_sema(&ftm_info->FTMSessionEvent);
// 				}
// 				break;
// 			case FTM_SESSION_STATE_MEASUREMENT_DONE:
// 				ftm_updateBurstEntity(adapter, &ftm_info->pFTMActiveSelectedResp->burstEntity);
// 				FTM_INFO_LOCK;
// 				ftm_info->sessionState = FTM_SESSION_STATE_MEASUREMENT;
// 				FTM_INFO_UNLOCK;
// 				_up_sema(&ftm_info->FTMSessionEvent);
// 				break;
// 			case FTM_SESSION_STATE_DONE:
// 				ftm_info->sessionThreadStateFlag &= 0xFFFFFBFF;
// 				FTM_INFO_LOCK;
// 				ftm_info->reqState = FTM_REQUEST_STATE_FTM_SESSION_END;
// 				FTM_INFO_UNLOCK;
// 				_up_sema(&ftm_info->FTMpreparingEvent);
// 				break;
// 			default:
// 				break;
// 			}
// 		}
// 	    //FTM_INFO_UNLOCK;
// #endif
// 		flush_signals_thread();

// 	} while (err != _FAIL);

// exit:
// 	_up_sema(&ftm_info->FTMSessionTerminate);
// 	pr_info("exit\n");
// 	thread_exit(NULL);
// 	return 0;
// }

// void FTM_SetFTMRole(struct halmac_adapter *adapter, char ftmRole)
// {
//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(adapter);

//   pr_info("!!![FTM] ---> %s !!!\n", __func__);
//   if ( ftmRole )
//   {
//     if ( ftmRole == FTM_INITIATOR )
//     {
//       ftm_info->bInitiator = _TRUE;
//     }
//   }
//   else
//   {
//     ftm_info->bResponder = _TRUE;
//   }
// }

// void   FTM_SetRMEnable(struct halmac_adapter *Adapter, char SetRMEnable)
// {
//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(Adapter);

//   pr_info("!!![FTM] ---> %s !!!\n", __func__);
//   if ( SetRMEnable )
//   {
// //LOG 36
//     ftm_info->bRMEnalbe = 1;
//   }
//   else
//   {
// //LOG 35
//     ftm_info->bRMEnalbe = 0;
//   }
// }

// void ftm_send_ftm_frame(struct halmac_adapter *padapter, u8 is_initial_frame, u8 *pdev_raddr)
// {
// 	struct xmit_frame			*pmgntframe;
// 	struct pkt_attrib			*pattrib;
// 	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);

// 	pmgntframe = alloc_mgtxmitframe(pxmitpriv);
// 	if (pmgntframe == NULL)
// 		return;

// 	pr_info("[%s] In\n", __FUNCTION__);
// 	/* update attribute */
// 	pattrib = &pmgntframe->attrib;


// 	update_mgntframe_attrib(padapter, pattrib);
// 	memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

// 	construct_ftm_frame_packet(padapter, is_initial_frame, pmgntframe, pdev_raddr);

// 	pattrib->last_txcmdsz = pattrib->pktlen;
// 	dump_mgntframe(padapter, pmgntframe);
// }

// void construct_ftm_frame_packet(struct halmac_adapter *padapter, u8 is_initial_frame, struct xmit_frame *pmgntframe, u8 *pdev_raddr)
// {
// 	u8          category    = RTW_WLAN_CATEGORY_PUBLIC;
// 	u8			action      = ACT_PUBLIC_FINE_TIMING_MEASUREMENT;
// 	u8			ftmie[16]   = { 0x00 };
// 	u8			ftmielen    = 0;
// 	u8			tod_error[2], toa_error[2];

// 	struct pkt_attrib			*pattrib;
// 	unsigned char				*pframe;
// 	struct rtw_ieee80211_hdr	*pwlanhdr;
// 	unsigned short				*fctrl;

// 	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
// 	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
// 	struct wifidirect_info	*pwdinfo = &(padapter->wdinfo);
// 	RT_FTM_INFO    *ftm_info = GET_HAL_FTM(padapter);
// 	RT_FTM_REQ_STA *req_sta  = &ftm_info->FTMReqSTAEntry[0];


// 	pr_info("[%s] In\n", __FUNCTION__);
// 	/* update attribute */
// 	pattrib = &pmgntframe->attrib;


// 	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
// 	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

// 	fctrl = &(pwlanhdr->frame_ctl);
// 	*(fctrl) = 0;

// 	memcpy(pwlanhdr->addr1, pdev_raddr, ETH_ALEN);
// 	memcpy(pwlanhdr->addr2, adapter_mac_addr(padapter), ETH_ALEN);
// 	memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

// 	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
// 	pmlmeext->mgnt_seq++;
// 	set_frame_sub_type(pframe, WIFI_ACTION);

// 	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
// 	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

// 	RTW_PUT_BE16(tod_error, req_sta->TODError);
// 	RTW_PUT_BE16(toa_error, req_sta->TOAError);

// 	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
// 	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));
// 	pframe = rtw_set_fixed_ie(pframe, 1, &(req_sta->DialogToken), &(pattrib->pktlen));
// 	pframe = rtw_set_fixed_ie(pframe, 1, &(req_sta->FollowUpDialogToken), &(pattrib->pktlen));
// 	pframe = rtw_set_fixed_ie(pframe, 6, (req_sta->TOD_t1), &(pattrib->pktlen));
// 	pframe = rtw_set_fixed_ie(pframe, 6, (req_sta->TOA_t4), &(pattrib->pktlen));
// 	pframe = rtw_set_fixed_ie(pframe, 2, tod_error, &(pattrib->pktlen));
// 	pframe = rtw_set_fixed_ie(pframe, 2, toa_error, &(pattrib->pktlen));

// 	if (is_initial_frame)
// 	{
// 		ftmielen = construct_ie_ftm_param_content(padapter, &req_sta->FTMParaInfo, ftmie, sizeof(ftmie));
// 		if (is_initial_frame > 1)
// 		{
// 			ftmie[0] = 3 | ((is_initial_frame-1) << 2);
// 		}
// 		pframe = rtw_set_ie(pframe, EID_FTMParams, ftmielen, (unsigned char *) ftmie, &pattrib->pktlen);
// 	}

// 	return;
// }

// void construct_ftm_frame_buffer(struct halmac_adapter *padapter, u8 is_initial_frame, u8 *pbuffer, u32 *plength)
// {
// 	u8          category    = RTW_WLAN_CATEGORY_PUBLIC;
// 	u8			action      = ACT_PUBLIC_FINE_TIMING_MEASUREMENT;
// 	u8			ftmie[16]   = { 0x00 };
// 	u8			ftmielen    = 0;
// 	u8			tod_error[2], toa_error[2];

// 	unsigned char				*pframe = pbuffer;
// 	struct rtw_ieee80211_hdr	*pwlanhdr;
// 	unsigned short				*fctrl;

// 	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
// 	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
// 	struct wifidirect_info	*pwdinfo = &(padapter->wdinfo);
// 	RT_FTM_INFO    *ftm_info = GET_HAL_FTM(padapter);
// 	RT_FTM_REQ_STA *req_sta  = &ftm_info->FTMReqSTAEntry[0];


// 	pr_info("[%s] In\n", __FUNCTION__);

// 	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

// 	fctrl = &(pwlanhdr->frame_ctl);
// 	*(fctrl) = 0;

// 	memcpy(pwlanhdr->addr1, req_sta->MacAddr, ETH_ALEN);
// 	memcpy(pwlanhdr->addr2, adapter_mac_addr(padapter), ETH_ALEN);
// 	memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

// 	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
// 	pmlmeext->mgnt_seq++;
// 	set_frame_sub_type(pframe, WIFI_ACTION);

// 	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
// 	*plength = sizeof(struct rtw_ieee80211_hdr_3addr);

// 	RTW_PUT_BE16(tod_error, req_sta->TODError);
// 	RTW_PUT_BE16(toa_error, req_sta->TOAError);

// 	pframe = rtw_set_fixed_ie(pframe, 1, &(category), plength);
// 	pframe = rtw_set_fixed_ie(pframe, 1, &(action), plength);
// 	pframe = rtw_set_fixed_ie(pframe, 1, &(req_sta->DialogToken), plength);
// 	pframe = rtw_set_fixed_ie(pframe, 1, &(req_sta->FollowUpDialogToken), plength);
// 	pframe = rtw_set_fixed_ie(pframe, 6, (req_sta->TOD_t1), plength);
// 	pframe = rtw_set_fixed_ie(pframe, 6, (req_sta->TOA_t4), plength);
// 	pframe = rtw_set_fixed_ie(pframe, 2, tod_error, plength);
// 	pframe = rtw_set_fixed_ie(pframe, 2, toa_error, plength);

// 	if (is_initial_frame)
// 	{
// 		ftmielen = construct_ie_ftm_param_content(padapter, &req_sta->FTMParaInfo, ftmie, sizeof(ftmie));
// 		pframe = rtw_set_ie(pframe, EID_FTMParams, ftmielen, (unsigned char *) ftmie, plength);
// 	}

// 	return;
// }

// void ftm_start_responding_process2(struct halmac_adapter *padapter)
// {
//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

//   pr_info("!!![FTM] ---> %s !!!\n", __func__);
//   ftm_info->bResponder = _TRUE;
//   ftm_info->Role = FTM_RESPONDER;
//   ftm_info->RespondingState = FTM_RESP_STATE_ON_INIT_FTM_REQ;
// }


// static unsigned int construct_ie_ftm_param_content(struct halmac_adapter *padapter,RT_FTM_SCHEDULE_PARA *pFTMParamInfo,  u8 *pbuf, unsigned int len)
// {
// 	uint64_t CurTsf, usPartialTSF;

// 	pr_info("!!![FTM] ---> %s !!!\n", __func__);
// 	if (len < 9)
// 		return 0;
// 	rtw_hal_get_hwreg(padapter, HW_VAR_FREERUN_CNT, (u8 *)(&CurTsf));

// 	usPartialTSF = CurTsf + pFTMParamInfo->TSFOffset + (pFTMParamInfo->PartialTSFStartOffset << 10);

// 	pr_info("%s CurTsf hi:%d, lo:%d\n", __FUNCTION__, (u32)(CurTsf>>32), (u32)CurTsf);

// 	pFTMParamInfo->PartialTSFdiscardBit = usPartialTSF & 0x3FF;

// 	pbuf[0] = 0;//1; /* Status */
// 	pbuf[1] = ((pFTMParamInfo->BurstTimeout << 4) & 0xF0) | ((pFTMParamInfo->BurstExpoNum) & 0x0F);
// 	pbuf[2] = pFTMParamInfo->MinDeltaFTM;;
// 	RTW_PUT_BE16(&pbuf[3],(usPartialTSF >> 10) & 0xFFFF);
// 	pbuf[5] = ((pFTMParamInfo->ASAPCapable << 1) & 0x02) |
// 			 ((pFTMParamInfo->ASAP << 2) & 0x04) |
// 			 ((pFTMParamInfo->FTMNumPerBust << 3) & 0xF8);
// 	pbuf[6] = ((pFTMParamInfo->FTMFormatAndBW << 2) & 0xFC);
// 	RTW_PUT_BE16(&pbuf[7], pFTMParamInfo->BurstPeriod);

// 	return 9;
// }

// static void construct_action_ftm_req_packet(struct halmac_adapter *padapter)
// {
// 	_irqL	irqL;
// 	_list	*plist, *phead;
// 	unsigned char category, action, trigger;
// 	struct xmit_frame			*pmgntframe;
// 	struct pkt_attrib			*pattrib;
// 	unsigned char				*pframe;
// 	struct rtw_ieee80211_hdr	*pwlanhdr;
// 	unsigned short			*fctrl;
// 	struct	wlan_network	*pnetwork = NULL;
// 	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);
// 	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
// 	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
// 	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
// 	_queue		*queue	= &(pmlmepriv->scanned_queue);
// 	u8 InfoContent[16] = {0};
// 	u8 ICS[8][15];
// 	u8 ie_ftmparams[9] = {0};
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);
// 	pr_info("!!![FTM] ---> %s !!!\n", __func__);

// 	if ((pmlmepriv->num_FortyMHzIntolerant == 0) || (pmlmepriv->num_sta_no_ht == 0))
// 		return;

// 	if (_TRUE == pmlmeinfo->bwmode_updated)
// 		return;

// 	if (rtw_rfctl_is_tx_blocked_by_ch_waiting(adapter_to_rfctl(padapter)))
// 		return;

// 	pr_info("%s\n", __FUNCTION__);


// 	category = RTW_WLAN_CATEGORY_PUBLIC;
// 	action = ACT_PUBLIC_FINE_TIMING_MEASUREMENT_REQUEST;

// 	pmgntframe = alloc_mgtxmitframe(pxmitpriv);
// 	if (pmgntframe == NULL)
// 		return;

// 	/* update attribute */
// 	pattrib = &pmgntframe->attrib;
// 	update_mgntframe_attrib(padapter, pattrib);

// 	memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

// 	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
// 	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

// 	fctrl = &(pwlanhdr->frame_ctl);
// 	*(fctrl) = 0;

// 	memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
// 	memcpy(pwlanhdr->addr2, adapter_mac_addr(padapter), ETH_ALEN);
// 	memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

// 	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
// 	pmlmeext->mgnt_seq++;
// 	set_frame_sub_type(pframe, WIFI_ACTION);

// 	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
// 	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

// 	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
// 	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));
// 	pframe = rtw_set_fixed_ie(pframe, 1, &(trigger), &(pattrib->pktlen));

// 	construct_ie_ftm_param_content(padapter, &ftm_info->FTMReqSTAEntry->FTMParaInfo, ie_ftmparams, sizeof(ie_ftmparams));

//     pframe = rtw_set_ie(pframe, EID_FTMParams,  sizeof(ie_ftmparams), ie_ftmparams, &(pattrib->pktlen));
// 	/*  */
// 	if (pmlmepriv->num_FortyMHzIntolerant > 0) {
// 		u8 iedata = 0;

// 		iedata |= BIT(2);/* 20 MHz BSS Width Request */

// 		pframe = rtw_set_ie(pframe, EID_BSSCoexistence,  1, &iedata, &(pattrib->pktlen));

// 	}
// #if 0

// 	/*  */
// 	memset(ICS, 0, sizeof(ICS));
// 	if (pmlmepriv->num_sta_no_ht > 0) {
// 		int i;

// 		_enter_critical_bh(&(pmlmepriv->scanned_queue.lock), &irqL);

// 		phead = get_list_head(queue);
// 		plist = get_next(phead);

// 		while (1) {
// 			int len;
// 			u8 *p;
// 			WLAN_BSSID_EX *pbss_network;

// 			if (rtw_end_of_queue_search(phead, plist) == _TRUE)
// 				break;

// 			pnetwork = LIST_CONTAINOR(plist, struct wlan_network, list);

// 			plist = get_next(plist);

// 			pbss_network = (WLAN_BSSID_EX *)&pnetwork->network;

// 			p = rtw_get_ie(pbss_network->IEs + _FIXED_IE_LENGTH_, _HT_CAPABILITY_IE_, &len, pbss_network->IELength - _FIXED_IE_LENGTH_);
// 			if ((p == NULL) || (len == 0)) { /* non-HT */
// 				if ((pbss_network->Configuration.DSConfig <= 0) || (pbss_network->Configuration.DSConfig > 14))
// 					continue;

// 				ICS[0][pbss_network->Configuration.DSConfig] = 1;

// 				if (ICS[0][0] == 0)
// 					ICS[0][0] = 1;
// 			}

// 		}

// 		_exit_critical_bh(&(pmlmepriv->scanned_queue.lock), &irqL);


// 		for (i = 0; i < 8; i++) {
// 			if (ICS[i][0] == 1) {
// 				int j, k = 0;

// 				InfoContent[k] = i;
// 				/* SET_BSS_INTOLERANT_ELE_REG_CLASS(InfoContent,i); */
// 				k++;

// 				for (j = 1; j <= 14; j++) {
// 					if (ICS[i][j] == 1) {
// 						if (k < 16) {
// 							InfoContent[k] = j; /* channel number */
// 							/* SET_BSS_INTOLERANT_ELE_CHANNEL(InfoContent+k, j); */
// 							k++;
// 						}
// 					}
// 				}

// 				pframe = rtw_set_ie(pframe, EID_BSSIntolerantChlReport, k, InfoContent, &(pattrib->pktlen));

// 			}

// 		}


// 	}


// 	pattrib->last_txcmdsz = pattrib->pktlen;

// 	dump_mgntframe(padapter, pmgntframe);
// #endif /* CONFIG_80211N_HT */
// }

// void ftm_set_peer(struct halmac_adapter *padapter, u8 idx, u8 *addr, u8 channel)
// {
// 	_irqL irqL;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

// 	if (idx >= 3)
// 		return;

// 	if (idx == 3)
// 	{
// 		_enter_critical(&ftm_info->lock, &irqL);
// 		ftm_info->FTMTargetList.chnl[idx] = channel;
// 		ftm_info->FTMTargetList.MacAddr[idx][0] = addr[0];
// 		ftm_info->FTMTargetList.MacAddr[idx][1] = addr[1];
// 		ftm_info->FTMTargetList.MacAddr[idx][2] = addr[2];
// 		ftm_info->FTMTargetList.MacAddr[idx][3] = addr[3];
// 		ftm_info->FTMTargetList.MacAddr[idx][4] = addr[4];
// 		ftm_info->FTMTargetList.MacAddr[idx][5] = addr[5];
// 		_exit_critical(&ftm_info->lock, &irqL);
// 		return;
// 	}

// 	if (channel == 0)
// 	{
// 		_enter_critical(&ftm_info->lock, &irqL);
// 		ftm_info->FTMTargetList.chnl[idx] = channel;
// 	    ftm_info->FTMTargetList.MacAddr[idx][0] = 0;
// 	    ftm_info->FTMTargetList.MacAddr[idx][1] = 0;
// 	    ftm_info->FTMTargetList.MacAddr[idx][2] = 0;
// 	    ftm_info->FTMTargetList.MacAddr[idx][3] = 0;
// 	    ftm_info->FTMTargetList.MacAddr[idx][4] = 0;
// 	    ftm_info->FTMTargetList.MacAddr[idx][5] = 0;
// 	    if (ftm_info->FTMTargetList.NumOfResponder > 0)
// 	    	ftm_info->FTMTargetList.NumOfResponder--;
// 	    if (ftm_info->FTMTargetList.NumOfResponder == 0)
// 	    	ftm_info->FTMTargetList.bValid[idx] = 0;
// 	    _exit_critical(&ftm_info->lock, &irqL);
// 	}
// 	else
// 	{
// 		_enter_critical(&ftm_info->lock, &irqL);
// 		ftm_info->FTMTargetList.chnl[idx] = channel;
// 		ftm_info->FTMTargetList.MacAddr[idx][0] = addr[0];
// 		ftm_info->FTMTargetList.MacAddr[idx][1] = addr[1];
// 		ftm_info->FTMTargetList.MacAddr[idx][2] = addr[2];
// 		ftm_info->FTMTargetList.MacAddr[idx][3] = addr[3];
// 		ftm_info->FTMTargetList.MacAddr[idx][4] = addr[4];
// 		ftm_info->FTMTargetList.MacAddr[idx][5] = addr[5];
// 		ftm_info->FTMTargetList.bValid[idx] = 0;
// 		if (ftm_info->FTMTargetList.NumOfResponder < 3)
// 			ftm_info->FTMTargetList.NumOfResponder++;
// 		_exit_critical(&ftm_info->lock, &irqL);
// 	}
// }

// void ftm_get_peer(struct halmac_adapter *padapter, u8 idx, u8 *valid, u8 *addr, u8 *channel, u32 *ts, u32 *range, u32 *t3, u32 *t4, u32 *t)
// {
// 	_irqL irqL;
// 	int i;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

// 	if (idx >= 3)
// 		return;
// 	_enter_critical(&ftm_info->lock, &irqL);
// 	*channel = ftm_info->FTMTargetList.chnl[idx];
//     addr[0] = ftm_info->FTMTargetList.MacAddr[idx][0];
//     addr[1] = ftm_info->FTMTargetList.MacAddr[idx][1];
//     addr[2] = ftm_info->FTMTargetList.MacAddr[idx][2];
//     addr[3] = ftm_info->FTMTargetList.MacAddr[idx][3];
//     addr[4] = ftm_info->FTMTargetList.MacAddr[idx][4];
//     addr[5] = ftm_info->FTMTargetList.MacAddr[idx][5];
//     *valid  = ftm_info->FTMTargetList.bValid[idx];
//     *ts     = ftm_info->FTMTargetList.timestamp[idx];
//     *range  = ftm_info->FTMTargetList.range[idx];
//     *t3     = ftm_info->FTMTargetList.t3[idx];
//     *t4     = ftm_info->FTMTargetList.t4[idx];
//     for(i = 0; i < 16; i++)
//     {
//     	t[i]    = ftm_info->FTMSelectedRespList[idx].t3List[i];
//     	t[i+16] = ftm_info->FTMSelectedRespList[idx].t4List[i];
//     	t[i+32] = ftm_info->FTMSelectedRespList[idx].bcList[i];
//     }
//     _exit_critical(&ftm_info->lock, &irqL);
// }

// int ftm_start_requesting_process(struct halmac_adapter *padapter, unsigned int period)
// {
// 	_irqL irqL;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

// 	pr_info("!!![FTM] ---> %s !!!\n", __func__);
// 	_enter_critical(&ftm_info->lock, &irqL);
// 	if (ftm_info->FTMTargetList.NumOfResponder == 0)
// 	{
// 		//LOG
// 		_exit_critical(&ftm_info->lock, &irqL);
// 		return 0;
// 	}
// 	if ( ftm_info->preThreadStateFlag & 0x400 )
// 	{
// 		//LOG
// 		_exit_critical(&ftm_info->lock, &irqL);
// 		return 0;
// 	}
// 	ftm_info->reqID = FTM_REQUEST_ID_START_REQUESTER_ROLE_WITH_TARGET_LIST_BY_USER;
// 	ftm_info->preThreadStateFlag |= 0x400;
// 	ftm_info->reqState = FTM_REQUEST_STATE_INIT;
// 	ftm_info->ftmTimeoutMs = period;
//     _exit_critical(&ftm_info->lock, &irqL);
//     if (period > 0)
//     	_set_timer(&ftm_info->FTMSessionTimer, ftm_info->ftmTimeoutMs);
//     _up_sema(&ftm_info->FTMpreparingEvent);

//     return 1;
// }

// int ftm_start_11k_requesting_process(struct halmac_adapter *padapter, u8* addr, u8 ch, unsigned int period)
// {
// 	_irqL irqL;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

// 	pr_info("!!![FTM] ---> %s !!!\n", __func__);
// 	_enter_critical(&ftm_info->lock, &irqL);
// 	if (ftm_info->preThreadStateFlag & 0x400)
// 	{
// 		//LOG
// 		_exit_critical(&ftm_info->lock, &irqL);
// 		return 0;
// 	}
// 	ftm_info->reqID = FTM_REQUEST_ID_START_REQUESTER_ROLE_WITH_TARGET_LIST_BY_RM_REQ;
// 	ftm_info->preThreadStateFlag |= 0x400;
// 	ftm_info->reqState = FTM_REQUEST_STATE_INIT_11K;
// 	memcpy(ftm_info->FTM11kResp.MacAddr, addr, ETH_ALEN);
// 	ftm_info->FTM11kResp.chnl = ch;
// 	ftm_info->ftmTimeoutMs = period;
// 	_exit_critical(&ftm_info->lock, &irqL);
// 	if (period > 0)
// 		_set_timer(&ftm_info->FTMSessionTimer, ftm_info->ftmTimeoutMs);
// 	_up_sema(&ftm_info->FTMpreparingEvent);

// 	return 1;
// }

// int ftm_start_responding_process(struct halmac_adapter *padapter, unsigned int timeout)
// {
// 	_irqL irqL;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

// 	pr_info("!!![FTM] ---> %s !!!\n", __func__);
// 	_enter_critical(&ftm_info->lock, &irqL);
// 	if ( ftm_info->preThreadStateFlag & 0x400 )
// 	{
// 		_exit_critical(&ftm_info->lock, &irqL);
// 		return 0;
// 	}
// 	memset(&ftm_info->FTMReqSTAEntry[0], 0, sizeof(RT_FTM_REQ_STA));

// 	ftm_info->FTMTargetList.MacAddr[3][0] = 0xFF;
// 	ftm_info->FTMTargetList.MacAddr[3][1] = 0xFF;
// 	ftm_info->FTMTargetList.MacAddr[3][2] = 0xFF;
// 	ftm_info->FTMTargetList.MacAddr[3][3] = 0xFF;
// 	ftm_info->FTMTargetList.MacAddr[3][4] = 0xFF;
// 	ftm_info->FTMTargetList.MacAddr[3][5] = 0xFF;
// 	ftm_info->FTMTargetList.chnl[3] = ftm_common_params.ChannelNum;
// 	ftm_info->reqID = FTM_REQUEST_ID_START_RESPONDER_ROLE;
// 	ftm_info->preThreadStateFlag |= 0x400;
// 	ftm_info->reqState = FTM_REQUEST_STATE_INIT;
// 	ftm_info->ftmTimeoutMs = timeout;
// 	//LOG
// 	_exit_critical(&ftm_info->lock, &irqL);
//     if (timeout > 0)
//     	_set_timer(&ftm_info->FTMSessionTimer, ftm_info->ftmTimeoutMs);
//     _up_sema(&ftm_info->FTMpreparingEvent);

//     return 1;
// }

// int FTM_Request(struct halmac_adapter *padapter, u8 is_responder, u8 start, u8 resp_num, u8 *InformationBuffer, unsigned int InformationBufferLength)
// {
// 	int res = 0;
// 	_irqL irqL;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);
// 	pr_info("!!![FTM] ---> %s !!!\n", __func__);
//     //LOG
//     if ( is_responder )
//     {

// 		if ( is_responder != _TRUE )
// 		{
// 			ftm_info->bInitiator = _FALSE;
// 			ftm_info->bResponder = _FALSE;
// 			ftm_info->Role = FTM_NONE;
// 			return res;
// 		}
// 		if ( start )
// 		{
// 			FTM_StartRespondingProcess(padapter);
// 			return res;
// 		}

// 		_enter_critical(&ftm_info->lock, &irqL);
// 		if ( ftm_info->preThreadStateFlag & 0x400 )
// 		{
// 			//LOG
// 			_exit_critical(&ftm_info->lock, &irqL);
// 			return res;
// 		}
// 		memset(&ftm_info->FTMReqSTAEntry[0], 0, sizeof(RT_FTM_REQ_STA));

// 		ftm_info->FTMTargetList.MacAddr[3][0] = InformationBuffer[0];
// 		ftm_info->FTMTargetList.MacAddr[3][1] = InformationBuffer[1];
// 		ftm_info->FTMTargetList.MacAddr[3][2] = InformationBuffer[2];
// 		ftm_info->FTMTargetList.MacAddr[3][3] = InformationBuffer[3];
// 		ftm_info->FTMTargetList.MacAddr[3][4] = InformationBuffer[4];
// 		ftm_info->FTMTargetList.MacAddr[3][5] = InformationBuffer[5];
// 		ftm_info->FTMTargetList.chnl[3] = ftm_common_params.ChannelNum;
// 		ftm_info->reqID = FTM_REQUEST_ID_START_RESPONDER_ROLE;
// 		ftm_info->preThreadStateFlag |= 0x400;
// 		ftm_info->reqState = FTM_REQUEST_STATE_INIT;
// 		//LOG
// 		_exit_critical(&ftm_info->lock, &irqL);
//     }
//     else
//     {
//     	int i, idx1;
//     	u8 *buffer;
//     	FTM_FlushAllList(padapter);

//         ftm_info->FTMTargetList.NumOfResponder = resp_num;

//         //LOG
//         if ( InformationBufferLength < (6 * resp_num) )
//         {
//           //LOG
//           return 8;
//         }
//         buffer = InformationBuffer;
//         idx1 = 0;
//         for ( i = 0; idx1 < resp_num; ++idx1 )
//         {
//           if ( idx1 )
//           {
//             if ( idx1 == _TRUE )
//             {
//               ftm_info->FTMTargetList.chnl[1] = ftm_common_params.ChannelNum;
//               ftm_info->FTMTargetList.MacAddr[1][0] = *((u8 *)buffer + (unsigned char)i);
//               ftm_info->FTMTargetList.MacAddr[1][1] = *((u8 *)buffer + (unsigned char)i + 1);
//               ftm_info->FTMTargetList.MacAddr[1][2] = *((u8 *)buffer + (unsigned char)i + 2);
//               ftm_info->FTMTargetList.MacAddr[1][3] = *((u8 *)buffer + (unsigned char)i + 3);
//               ftm_info->FTMTargetList.MacAddr[1][4] = *((u8 *)buffer + (unsigned char)i + 4);
//               ftm_info->FTMTargetList.MacAddr[1][5] = *((u8 *)buffer + (unsigned char)i + 5);
//             }
//             else if ( idx1 == 2 )
//             {
//               ftm_info->FTMTargetList.chnl[2] = ftm_common_params.ChannelNum;
//               ftm_info->FTMTargetList.MacAddr[2][0] = *((u8 *)buffer + (unsigned char)i);
//               ftm_info->FTMTargetList.MacAddr[2][1] = *((u8 *)buffer + (unsigned char)i + 1);
//               ftm_info->FTMTargetList.MacAddr[2][2] = *((u8 *)buffer + (unsigned char)i + 2);
//               ftm_info->FTMTargetList.MacAddr[2][3] = *((u8 *)buffer + (unsigned char)i + 3);
//               ftm_info->FTMTargetList.MacAddr[2][4] = *((u8 *)buffer + (unsigned char)i + 4);
//               ftm_info->FTMTargetList.MacAddr[2][5] = *((u8 *)buffer + (unsigned char)i + 5);
//             }
//           }
//           else
//           {
//         	ftm_info->FTMTargetList.chnl[0] = ftm_common_params.ChannelNum;
//             ftm_info->FTMTargetList.MacAddr[0][0] = *((u8 *)buffer + (unsigned char)i);
//             ftm_info->FTMTargetList.MacAddr[0][1] = *((u8 *)buffer + (unsigned char)i + 1);
//             ftm_info->FTMTargetList.MacAddr[0][2] = *((u8 *)buffer + (unsigned char)i + 2);
//             ftm_info->FTMTargetList.MacAddr[0][3] = *((u8 *)buffer + (unsigned char)i + 3);
//             ftm_info->FTMTargetList.MacAddr[0][4] = *((u8 *)buffer + (unsigned char)i + 4);
//             ftm_info->FTMTargetList.MacAddr[0][5] = *((u8 *)buffer + (unsigned char)i + 5);
//           }
//           i += 6;
//         }//for
//       }

//       if ( start )
//       {
//         if ( start != 1 )
//         {
//           return res;
//         }
//         if ( resp_num != 0 )
//         {
//         	rtw_msleep_os(1000);
//         	_enter_critical(&ftm_info->lock, &irqL);
//         	if ( ftm_info->preThreadStateFlag & 0x400 )
//         	{
//         		//LOG
//         		_exit_critical(&ftm_info->lock, &irqL);
//         		return res;
//         	}
//         	ftm_info->reqID = FTM_REQUEST_ID_START_REQUESTER_ROLE_WITH_TARGET_LIST_BY_USER;
//         	ftm_info->preThreadStateFlag |= 0x400;
//         	ftm_info->reqState = FTM_REQUEST_STATE_INIT;
//         	//LOG
//         	_exit_critical(&ftm_info->lock, &irqL);
//         }
//         else
//         {
//         	_enter_critical(&ftm_info->lock, &irqL);
//         	if ( ftm_info->preThreadStateFlag & 0x400 )
//         	{
//         		//LOG
//         		_exit_critical(&ftm_info->lock, &irqL);
//         		return res;
//         	}
//         	ftm_info->reqID = FTM_REQUEST_ID_START_REQUESTER_ROLE_WITHOUT_TARGET_LIST_BY_USER;
//         	ftm_info->preThreadStateFlag |= 0x400;
//         	ftm_info->reqState = FTM_REQUEST_STATE_INIT;
//         	//LOG
//         	_exit_critical(&ftm_info->lock, &irqL);
//         }
//       }


//     _up_sema(&ftm_info->FTMpreparingEvent);

//   //LOG
//     return res;
// }

// #if 0

// void   FTM_SetReqFTMBurstActionTimer(struct halmac_adapter *padapter, uint64_t Timeout)
// {

//   ACTION_TIMER_ITEM ActionItem; // [sp+Ch] [bp-2Ch]@9
//   uint64_t CurTsf; // [sp+2Ch] [bp-Ch]@1
//   RT_FTM_INFO *ftm_info; // [sp+34h] [bp-4h]@1

//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);
//   padapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_FREERUN_CNT, (char *)&CurTsf);

//   rtw_memzero(&ActionItem, 0x20u);
//   ActionItem.pContext = padapter;
//   ActionItem.ActionType = 4;
//   ActionItem.CallbackFunc = FTM_ReqBurstActionTimerCallback;
//   ActionItem.usTimeout = Timeout;
//   ActionTimerRegisterActionItem(padapter, &ActionItem, ftm_info->FTMReqActionTimer);
// }

// void   FTM_ReqSessionActionTimerCallback(_ACTION_TIMER_ITEM *const pOneShotActionItem)
// {
//   RT_FTM_INFO *ftm_info; // edi@1
//   FTM_SESSION_STATE session_state; // eax@1
//   KIRQL v3; // al@11

//   ftm_info = *(_RT_FTM_INFO **)((char *)&loc_619C9A + (unsigned int)pOneShotActionItem->pContext + 2);
//   session_state = ftm_info->sessionState;
//   if ( session_state == FTM_SESSION_STATE_NEGOTIATION_GET_IFTM )
//   {
//     //LOG
//   }
//   else if ( session_state == FTM_SESSION_STATE_NEGOTIATION_WAIT_IFTM )
//   {
//     v3 = KfAcquireSpinLock(&ftm_info->lock.SpinLock);
//     ftm_info->lock.OldIrql = v3;
//     ftm_info->sessionState = FTM_SESSION_STATE_NEGOTIATION_WAIT_IFTM_TIMEOUT;
//     KfReleaseSpinLock(&ftm_info->lock.SpinLock, v3);
//     j_NdisSetEvent(&ftm_info->FTMSessionEvent);
//   }
// }

// void   FTM_SetReqFTMSessionActionTimer(struct halmac_adapter *padapter, uint64_t timeout)
// {
//   struct halmac_adapter *adapter; // edi@1
//   RT_FTM_INFO *ftm_info; // ebx@1
//   ACTION_TIMER_ITEM pInputDataItem; // [sp+Ch] [bp-28h]@5
//   uint64_t curr_tsf; // [sp+2Ch] [bp-8h]@5

//   adapter = padapter->pPortCommonInfo->pDefaultAdapter;
//   ftm_info = *(RT_FTM_INFO **)((char *)&adapter->NdisUsbDev.hNdisAdapter + (_DWORD)&loc_619C9A + 2);

//   rtw_memzero(&pInputDataItem, 0x20u);
//   padapter->HalFunc.GetHwRegHandler(padapter, HW_VAR_FREERUN_CNT, (char *)&curr_tsf);

//   pInputDataItem.ActionType = 4;
//   pInputDataItem.CallbackFunc = FTM_ReqSessionActionTimerCallback;
//   pInputDataItem.pContext = adapter;
//   pInputDataItem.usTimeout = timeout + curr_tsf;
//   ActionTimerRegisterActionItem(padapter, &pInputDataItem, ftm_info->FTMReqActionTimer);
// }
// #endif

// char checkSC(struct halmac_adapter *padapter)
// {
//   char v1;
//   char result;

//   v1 = rtw_read8(padapter, 0x8AC);
//   result = rtw_read8(padapter, REG_DATA_SC);

//   return result | v1;
// }

// uint64_t   ftm_DurationValueTranslate(u8 Idx)
// {
//   unsigned int res; // edi@1

//   res = 0;
//   switch ( Idx )
//   {
//     case 2:
//       res = 250;
//       break;
//     case 3:
//       res = 500;
//       break;
//     case 4:
//       res = 0x400;
//       break;
//     case 5:
//       res = 0x800;
//       break;
//     case 6:
//       res = 0x1000;
//       break;
//     case 7:
//       res = 0x2000;
//       break;
//     case 8:
//       res = 0x4000;
//       break;
//     case 9:
//       res = 0x8000;
//       break;
//     case 10:
//       res = 0x10000;
//       break;
//     case 11:
//       res = 0x20000;
//       break;
//     default:
//       break;
//   }
//   return res;
// }

// void ftm_updateBurstEntity(struct halmac_adapter *padapter, RT_FTM_BURST_ENTITY *pBurstEntity)
// {
//   _irqL irqL;
//   RT_FTM_BURST_ENTITY *burst_entity;
//   RT_FTM_SELECT_RESP *sel_resp;
//   uint16_t burst_expo;
//   uint64_t burst_duration;
//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

//   burst_entity = pBurstEntity;
//   sel_resp = ftm_info->pFTMActiveSelectedResp;
//   burst_expo = 1 << sel_resp->finalFTMParaInfo.BurstExpoNum;
//   pBurstEntity->CurBurstStartTsf += 102400 * sel_resp->finalFTMParaInfo.BurstPeriod;
//   burst_duration = ftm_DurationValueTranslate(ftm_info->pFTMActiveSelectedResp->finalFTMParaInfo.BurstTimeout);
//   burst_entity->CurBurstDurationTsf = burst_entity->CurBurstStartTsf + burst_duration;
//   burst_entity->CurBurstPeriodTsf += 102400 * ftm_info->pFTMActiveSelectedResp->finalFTMParaInfo.BurstPeriod;
//   ++burst_entity->BurstCnt;

//   if ( pBurstEntity->BurstCnt >= burst_expo )
//   {
//     FTM_INFO_LOCK;
//     pBurstEntity->BurstState = FTM_BURST_STATE_MAX;
//     FTM_INFO_UNLOCK;
//   }
//   pBurstEntity->recvFTMnum = 0;
// }

// void   ftm_StoreToFinalFTMParam(struct halmac_adapter *padapter)
// {
//   RT_FTM_SELECT_RESP *sel_resp; // esi@5
//   int curr_tsf_offset; // ecx@5
//   int target_tsf_offet; // edx@5
//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

//   sel_resp = ftm_info->pFTMActiveSelectedResp;
//   curr_tsf_offset = sel_resp->FTMParaInfo.PartialTSFStartOffset;
//   target_tsf_offet = sel_resp->targetFTMParaInfo.PartialTSFStartOffset;
//   if ( curr_tsf_offset < target_tsf_offet )
//   {
//       sel_resp->burstEntity.CurBurstStartTsf += target_tsf_offet - curr_tsf_offset;
//   }

//   sel_resp->finalFTMParaInfo.BurstExpoNum          = sel_resp->FTMParaInfo.BurstExpoNum;
//   sel_resp->finalFTMParaInfo.BurstTimeout          = sel_resp->FTMParaInfo.BurstTimeout;
//   sel_resp->finalFTMParaInfo.PartialTSFStartOffset = sel_resp->FTMParaInfo.PartialTSFStartOffset;
//   sel_resp->finalFTMParaInfo.MinDeltaFTM           = sel_resp->FTMParaInfo.MinDeltaFTM;
//   sel_resp->finalFTMParaInfo.ASAPCapable           = sel_resp->FTMParaInfo.ASAPCapable;
//   sel_resp->finalFTMParaInfo.ASAP                  = sel_resp->FTMParaInfo.ASAP;
//   sel_resp->finalFTMParaInfo.FTMNumPerBust         = sel_resp->FTMParaInfo.FTMNumPerBust;
//   sel_resp->finalFTMParaInfo.FTMFormatAndBW        = sel_resp->FTMParaInfo.FTMFormatAndBW;
//   sel_resp->finalFTMParaInfo.BurstPeriod           = sel_resp->FTMParaInfo.BurstPeriod;
// }


// const u32 burst_duration_enc[16] = {
// 		0, 0,  /* 0-1 Reserved*/
// 		250, 500, 1000, 2000, 4000, 8000, 16000, 32000, 64000, 128000,
// 		0,0,0, /* 12-14 Reserved */
// 		0      /* No preference */
// };


// unsigned int rtw_process_public_act_ftm_frame(struct halmac_adapter *padapter, u8 *pframe, uint frame_len)
// {
// 	unsigned int ret = _FAIL;
// 	OCTET_STRING osMpdu;

// 	osMpdu.Octet = pframe;
// 	osMpdu.Length = frame_len;

// 	if ( FTM_OnFTMframe(padapter, &osMpdu) == RT_STATUS_SUCCESS)
// 	{
// 		ret = _SUCCESS;
// 	}

// 	return ret;
// }

// unsigned int rtw_process_public_act_ftm_req(struct halmac_adapter *padapter, u8 *pframe, uint frame_len)
// {
// 	unsigned int ret = _FAIL;
// 	OCTET_STRING osMpdu;

// 	osMpdu.Octet = pframe;
// 	osMpdu.Length = frame_len;
// /*
// 	if ( FTM_OnFTMRequest(padapter, &osMpdu) == RT_STATUS_SUCCESS)
// 	{
// 		ret = _SUCCESS;
// 	}
// 	*/
// 	return ret;
// }

// int FTM_OnFTMRequest(struct halmac_adapter *padapter, OCTET_STRING *posMpdu)
// {
//   char *buffer;
//   FTM_RESP_STATE resp_state;
//   u8 bFTMParam = 0;
//   u8 bLCIReq = 0;
//   u8 bLocCivicReq = 0;
//   uint64_t NextTsf;
//   uint64_t CurTsf = 0;

//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

//   resp_state = ftm_info->RespondingState;
//   if ( resp_state == FTM_RESP_STATE_ON_INIT_FTM_REQ || resp_state == FTM_RESP_STATE_ON_FTM_REQ )
//   {
//     if ( (unsigned int)posMpdu->Length < 0x1A )
//     {

//       return 9; //RT_STATUS_INVALID_LENGTH;
//     }

//     buffer = posMpdu->Octet;
//     /* Checked in on_action_public() */
//     FTM_ParsingFTMRequestOptionField(padapter, posMpdu, 0x1B, &bFTMParam, &bLCIReq, &bLocCivicReq);

//     ftm_info->FTMReqSTAEntry[0].MacAddr[0] = buffer[10];
//     ftm_info->FTMReqSTAEntry[0].MacAddr[1] = buffer[11];
//     ftm_info->FTMReqSTAEntry[0].MacAddr[2] = buffer[12];
//     ftm_info->FTMReqSTAEntry[0].MacAddr[3] = buffer[13];
//     ftm_info->FTMReqSTAEntry[0].MacAddr[4] = buffer[14];
//     ftm_info->FTMReqSTAEntry[0].MacAddr[5] = buffer[15];
//     resp_state = ftm_info->RespondingState;
//     if ( resp_state == FTM_RESP_STATE_ON_INIT_FTM_REQ )
//     {

//       ftm_info->RespondingState = FTM_RESP_STATE_INIT_FTM_REQ_RECV;
//       if ( bFTMParam == 0 )
//       {
//         return RT_STATUS_INVALID_PARAMETER;
//       }
//       if ( ftm_info->FTMReqSTAEntry[0].FTMParaInfo.MinDeltaFTM < ftm_info->RespCapMinDeltaFTM )
//       {
//         ftm_info->FTMReqSTAEntry[0].FTMParaInfo.MinDeltaFTM = ftm_info->RespCapMinDeltaFTM;
//       }

//       if ( ftm_info->FTMReqSTAEntry[0].FTMParaInfo.ASAP == _TRUE && !ftm_info->bRespCapSupportASAP )
//       {
//         ftm_info->FTMReqSTAEntry[0].FTMParaInfo.ASAP = 0;
//       }

//       if ( ftm_info->FTMReqSTAEntry[0].FTMParaInfo.ASAP )
//       {
//         ftm_info->RespondingState = FTM_RESP_STATE_MESUREMENT_EXC_STARTED;
//         return RT_STATUS_SUCCESS;
//       }
//       else
//       {
//         //padapter->hal_func.GetHwRegHandler(padapter, HW_VAR_FREERUN_CNT, (char *)&CurTsf);

//         ftm_send_ftm_frame(padapter, 2, ftm_info->FTMReqSTAEntry[0].MacAddr);

//         //FTM_SendFTMFrame(padapter, 1, ftm_info->FTMReqSTAEntry[0].MacAddr);
//         ftm_info->RespondingState = FTM_RESP_STATE_INIT_FTM_SEND;
//         ftm_info->FTMRespondingParam[0].BurstsNumber = 1 << ftm_info->FTMReqSTAEntry[0].FTMParaInfo.BurstExpoNum;
//         ftm_info->FTMRespondingParam[0].BurstCount = 0;
//         ftm_info->FTMRespondingParam[0].BurstTimeout = burst_duration_enc[(int)ftm_info->FTMReqSTAEntry[0].FTMParaInfo.BurstTimeout];
//         ftm_info->FTMRespondingParam[0].BurstPeriod = ftm_info->FTMReqSTAEntry[0].FTMParaInfo.BurstPeriod;
//         ftm_info->FTMRespondingParam[0].MinDeltaFTM = ftm_info->FTMReqSTAEntry[0].FTMParaInfo.MinDeltaFTM;
//         ftm_info->FTMRespondingParam[0].FTMsPerBurst = ftm_info->FTMReqSTAEntry[0].FTMParaInfo.FTMNumPerBust;
//         ftm_info->FTMRespondingParam[0].FTMSendCountPerBurst = 0;
//         ftm_info->FTMRespondingParam[0].NextBurstStartTsf = CurTsf
//                                                           + 1000
//                                                           * ftm_info->FTMReqSTAEntry[0].FTMParaInfo.PartialTSFStartOffset;

//         ftm_info->RespondingState = FTM_RESP_STATE_MESUREMENT_EXC_STARTED;

//         //FTM_SetRespBurstState(padapter, FTM_BURST_STATE_START, ftm_info->FTMRespondingParam[0].NextBurstStartTsf);
//       }
// 		ftm_info->reqID = FTM_REQUEST_ID_START_RESPONDER_ROLE;
// 		ftm_info->preThreadStateFlag |= 0x400;
// 		ftm_info->reqState = FTM_REQUEST_STATE_INIT;
// 		_up_sema(&ftm_info->FTMpreparingEvent);
//     }
//     else if ( resp_state == FTM_RESP_STATE_ON_FTM_REQ )
//     {
//       //padapter->hal_func.GetHwRegHandler(padapter, HW_VAR_FREERUN_CNT, (char *)&CurTsf);

//       if ( !ftm_info->FTMRespondingParam[0].BurstCount )
//       {
//     	ftm_send_ftm_frame(padapter, 0, ftm_info->FTMReqSTAEntry[0].MacAddr);
//         ftm_info->FTMRespondingParam[0].FTMSendCountPerBurst = 1;

//       }
//       if ( ftm_info->FTMRespondingParam[0].FTMSendCountPerBurst >= ftm_info->FTMRespondingParam[0].BurstsNumber )
//     	  return RT_STATUS_SUCCESS;
//       NextTsf = CurTsf + 100 * ftm_info->FTMRespondingParam[0].MinDeltaFTM;
//       ftm_info->FTMRespondingParam[0].NextFTMSendTsf = NextTsf;
//       FTM_SetRespBurstState(padapter, FTM_BURST_STATE_SEND_FTM, NextTsf);
//     }

//     return RT_STATUS_SUCCESS;
//   }

//   return 15; //RT_STATUS_INVALID_STATE;
// }

// int FTM_OnFTMframe(struct halmac_adapter *padapter,  OCTET_STRING *posMpdu)
//  {
// 	int ftm_timeout, len;
// 	char *ftm_frame;
// 	char fup_dia_token;
// 	RT_FTM_BURST_ENTITY *burst_entity;
// 	char bLCIReq;
// 	char bFTMParam;
// 	RT_FTM_SELECT_RESP *sel_resp;
// 	char dialog_token;
// 	_irqL irqL;

// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

// 	bFTMParam = 0;
// 	ftm_frame = posMpdu->Octet;
// 	len = posMpdu->Length;
// 	pr_info("!!![FTM] ---> %s  %d !!!\n", __func__, len);
// 	burst_entity = &ftm_info->pFTMActiveSelectedResp->burstEntity;
// 	if (ftm_info->bFTMSupportByFW == _TRUE) {

// 		dialog_token = ftm_frame[26];
// 		fup_dia_token = ftm_frame[27];

// 		pr_info("!!![FTM] %s dialog_token:%d, fup_dia_token:%d !!!\n", __func__, dialog_token, fup_dia_token);

// 		/*
// 		ftm_timeout = 0;//ftm_Calc_TimeoutValue(padapter, posMpdu, 0x2Cu, &bFTMParam);

// 		if (bFTMParam != 0) {
// 			ftm_info->ftmTimeoutMs = 2 * ftm_timeout;
// 		} else {
// 			ftm_info->ftmTimeoutMs = ftm_timeout;
// 		}
// 		pr_info("!!![FTM] %s ftm_timeout:%d, dt:%d, fpb:%d !!!\n", __func__, ftm_info->ftmTimeoutMs, dialog_token, ftm_info->pFTMActiveSelectedResp->FTMParaInfo.FTMNumPerBust);
// 		*/
// 		/*
// 		if (dialog_token > ftm_info->pFTMActiveSelectedResp->FTMParaInfo.FTMNumPerBust)
// 		{
// 			if (ftm_info->preThreadStateFlag & 0x400)
// 			{
// 				FTM_INFO_LOCK;
// 			    ftm_info->reqState = FTM_REQUEST_STATE_COMPLETE;
// 				FTM_INFO_UNLOCK;
// 				_up_sema(&ftm_info->FTMpreparingEvent);
// 			}
// 			else
// 			{
// 				bFTMParam = 0;
// 			    padapter->hal_func.set_hw_reg_handler(padapter, HW_VAR_DL_FTM_CMD, &bFTMParam);
// 			}
// 		}
// 		*/
// 		return 0;
// 	}

// 	if (posMpdu->Length < 0x1A) {
// 		return 9;
// 	}

// 	/* RA match checked in on_action_public() */

// 	sel_resp = ftm_info->pFTMActiveSelectedResp;

// 	if (memcmp(sel_resp->MacAddr, get_addr2_ptr(ftm_frame), ETH_ALEN))
// 	{
// 		dialog_token = ftm_frame[26];
// 		fup_dia_token = ftm_frame[27];
// 		bLCIReq = ftm_frame[27];

// 		if (!dialog_token) {
// 			FTM_INFO_LOCK;
// 			burst_entity->BurstState = FTM_REQ_BURST_STATE_DONE;
// 			FTM_INFO_UNLOCK;
// 		}

// 		if (ftm_info->sessionState == FTM_SESSION_STATE_NEGOTIATION_WAIT_IFTM) {
// 			ftm_ParsingFTMOptionField(padapter, posMpdu, 0x2C, &bFTMParam, &bLCIReq, &bLCIReq);
// 			if (bFTMParam) {
// 				if (ftm_info->pFTMActiveSelectedResp->targetFTMParaInfo.StatusIndication == 1) {
// 					ftm_StoreToFinalFTMParam(padapter);
// 					FTM_INFO_LOCK;
// 					ftm_info->sessionState = FTM_SESSION_STATE_NEGOTIATION_GET_IFTM;
// 					FTM_INFO_UNLOCK;
// 					_up_sema(&ftm_info->FTMSessionEvent);

// 					if (ftm_info->pFTMActiveSelectedResp->FTMParaInfo.ASAP == 1) {
// 						++ftm_info->pFTMActiveSelectedResp->burstEntity.recvFTMnum;
// 						ftm_info->pFTMActiveSelectedResp->burstEntity.lastDialogToken = dialog_token;
// 					}
// 				}
// 				return 0;
// 			}
// 		} else {
// 			if (ftm_info->sessionState != FTM_SESSION_STATE_MEASUREMENT
// 					&& ftm_info->sessionState != FTM_SESSION_STATE_MEASUREMENT_ASAP1) {
// 				return 0;
// 			}

// 			if (burst_entity->BurstState != FTM_REQ_BURST_STATE_RECV_FTM) {
// 				return 0;
// 			}

// 			++burst_entity->recvFTMnum;
// 			pr_info("!!![FTM] %s Burst: %d !!!\n", __func__, burst_entity->recvFTMnum);
// 			if (fup_dia_token <= burst_entity->lastDialogToken) {
// 				burst_entity->lastDialogToken = dialog_token;
// 				return 0;
// 			}
// 		}
// 	}

// 	return 12;
// }

// int FTM_RecvFTMParamIE(struct halmac_adapter *Adapter, OCTET_STRING *pOsBuffer, char bOnResp)
// {

//   u8 *buffer;
//   int size;
//   RT_FTM_SCHEDULE_PARA *sch_param;
//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(Adapter);

//   buffer = pOsBuffer->Octet;
//   size =   pOsBuffer->Length;
//   if ( size == 9 )
//   {
//     if ( bOnResp )
//     {
//       sch_param = &ftm_info->pFTMActiveSelectedResp->targetFTMParaInfo;
//       sch_param->StatusIndication = buffer[0] & 3;
//     }
//     else
//     {
//       sch_param = &ftm_info->FTMReqSTAEntry[0].FTMParaInfo;
//     }
//     sch_param->BurstExpoNum   =  buffer[1] & 0xF;
//     sch_param->BurstTimeout   =  buffer[1] >> 4;
//     sch_param->MinDeltaFTM    =  buffer[2];
//     sch_param->ASAPCapable    = (buffer[5] >> 1) & 1;
//     sch_param->ASAP           = (buffer[5] >> 2) & 1;
//     sch_param->FTMNumPerBust  =  buffer[5] >> 3;
//     sch_param->FTMFormatAndBW =  buffer[6] >> 2;
//     sch_param->PartialTSFStartOffset = RTW_GET_BE16(buffer + 3);
//     sch_param->BurstPeriod           = RTW_GET_BE16(buffer + 7);
//     pr_info("!!![FTM] %s Received Req: FTMNumPerBust:%d !!!\n", __func__, sch_param->FTMNumPerBust);
//   }

//   return size;
// }

// void FTM_ParsingFTMRequestOptionField(struct halmac_adapter *padapter, OCTET_STRING *posMpdu, unsigned int ElementStartOffset, char *bFTMParam, char *bLCIReq, char *bLocCivicReq)
// {
//   RT_DOT11_IE *pDot11IE;
//   RT_DOT11_IE Dot11IE;
//   u8 CurrID;

//   OCTET_STRING NeighborRptIE;

//   if ( HasNextIE(posMpdu, ElementStartOffset) )
//   {
//     do
//     {
//       pDot11IE = AdvanceToNextIE(&Dot11IE, posMpdu, &ElementStartOffset);
//       CurrID = pDot11IE->Id;
//       NeighborRptIE.Octet = pDot11IE->Content.Octet;
//       NeighborRptIE.Length = pDot11IE->Content.Length;
//       pr_info("!!![FTM] %s CurrID:%x !!!\n", __func__, CurrID);
//       if ( CurrID == (u8)EID_FTMParams )
//       {
//         FTM_RecvFTMParamIE(padapter, &NeighborRptIE, 0);
//         *bFTMParam = 1;
//       }
//     }
//     while ( HasNextIE(posMpdu, ElementStartOffset) );
//   }
// }

// void ftm_ParsingFTMOptionField(struct halmac_adapter *padapter, OCTET_STRING *posMpdu, unsigned int ElementStartOffset, char *bFTMParam, char *bLCIReq, char *bLocCivicReq)
// {
//   RT_DOT11_IE *pDot11IE;
//   RT_DOT11_IE Dot11IE;
//   int CurrID;

//   OCTET_STRING NeighborRptIE;

//   if ( HasNextIE(posMpdu, ElementStartOffset) )
//   {
// 	do
// 	{
// 	  pDot11IE = AdvanceToNextIE(&Dot11IE, posMpdu, &ElementStartOffset);
// 	  CurrID = pDot11IE->Id;
// 	  NeighborRptIE.Octet = pDot11IE->Content.Octet;
// 	  NeighborRptIE.Length = pDot11IE->Content.Length;
// 	  if ( CurrID == EID_FTMParams )
// 	  {
// 		FTM_RecvFTMParamIE(padapter, &NeighborRptIE, 1);
// 		*bFTMParam = 1;
// 	  }
// 	}
// 	while ( HasNextIE(posMpdu, ElementStartOffset) );
//   }
// }

// int HasNextIE(OCTET_STRING *posMpdu, int Offset)
// {
//   unsigned int len;
//   int result;

//   len = posMpdu->Length;
//   if ( Offset + 2 <= len )
//     result = len >= Offset + (unsigned int)posMpdu->Octet[Offset + 1] + 2;
//   else
//     result = 0;
//   return result;
// }

// RT_DOT11_IE *AdvanceToNextIE(RT_DOT11_IE *Ie, OCTET_STRING *posMpdu, unsigned int *pOffset)
// {
// 	Ie->Id = *(posMpdu->Octet + *pOffset);
// 	Ie->Content.Length = *(posMpdu->Octet + *pOffset + 1);
// 	Ie->Content.Octet = posMpdu->Octet + *pOffset + 2;

// 	*pOffset += (2 + *(posMpdu->Octet + *pOffset + 1));
// 	return Ie;
// }

// int ftm_Calc_TimeoutValue(struct halmac_adapter *padapter, OCTET_STRING *posMpdu, unsigned int ElementStartOffset, char *bFTMParam)
// {

//   uint16_t burst_period; // si@1
//   RT_DOT11_IE *pIe; // eax@2
//   signed int bursts_num; // ebx@11
//   int result; // eax@12
//   RT_DOT11_IE Ie; // [sp+Ch] [bp-20h]@2
//   unsigned int pOffset; // [sp+28h] [bp-4h]@1
//   unsigned int burst_expo; // [sp+3Ch] [bp+10h]@1

//   burst_period = 0;
//   pOffset = ElementStartOffset;
//   burst_expo = 0;
//   *bFTMParam = 0;
//   if ( HasNextIE(posMpdu, ElementStartOffset) )
//   {
//     while ( 1 )
//     {
//       pIe = AdvanceToNextIE(&Ie, posMpdu, &pOffset);
//       if ( pIe->Id == EID_FTMParams )
//       {
//     	burst_period = RTW_GET_BE16(&pIe->Content.Octet[7]);
//     	burst_expo = pIe->Content.Octet[1] & 0xF;
//     	*bFTMParam = 1;
//         break;
//       }
//       if ( !HasNextIE(posMpdu, pOffset) )
//       {
//         break;
//       }
//     }
//   }

//   if ( *bFTMParam == _TRUE )
//   {
//     bursts_num = 1 << burst_expo;
//     if ( burst_period )
//       result = 100 * burst_period * bursts_num;
//     else
//       result = 1000 * bursts_num;
//   }
//   else
//   {
//     result = 0;
//   }
//   return result;
// }




// void FTM_SetRespBurstState(struct halmac_adapter *padapter, FTM_BURST_STATE BurstStateToSet, uint64_t Timeout)
// {
//   ACTION_TIMER_ITEM pInputDataItem; // [sp+Ch] [bp-28h]@5

//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);
//   ftm_info->FTMRespondingParam[0].BurstState = BurstStateToSet;
//   memset(&pInputDataItem, 0, 0x20u);
//   pInputDataItem.pContext = padapter;
//   pInputDataItem.ActionType = ACTION_TYPE_FTM;
//   //pInputDataItem.CallbackFunc = FTM_ActionTimerCallback;
//   pInputDataItem.usTimeout = Timeout;
//   //ActionTimerRegisterActionItem(padapter, &pInputDataItem, ftm_info->FTMActionTimer);
// }

// #if 0
// void ConstructMaFrameHdr(
// 	struct halmac_adapter		Adapter,
// 	u8*			pAddr,
// 	u8			Category,
// 	u8			Action,
// 	OCTET_STRING*		posMaFrame
// 	)
// {
// 	u8*				pMaPartial = posMaFrame->Octet;
// 	u8				MaHdr[5];
// 	OCTET_STRING			osTemp;

// 	SET_80211_HDR_FRAME_CONTROL(pMaPartial,0);
// 	SET_80211_HDR_TYPE_AND_SUBTYPE(pMaPartial,Type_Action);
// 	SET_80211_HDR_DURATION(pMaPartial,0);
// 	SET_80211_HDR_FRAGMENT_SEQUENCE(pMaPartial,0);
// 	SET_80211_HDR_ADDRESS1(pMaPartial, pAddr);
// 	SET_80211_HDR_ADDRESS2(pMaPartial, Adapter->CurrentAddress);
// 	SET_80211_HDR_ADDRESS3(pMaPartial, Adapter->MgntInfo.Bssid);
// 	posMaFrame->Length = sMacHdrLng;

// 	MaHdr[0] = Category;
// 	MaHdr[1] = Action;

// 	FillOctetString(osTemp, MaHdr, 2);
// 	PacketAppendData(posMaFrame, osTemp);

// }
// #endif

// void ftm_StartFTMResponderRoleByFW(struct halmac_adapter *adapter)
// {
// 	_irqL irqL;
// 	char FtmCmd;
// 	int i;
// 	u32 reg32_1, reg32_2;
// 	struct mlme_priv *pmlmepriv = &adapter->mlmepriv;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(adapter);
// 	FTM_REQUEST_STATE ReqState = ftm_info->reqState;

// 	pr_info("!!![FTM] ---> %s !!!\n", __func__);

// 	ftm_info->bInitiator = _FALSE;
// 	ftm_info->bResponder = _TRUE;
// 	ftm_info->Role = FTM_RESPONDER;
// 	switch (ftm_info->reqState)
// 	{
// 	case FTM_REQUEST_STATE_INIT:
// 		pr_info("!!![FTM] RESP %s FTM_REQUEST_STATE_INIT !!!\n", __func__);
// 		ftm_SetFixRequester(adapter);
// 		//rtw_set_scan_deny(adapter, 60000);
// 		ATOMIC_SET(&pmlmepriv->set_scan_deny, 1);
// 		if (check_fwstate(pmlmepriv, _FW_UNDER_SURVEY))
// 		{
// 			FTM_INFO_LOCK;
// 			ftm_info->reqState = FTM_REQUEST_STATE_SCAN;
// 			FTM_INFO_UNLOCK;
// 		}
// 		else
// 		{
// 			FTM_INFO_LOCK;
// 			ftm_info->reqState = FTM_REQUEST_STATE_SCAN_COMPLETE;
// 			FTM_INFO_UNLOCK;
// 		}
// 		_up_sema(&ftm_info->FTMpreparingEvent);
// 		break;
// 	case FTM_REQUEST_STATE_SCAN_COMPLETE:
// 		pr_info("!!![FTM] RESP %s FTM_REQUEST_STATE_SCAN_COMPLETE !!!\n", __func__);
// 		FTM_INFO_LOCK;
// 		ftm_info->reqState = FTM_REQUEST_STATE_DL_RSVD_PAGE;
// 		FTM_INFO_UNLOCK;
// 		_up_sema(&ftm_info->FTMpreparingEvent);
// 		break;
// 	case FTM_REQUEST_STATE_DL_RSVD_PAGE:
// 		pr_info("!!![FTM] RESP %s FTM_REQUEST_STATE_DL_RSVD_PAGE !!!\n", __func__);
// 		ftm_SetFixFTMReqParam(adapter);
// 		ftm_info->FTMFrame.Length = 0;
// 		ftm_info->FTMFrame.Octet = ftm_info->FTMFrameBuf;
// 		construct_ftm_frame_buffer(adapter, 1, ftm_info->FTMFrameBuf, (unsigned int *) &ftm_info->FTMFrame.Length);
// 		FTM_SelectBWandDataRate( ftm_info->FTMReqSTAEntry[0].FTMParaInfo.FTMFormatAndBW, &ftm_info->ftmBandWidth, &ftm_info->ftmDataRate);
// 		ftm_info->bNeedToDLFTMpkts = 1;
// 		rtw_hal_set_hwreg(adapter, HW_VAR_DL_FTM_RELATED_FRAME, 0);
// 		ftm_info->bNeedToDLFTMpkts = 0;

// 		FTM_INFO_LOCK;
// 		ftm_info->reqState = FTM_REQUEST_STATE_SET_H2C_CMD;
// 		FTM_INFO_UNLOCK;
// 		_up_sema(&ftm_info->FTMpreparingEvent);
// 		break;
// 	case FTM_REQUEST_STATE_SET_H2C_CMD:
// 		{
// 	      int idx, timeout;
// 	      u8 bw, dr, center_freq;
// 		  pr_info("!!![FTM] RESP %s FTM_REQUEST_STATE_SET_H2C_CMD !!!\n", __func__);
// 		  /*
// 		  if (pmlmepriv->bScanInProcess == _TRUE)
// 		  {

// 		  }
// 		  rtw_set_scan_deny(adapter, 5000);
// 		  */
// 	      ftm_info->mlmeScanInterval = pmlmepriv->auto_scan_int_ms;
// 	      pmlmepriv->auto_scan_int_ms = 0;

// 	      checkSC(adapter);
// 	      bw = 0; dr = 12;
// 	      FTM_SelectBWandDataRate( ftm_info->FTMReqSTAEntry[0].FTMParaInfo.FTMFormatAndBW, &bw, &dr);
// 	      ftm_info->ftmDataRate = dr;
// 	      adapter->hal_func.set_chnl_bw_handler(adapter, ftm_info->FTMTargetList.chnl[3], bw,
// 	    		  	  	  	  	  HAL_PRIME_CHNL_OFFSET_DONT_CARE,
// 								  HAL_PRIME_CHNL_OFFSET_DONT_CARE);

// 	      timeout = 100;
// 	      center_freq = rtw_get_center_ch(ftm_info->FTMTargetList.chnl[3], bw, HAL_PRIME_CHNL_OFFSET_DONT_CARE);
// 		  while (1) {
// 			  u32 _ccf = phy_query_rf_reg(adapter, RF_PATH_A, 0x18, 0xFF);
// 			  u8 _bw = rtw_read8(adapter, 0x8AC);
// 			  if ((_ccf == center_freq) && ((_bw & 3) == bw)) {
// 				  pr_info("!!![FTM] %s ABW %d, BW %d, ACCF %d, CCF %d !!!\n", __func__, _bw, bw, _ccf, center_freq);
// 				  break;
// 			  }
// 			  rtw_udelay_os(1000);
// 			  if (!--timeout) {
// 				  pr_info("!!![FTM] %s ABW %d, BW %d, ACCF %d, CCF %d !!!\n", __func__, _bw, bw, _ccf, center_freq);
// 				  break;
// 			  }
// 		  }
// 	      checkSC(adapter);

// 	      FtmCmd = 1;
// 		  rtw_hal_set_hwreg(adapter, HW_VAR_DL_FTM_CMD, &FtmCmd);

// 		  FTM_INFO_LOCK;
// 		  ftm_info->reqState = FTM_REQUEST_STATE_WAITING_C2H_CMD;
// 		  FTM_INFO_UNLOCK;
// 		  _up_sema(&ftm_info->FTMpreparingEvent);
// 		  rtw_rm_ftm_responder_ready(adapter);
// 		}
// 		break;
// 	case FTM_REQUEST_STATE_WAITING_C2H_CMD:
// 		pr_info("!!![FTM] RESP %s FTM_REQUEST_STATE_WAITING_C2H_CMD !!!\n", __func__);
// 	    //reg32_1 = rtw_read32(adapter, REG_FREERUN_CNT_8822B);
// 	    ////rtw_udelay_os(100);
// 	    //reg32_2 = rtw_read32(adapter, REG_FREERUN_CNT_8822B);
// 	    //pr_info("!!![FTM] %s === FREERUN_CNT for 100usec = %u !!!\n", __func__, reg32_2-reg32_1);
// 		break;
// 	case FTM_REQUEST_STATE_COMPLETE:
// 		pr_info("!!![FTM] RESP %s FTM_REQUEST_STATE_COMPLETE !!!\n", __func__);
// 		pmlmepriv->auto_scan_int_ms = ftm_info->mlmeScanInterval;
// 		rtw_mdelay_os(10);
// 		FtmCmd = 0;
// 		rtw_hal_set_hwreg(adapter, HW_VAR_DL_FTM_CMD, &FtmCmd);
// 		ftm_info->preThreadStateFlag &= ~(0x400);
// 		break;
// 	default:
// 		break;
// 	}
// }

// void ftm_StartFTMRequestRoleByFW(struct halmac_adapter *padapter, char *pscanCnt)
// {
// 	_irqL irqL;
// 	char FtmCmd;
// 	int i;
// 	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

// 	FTM_REQUEST_STATE ReqState = ftm_info->reqState;
// 	ftm_info->bInitiator = _TRUE;
// 	ftm_info->bResponder = _FALSE;
// 	pr_info("!!![FTM] ---> %s !!!\n", __func__);
//     switch (ReqState)
//     {
// 	  case FTM_REQUEST_STATE_INIT:
// 		  FTM_INFO_LOCK;
// 	      ftm_info->reqState = FTM_REQUEST_STATE_SEARCH_CANDIDATE_LIST;
// 		  FTM_INFO_UNLOCK;
// 		  pr_info("!!![FTM] %s FTM_REQUEST_STATE_INIT !!!\n", __func__);
// 		  _up_sema(&ftm_info->FTMpreparingEvent);
// 		  break;
// 	  case FTM_REQUEST_STATE_INIT_11K:
// 		  FTM_INFO_LOCK;
// 		  ftm_info->reqState = FTM_REQUEST_STATE_DL_RSVD_PAGE;
// 		  FTM_INFO_UNLOCK;
// 		  pr_info("!!![FTM] %s FTM_REQUEST_STATE_INIT_11K !!!\n", __func__);
// 		  ftm_Set11kResponder(padapter);
// 		  _up_sema(&ftm_info->FTMpreparingEvent);
// 		  break;
// 	  case FTM_REQUEST_STATE_SEARCH_CANDIDATE_LIST:
// 		  FTM_INFO_LOCK;
// 	      ftm_info->reqState = FTM_REQUEST_STATE_SCAN;
// 		  FTM_INFO_UNLOCK;
// 		  pr_info("!!![FTM] %s FTM_REQUEST_STATE_SEARCH_CANDIDATE_LIST !!!\n", __func__);
// 		  _up_sema(&ftm_info->FTMpreparingEvent);
// 		  break;
// 	  //case FTM_REQUEST_STATE_FLUSH_ALL_LISTS:
// 	  case FTM_REQUEST_STATE_SCAN:
// 		  //FTM_ScanPhase(padapter);
// 		  FTM_INFO_LOCK;
// 	      ftm_info->reqState = FTM_REQUEST_STATE_SCAN_COMPLETE;
// 		  FTM_INFO_UNLOCK;
// 		  pr_info("!!![FTM] %s FTM_REQUEST_STATE_SCAN !!!\n", __func__);
// 		  _up_sema(&ftm_info->FTMpreparingEvent);
// 		  break;
// 	  case FTM_REQUEST_STATE_SCAN_COMPLETE:
// 		  FTM_INFO_LOCK;
// 	      ftm_info->reqState = FTM_REQUEST_STATE_DL_RSVD_PAGE;
// 		  FTM_INFO_UNLOCK;
// 		  pr_info("!!![FTM] %s FTM_REQUEST_STATE_SCAN_COMPLETE !!!\n", __func__);
// 		  ftm_SetFixResponder(padapter, ftm_info->FTMTargetList.NumOfResponder);
// 		  _up_sema(&ftm_info->FTMpreparingEvent);
// 		  break;
// 	  case FTM_REQUEST_STATE_DL_RSVD_PAGE:
// 			{
// 			pr_info("!!![FTM] %s FTM_REQUEST_STATE_DL_RSVD_PAGE !!!\n", __func__);
// 			/*
// 			if (ftm_info->bFTMParamFromFile == 1) {
// 				//FTM_SelectRspReadFile(padapter);
// 			} else {
// 				ftm_SetFixFTMParam(padapter);
// 			}
// 			*/
// 			ftm_DecideFTMFormatAndBWValue(padapter);
// 			/*
// 			if (ftm_info->bFTMParaFromSigma == 1) {
// 				//ftm_UpdateFTMParaFromSigma(padapter);
// 			}
// 			*/
// 			for (i = 0; i < 3; i++) {
// 				if (ftm_info->FTMSelectedRespList[i].bValid == 1) {
// 					construct_ftm_req_buffer(padapter, 1,
// 							ftm_info->FTMSelectedRespList[i].MacAddr,
// 							&ftm_info->FTMSelectedRespList[i].FTMParaInfo,
// 							ftm_info->FTMSelectedRespList[i].iFTMRFrameBuf,
// 							(u32*)&ftm_info->FTMSelectedRespList[i].iFTMRFrame.Length);
// 				}
// 				ftm_info->FTMSelectedRespList[i].bFTMdone = 0;
// 			}

// 			FTM_ConstructFTMReqInfo(padapter);

// 			ftm_info->bNeedToDLFTMpkts = 1;
// 			padapter->hal_func.set_hw_reg_handler(padapter, HW_VAR_DL_FTM_RELATED_FRAME, 0);
// 			ftm_info->bNeedToDLFTMpkts = 0;

// 			//FTM_INFO_LOCK;
// 			ftm_info->reqState = FTM_REQUEST_STATE_SET_H2C_CMD;
// 			//FTM_INFO_UNLOCK;
// 			_up_sema(&ftm_info->FTMpreparingEvent);
// 			}
// 		  break;
// 	  case FTM_REQUEST_STATE_SET_H2C_CMD:
// 	    {
// 	      int i, idx, timeout;
// 	      u8 bw, dr, center_freq;
// 	      pr_info("!!![FTM] %s FTM_REQUEST_STATE_SET_H2C_CMD !!!\n", __func__);
// 	      //bNoScanInFTMBurstInProgess = 1;
// 	      ftm_info->mlmeScanInterval = pmlmepriv->auto_scan_int_ms;
// 	      pmlmepriv->auto_scan_int_ms = 0;

// 	      for (idx = 0; idx < 3; idx++)
// 	      {
// 	        if ( ftm_info->FTMSelectedRespList[idx].bValid == _TRUE && ftm_info->FTMSelectedRespList[idx].bFTMdone != 1 )
// 	          break;
// 	      }
// 	      if (idx >= 3)
// 	      {
// 	    	  FTM_INFO_LOCK;
// 		      ftm_info->reqState = FTM_REQUEST_STATE_COMPLETE;
// 		      FTM_INFO_UNLOCK;
// 		      pr_info("!!![FTM] %s Incorrect responder idx !!!\n", __func__);
// 		      _up_sema(&ftm_info->FTMpreparingEvent);
// 	    	  break;
// 	      }
// 	      for(i = 0; i < 16; i++)
// 	      {
// 	      	ftm_info->FTMSelectedRespList[idx].t3List[i] = 0;
// 	      	ftm_info->FTMSelectedRespList[idx].t4List[i] = 0;
// 	      	ftm_info->FTMSelectedRespList[idx].bcList[i] = 0;
// 	      }
// 	      ftm_info->FTMTargetList.bValid[idx] = 0;
// 	      ftm_info->FTMSelectRspNum = idx;
// 	      ftm_info->FTMSelectedRespList[idx].burstEntity.BurstCnt = 0;
// 	      checkSC(padapter);
// 	      bw = 0; dr = 12;
// 	      FTM_SelectBWandDataRate( ftm_info->FTMSelectedRespList[idx].FTMParaInfo.FTMFormatAndBW, &bw, &dr);
// 	      ftm_info->ftmDataRate = dr;
// 	      padapter->hal_func.set_chnl_bw_handler(padapter, ftm_info->FTMSelectedRespList[idx].ChannelNum, bw,
// 	    		  	  	  	  	  ftm_info->FTMSelectedRespList[idx].ExtChnlOffsetOf40MHz,
// 								  HAL_PRIME_CHNL_OFFSET_DONT_CARE);
// 	      dr = 0;

// 	      if (bw > HALMAC_BW_80)
// 	      {
// 	    	  pr_info("!!![FTM] %s Unsupported BW !!!\n", __func__);
// 	      }

// 	      timeout = 100;
// 	      center_freq = rtw_get_center_ch(ftm_info->FTMSelectedRespList[idx].ChannelNum, bw, ftm_info->FTMSelectedRespList[idx].ExtChnlOffsetOf40MHz);
// 		  while (1) {
// 			  u32 _ccf = phy_query_rf_reg(padapter, RF_PATH_A, 0x18, 0xFF);
// 			  u8 _bw = rtw_read8(padapter, 0x8AC);
// 			  if ((_ccf == center_freq) && ((_bw & 3) == bw)) {
// 				  pr_info("!!![FTM] %s ABW %d, BW %d, ACCF %d, CCF %d !!!\n", __func__, _bw, bw, _ccf, center_freq);
// 				  break;
// 			  }
// 			  rtw_udelay_os(1000);
// 			  if (!--timeout) {
// 				  pr_info("!!![FTM] %s ABW %d, BW %d, ACCF %d, CCF %d !!!\n", __func__, _bw, bw, _ccf, center_freq);
// 				  break;
// 			  }
// 		  }
// 	      /*
// 	      if (timeout <= 0)
// 	      {
// 	    	  FTM_INFO_LOCK;
// 		      ftm_info->reqState = FTM_REQUEST_STATE_COMPLETE;
// 		      FTM_INFO_UNLOCK;
// 		      pr_info("!!![FTM] %s Channel timeout !!!\n", __func__);
// 		      _up_sema(&ftm_info->FTMpreparingEvent);
// 	    	  break;
// 	      }
// 	      */
// 	      checkSC(padapter);
// 	      ftm_info->FTMSelectedRespList[idx].bFTMdone = 1;

// 	      rtw_ps_deny(padapter, PS_DENY_OTHERS);
// 	      rtw_pm_set_lps(padapter, PS_MODE_ACTIVE);

//           FtmCmd = 1;
// 	      padapter->hal_func.set_hw_reg_handler(padapter, HW_VAR_DL_FTM_CMD, &FtmCmd);
// 	      ftm_info->reqState = FTM_REQUEST_STATE_WAITING_C2H_CMD;
// 	      _up_sema(&ftm_info->FTMpreparingEvent);
// 	      ftm_info->curMicroTime = rtw_get_current_time();
// 	    }
// 		  break;
// 	  case FTM_REQUEST_STATE_WAITING_C2H_CMD:
// 		  pr_info("!!![FTM] %s FTM_REQUEST_STATE_WAITING_C2H_CMD !!!\n", __func__);
// 	      if (((rtw_get_current_time() - ftm_info->curMicroTime)/1024) > 300000 )
// 	      {
// 	    	FTM_INFO_LOCK;
// 	        ftm_info->reqState = FTM_REQUEST_STATE_COMPLETE;
// 	        FTM_INFO_UNLOCK;
// 	        _up_sema(&ftm_info->FTMpreparingEvent);
// 	      }
// 		  break;
// 	  case FTM_REQUEST_STATE_GET_C2H_CMD:
// 	  case FTM_REQUEST_STATE_START_FTM_SESSION:
// 	  case FTM_REQUEST_STATE_WAIT_FTM_SESSION_END:
// 		  break;
// 	  case FTM_REQUEST_STATE_FTM_SESSION_END:
// 		  pr_info("!!![FTM] %s FTM_REQUEST_STATE_FTM_SESSION_END !!!\n", __func__);
// 	      {
// 	    	FTM_INFO_LOCK;
// 	        ftm_info->reqState = FTM_REQUEST_STATE_COMPLETE;
// 	        FTM_INFO_UNLOCK;
// 	        rtw_mdelay_os(5000);
// 	        _up_sema(&ftm_info->FTMpreparingEvent);
// 	      }
// 		  break;
// 	  case FTM_REQUEST_STATE_COMPLETE:
// 	  {
// 		  int idx;
// 		  pr_info("!!![FTM] %s FTM_REQUEST_STATE_COMPLETE !!!\n", __func__);
// 		  // bNoScanInFTMBurstInProgess = 0
// 		  pmlmepriv->auto_scan_int_ms = ftm_info->mlmeScanInterval;
// 		  //rtw_mdelay_os(50);
//           FtmCmd = 0;
// 	      padapter->hal_func.set_hw_reg_handler(padapter, HW_VAR_DL_FTM_CMD, &FtmCmd);

// 	      rtw_ps_deny_cancel(padapter, PS_DENY_OTHERS);

// 	      ftm_process_range(padapter, ftm_info->FTMSelectRspNum, &ftm_info->FTMSelectedRespList[(int)ftm_info->FTMSelectRspNum]);

// 	      rtl8822b_mac_verify(padapter);

// 	      for (idx = 0; idx < 3; idx++)
// 	      {
// 	        if ( (ftm_info->FTMSelectedRespList[idx].bValid == 1) && (ftm_info->FTMSelectedRespList[idx].bFTMdone != 1) )
// 	        {
// 	        	pr_info("!!![FTM] %s =========== Next request: %d!!!\n", __func__, idx);
// 	            ++ftm_info->FTMReqFrameStartLoc;
// 	            ++ftm_info->FTMReqInfoStartLoc;
// 	            ftm_info->reqState = FTM_REQUEST_STATE_SET_H2C_CMD;
// 	            _up_sema(&ftm_info->FTMpreparingEvent);
// 	            break;
// 	        }
// 	      }
// 	      if (idx >= 3)
// 	      {
// 			  ftm_info->preThreadStateFlag &= ~(0x400);
// 		  }
// 		  break;
// 	  }
// 	  default:
// 	      break;
// 	 }

// }

// void ftm_process_range(struct halmac_adapter *adapter, int idx, RT_FTM_SELECT_RESP *sel_resp)
// {
// 	_irqL irqL;
// 	int i;
// 	u32 distance;
// 	u32 cnt = 0, avgt4 = 0, avgt3 = 0, rtt = 0;

// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(adapter);
// 	int bcnt = ftm_info->FTMSelectedRespList[idx].burstEntity.BurstCnt;

// 	pr_info("!!![FTM] ---> %s !!!\n", __func__);

// 	for(i = 0; i < bcnt; i++)
// 		pr_info("!!![FTM] %s == [%d] ================ timing t3:%d, t4:%d !!!\n",
// 				__func__,
// 				idx,
// 				ftm_info->FTMSelectedRespList[idx].t3List[i],
// 				ftm_info->FTMSelectedRespList[idx].t4List[i]
// 				);

// 	for(i = 0; i < bcnt-1; i++)
// 	{
// 		if (ftm_info->FTMSelectedRespList[idx].t4List[i+1] != 0)
// 		{
// 		avgt4 += ftm_info->FTMSelectedRespList[idx].t4List[i+1];
// 		avgt3 += ftm_info->FTMSelectedRespList[idx].t3List[i];
// 		rtt += ftm_info->FTMSelectedRespList[idx].t4List[i+1] - ftm_info->FTMSelectedRespList[idx].t3List[i];
// 		cnt++;
// 		}
// 	}
// 	/*
// 	rtt = (bcnt > 2) ? rtt / (bcnt-2) : 0;
// 	avgt4 = (bcnt > 2) ? avgt4 / (bcnt-2) : 0;
// 	avgt3 = (bcnt > 2) ? avgt3 / (bcnt-2) : 0;
// 	*/
// 	rtt =   cnt ? rtt   / cnt : 0;
// 	avgt4 = cnt ? avgt4 / cnt : 0;
// 	avgt3 = cnt ? avgt3 / cnt : 0;

// 	distance = (rtt-261375)/2;
// 	if (ftm_info->FTMSelectedRespList[idx].burstEntity.BurstCnt == 0)
// 	{
// 		if (ftm_info->hang == 0xE0)
// 			avgt4 = avgt3 = 123;
// 		distance = 0;
// 	}
// 	if (ftm_info->FTMSelectedRespList[idx].b11k == 1) {
// 		// Send event to RM 11k state machine
// 		rtw_rm_ftm_measurement_completed(adapter, rtw_get_passing_time_ms(0), 
// 			distance, ftm_info->FTMSelectedRespList[idx].MacAddr);
// 	}
// 	else {

// 		FTM_INFO_LOCK;
// 		ftm_info->FTMTargetList.timestamp[idx] = rtw_get_passing_time_ms(0);
// 		ftm_info->FTMTargetList.t4[idx] = avgt4;
// 		ftm_info->FTMTargetList.t3[idx] = avgt3;
// 		ftm_info->FTMTargetList.range[idx] = distance;//rtt;
// 		ftm_info->FTMTargetList.bValid[idx] = 1;
// 		FTM_INFO_UNLOCK;
// 	}
// 	//distance = (((rtt-261375)/2)*100)/333;

// 	pr_info("!!![FTM] %s ======================== %d, %x >> Dist: %d.%d !!!\n",
// 					__func__,
// 					ftm_info->FTMSelectedRespList[idx].burstEntity.BurstCnt,ftm_info->hang,
// 					/*((rtt-261375)/2),*/ distance/100, distance%100);
// }

// void ftm_StartFTMResponderRoleByDriver(struct halmac_adapter *adapter)
// {
// 	pr_info("!!![FTM] ---> %s !!!\n", __func__);
// }

// void ftm_StartFTMRequestRoleByDriver(struct halmac_adapter *padapter, char *pscanCnt)
// {
// 	pr_info("!!![FTM] ---> %s !!!\n", __func__);
// }

// void ftm_set_EOME_status(struct halmac_adapter *padapter)
// {
// 	_irqL irqL;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

// 	pr_info("!!![FTM] ---> %s !!!\n", __func__);

// 	FTM_INFO_LOCK;
//     ftm_info->reqState = FTM_REQUEST_STATE_COMPLETE;
// 	FTM_INFO_UNLOCK;
// 	_up_sema(&ftm_info->FTMpreparingEvent);

// }

// void ftm_set_timing_measure(struct halmac_adapter *padapter, u8 burst_num, u32 t1, u32 t2, u32 t3, u32 t4)
// {
// 	_irqL irqL;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);
// 	int idx = ftm_info->FTMSelectRspNum;
// 	int burst = ftm_info->FTMSelectedRespList[idx].burstEntity.BurstCnt;

// 	pr_info("!!![FTM] ---> %s [%d], %d !!!\n", __func__, burst_num, ftm_info->FTMSelectedRespList[idx].FTMParaInfo.FTMNumPerBust);
// 	if((burst == 0) && (burst_num > ftm_info->FTMSelectedRespList[idx].FTMParaInfo.FTMNumPerBust))
// 		return;
// 	if ((burst < 16) /*&& (ftm_info->reqState == FTM_REQUEST_STATE_WAITING_C2H_CMD)*/)
// 	{
// 		//FTM_INFO_LOCK;
// 		ftm_info->FTMSelectedRespList[idx].t3List[burst] = t3;
// 		ftm_info->FTMSelectedRespList[idx].t4List[burst] = t4;
// 		ftm_info->FTMSelectedRespList[idx].bcList[burst] = burst_num;
// 		//FTM_INFO_UNLOCK;
// 		ftm_info->FTMSelectedRespList[idx].burstEntity.BurstCnt++;
// 	}


// }

// #if 0
// void FTM_ActionTimerCallback(ACTION_TIMER_ITEM *const pOneShotActionItem)
// {
//   struct halmac_adapter *padapter; // edi@1
//   unsigned int burst_state; // eax@5
//   char dialogToken; // al@27
//   uint64_t nextTSF, currTSF; // [sp+Ch] [bp-8h]@1
//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);


//   padapter = (struct halmac_adapter *)pOneShotActionItem->pContext;

//   currTSF = 0;

//   burst_state = ftm_info->FTMRespondingParam[0].BurstState;
//   if ( burst_state == FTM_BURST_STATE_START)
//   {
//     FTM_BurstStart(padapter);
//     return;
//   }
//   if ( burst_state == FTM_BURST_STATE_TIMEOUT )
//   {
//     if ( ftm_info->FTMRespondingParam[0].FTMSendCountPerBurst < ftm_info->FTMRespondingParam[0].FTMsPerBurst)
//     {
//       //LOG
//     }
//     if ( ftm_info->FTMRespondingParam[0].BurstCount == ftm_info->FTMRespondingParam[0].BurstsNumber )
//     {
//         FTM_MeasurementExchangeEnd(padapter);
//         return;
//     }
//   }
//   else if ( burst_state == FTM_BURST_STATE_SEND_FTM )
//   {

//     dialogToken = ftm_info->FTMReqSTAEntry[0].DialogToken;
//     ftm_info->FTMReqSTAEntry[0].FollowUpDialogToken = dialogToken;
//     ftm_info->FTMReqSTAEntry[0].DialogToken = dialogToken + 1;
//     ftm_info->FTMReqSTAEntry[0].TODError = 0;
//     ftm_send_ftm_frame(padapter, 0, ftm_info->FTMReqSTAEntry[0].MacAddr);
//     ++ftm_info->FTMRespondingParam[0].FTMSendCountPerBurst;
//     padapter->hal_func.GetHwRegHandler(padapter, HW_VAR_FREERUN_CNT, (char *)&currTSF);

//     if ( ftm_info->FTMRespondingParam[0].FTMSendCountPerBurst < ftm_info->FTMRespondingParam[0].FTMsPerBurst )
//     {
//       nextTSF = currTSF + 100 * ftm_info->FTMRespondingParam[0].MinDeltaFTM;
//       ftm_info->FTMRespondingParam[0].NextFTMSendTsf = nextTSF;
//       FTM_SetRespBurstState(padapter, FTM_BURST_STATE_SEND_FTM, nextTSF);
//       return;
//     }

//     ftm_info->FTMRespondingParam[0].FTMSendCountPerBurst = 0;
//     if ( ftm_info->FTMRespondingParam[0].BurstTimeout <= 1 )
//     {
//       ftm_info->FTMRespondingParam[0].BurstState = FTM_BURST_STATE_TIMEOUT;
//     }
//     if ( ftm_info->FTMRespondingParam[0].BurstPeriod == 0 )
//     {
//       if ( ftm_info->FTMRespondingParam[0].BurstCount != ftm_info->FTMRespondingParam[0].BurstsNumber )
//       {
//         ftm_info->FTMRespondingParam[0].NextBurstStartTsf = currTSF;
//         ftm_info->FTMRespondingParam[0].BurstState = FTM_BURST_STATE_START;
//         FTM_BurstStart(padapter);
//         return;
//       }
//       FTM_MeasurementExchangeEnd(padapter);
//       return;
//     }
//   }

// }
// #endif

// void FTM_BurstStart(struct halmac_adapter *padapter)
// {
//   unsigned int burst_timeout; // ecx@5
//   uint64_t bp; // rax@13

//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);
//   ++ftm_info->FTMRespondingParam[0].BurstCount;

//   burst_timeout = ftm_info->FTMRespondingParam[0].BurstTimeout;
//   ftm_info->FTMRespondingParam[0].CurBurstStartTsf = ftm_info->FTMRespondingParam[0].NextBurstStartTsf;

//   if ( burst_timeout > 1 )
//   {
//     ftm_info->FTMRespondingParam[0].BurstTimeoutTsf = ftm_info->FTMRespondingParam[0].CurBurstStartTsf + burst_timeout;
//     FTM_SetRespBurstState(padapter, FTM_BURST_STATE_TIMEOUT, ftm_info->FTMRespondingParam[0].BurstTimeoutTsf);
//   }

//   if ( ftm_info->FTMRespondingParam[0].BurstCount < ftm_info->FTMRespondingParam[0].BurstsNumber )
//   {
//     if ( ftm_info->FTMRespondingParam[0].BurstPeriod )
//     {
//       bp = 100000 * ftm_info->FTMRespondingParam[0].BurstPeriod;
//       ftm_info->FTMRespondingParam[0].NextBurstStartTsf = ftm_info->FTMRespondingParam[0].CurBurstStartTsf + bp;

//       FTM_SetRespBurstState(padapter, FTM_BURST_STATE_START, ftm_info->FTMRespondingParam[0].NextBurstStartTsf);
//     }
//   }
//   ftm_info->RespondingState = FTM_RESP_STATE_ON_FTM_REQ;

// }

// void FTM_MeasurementExchangeEnd(struct halmac_adapter *padapter)
// {
//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);
//   ftm_info->RespondingState = FTM_RESP_STATE_MESUREMENT_EXC_END;
// }

// static void ftm_SetFixFTMReqParam(struct halmac_adapter *padapter)
// {
//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

//   ftm_info->FTMReqSTAEntry[0].FTMParaInfo.PartialTSFStartOffset = ftm_common_params.targetFTMParaInfo.PartialTSFStartOffset; // 10;
//   ftm_info->FTMReqSTAEntry[0].FTMParaInfo.MinDeltaFTM           = ftm_common_params.targetFTMParaInfo.MinDeltaFTM;           // 3;//50;
//   ftm_info->FTMReqSTAEntry[0].FTMParaInfo.BurstExpoNum          = ftm_common_params.targetFTMParaInfo.BurstExpoNum;          // 0;
//   ftm_info->FTMReqSTAEntry[0].FTMParaInfo.BurstTimeout          = ftm_common_params.targetFTMParaInfo.BurstTimeout;          // 7;//11;
//   ftm_info->FTMReqSTAEntry[0].FTMParaInfo.ASAPCapable           = ftm_common_params.targetFTMParaInfo.ASAPCapable;           // 0;
//   ftm_info->FTMReqSTAEntry[0].FTMParaInfo.ASAP                  = ftm_common_params.targetFTMParaInfo.ASAP;                  // 0;
//   ftm_info->FTMReqSTAEntry[0].FTMParaInfo.FTMNumPerBust         = ftm_common_params.targetFTMParaInfo.FTMNumPerBust;         // 16;//2;
//   ftm_info->FTMReqSTAEntry[0].FTMParaInfo.FTMFormatAndBW        = ftm_common_params.targetFTMParaInfo.FTMFormatAndBW;        // 13;
//   ftm_info->FTMReqSTAEntry[0].FTMParaInfo.BurstPeriod           = ftm_common_params.targetFTMParaInfo.BurstPeriod;           // 0;
// }
// /*
// static void ftm_SetFixFTMParam(struct halmac_adapter *padapter)
// {
//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

//   ftm_info->FTMSelectedRespList[0].FTMParaInfo.PartialTSFStartOffset = 50;
//   ftm_info->FTMSelectedRespList[0].FTMParaInfo.MinDeltaFTM           = 100;//50;
//   ftm_info->FTMSelectedRespList[0].FTMParaInfo.BurstExpoNum          = 0;
//   ftm_info->FTMSelectedRespList[0].FTMParaInfo.BurstTimeout          = 11;
//   ftm_info->FTMSelectedRespList[0].FTMParaInfo.ASAPCapable           = 0;
//   ftm_info->FTMSelectedRespList[0].FTMParaInfo.ASAP                  = 0;
//   ftm_info->FTMSelectedRespList[0].FTMParaInfo.FTMNumPerBust         = 2;
//   ftm_info->FTMSelectedRespList[0].FTMParaInfo.FTMFormatAndBW        = 9;//13;
//   ftm_info->FTMSelectedRespList[0].FTMParaInfo.BurstPeriod           = 0;
// }
// */
// static void ftm_set_fix_para_info(RT_FTM_SCHEDULE_PARA *pFTMParaInfo)
// {
//     pFTMParaInfo->PartialTSFStartOffset = ftm_common_params.targetFTMParaInfo.PartialTSFStartOffset; // 10;
//     pFTMParaInfo->MinDeltaFTM           = ftm_common_params.targetFTMParaInfo.MinDeltaFTM;           // 3;//50;
//     pFTMParaInfo->BurstExpoNum          = ftm_common_params.targetFTMParaInfo.BurstExpoNum;          // 0;
//     pFTMParaInfo->BurstTimeout          = ftm_common_params.targetFTMParaInfo.BurstTimeout;          // 7;//11;
//     pFTMParaInfo->ASAPCapable           = ftm_common_params.targetFTMParaInfo.ASAPCapable;           // 0;
//     pFTMParaInfo->ASAP                  = ftm_common_params.targetFTMParaInfo.ASAP;                  // 0;
//     pFTMParaInfo->FTMNumPerBust         = ftm_common_params.targetFTMParaInfo.FTMNumPerBust;         // 16;//2;
//     pFTMParaInfo->FTMFormatAndBW        = ftm_common_params.targetFTMParaInfo.FTMFormatAndBW;        // 13;
//     pFTMParaInfo->BurstPeriod           = ftm_common_params.targetFTMParaInfo.BurstPeriod;           // 0;
// }

// static void ftm_SetFixResponder(struct halmac_adapter *padapter, char cnt)
// {
//   int i;
//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

//   for (i = 0; i < 3; i++)
//   {
// 	  memset(&ftm_info->FTMSelectedRespList[i], 0, sizeof(RT_FTM_SELECT_RESP));
//   }
//   for (i = 0; i < cnt; i++)
//   {
// 	  if (ftm_info->FTMTargetList.chnl[i])
// 		  ftm_info->FTMSelectedRespList[i].bValid = 1;
// 	  ftm_info->FTMSelectedRespList[i].ChannelNum = ftm_info->FTMTargetList.chnl[i];
// 	  ftm_info->FTMSelectedRespList[i].bFTMdone = 0;
// 	  ftm_info->FTMSelectedRespList[i].MacId = 0xFF;
// 	  ftm_info->FTMSelectedRespList[i].wirelessmode = ftm_common_params.wirelessmode;
// 	  ftm_info->FTMSelectedRespList[i].bCIVICCap    = ftm_common_params.bCIVICCap;
// 	  ftm_info->FTMSelectedRespList[i].bLCICap      = ftm_common_params.bLCICap;
// 	  ftm_info->FTMSelectedRespList[i].Bandwidth    = ftm_common_params.Bandwidth;
// 	  ftm_info->FTMSelectedRespList[i].ExtChnlOffsetOf40MHz = ftm_common_params.ExtChnlOffsetOf40MHz;
// 	  ftm_info->FTMSelectedRespList[i].ExtChnlOffsetOf80MHz = ftm_common_params.ExtChnlOffsetOf80MHz;

// 	  ftm_info->FTMSelectedRespList[i].MacAddr[0] = ftm_info->FTMTargetList.MacAddr[i][0];
// 	  ftm_info->FTMSelectedRespList[i].MacAddr[1] = ftm_info->FTMTargetList.MacAddr[i][1];
// 	  ftm_info->FTMSelectedRespList[i].MacAddr[2] = ftm_info->FTMTargetList.MacAddr[i][2];
// 	  ftm_info->FTMSelectedRespList[i].MacAddr[3] = ftm_info->FTMTargetList.MacAddr[i][3];
// 	  ftm_info->FTMSelectedRespList[i].MacAddr[4] = ftm_info->FTMTargetList.MacAddr[i][4];
// 	  ftm_info->FTMSelectedRespList[i].MacAddr[5] = ftm_info->FTMTargetList.MacAddr[i][5];
// 	  ftm_set_fix_para_info(&ftm_info->FTMSelectedRespList[i].FTMParaInfo);
//   }

// }

// static void ftm_Set11kResponder(struct halmac_adapter *padapter)
// {
// 	int i;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

// 	for (i = 0; i < 3; i++) {
// 		memset(&ftm_info->FTMSelectedRespList[i], 0, sizeof(RT_FTM_SELECT_RESP));
// 	}
// 	if (ftm_info->FTM11kResp.chnl) {
// 		ftm_info->FTMSelectedRespList[0].bValid = 1;
// 		ftm_info->FTMSelectedRespList[0].b11k = 1;
// 	}
// 	ftm_info->FTMSelectedRespList[0].ChannelNum = ftm_info->FTM11kResp.chnl;
// 	ftm_info->FTMSelectedRespList[0].bFTMdone = 0;
// 	ftm_info->FTMSelectedRespList[0].MacId = 0xFF;
// 	ftm_info->FTMSelectedRespList[0].wirelessmode = ftm_common_params.wirelessmode;
// 	ftm_info->FTMSelectedRespList[0].bCIVICCap = ftm_common_params.bCIVICCap;
// 	ftm_info->FTMSelectedRespList[0].bLCICap = ftm_common_params.bLCICap;
// 	ftm_info->FTMSelectedRespList[0].Bandwidth = ftm_common_params.Bandwidth;
// 	ftm_info->FTMSelectedRespList[0].ExtChnlOffsetOf40MHz = ftm_common_params.ExtChnlOffsetOf40MHz;
// 	ftm_info->FTMSelectedRespList[0].ExtChnlOffsetOf80MHz = ftm_common_params.ExtChnlOffsetOf80MHz;

// 	memcpy(ftm_info->FTMSelectedRespList[0].MacAddr, ftm_info->FTM11kResp.MacAddr, ETH_ALEN);
// 	ftm_set_fix_para_info(&ftm_info->FTMSelectedRespList[0].FTMParaInfo);
// }

// static void ftm_SetFixRequester(struct halmac_adapter *padapter)
// {
//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

//   ftm_info->FTMReqSTAEntry[0].DialogToken = 0;
//   ftm_info->FTMReqSTAEntry[0].FollowUpDialogToken = 0;
//   ftm_info->FTMReqSTAEntry[0].TOAError = 0;
//   ftm_info->FTMReqSTAEntry[0].TODError = 0;
//   memset(ftm_info->FTMReqSTAEntry[0].TOD_t1, 0, 6);
//   memset(ftm_info->FTMReqSTAEntry[0].TOA_t4, 0, 6);
//   ftm_info->FTMReqSTAEntry[0].bUsed = 1;

//   ftm_info->FTMReqSTAEntry[0].MacAddr[0] = ftm_info->FTMTargetList.MacAddr[3][0];
//   ftm_info->FTMReqSTAEntry[0].MacAddr[1] = ftm_info->FTMTargetList.MacAddr[3][1];
//   ftm_info->FTMReqSTAEntry[0].MacAddr[2] = ftm_info->FTMTargetList.MacAddr[3][2];
//   ftm_info->FTMReqSTAEntry[0].MacAddr[3] = ftm_info->FTMTargetList.MacAddr[3][3];
//   ftm_info->FTMReqSTAEntry[0].MacAddr[4] = ftm_info->FTMTargetList.MacAddr[3][4];
//   ftm_info->FTMReqSTAEntry[0].MacAddr[5] = ftm_info->FTMTargetList.MacAddr[3][5];
// }

// void FTM_SelectBWandDataRate(FTM_PARAM_FORMATBW FTMFormatAndBW, char *bw, char *dr)
// {
//   switch (FTMFormatAndBW) {
//   case FTM_PARAM_FORMATBW_HT_20:
// 	  *bw = CHANNEL_WIDTH_20;
// 	  *dr = MGN_MCS0;
// 	  break;
//   case FTM_PARAM_FORMATBW_VHT_20:
// 	  *bw = CHANNEL_WIDTH_20;
// 	  *dr = MGN_VHT1SS_MCS0;
// 	  break;
//   case FTM_PARAM_FORMATBW_HT_40:
// 	  *bw = CHANNEL_WIDTH_40;
// 	  *dr = MGN_MCS0;
// 	  break;
//   case FTM_PARAM_FORMATBW_VHT_40:
// 	  *bw = CHANNEL_WIDTH_40;
// 	  *dr = MGN_VHT1SS_MCS0;
// 	  break;
//   case FTM_PARAM_FORMATBW_VHT_80:
// 	  *bw = CHANNEL_WIDTH_80;
// 	  *dr = MGN_VHT1SS_MCS0;
// 	  break;
//   default:
// 	  *bw = CHANNEL_WIDTH_20;
// 	  *dr = MGN_MCS0;
// 	  break;
//   }
// }

// void ftm_DecideFTMFormatAndBWValue(struct halmac_adapter *padapter)
// {
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);
// 	FTM_PARAM_FORMATBW format_bw = FTM_PARAM_FORMATBW_HT_20;
// 	int Idx;

// 	for (Idx = 0; Idx < 3; Idx++)
// 	{
// 		if (ftm_info->FTMSelectedRespList[Idx].bValid == _TRUE )
// 		{  /* WIRELESS_MODE_5G */
// 			if (ftm_info->FTMSelectedRespList[Idx].wirelessmode & (WIRELESS_MODE_AC_ONLY | WIRELESS_MODE_AC_5G | WIRELESS_MODE_N_5G | WIRELESS_MODE_A))
// 			{
// 				/* WIRELESS_11_5AC | WIRELESS_11_24AC*/
// 				if (ftm_info->FTMSelectedRespList[Idx].wirelessmode & (WIRELESS_MODE_AC_ONLY | WIRELESS_MODE_AC_24G |WIRELESS_MODE_AC_5G))
// 				{
// 					switch (ftm_info->FTMSelectedRespList[Idx].Bandwidth)
// 					{
// 					case CHANNEL_WIDTH_20:
// 						format_bw = FTM_PARAM_FORMATBW_VHT_20;
// 						break;
// 					case CHANNEL_WIDTH_40:
// 						format_bw = FTM_PARAM_FORMATBW_VHT_40;
// 						break;
// 					case CHANNEL_WIDTH_80:
// 						format_bw = FTM_PARAM_FORMATBW_VHT_80;
// 						break;
// 					default:
// 						break;
// 					}
// 				}

// 				else
// 				{
// 					if (ftm_info->FTMSelectedRespList[Idx].Bandwidth == CHANNEL_WIDTH_20)
// 					{
// 						format_bw = FTM_PARAM_FORMATBW_HT_20;
// 					}
// 					if (ftm_info->FTMSelectedRespList[Idx].Bandwidth == CHANNEL_WIDTH_40)
// 					{
// 						format_bw = FTM_PARAM_FORMATBW_HT_40;
// 					}
// 				}
// 			}
// 			else
// 			{   /* WIRELESS_11_24AC | WIRELESS_MODE_24G */
// 				if (ftm_info->FTMSelectedRespList[Idx].wirelessmode & (WIRELESS_MODE_AC_24G | WIRELESS_MODE_N_24G | WIRELESS_MODE_G | WIRELESS_MODE_B))
// 				{
// 					format_bw = FTM_PARAM_FORMATBW_HT_20;
// 				}
// 			}
// 			ftm_info->FTMSelectedRespList[Idx].FTMParaInfo.FTMFormatAndBW = format_bw;
// 		}
// 	}
// }

// void ftm_send_ftm_request(struct halmac_adapter *padapter, u8 is_initial_frame, u8 *pdev_raddr)
// {
// 	struct xmit_frame			*pmgntframe;
// 	struct pkt_attrib			*pattrib;
// 	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);

// 	pmgntframe = alloc_mgtxmitframe(pxmitpriv);
// 	if (pmgntframe == NULL)
// 		return;

// 	pr_info("[%s] In\n", __FUNCTION__);
// 	/* update attribute */
// 	pattrib = &pmgntframe->attrib;


// 	update_mgntframe_attrib(padapter, pattrib);
// 	memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);


// 	construct_ftm_req_packet(padapter, is_initial_frame, pmgntframe, pdev_raddr);

// 	pattrib->last_txcmdsz = pattrib->pktlen;
// 	dump_mgntframe(padapter, pmgntframe);
// }

// void construct_ftm_req_packet(struct halmac_adapter *padapter, u8 is_initial_frame, struct xmit_frame *pmgntframe, u8 *pdev_raddr)
// {
// 	u8          category    = RTW_WLAN_CATEGORY_PUBLIC;
// 	u8			action      = ACT_PUBLIC_FINE_TIMING_MEASUREMENT_REQUEST;
// 	u8          locie[10]   = {0x26, 0x08, 0x01, 0x00, 0x0B, 0x01, 0x00, 0x00, 0x00, 0x00};
// 	u8          lciie[6]    = {0x26, 0x04, 0x02, 0x00, 0x08, 0x01};
// 	u8			ftmie[16]   = { 0x00 };
// 	u8			ftmielen    = 0;
// 	u8			trigger     = 1;

// 	struct pkt_attrib			*pattrib;
// 	unsigned char				*pframe;
// 	struct rtw_ieee80211_hdr	*pwlanhdr;
// 	unsigned short				*fctrl;

// 	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
// 	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
// 	struct wifidirect_info	*pwdinfo = &(padapter->wdinfo);
// 	RT_FTM_INFO        *ftm_info = GET_HAL_FTM(padapter);
// 	RT_FTM_SELECT_RESP *sel_resp = ftm_info->pFTMActiveSelectedResp;

// 	pr_info("[%s] In\n", __FUNCTION__);
// 	/* update attribute */
// 	pattrib = &pmgntframe->attrib;

// 	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
// 	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

// 	fctrl = &(pwlanhdr->frame_ctl);
// 	*(fctrl) = 0;

// 	memcpy(pwlanhdr->addr1, pdev_raddr, ETH_ALEN);
// 	memcpy(pwlanhdr->addr2, adapter_mac_addr(padapter), ETH_ALEN);
// 	memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

// 	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
// 	pmlmeext->mgnt_seq++;
// 	set_frame_sub_type(pframe, WIFI_ACTION);

// 	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
// 	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

// 	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
// 	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));
// 	pframe = rtw_set_fixed_ie(pframe, 1, &(trigger), &(pattrib->pktlen));

//     if ( ftm_info->bNeedLCI && is_initial_frame )

//     {
//       pframe = rtw_set_fixed_ie(pframe, 6, (lciie), &(pattrib->pktlen));
//     }
//     if ( ftm_info->bNeedCIVICLocation && is_initial_frame)
//     {
//       pframe = rtw_set_fixed_ie(pframe, 10, (locie), &(pattrib->pktlen));
//     }
// 	if (is_initial_frame)
// 	{
// 		ftmielen = construct_ie_ftm_param_content(padapter, &sel_resp->FTMParaInfo, ftmie, sizeof(ftmie));
// 		pframe = rtw_set_ie(pframe, EID_FTMParams, ftmielen, (unsigned char *) ftmie, &pattrib->pktlen);
// 	}

// 	return;
// }

// void construct_ftm_req_buffer(struct halmac_adapter *padapter, u8 is_initial_frame, u8 *pdev_raddr, RT_FTM_SCHEDULE_PARA *pftm_param_info, u8 *pbuffer, u32 *plength)
// {
// 	u8          category    = RTW_WLAN_CATEGORY_PUBLIC;
// 	u8			action      = ACT_PUBLIC_FINE_TIMING_MEASUREMENT_REQUEST;
// 	u8          locie[10]   = {0x26, 0x08, 0x01, 0x00, 0x0B, 0x01, 0x00, 0x00, 0x00, 0x00};
// 	u8          lciie[6]    = {0x26, 0x04, 0x02, 0x00, 0x08, 0x01};
// 	u8			ftmie[16]   = { 0x00 };
// 	u8			ftmielen    = 0;
// 	u8			trigger     = 1;
// 	u8			broadcast[6]     = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// 	unsigned char				*pframe = pbuffer;
// 	struct rtw_ieee80211_hdr	*pwlanhdr;
// 	unsigned short				*fctrl;

// 	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
// 	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

// 	RT_FTM_INFO        *ftm_info = GET_HAL_FTM(padapter);
// 	//RT_FTM_SELECT_RESP *sel_resp = ftm_info->pFTMActiveSelectedResp;


// 	pr_info("[%s] In\n", __FUNCTION__);

// 	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

// 	fctrl = &(pwlanhdr->frame_ctl);
// 	*(fctrl) = 0;

// 	memcpy(pwlanhdr->addr1, pdev_raddr, ETH_ALEN);
// 	memcpy(pwlanhdr->addr2, adapter_mac_addr(padapter), ETH_ALEN);
// 	memcpy(pwlanhdr->addr3, broadcast/*get_my_bssid(&(pmlmeinfo->network))*/, ETH_ALEN);


// 	set_frame_sub_type(pframe, WIFI_ACTION);

// 	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
// 	*plength = sizeof(struct rtw_ieee80211_hdr_3addr);

// 	pframe = rtw_set_fixed_ie(pframe, 1, &(category), plength);
// 	pframe = rtw_set_fixed_ie(pframe, 1, &(action), plength);
// 	pframe = rtw_set_fixed_ie(pframe, 1, &(trigger), plength);

//     if ( ftm_info->bNeedLCI && is_initial_frame )
//     {
//       pframe = rtw_set_fixed_ie(pframe, 6, (lciie), plength);
//     }
//     if ( ftm_info->bNeedCIVICLocation && is_initial_frame)
//     {
//       pframe = rtw_set_fixed_ie(pframe, 10, (locie), plength);
//     }
// 	if (is_initial_frame)
// 	{
// 		ftmielen = construct_ie_ftm_param_content(padapter, pftm_param_info, ftmie, sizeof(ftmie));
// 		pframe = rtw_set_ie(pframe, EID_FTMParams, ftmielen, (unsigned char *) ftmie, plength);
// 	}

// 	return;
// }

// void FTM_FlushSelectResponderList(struct halmac_adapter *padapter)
// {
// 	int i;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

// 	for (i = 0; i < 3; i++) {
// 		ftm_info->FTMSelectedRespList[i].bValid = 0;
// 	}
// 	ftm_info->FTMSelectRspNum = 0;
// }

// void FTM_FlushTargetList(struct halmac_adapter *padapter)
// {
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

// 	memset(ftm_info->FTMTargetList.MacAddr, 0, 18);
// 	ftm_info->FTMTargetList.NumOfResponder = 0;
// }

// void FTM_FlushAllList(struct halmac_adapter *padapter)
// {
// 	int i;
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

// 	FTM_FlushTargetList(padapter);
// 	FTM_FlushSelectResponderList(padapter);
// 	for (i = 0; i < 64; i++) {
// 		ftm_info->FTMCandicateRespList[i].bValid = 0;
// 	}
// 	ftm_info->FTMCandidateRspNum = 0;
// }

// void FTM_StartRespondingProcess(struct halmac_adapter *padapter)
// {
// 	RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);
// 	ftm_info->bResponder = _TRUE;
// 	ftm_info->Role = FTM_RESPONDER;
// 	ftm_info->RespondingState = FTM_RESP_STATE_ON_INIT_FTM_REQ;
// }

// void FTM_AddCandidateResponderList(struct halmac_adapter *padapter, RT_FTM_CAND_RESP *pFTMCandicateResp)
// {
//   int i;
//   RT_FTM_CAND_RESP *cur_resp;

//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);
//   for ( i = 0; i < 64; i++)
//   {
//     if ( ftm_info->FTMCandicateRespList[i].bValid )
//     {
//       if ( !memcmp(ftm_info->FTMCandicateRespList[i].MacAddr, pFTMCandicateResp->MacAddr, 6) )
//       {
//     	ftm_info->FTMCandicateRespList[i].TSFOffset = pFTMCandicateResp->TSFOffset;
//         return;
//       }
//     }
//   }
//   i = 0;
//   while ( ftm_info->FTMCandicateRespList[i].bValid )
//   {
//     if ( ++i >= 64 )
//     {
//       return;
//     }
//   }
//   cur_resp = &ftm_info->FTMCandicateRespList[i];


//   if ( !cur_resp )
//   {
// 	  return;
//   }
//   cur_resp->bValid = pFTMCandicateResp->bValid;
//   cur_resp->bCIVICCap = pFTMCandicateResp->bCIVICCap;
//   cur_resp->bLCICap = pFTMCandicateResp->bLCICap;
//   cur_resp->MacId = pFTMCandicateResp->MacId;
//   cur_resp->ChannelNum = pFTMCandicateResp->ChannelNum;
//   cur_resp->wirelessmode = pFTMCandicateResp->wirelessmode;
//   cur_resp->Bandwidth = pFTMCandicateResp->Bandwidth;
//   cur_resp->ExtChnlOffsetOf40MHz = pFTMCandicateResp->ExtChnlOffsetOf40MHz;
//   cur_resp->ExtChnlOffsetOf80MHz = pFTMCandicateResp->ExtChnlOffsetOf80MHz;
//   memcpy(cur_resp->MacAddr, pFTMCandicateResp->MacAddr, 6);
//   cur_resp->TSFOffset = pFTMCandicateResp->TSFOffset;
// }


// void FTM_ConstructFTMReqInfo(struct halmac_adapter *padapter)
// {
//   int i;
//   u8 *buffer;
//   RT_FTM_INFO *ftm_info = GET_HAL_FTM(padapter);

//   for (i = 0; i < 3; i++)
//   {
//     if ( ftm_info->FTMSelectedRespList[i].bValid == _TRUE )
//     {
//       buffer = ftm_info->FTMSelectedRespList[i].FTMReqInfoBuf;
//       memset(buffer, 0, 32);

//       ftm_info->FTMSelectedRespList[i].FTMReqInfo.Octet = ftm_info->FTMSelectedRespList[i].FTMReqInfoBuf;
//       ftm_info->FTMSelectedRespList[i].FTMReqInfo.Length = 32;

//       buffer[0] = ftm_info->FTMSelectedRespList[i].bValid;
//       buffer[1] = ftm_info->FTMSelectedRespList[i].MacId;
//       buffer[2] = ftm_info->FTMSelectedRespList[i].MacAddr[0];
//       buffer[3] = ftm_info->FTMSelectedRespList[i].MacAddr[1];
//       buffer[4] = ftm_info->FTMSelectedRespList[i].MacAddr[2];
//       buffer[5] = ftm_info->FTMSelectedRespList[i].MacAddr[3];
//       buffer[6] = ftm_info->FTMSelectedRespList[i].MacAddr[4];
//       buffer[7] = ftm_info->FTMSelectedRespList[i].MacAddr[5];
//       buffer[8] = ftm_info->FTMSelectedRespList[i].FTMParaInfo.BurstExpoNum;
//       buffer[9] = ftm_info->FTMSelectedRespList[i].FTMParaInfo.BurstTimeout;
//       *(u16*)(&buffer[10]) = ftm_info->FTMSelectedRespList[i].FTMParaInfo.PartialTSFStartOffset;
//       buffer[12] = ftm_info->FTMSelectedRespList[i].FTMParaInfo.MinDeltaFTM;
//       buffer[13] = ftm_info->FTMSelectedRespList[i].FTMParaInfo.ASAPCapable;
//       buffer[14] = ftm_info->FTMSelectedRespList[i].FTMParaInfo.ASAP;
//       buffer[15] = ftm_info->FTMSelectedRespList[i].FTMParaInfo.FTMNumPerBust;
//       buffer[16] = (u8)ftm_info->FTMSelectedRespList[i].FTMParaInfo.FTMFormatAndBW;
//       *(u16*)(&buffer[18]) = ftm_info->FTMSelectedRespList[i].FTMParaInfo.BurstPeriod;
//       *(u32*)(&buffer[20]) = 0;//ftm_info->FTMSelectedRespList[i].FTMParaInfo.TSFOffset;
//       buffer[24] = ftm_info->FTMSelectedRespList[i].ChannelNum;
//       buffer[25] = ftm_info->FTMSelectedRespList[i].Bandwidth;
//       buffer[26] = ftm_info->FTMSelectedRespList[i].ExtChnlOffsetOf40MHz;
//       buffer[27] = ftm_info->FTMSelectedRespList[i].ExtChnlOffsetOf80MHz;
//     }
//   }
// }
