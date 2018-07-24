#include <rtl_halmac.h>
#include <rtl_proc.h>

#include <linux/fs.h>		// for basic filesystem
#include <linux/proc_fs.h>	// for the proc filesystem
#include <linux/seq_file.h>	// for sequence files

static int halmac_open_proc(struct inode *inode, struct file *file);
static ssize_t halmac_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *pos);
static int read_proc_dummy(struct seq_file *m, void *v);
static int read_proc_11k(struct seq_file *m, void *v);
static int write_proc_11k(struct file *filp,const char *buf,size_t count,loff_t *offp);
static int read_proc_ftm_test(struct seq_file *m, void *v);
static ssize_t write_proc_ftm_test(struct file *filp,const char *buf,size_t count,loff_t *offp);
static int read_proc_ftm_peer(struct seq_file *m, void *v);
static ssize_t write_proc_ftm_peer(struct file *filp,const char *buf,size_t count,loff_t *offp);
static int read_proc_ftm_params(struct seq_file *m, void *v);
static ssize_t write_proc_ftm_params(struct file *filp,const char *buf,size_t count,loff_t *offp);
static int read_proc_ftm_ch_mode(struct seq_file *m, void *v);
static ssize_t write_proc_ftm_ch_mode(struct file *filp,const char *buf,size_t count,loff_t *offp);

#define RTL_PROC_NAME "rtl_halmac"

const static struct rtl_proc_hdl halmac_proc_hdls[] = {
	RTL_PROC_HDL_ITEM("ftm_11k", read_proc_11k, write_proc_11k),
	RTL_PROC_HDL_ITEM("ftm_test", read_proc_ftm_test, write_proc_ftm_test),
	RTL_PROC_HDL_ITEM("ftm_peer", read_proc_ftm_peer, write_proc_ftm_peer),
	RTL_PROC_HDL_ITEM("ftm_params", read_proc_ftm_params, write_proc_ftm_params),
	RTL_PROC_HDL_ITEM("ftm_ch_mode", read_proc_ftm_ch_mode, write_proc_ftm_ch_mode)
};

const static int halmac_proc_hdls_num = sizeof(halmac_proc_hdls) / sizeof(struct rtl_proc_hdl);

static struct proc_dir_entry *dir_dev = NULL;

struct file_operations halmac_proc_fops = {
    .owner	    = THIS_MODULE,
    .open	    = halmac_open_proc,
    .read	    = seq_read,
    .write      = halmac_write_proc,
    .llseek	    = seq_lseek,
    .release	= single_release};

int proc_init (void* cookie) {
    int ret = _FAIL;
   	ssize_t i;
	struct proc_dir_entry *entry = NULL;

    if (dir_dev != NULL) {
		WARN_ON(1);
		goto exit;
    }
    dir_dev = proc_mkdir_data(RTL_PROC_NAME, S_IRUGO | S_IXUGO, init_net.proc_net, cookie);
	if (dir_dev == NULL) {
		WARN_ON(1);
		goto exit;
	}

	for (i = 0; i < halmac_proc_hdls_num; i++) {

        entry = proc_create_data(halmac_proc_hdls[i].name, S_IFREG | S_IRUGO | S_IWUGO, 
            dir_dev, &halmac_proc_fops, (void*) i);
		if (!entry) {
		    WARN_ON(1);
            break;
        }
	}
	ret = _SUCCESS;
exit:
	return ret;
}

void proc_cleanup(void) {
   	ssize_t i;
	if (dir_dev == NULL) {
		return;
    }
	for (i = 0; i < halmac_proc_hdls_num; i++) {
		remove_proc_entry(halmac_proc_hdls[i].name, dir_dev);
    }
    remove_proc_entry(RTL_PROC_NAME, init_net.proc_net);
}

static int halmac_open_proc(struct inode *inode, struct file *file) {
	ssize_t index = (ssize_t)PDE_DATA(inode);
	const struct rtl_proc_hdl *hdl = halmac_proc_hdls + index;
	void *private = proc_get_parent_data(inode);
	int (*show)(struct seq_file *, void *) = hdl->show ? hdl->show : read_proc_dummy;
	return single_open(file, show, private);
}

static ssize_t halmac_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *pos) {
	ssize_t index = (ssize_t)PDE_DATA(file_inode(file));
	const struct rtl_proc_hdl *hdl = halmac_proc_hdls + index;
	if (hdl->write) {
		return hdl->write(file, buffer, count, pos);
    }
	return -EROFS;
}

static int read_proc_dummy(struct seq_file *m, void *v) {
	return 0;
}

/***********************************************************************/
/*************************** Handlers **********************************/
/***********************************************************************/
static int read_proc_11k(struct seq_file *m, void *v) {
    struct rtl_priv *rtlpriv = m->private;
	PRT_RM_INFO rm_info = GET_HAL_RM(rtlpriv_to_halmac(rtlpriv));
	PRT_RM_FTM_REPORT ftm_rep = NULL;
	PRT_RM_FTM_MEASUREMENT ftm_meas = NULL;

	// FTM report list (list for local measurement as FTM Requester)
	// requested via 11k
	seq_printf(m, "Timestamp\tRange\tRequester\t\tReporter\n");
	list_for_each_entry(ftm_rep, &rm_info->FtmReportList, list) {
		seq_printf(m, "%010u\t%u\t"/*MAC_FMT"\t"*/MAC_FMT"\n",
			ftm_rep->Timestamp, ftm_rep->Range, /* TODO: MAC_ARG(adapter_mac_addr(padapter)),*/ MAC_ARG(ftm_rep->DevAddress));
	}
	// All FTM measurements reported via 11k RM report
	list_for_each_entry(ftm_meas, &rm_info->FtmMeasurementList, list) {
		seq_printf(m, "%010u\t%u\t"MAC_FMT"\t"MAC_FMT"\n",
			ftm_meas->Timestamp, ftm_meas->Range, MAC_ARG(ftm_meas->RequesterAddr), MAC_ARG(ftm_meas->ReporterAddr));
	}
	return 0;
}

static int write_proc_11k(struct file *filp,const char *buf,size_t count,loff_t *offp) {
    struct rtl_priv *rtlpriv = PDE_DATA(file_inode(filp));

	char tmp[32];
	char addr[6];

	if (count < 1)
		return -EINVAL;

	if (count > sizeof(tmp)) {
		WARN_ON(1);
		return -EFAULT;
	}

	if (buf && !copy_from_user(tmp, buf, count)) {
		/* macaddr */
		/* "11:22:33:44:55:66" */
		if (sscanf(tmp, MAC_SFMT, MAC_SARG(addr)) != 6) {
			pr_info("[RM] %s: wrong format\n", __FUNCTION__);
			return count;
		} else {
			pr_info("[RM] %s: "MAC_FMT"\n", __FUNCTION__, MAC_ARG(addr));
			//TODO:
            //rtw_rm_start_ftm_measurement(rtlpriv, addr);
		}

	} else {
		return -EFAULT;
	}

	return count;
}

static int read_proc_ftm_test(struct seq_file *m, void *v) {
	//dump_ftm_test(m);
	return 0;
}

static ssize_t write_proc_ftm_test(struct file *filp,const char *buf,size_t count,loff_t *offp) {
    struct rtl_priv *rtlpriv = PDE_DATA(file_inode(filp));

	char tmp[32];
	int ftm_test;
	char addr[6] = {0x58,0xef,0x68,0x27,0x1b,0x98};

	if (count < 1)
		return -EINVAL;

	if (count > sizeof(tmp)) {
		WARN_ON(1);
		return -EFAULT;
	}


	if (buf && !copy_from_user(tmp, buf, count)) {

		int num = sscanf(tmp, "%d ", &ftm_test);
		pr_info("%s: %d\n", __FUNCTION__, ftm_test);

		switch (ftm_test) {
		// case 1:
		// 	FTM_Request(padapter, 0, 1, 1, addr, 6);
		// 	break;
		// case 2:
		// 	FTM_Request(padapter, 1, 0, 1, addr, 6);
		// 	break;
		// case 3:
		// 	ftm_start_requesting_process(padapter, 1000);
		// 	break;
		// case 4:
		// 	ftm_start_requesting_process(padapter, 500);
		// 	break;
		// case 5:
		// 	ftm_start_requesting_process(padapter, 0);
		// 	break;
		// case 6:
		// 	ftm_start_responding_process(padapter, 1000);
		// 	break;
		default:
			pr_info("%s: %d out of range\n", __FUNCTION__, ftm_test);
			break;
		}
	} else
		return -EFAULT;

	return count;
}

static int read_proc_ftm_peer(struct seq_file *m, void *v) {
    struct rtl_priv *rtlpriv = m->private;

	int i, j;
	u8 valid, channel, addr[ETH_ALEN];
	u32 ts, range, t3a, t4a, t[48];

	for (i = 0; i < 3; i++)	{
		//ftm_get_peer(rtlpriv, i, &valid, addr, &channel, &ts, &range, &t3a, &t4a, t);
		seq_printf(m, "%010u, %c, peer:%d, addr:"MAC_FMT", ch:%u, range:%u, t3a:%u, t4a:%u, ", ts, valid?'V':'I', i, MAC_ARG(addr), channel, range, t3a, t4a);
		seq_printf(m, "t3[");
		for(j = 0; j < 15; j++)	{
			seq_printf(m, "%d, ", t[j]);
		}
		seq_printf(m, "%d], t4[", t[j++]);
		for(; j < 31; j++) {
			seq_printf(m, "%d, ", t[j]);
		}
		seq_printf(m, "%d], bc[", t[j++]);
		for(; j < 47; j++) {
			seq_printf(m, "%d, ", t[j]);
		}

		seq_printf(m, "%d]\n", t[j]);
	}
	return 0;
}

static ssize_t write_proc_ftm_peer(struct file *filp,const char *buf,size_t count,loff_t *offp) {
    struct rtl_priv *rtlpriv = PDE_DATA(file_inode(filp));

	char tmp[32] = {0};
	u8 idx, channel;
	u8 addr[ETH_ALEN];

	if (count < 1 || count > sizeof(tmp)) {
		WARN_ON(1);
		return -EFAULT;
	}

	if (buf && !copy_from_user(tmp, buf, count)) {
		/* idx macaddr channel */
		/* "0 11:22:33:44:55:66 151" */
		char *c, *next;

		next = tmp;
		c = strsep(&next, ";, \t");

		if (sscanf(c, "%hhu", &idx) != 1)
			return count;

		c = strsep(&next, ";, \t");
		if (sscanf(c, MAC_SFMT, MAC_SARG(addr)) != 6)
			return count;

		c = strsep(&next, ";, \t");
		if (sscanf(c, "%hhu", &channel) != 1)
			return count;

		//ftm_set_peer(rtlpriv, idx, addr, channel);
	}

	return count;
}

static int read_proc_ftm_params(struct seq_file *m, void *v) {
	pr_info("%s not implemented\n", __FUNCTION__);
    return 0;
}

static ssize_t write_proc_ftm_params(struct file *filp,const char *buf,size_t count,loff_t *offp) {
    struct rtl_priv *rtlpriv = PDE_DATA(file_inode(filp));

	char tmp[32] = {0};
	u8 idx;
	unsigned params[9];

	if (count < 1 || count > sizeof(tmp)) {
		WARN_ON(1);
		return -EFAULT;
	}

	if (buf && !copy_from_user(tmp, buf, count)) {
		/* PartialTSFStartOffset MinDeltaFTM BurstExpoNum BurstTimeout ASAPCapable ASAP FTMNumPerBust FTMFormatAndBW BurstPeriod */
		/* "10;3;0;10;0;0;10;13;0" */
		char *c, *next;

		next = tmp;
		for (idx = 0; idx < 9; idx++) {
			c = strsep(&next, ";, \t");
			if (sscanf(c, "%i", &params[idx]) != 1)
				return count;
		}
		pr_info("%s not implemented\n", __FUNCTION__);
		//ftm_set_params(rtlpriv, params);
	}
	return count;
}

static int read_proc_ftm_ch_mode(struct seq_file *m, void *v) {
	pr_info("%s not implemented\n", __FUNCTION__);
	return 0;
}

static ssize_t write_proc_ftm_ch_mode(struct file *filp,const char *buf,size_t count,loff_t *offp) {
    struct rtl_priv *rtlpriv = PDE_DATA(file_inode(filp));

	char tmp[32] = {0};
	u8 idx;
	unsigned params[4];

	if (count < 1 || count > sizeof(tmp)) {
		WARN_ON(1);
		return -EFAULT;
	}

	if (buf && !copy_from_user(tmp, buf, count)) {
		/* ChannelNum wirelessmode Bandwidth ExtChnlOffset */
		/* "149,0x40,2,0" */
		char *c, *next;

		next = tmp;
		for (idx = 0; idx < 4; idx++) {
			c = strsep(&next, ";, \t");
			if (sscanf(c, "%i", &params[idx]) != 1)
				return count;
		}
		pr_info("%s not implemented\n", __FUNCTION__);
		//ftm_set_params(rtlpriv, params);
	}
	return count;
}
