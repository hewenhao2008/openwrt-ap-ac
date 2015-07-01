#ifndef CLOUD_WLAN_LOG_H_
#define CLOUD_WLAN_LOG_H_




extern u32 g_cloud_wlan_klog_switch;


extern u32 cloud_wlan_generate_klog_main(struct sk_buff *skb, cloud_wlan_quintuple_s *quintuple_info, u32 type);


#endif

