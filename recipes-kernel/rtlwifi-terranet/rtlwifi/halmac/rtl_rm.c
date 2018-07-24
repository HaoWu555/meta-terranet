//#include "../wifi.h"
#include <halmac_type.h>
#include <linux/kthread.h>
#include "rtl_rm.h"
//#include "ieee80211.h"
//#include "osdep_service.h"

#define RM_TERRANET_VENDOR_SUBELEMENT 0x5555

static const char* RM_SESSION_EVENT_TABLE[] = {
	"RM_EVENT_RESET",
	"RM_EVENT_FTM_REQ_RECEIVED",
	"RM_EVENT_START_FTM_MEASUREMENT",
	"RM_EVENT_FTM_MEASUREMENT_DONE",
	"RM_EVENT_FTM_REP_RECEIVED",
	"RM_EVENT_FTM_RESPONDER_READY"};

static const size_t RM_SESSION_EVENT_TABLE_SIZE = sizeof(RM_SESSION_EVENT_TABLE) / sizeof(RM_SESSION_EVENT_TABLE[0]);

static const char* RM_SESSION_STATE_TABLE[] = {
	"RM_SESSION_STATE_INIT",
	"RM_SESSION_STATE_FTM_RANGE_REQ_RECEIVED",
	"RM_SESSION_STATE_SET_FTM_REQUESTER",
	"RM_SESSION_STATE_SEND_FTM_RANGE_REPORT",
	"RM_SESSION_STATE_SEND_FTM_RANGE_REQ",
	"RM_SESSION_STATE_SET_FTM_RESPONDER",
	"RM_SESSION_STATE_REPORT_LAST_MEASUREMENT" };

static const size_t RM_SESSION_STATE_TABLE_SIZE = sizeof(RM_SESSION_STATE_TABLE) / sizeof(RM_SESSION_STATE_TABLE[0]);

static const char *unknown_str = "UNKNOWN";

const char* get_session_event_str(RM_SESSION_EVENT_ID id) {
	const char* str = unknown_str;
	if (id < RM_SESSION_EVENT_TABLE_SIZE) {
		str = RM_SESSION_EVENT_TABLE[id];
	}
	return str;
}

const char* get_session_state_str(RM_SESSION_STATE id) {
	const char* str = unknown_str;
	if (id < RM_SESSION_STATE_TABLE_SIZE) {
		str = RM_SESSION_STATE_TABLE[id];
	}
	return str;
}

// s32 rm_send_request_ftm(struct halmac_adapter *padapter, u8 *pdev_raddr) {
// 	struct xmit_frame			*pmgntframe;
// 	struct pkt_attrib			*pattrib;
// 	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);

// 	pmgntframe = alloc_mgtxmitframe(pxmitpriv);
// 	if (pmgntframe == NULL)
// 		return _FAIL;

// 	//pr_info("[RM] [%s] In\n", __FUNCTION__);
// 	/* update attribute */
// 	pattrib = &pmgntframe->attrib;


// 	update_mgntframe_attrib(padapter, pattrib);
// 	memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

// 	construct_rm_request_ftm(padapter, pmgntframe, pdev_raddr);

// 	pattrib->last_txcmdsz = pattrib->pktlen;
// 	//dump_mgntframe(padapter, pmgntframe);
// 	return dump_mgntframe_and_wait_ack(padapter, pmgntframe);
// }

// void construct_rm_request_ftm(struct halmac_adapter *padapter, struct xmit_frame *pmgntframe, u8 *pdev_raddr) {
// 	u8          category = RTW_WLAN_CATEGORY_RADIO_MEASUREMENT;
// 	u8			action = ACT_RM_REQUEST;
// 	u16         number_of_repetitions = 0;

// 	struct pkt_attrib			*pattrib;
// 	unsigned char				*pframe;
// 	struct rtw_ieee80211_hdr	*pwlanhdr;
// 	RT_RM_INFO					*rm_info = GET_HAL_RM(padapter);

// 	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
// 	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

// 	//pr_info("[RM] [%s] In\n", __FUNCTION__);
// 	/* update attribute */
// 	pattrib = &pmgntframe->attrib;


// 	pframe = (u8*)(pmgntframe->buf_addr) + TXDESC_OFFSET;
// 	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

// 	pwlanhdr->frame_ctl = 0;

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
// 	pframe = rtw_set_fixed_ie(pframe, 1, &(rm_info->DialogToken), &(pattrib->pktlen));
// 	rm_info->DialogToken++;
// 	pframe = rtw_set_fixed_ie(pframe, 2, (u8*)&(number_of_repetitions), &(pattrib->pktlen));
// 	pframe = add_measurement_req_element(DOT11K_RM_TYPE_FTM_RANGE, pframe, &(pattrib->pktlen));

// 	return;
// }


// u8* add_measurement_req_element(DOT11K_RM_TYPE type, unsigned char *pbuf, unsigned int *frlen) {

// 	PDOT11_RM_MEASUREMENT_REQ_IE ie = (PDOT11_RM_MEASUREMENT_REQ_IE)pbuf;
// 	unsigned int request_length = 0;

// 	ie->ElementId = EID_MeasureRequest;
// 	ie->MeasurementToken = 1;
// 	ie->MeasurementRequestMode = DOT11K_RM_REQ_MODE_ENABLE | 
// 		DOT11K_RM_REQ_MODE_REQUEST | DOT11K_RM_REQ_MODE_REPORT;
// 	ie->MeasurementType = type;
	

// 	switch (type) {
// 		case DOT11K_RM_TYPE_FTM_RANGE:
// 			add_ftm_range_req(pbuf + sizeof(*ie), &request_length);
// 			break;
// 		//TODO: other RM types
// 		default:
// 			break;
// 	}

// 	// Total request length
// 	request_length += sizeof(*ie);
// 	ie->Length = request_length - 2;
// 	*frlen += request_length;

// 	return pbuf + request_length;
// }

// u8* add_ftm_range_req(unsigned char *pbuf, unsigned int *frlen) {

// 	PDOT11_RM_FTM_RANGE_REQ req = (PDOT11_RM_FTM_RANGE_REQ)pbuf;
// 	unsigned int request_length = 0;
// 	RTW_PUT_BE16((u8*)&(req->RandomizationInterval), 0);
// 	req->MinimumApCount = 1;

// 	add_ftm_range_vendor_sub(pbuf + sizeof(*req), &request_length);

// 	// Total request length
// 	request_length += sizeof(*req);
// 	*frlen += request_length;
	
// 	return pbuf + request_length;
// }

// u8* add_ftm_range_vendor_sub(unsigned char *pbuf, unsigned int *frlen) {

// 	PDOT11_RM_FTM_RANGE_SUB_VENDOR sub = (PDOT11_RM_FTM_RANGE_SUB_VENDOR)pbuf;
// 	unsigned int request_length = 0;

// 	sub->SubelementId = DOT11K_RM_FTM_SUB_ID_VENDOR;
// 	RTW_PUT_BE16((u8*)&(sub->VendorId), RM_TERRANET_VENDOR_SUBELEMENT);

// 	// Total request length
// 	request_length += sizeof(*sub);
// 	sub->Length = request_length - 2;
// 	*frlen += request_length;

// 	return pbuf + request_length;
// }

// void rm_send_report_ftm(struct halmac_adapter *padapter, u8 *pdev_raddr) {
// 	struct xmit_frame			*pmgntframe;
// 	struct pkt_attrib			*pattrib;
// 	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);

// 	pmgntframe = alloc_mgtxmitframe(pxmitpriv);
// 	if (pmgntframe == NULL)
// 		return;

// 	//pr_info("[RM] [%s] In\n", __FUNCTION__);
// 	/* update attribute */
// 	pattrib = &pmgntframe->attrib;


// 	update_mgntframe_attrib(padapter, pattrib);
// 	memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

// 	construct_rm_report_ftm(padapter, pmgntframe, pdev_raddr);

// 	pattrib->last_txcmdsz = pattrib->pktlen;
// 	dump_mgntframe(padapter, pmgntframe);
// }

// void construct_rm_report_ftm(struct halmac_adapter *padapter, struct xmit_frame *pmgntframe, u8 *pdev_raddr) {
// 	u8          category = RTW_WLAN_CATEGORY_RADIO_MEASUREMENT;
// 	u8			action = ACT_RM_REPORT;
// 	u16         number_of_repetitions = 0;

// 	struct pkt_attrib			*pattrib;
// 	unsigned char				*pframe;
// 	struct rtw_ieee80211_hdr	*pwlanhdr;
// 	RT_RM_INFO					*rm_info = GET_HAL_RM(padapter);

// 	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
// 	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

// 	//pr_info("[RM] [%s] In\n", __FUNCTION__);
// 	/* update attribute */
// 	pattrib = &pmgntframe->attrib;


// 	pframe = (u8*)(pmgntframe->buf_addr) + TXDESC_OFFSET;
// 	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

// 	pwlanhdr->frame_ctl = 0;

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
// 	pframe = rtw_set_fixed_ie(pframe, 1, &(rm_info->DialogToken), &(pattrib->pktlen));
// 	rm_info->DialogToken++;
// 	pframe = rtw_set_fixed_ie(pframe, 2, (u8*)&(number_of_repetitions), &(pattrib->pktlen));
// 	pframe = add_measurement_rep_element(rm_info, DOT11K_RM_TYPE_FTM_RANGE, pframe, &(pattrib->pktlen));

// 	return;
// }

// u8* add_measurement_rep_element(PRT_RM_INFO rm, DOT11K_RM_TYPE type, unsigned char *pbuf, unsigned int *frlen) {

// 	PDOT11_RM_MEASUREMENT_REP_IE ie = (PDOT11_RM_MEASUREMENT_REP_IE)pbuf;
// 	unsigned int report_length = 0;

// 	ie->ElementId = EID_MeasureReport;
// 	ie->MeasurementToken = 1;
// 	ie->MeasurementReportMode = 0;
// 	ie->MeasurementType = type;


// 	switch (type) {
// 		case DOT11K_RM_TYPE_FTM_RANGE:
// 			add_ftm_range_rep(rm, pbuf + sizeof(*ie), &report_length);
// 			break;
// 			//TODO: other RM types
// 		default:
// 			break;
// 	}

// 	// Total request length
// 	report_length += sizeof(*ie);
// 	ie->Length = report_length - 2;
// 	*frlen += report_length;

// 	return pbuf + report_length;
// }

// u8* add_ftm_range_rep(PRT_RM_INFO rm, unsigned char *pbuf, unsigned int *frlen) {

// 	unsigned int report_length = 0;
// 	u8* entries;
// 	PRT_RM_FTM_REPORT rep;
// 	PDOT11_RM_FTM_RANGE_ENTRY entry;
// 	_irqL irqL;

// 	// Add range report entry
// 	entries = pbuf;
// 	*entries = 0;
// 	report_length++;
// 	entry = (PDOT11_RM_FTM_RANGE_ENTRY)(pbuf + report_length);
// 	_enter_critical(&rm->lock, &irqL);
// 	list_for_each_entry(rep, &rm->FtmReportList, list) {
// 		entry->MeasStartTime = rep->Timestamp;
// 		entry->Range.value = rep->Range;
// 		memcpy(entry->Bssid, rep->DevAddress, ETH_ALEN);
// 		++ entry;
// 		++ *entries;
// 	}
// 	_exit_critical(&rm->lock, &irqL);
// 	report_length += sizeof(*entry) * *entries;

// 	// Add error entry
// 	*(pbuf + report_length) = 0;
// 	report_length++;
// 	// TODO: implement error report if needed

// 	// Add Optional Vendor Subelement
// 	// TODO: implement if needed

// 	// Total request length
// 	*frlen += report_length;

// 	return pbuf + report_length;
// }

// unsigned int on_action_rm_request(struct halmac_adapter *padapter, union recv_frame *precv_frame)
// {
// 	unsigned int ret = _FAIL;
// 	RT_RM_INFO *rm_info = GET_HAL_RM(padapter);
// 	u8 *pframe = precv_frame->u.hdr.rx_data;
// 	uint frame_len = precv_frame->u.hdr.len;
// 	struct rx_pkt_attrib *attrib = &(precv_frame->u.hdr.attrib);
// 	struct rtw_ieee80211_hdr_3addr *hdr80211 = (struct rtw_ieee80211_hdr_3addr*)pframe;
// 	u8 *frame_body = pframe + sizeof(struct rtw_ieee80211_hdr_3addr);
// 	PDOT11_RM_MEASUREMENT_REQ_IE ie = (PDOT11_RM_MEASUREMENT_REQ_IE)(frame_body + 5);
// 	pr_info("[RM] [%s] Got ACT_RM_REQUEST Frame\n", __FUNCTION__);

// 	if (ie->ElementId == EID_MeasureRequest) {

// 		switch (ie->MeasurementType) {
// 			case DOT11K_RM_TYPE_FTM_RANGE: {
// 				// Allocate linked list element
// 				PRT_RM_FTM_REQUEST ftm_req = (PRT_RM_FTM_REQUEST)rtw_zmalloc(sizeof(*ftm_req));
// 				_rtw_init_listhead(&ftm_req->list);
// 				ftm_req->Channel = attrib->ch;
// 				memcpy(ftm_req->DevAddress, hdr80211->addr2, ETH_ALEN);
// 				ret = on_action_rm_ftm_range_request(ie, ftm_req);
// 				if (ret == _SUCCESS) {
// 					_irqL irqL;
// 					_enter_critical(&rm_info->lock, &irqL);
// 					rtw_list_insert_head(&ftm_req->list, &rm_info->Ftm11kRequestList);
// 					_exit_critical(&rm_info->lock, &irqL);
// 					rtw_rm_ftm_request_received(padapter);
// 				} else {
// 					rtw_mfree((u8*)ftm_req, sizeof(*ftm_req));
// 				}
// 				break;
// 			}
// 			//TODO: other RM types
// 			default:
// 				pr_err("[RM] [%s] Unknown RM request element!!!\n", __FUNCTION__);
// 				break;
// 		}
// 	}
// 	else {
// 		pr_err("[RM] [%s] Unknown IE!!!\n", __FUNCTION__);
// 	}

// 	return ret;
// }

// unsigned int on_action_rm_ftm_range_request(PDOT11_RM_MEASUREMENT_REQ_IE ie, PRT_RM_FTM_REQUEST ftm_req) {

// 	unsigned int ret = _SUCCESS;

// 	pr_info("[RM] DOT11K_RM_TYPE_FTM_RANGE to "MAC_FMT"\n", MAC_ARG(ftm_req->DevAddress));

// 	if (ie->Length > (sizeof(DOT11_RM_MEASUREMENT_REQ_IE) + sizeof(DOT11_RM_FTM_RANGE_REQ) - 2)) {
// 		PDOT11_RM_FTM_RANGE_REQ req = (PDOT11_RM_FTM_RANGE_REQ)ie->MeasurementRequest;
// 		//TODO: extract more request fields here
// 	}

// 	return ret;
// }

// unsigned int on_action_rm_report(struct halmac_adapter *padapter, union recv_frame *precv_frame)
// {
// 	unsigned int ret = _FAIL;
// 	RT_RM_INFO *rm_info = GET_HAL_RM(padapter);
// 	u8 *pframe = precv_frame->u.hdr.rx_data;
// 	uint frame_len = precv_frame->u.hdr.len;
// 	struct rtw_ieee80211_hdr_3addr *hdr80211 = (struct rtw_ieee80211_hdr_3addr*)pframe;
// 	u8 *frame_body = pframe + sizeof(struct rtw_ieee80211_hdr_3addr);
// 	PDOT11_RM_MEASUREMENT_REP_IE ie = (PDOT11_RM_MEASUREMENT_REP_IE)(frame_body + 5);
// 	pr_info("[RM] [%s] Got ACT_RM_REPORT Frame\n", __FUNCTION__);

// 	if (ie->ElementId == EID_MeasureReport) {

// 		switch (ie->MeasurementType) {
// 			case DOT11K_RM_TYPE_FTM_RANGE: {
// 				ret = on_action_rm_ftm_range_report(rm_info, ie, hdr80211->addr2);
// 				rtw_rm_ftm_report_received(padapter);
// 				break;
// 			}
// 			//TODO: other RM types
// 		default:
// 			pr_err("[RM] [%s] Unknown RM request element!!!\n", __FUNCTION__);
// 			break;
// 		}
// 	}
// 	else {
// 		pr_err("[RM] [%s] Unknown IE!!!\n", __FUNCTION__);
// 	}

// 	return ret;
// }

// unsigned int on_action_rm_ftm_range_report(PRT_RM_INFO rm, PDOT11_RM_MEASUREMENT_REP_IE ie, u8* sender_addr) {

// 	unsigned int ret = _FAIL;

// 	if (ie->Length > (sizeof(DOT11_RM_MEASUREMENT_REP_IE) + 2 /* min range report field size */ - 2)) {
// 		PDOT11_RM_FTM_RANGE_ENTRY entry;
// 		u8* pbuf = ie->MeasurementReport;
// 		u8 n, entries = *pbuf;
// 		pbuf++;

// 		// Get range entries
// 		entry = (PDOT11_RM_FTM_RANGE_ENTRY)pbuf;
// 		for (n = 0; n < entries; n++) {

// 			PRT_RM_FTM_MEASUREMENT ftm_meas = NULL;
// 			bool is_found = false;
// 			_irqL irqL;
// 			// Find existing entry with the same address
// 			_enter_critical(&rm->lock, &irqL);
// 			list_for_each_entry(ftm_meas, &rm->FtmMeasurementList, list) {
// 				if (memcmp(ftm_meas->RequesterAddr, sender_addr, ETH_ALEN) &&
// 					memcmp(ftm_meas->ReporterAddr, entry->Bssid, ETH_ALEN)) {
// 					is_found = true;
// 					break;
// 				}
// 			}
// 			// Allocate linked list element if not found
// 			if (!is_found) {
// 				ftm_meas = (PRT_RM_FTM_MEASUREMENT)rtw_zmalloc(sizeof(*ftm_meas));
// 				_rtw_init_listhead(&ftm_meas->list);
// 				rtw_list_insert_head(&ftm_meas->list, &rm->FtmMeasurementList);
// 			}
// 			memcpy(ftm_meas->RequesterAddr, sender_addr, ETH_ALEN);
// 			memcpy(ftm_meas->ReporterAddr, entry->Bssid, ETH_ALEN);
// 			ftm_meas->Range = entry->Range.value;
// 			ftm_meas->Timestamp = entry->MeasStartTime;
// 			_exit_critical(&rm->lock, &irqL);

// 			pr_info("[RM] DOT11K_RM_TYPE_FTM_RANGE from "MAC_FMT" -> range = %d\n",
// 			MAC_ARG(entry->Bssid),
// 			entry->Range.value);

// 			entry++;
// 		}
// 		//TODO: extract error and vendor fields here
// 	}

// 	return ret;
// }

// static RM_SESSION_EVENT_ID rm_get_event(PRT_RM_INFO rm_info) {

// 	_irqL irqL;
// 	RM_SESSION_EVENT_ID event;
// 	if (_FAIL == _down_sema(&rm_info->RMSessionEvent)) {
// 		pr_err("[RM] RMSessionEvent fail!!!\n");
// 		event = RM_EVENT_TERMINATE;
// 	} else {
// 		_enter_critical(&rm_info->lock, &irqL);
// 		if (!list_empty(&rm_info->EventQueue)) {
// 			PRM_SESSION_EVENT event_item = list_last_entry(&rm_info->EventQueue, RM_SESSION_EVENT, list);
// 			list_del(&event_item->list);
// 			_exit_critical(&rm_info->lock, &irqL);
// 			event = event_item->event;
// 			rtw_mfree((u8*)event_item, sizeof(*event_item));
// 		} else {
// 			_exit_critical(&rm_info->lock, &irqL);
// 			pr_err("[RM] EventQueue is empty!!!\n");
// 			event = RM_EVENT_UNKNOWN;
// 		}
// 	}

// 	return event;
// }

static void rm_set_event(PRT_RM_INFO rm_info, RM_SESSION_EVENT_ID event) {

	_irqL irqL;
	PRM_SESSION_EVENT event_item = (PRM_SESSION_EVENT)kzalloc(sizeof(*event_item), GFP_KERNEL);
	INIT_LIST_HEAD(&event_item->list);
	event_item->event = event;
	_enter_critical(&rm_info->lock, &irqL);
	list_add(&event_item->list, &rm_info->EventQueue);
	_exit_critical(&rm_info->lock, &irqL);
	_up_sema(&rm_info->RMSessionEvent);

	return;
}

// int rm_session_thread(void *context)
// {
// 	_irqL irqL;
// 	struct halmac_adapter *adapter = (struct halmac_adapter *)context;
// 	struct recv_priv *recvpriv = &adapter->recvpriv;
// 	RT_RM_INFO *rm_info = GET_HAL_RM(adapter);
// 	RM_SESSION_STATE sessionState = RM_SESSION_STATE_INIT;
// 	RM_SESSION_EVENT_ID event;
// 	u8 sender_addr[ETH_ALEN];
// 	s32 err = _SUCCESS;
// 	u32 timeout = 0;

// 	thread_enter("RTW_RM_S_THREAD");

// 	pr_info("[RM] ""enter\n");


// 	do {
// 		if (RTL_CANNOT_RUN(adapter)) {
//			pr_info("AdapterValidate: %d, ApiValidate: %d\n",
//				halmac_adapter_validate(adapter), halmac_api_validate(adapter));
// 			goto exit;
// 		}

// 		// Read next Event from EventQueue
// 		event = rm_get_event(rm_info);
// 		pr_info("[RM] %s (%d) received in state %s (%d)\n", get_session_event_str(event),
// 			event, get_session_state_str(sessionState), sessionState);

// 		// Reset state machine if state timeout was initiated
// 		if (sessionState == RM_SESSION_STATE_INIT) {
// 			// Reset timeout if SM in INIT state
// 			timeout = 0;
// 		} else {
// 			if (timeout != 0 && rtw_get_passing_time_ms(timeout) > 1000) {
// 				pr_err("[RM] Timeout in %s (%d). Reset to RM_SESSION_STATE_INIT\n", 
// 					get_session_state_str(sessionState), sessionState);
// 				timeout = 0;
// 				sessionState = RM_SESSION_STATE_INIT;
// 			}
// 		}

// 		switch (event) {
// 			case RM_EVENT_RESET:
// 				timeout = 0;
// 				sessionState = RM_SESSION_STATE_INIT;
// 				break;

// 			case RM_EVENT_FTM_REQ_RECEIVED:
// 				if (sessionState == RM_SESSION_STATE_INIT) {
// 					_irqL irqL;
// 					_enter_critical(&rm_info->lock, &irqL);
// 					if (!list_empty(&rm_info->Ftm11kRequestList)) {
// 						PRT_RM_FTM_REQUEST entry = list_last_entry(&rm_info->Ftm11kRequestList, RT_RM_FTM_REQUEST, list);
// 						list_del(&entry->list);
// 						_exit_critical(&rm_info->lock, &irqL);
// 						memcpy(sender_addr, entry->DevAddress, ETH_ALEN);
// 						if (ftm_start_11k_requesting_process(adapter, entry->DevAddress, entry->Channel, 500)) {
// 							sessionState = RM_SESSION_STATE_FTM_RANGE_REQ_RECEIVED;
// 							// Timeout if no RM_EVENT_FTM_MEASUREMENT_DONE in 1000ms
// 							timeout = rtw_get_current_time();
// 						} else {
// 							// TODO: send old report for now and stay in INIT STATE
// 							pr_err("[RM] ftm_start_11k_requesting_process FAILED, sending old report!!!\n");
// 							rm_send_report_ftm(adapter, sender_addr);
// 						}
// 						rtw_mfree((u8*)entry, sizeof(*entry));
// 					} else {
// 						_exit_critical(&rm_info->lock, &irqL);
// 						sessionState = RM_SESSION_STATE_INIT;
// 					}
// 				} else {
// 					//TODO:
// 				}
// 				break;

// 			case RM_EVENT_START_FTM_MEASUREMENT:
// 				if (sessionState == RM_SESSION_STATE_INIT) {
// 					ftm_start_responding_process(adapter, 500);
// 					sessionState = RM_SESSION_STATE_SET_FTM_RESPONDER;
// 					// Timeout if no RM_EVENT_FTM_RESPONDER_READY in 1000ms
// 					timeout = rtw_get_current_time();
// 				} else {
// 					//TODO:
// 				}
// 				break;

// 			case RM_EVENT_FTM_RESPONDER_READY:
// 				if (sessionState == RM_SESSION_STATE_SET_FTM_RESPONDER) {
// 					_irqL irqL;
// 					_enter_critical(&rm_info->lock, &irqL);
// 					if (!list_empty(&rm_info->FtmUserRequestList)) {
// 						PRT_RM_FTM_REQUEST entry = list_last_entry(&rm_info->FtmUserRequestList, RT_RM_FTM_REQUEST, list);
// 						list_del(&entry->list);
// 						_exit_critical(&rm_info->lock, &irqL);
// 						if (rm_send_request_ftm(adapter, entry->DevAddress) == _SUCCESS) {
// 							sessionState = RM_SESSION_STATE_SEND_FTM_RANGE_REQ;
// 							// Timeout if no RM_EVENT_FTM_REP_RECEIVED in 1000ms
// 							timeout = rtw_get_current_time();
// 						} else {
// 							pr_err("[RM] Failed FTM Range request to "MAC_FMT"\n", MAC_ARG(entry->DevAddress));
// 						}
// 						rtw_mfree((u8*)entry, sizeof(*entry));
// 					} else {
// 						_exit_critical(&rm_info->lock, &irqL);
// 						sessionState = RM_SESSION_STATE_INIT;
// 					}
// 				} else {
// 					//TODO:
// 				}
// 				break;

// 			case RM_EVENT_FTM_MEASUREMENT_DONE:
// 				sessionState = RM_SESSION_STATE_INIT;
// 				rm_send_report_ftm(adapter, sender_addr);
// 				break;

// 			case RM_EVENT_FTM_REP_RECEIVED:
// 				//TODO: Handle report. Just goto INIT state for now.
// 				sessionState = RM_SESSION_STATE_INIT;
// 				break;

// 			case RM_EVENT_TERMINATE:
// 				goto exit;
// 				break;

// 			default:
// 				pr_err("[RM] Unknown RM_SESSION_STATE = %d\n", sessionState);
// 				break;
// 		}

// 		flush_signals_thread();

// 	} while (TRUE);

// exit:
// 	_up_sema(&rm_info->RMSessionTerminate);
// 	pr_info("exit\n");
// 	thread_exit(NULL);
// 	return 0;
// }

int rtw_rm_run_threads(struct halmac_adapter *padapter) {
	RT_RM_INFO *rm_info = GET_HAL_RM(padapter);

	pr_info("[RM] ---> %s !!!\n", __func__);
	rm_info->DialogToken = 0;

	spin_lock_init(&rm_info->lock);

	sema_init(&rm_info->RMSessionEvent, 0);
	sema_init(&rm_info->RMSessionTerminate, 0);

	/* Init Lists */
	INIT_LIST_HEAD(&rm_info->EventQueue);
	INIT_LIST_HEAD(&rm_info->Ftm11kRequestList);
	INIT_LIST_HEAD(&rm_info->FtmUserRequestList);
	INIT_LIST_HEAD(&rm_info->FtmReportList);
	INIT_LIST_HEAD(&rm_info->FtmMeasurementList);

	/* SM init */
	rtw_rm_reset(padapter);

	//TODO: fix thread
	//rm_info->RMSessionThread = kthread_run(rm_session_thread, padapter, "RM_THREAD");
	if (IS_ERR(rm_info->RMSessionThread)) {
		pr_err("[RM] ""RM_THREAD run fail!\n");
		return _FAIL;
	}

	pr_info("[RM] <--- %s !!!\n", __func__);
	return _SUCCESS;
}

int rtw_rm_cancel_threads(struct halmac_adapter *padapter) {
	_irqL irqL;
	RT_RM_INFO *rm_info = GET_HAL_RM(padapter);

	pr_info("[RM] ---> %s !!!\n", __func__);

	rm_set_event(rm_info, RM_EVENT_TERMINATE);
	_down_sema(&rm_info->RMSessionTerminate);
	
	// _rtw_free_sema(&rm_info->RMSessionEvent);
	// _rtw_free_sema(&rm_info->RMSessionTerminate);

	// _rtw_spinlock_free(&rm_info->lock);

	pr_info("[RM] <--- %s !!!\n", __func__);
	return _SUCCESS;
}

// void rtw_rm_start_ftm_measurement(struct halmac_adapter *padapter, u8* address) {
// 	RT_RM_INFO *rm_info = GET_HAL_RM(padapter);
// 	_irqL irqL;

// 	// Allocate linked list element
// 	PRT_RM_FTM_REQUEST ftm_req = (PRT_RM_FTM_REQUEST)rtw_zmalloc(sizeof(*ftm_req));
// 	_rtw_init_listhead(&ftm_req->list);
// 	memcpy(ftm_req->DevAddress, address, ETH_ALEN);
// 	_enter_critical(&rm_info->lock, &irqL);
// 	rtw_list_insert_head(&ftm_req->list, &rm_info->FtmUserRequestList);
// 	_exit_critical(&rm_info->lock, &irqL);

// 	rm_set_event(rm_info, RM_EVENT_START_FTM_MEASUREMENT);

// }

// void rtw_rm_ftm_request_received(struct halmac_adapter *padapter) {
// 	RT_RM_INFO *rm_info = GET_HAL_RM(padapter);

// 	rm_set_event(rm_info, RM_EVENT_FTM_REQ_RECEIVED);

// }

// void rtw_rm_ftm_report_received(struct halmac_adapter *padapter) {
// 	RT_RM_INFO *rm_info = GET_HAL_RM(padapter);

// 	rm_set_event(rm_info, RM_EVENT_FTM_REP_RECEIVED);

// }

// void rtw_rm_ftm_measurement_completed(struct halmac_adapter *padapter, u32 timestamp, u32 range, u8* address) {
// 	RT_RM_INFO *rm_info = GET_HAL_RM(padapter);
// 	_irqL irqL;

// 	PRT_RM_FTM_REPORT ftm_rep = NULL;
// 	bool is_found = false;
// 	// Find existing entry with the same 
// 	_enter_critical(&rm_info->lock, &irqL);
// 	list_for_each_entry(ftm_rep, &rm_info->FtmReportList, list) {
// 		if (memcmp(ftm_rep->DevAddress, address, ETH_ALEN)) {
// 			is_found = true;
// 			break;
// 		}
// 	}
// 	// Allocate linked list element if not found
// 	if (!is_found) {
// 		ftm_rep = (PRT_RM_FTM_REPORT)rtw_zmalloc(sizeof(*ftm_rep));
// 		_rtw_init_listhead(&ftm_rep->list);
// 		rtw_list_insert_head(&ftm_rep->list, &rm_info->FtmReportList);
// 	}
// 	memcpy(ftm_rep->DevAddress, address, ETH_ALEN);
// 	ftm_rep->Range = range;
// 	ftm_rep->Timestamp = timestamp;
// 	_exit_critical(&rm_info->lock, &irqL);

// 	rm_set_event(rm_info, RM_EVENT_FTM_MEASUREMENT_DONE);

// }

// void rtw_rm_ftm_responder_ready(struct halmac_adapter *padapter) {
// 	RT_RM_INFO *rm_info = GET_HAL_RM(padapter);

// 	rm_set_event(rm_info, RM_EVENT_FTM_RESPONDER_READY);

// }

void rtw_rm_reset(struct halmac_adapter *padapter) {
	RT_RM_INFO *rm_info = GET_HAL_RM(padapter);

	rm_set_event(rm_info, RM_EVENT_RESET);

}
