// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) STMicroelectronics 2019 - All Rights Reserved
 * Author: Jean-Philippe Romain <jean-philippe.romain@st.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/rpmsg.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/miscdevice.h>
#include <linux/eventfd.h>
#include <linux/of_platform.h>
#include <linux/list.h>
#include <net/sock.h> 
#include <linux/netlink.h>
#include <linux/skbuff.h>

#define RPMSG_NETLINK_DRIVER_VERSION "1.0"
#define NETLINK_USER 31

/*
 * Static global variables
 */
static const char rpmsg_netlink_driver_name[] = "rpmsg-netlink";

struct sock *nl_sk = NULL;
struct rpmsg_device *rpmsg_dev = NULL;

struct rpmsg_data_t {
	uint16_t recv_data_len;
	struct sock *nlsck;
	int client_pid;
};

static void netlink_recv_cbk(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
	int msg_size, rpmsg_msg_size;
	int ret;
	struct rpmsg_data_t *data = dev_get_drvdata(&rpmsg_dev->dev);

	pr_info("Entering: %s\n", __FUNCTION__);

	msg_size = skb->len;
	rpmsg_msg_size = rpmsg_get_buffer_size(rpmsg_dev->ept);
	data->recv_data_len = rpmsg_msg_size;
	data->client_pid = NETLINK_CB(skb).portid;

	nlh = (struct nlmsghdr*)skb->data;
	pr_info("%s received %d bytes in port: %d\n",__FUNCTION__, msg_size, data->client_pid);

	do {
		/* send a message to our remote processor */
		ret = rpmsg_send(rpmsg_dev->ept, (void *)nlmsg_data(nlh),
				 msg_size > rpmsg_msg_size ? rpmsg_msg_size : msg_size);
		if (ret) {
			dev_err(&rpmsg_dev->dev, "rpmsg_send failed: %d\n", ret);
			return;
		}

		if (msg_size > rpmsg_msg_size) {
			msg_size -= rpmsg_msg_size;
			nlh += msg_size;
		} else {
			msg_size = 0;
		}
	} while (msg_size > 0);
}


static int rpmsg_drv_cb(struct rpmsg_device *rpdev, void *data, int len,
			void *priv, u32 src)
{
	int ret = 0;
	struct sk_buff *skb_out = NULL;
	uint16_t * cm4_recv_bytes = (uint16_t*) data;
	struct rpmsg_data_t *rpmsg_data = dev_get_drvdata(&rpdev->dev);
	struct nlmsghdr	*nlh;

	pr_info("%s: CM4 received %d bytes\n", __func__, *cm4_recv_bytes);

	/* sent the reply to netlink */
	skb_out = nlmsg_new(len, GFP_KERNEL);
	if (!skb_out) {
		pr_err("%s: Failed to allocate buffers\n", __func__);
		return(-ENOBUFS);
	}

	nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, len, 0);
	if (!nlh) {
		pr_err("%s: nlmsg_put failed\n", __func__);
		goto err;
	}
	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
	memcpy(NLMSG_DATA(nlh), &rpmsg_data->recv_data_len, sizeof(rpmsg_data->recv_data_len));
	ret = nlmsg_unicast(nl_sk, skb_out, rpmsg_data->client_pid);
	if (ret != 0) {
	    pr_err("%s: error while sending back to user\n", __func__);
		goto err;
	}
	else
		pr_info("%s: sent response to port: %d\n", __func__, rpmsg_data->client_pid);
	goto exit;

err:
	kfree_skb(skb_out);
exit:
	return ret;
}

static int rpmsg_drv_probe(struct rpmsg_device *rpdev)
{
	int ret = 0;
	struct device *dev = &rpdev->dev;
	rpmsg_dev = rpdev;

	struct netlink_kernel_cfg cfg = {
		.input = netlink_recv_cbk,
	};

	struct rpmsg_data_t *rpmsg_data;
	rpmsg_data = devm_kzalloc(dev, sizeof(*rpmsg_data), GFP_KERNEL);
	if (!rpmsg_data)
		return -ENOMEM;


	nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
	if(!nl_sk) {
		dev_err(dev, "Error creating socket.\n");
		return -10;
	}
	dev_info(dev, "%s created netlink socket\n", rpmsg_netlink_driver_name);

	rpmsg_data->recv_data_len = 0;
	rpmsg_data->nlsck = nl_sk;
	dev_set_drvdata(&rpdev->dev, rpmsg_data);

	return ret;
}

static void rpmsg_drv_remove(struct rpmsg_device *rpmsgdev)
{
	netlink_kernel_release(nl_sk);
}

static struct rpmsg_device_id rpmsg_netlink_driver_id_table[] = {
	{ .name	= "rpmsg-netlink" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_netlink_driver_id_table);

static struct rpmsg_driver rpmsg_netlink_drv = {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= rpmsg_netlink_driver_id_table,
	.probe		= rpmsg_drv_probe,
	.callback	= rpmsg_drv_cb,
	.remove		= rpmsg_drv_remove,
};

static int __init rpmsg_netlink_drv_init(void)
{
	int ret = 0;

	/* Register rpmsg device */
	ret = register_rpmsg_driver(&rpmsg_netlink_drv);

	if (ret) {
		pr_err("%s(rpmsg-netlink): Failed to register device\n", __func__);
		return ret;
	}

	pr_info("%s(rpmsg-netlink): Init done\n", __func__);

	return ret;
}

static void __exit rpmsg_netlink_drv_exit(void)
{
	unregister_rpmsg_driver(&rpmsg_netlink_drv);
	pr_info("%s(rpmsg-netlink): Exit\n", __func__);
}

module_init(rpmsg_netlink_drv_init);
module_exit(rpmsg_netlink_drv_exit);


MODULE_AUTHOR("Dimitris Tassopoulos <dimtass@gmail.com>");
MODULE_DESCRIPTION("Testing RPMSG with netlink");
MODULE_VERSION(RPMSG_NETLINK_DRIVER_VERSION);
MODULE_LICENSE("GPL v2");