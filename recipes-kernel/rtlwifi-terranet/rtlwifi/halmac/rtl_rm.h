/*
* rtw_rm.h
*
*  Created on: 30 Nov 2017
*/

#ifndef INCLUDE_RTW_RM_H_
#define INCLUDE_RTW_RM_H_

#define GET_HAL_RM(__pAdapter)			(&((__pAdapter)->hal_rm))

struct halmac_adapter;

typedef enum _RM_SESSION_EVENT_ID
{
	RM_EVENT_RESET = 0x0,
	RM_EVENT_FTM_REQ_RECEIVED = 0x1,
	RM_EVENT_START_FTM_MEASUREMENT = 0x2,
	RM_EVENT_FTM_MEASUREMENT_DONE = 0x3,
	RM_EVENT_FTM_REP_RECEIVED = 0x4,
	RM_EVENT_FTM_RESPONDER_READY = 0x5,

	RM_EVENT_TERMINATE = 0xA,
	RM_EVENT_UNKNOWN
} RM_SESSION_EVENT_ID;

typedef enum _RM_SESSION_STATE
{
	RM_SESSION_STATE_INIT = 0x0,
	RM_SESSION_STATE_FTM_RANGE_REQ_RECEIVED = 0x1,
	RM_SESSION_STATE_SET_FTM_REQUESTER = 0x2,
	RM_SESSION_STATE_SEND_FTM_RANGE_REPORT = 0x3,
	RM_SESSION_STATE_SEND_FTM_RANGE_REQ = 0x4,
	RM_SESSION_STATE_SET_FTM_RESPONDER = 0x5,
	RM_SESSION_STATE_REPORT_LAST_MEASUREMENT = 0x6,
	RM_SESSION_STATE_DONE = 0x9,
	RM_SESSION_STATE_MAX = 0xA,
} RM_SESSION_STATE;

typedef struct _RM_SESSION_EVENT {
	RM_SESSION_EVENT_ID event;

	struct list_head list; /* kernel's list structure */
} RM_SESSION_EVENT, *PRM_SESSION_EVENT;

// RM FTM request
typedef struct _RT_RM_FTM_REQUEST
{
	u8 Channel;
	u8 DevAddress[ETH_ALEN];
	u8 Token;

	struct list_head list; /* kernel's list structure */
} RT_RM_FTM_REQUEST, *PRT_RM_FTM_REQUEST;

// RM FTM report
typedef struct _RT_RM_FTM_REPORT
{
	u8 Channel;
	u8 DevAddress[ETH_ALEN];
	u32 Range;
	u32 Timestamp;

	struct list_head list; /* kernel's list structure */
} RT_RM_FTM_REPORT, *PRT_RM_FTM_REPORT;

// RM FTM measurement list
typedef struct _RT_RM_FTM_MEASUREMENT
{
	u8 RequesterAddr[ETH_ALEN];
	u8 ReporterAddr[ETH_ALEN];
	u32 Range;
	u32 Timestamp;

	struct list_head list; /* kernel's list structure */
} RT_RM_FTM_MEASUREMENT, *PRT_RM_FTM_MEASUREMENT;

// Context
typedef struct _RT_RM_INFO
{
	spinlock_t lock;
	char DialogToken;
	struct	semaphore RMSessionEvent;
	struct	semaphore RMSessionTerminate;
	void* RMSessionThread;
	struct list_head EventQueue;
	struct list_head Ftm11kRequestList;
	struct list_head FtmUserRequestList;
	struct list_head FtmReportList;
	struct list_head FtmMeasurementList;

} RT_RM_INFO, *PRT_RM_INFO;

// Measurment request element 802.11-2016 9.4.2.21
typedef struct _DOT11_RM_MEASUREMENT_REQ_IE {
	u8 ElementId;
	u8 Length;
	u8 MeasurementToken;
	u8 MeasurementRequestMode;
	u8 MeasurementType;
	u8 MeasurementRequest[0];
} __attribute__((packed)) DOT11_RM_MEASUREMENT_REQ_IE, *PDOT11_RM_MEASUREMENT_REQ_IE;

// Measurment report element 802.11-2016 9.4.2.22
typedef struct _DOT11_RM_MEASUREMENT_REP_IE {
	u8 ElementId;
	u8 Length;
	u8 MeasurementToken;
	u8 MeasurementReportMode;
	u8 MeasurementType;
	u8 MeasurementReport[0];
} __attribute__((packed)) DOT11_RM_MEASUREMENT_REP_IE, *PDOT11_RM_MEASUREMENT_REP_IE;

// FTM range request 802.11-2016 9.4.2.21.19
typedef struct _DOT11_RM_FTM_RANGE_REQ {
	u16 RandomizationInterval;
	u8 MinimumApCount;
	u8 FtmRangeSubelements[0];
} __attribute__((packed)) DOT11_RM_FTM_RANGE_REQ, *PDOT11_RM_FTM_RANGE_REQ;

// FTM range request 802.11-2016 9.4.2.22.18
typedef struct _DOT11_RM_FTM_RANGE_ENTRY {
	u32 MeasStartTime;
	u8 Bssid[ETH_ALEN];
	union {
		u8 arr[3];
		u32 value : 24;
	} __attribute__((packed)) Range;
	u8 MaxRangeErrorExponent;
	u8 Reserved;
} __attribute__((packed)) DOT11_RM_FTM_RANGE_ENTRY, *PDOT11_RM_FTM_RANGE_ENTRY;

// Vendor subelement
typedef struct _DOT11_RM_FTM_RANGE_SUB_VENDOR {
	u8 SubelementId;
	u8 Length;
	u16 VendorId;
	//TODO: extend if need more data
} __attribute__((packed)) DOT11_RM_FTM_RANGE_SUB_VENDOR, *PDOT11_RM_FTM_RANGE_SUB_VENDOR;

// Dot11k FTM Range request Subelements
typedef	enum _DOT11K_RM_FTM_RANGE_SUBELEMENT_ID {
	DOT11K_RM_FTM_SUB_ID_MAX_AGE = 3,
	DOT11K_RM_FTM_SUB_ID_NEIGHBOR_REPORT = 52,
	DOT11K_RM_FTM_SUB_ID_VENDOR = 221,
} DOT11K_RM_FTM_RANGE_SUBELEMENT_ID, *PDOT11K_RM_FTM_RANGE_SUBELEMENT_ID;

// Dot11k measurement request type
typedef	enum _DOT11K_RM_TYPE {
	DOT11K_RM_TYPE_CHANNEL_LOAD = 3,
	DOT11K_RM_TYPE_NOISE_HISTOGRAM = 4,
	DOT11K_RM_TYPE_FTM_RANGE = 16,
} DOT11K_RM_TYPE, *PDOT11K_RM_TYPE;

// Dot11k RM request mode
#define	DOT11K_RM_REQ_MODE_PARALLEL		BIT0	// TRUE: Multiple measurements are started at the same time.
#define	DOT11K_RM_REQ_MODE_ENABLE		BIT1	// Indicate the Request or Report information.
#define	DOT11K_RM_REQ_MODE_REQUEST		BIT2	// Request is enabled.
#define	DOT11K_RM_REQ_MODE_REPORT		BIT3	// Report is enabled.
#define	DOT11K_RM_REQ_MODE_MAN_DUR		BIT4	// The measurement duaration is mandatory or shorter.

// Dot11k RM report mode
#define	DOT11K_RM_RPT_MODE_LATE			BIT0	// The request was too late.
#define	DOT11K_RM_RPT_MODE_INCAPABLE	BIT1	// The capabilities of current setting is incapable to perform this RM.
#define	DOT11K_RM_RPT_MODE_REFUSED		BIT2	// The request rm condition is not acceptable for current threshold.


/* Functions */
s32 rm_send_request_ftm(struct halmac_adapter *padapter, u8 *pdev_raddr);
void construct_rm_request_ftm(struct halmac_adapter *padapter, struct xmit_frame *pmgntframe, u8 *pdev_raddr);
u8* add_measurement_req_element(DOT11K_RM_TYPE type, unsigned char *pbuf, unsigned int *frlen);
u8* add_ftm_range_req(unsigned char *pbuf, unsigned int *frlen);
u8* add_ftm_range_vendor_sub(unsigned char *pbuf, unsigned int *frlen);
void rm_send_report_ftm(struct halmac_adapter *padapter, u8 *pdev_raddr);
void construct_rm_report_ftm(struct halmac_adapter *padapter, struct xmit_frame *pmgntframe, u8 *pdev_raddr);
u8* add_measurement_rep_element(PRT_RM_INFO rm, DOT11K_RM_TYPE type, unsigned char *pbuf, unsigned int *frlen);
u8* add_ftm_range_rep(PRT_RM_INFO rm, unsigned char *pbuf, unsigned int *frlen);

unsigned int on_action_rm_request(struct halmac_adapter *padapter, union recv_frame *precv_frame);
unsigned int on_action_rm_ftm_range_request(PDOT11_RM_MEASUREMENT_REQ_IE ie, PRT_RM_FTM_REQUEST ftm_req);
unsigned int on_action_rm_report(struct halmac_adapter *padapter, union recv_frame *precv_frame);
unsigned int on_action_rm_ftm_range_report(PRT_RM_INFO rm, PDOT11_RM_MEASUREMENT_REP_IE ie, u8* sender_addr);

void rtw_rm_start_ftm_measurement(struct halmac_adapter *padapter, u8* address);
void rtw_rm_ftm_request_received(struct halmac_adapter *padapter);
void rtw_rm_ftm_report_received(struct halmac_adapter *padapter);
void rtw_rm_ftm_measurement_completed(struct halmac_adapter *padapter, u32 timestamp, u32 range, u8* address);
void rtw_rm_ftm_responder_ready(struct halmac_adapter *padapter);
void rtw_rm_reset(struct halmac_adapter *padapter);

int rtw_rm_run_threads(struct halmac_adapter *padapter);
int rtw_rm_cancel_threads(struct halmac_adapter *padapter);


#endif /* INCLUDE_RTW_RM_H_ */
