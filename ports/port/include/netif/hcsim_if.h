
#ifndef __HCSIM_IF_H__
#define __HCSIM_IF_H__

#include "lwip/netif.h"
#include "netif/lowpan6.h"
err_t hcsim_if_init(struct netif *netif);
unsigned int get_dest_device_id(char* buf, int len, int this_id);
#if LWIP_IPV6 && LWIP_6LOWPAN
err_t hcsim_if_init_6lowpan(struct netif *netif);
#endif //LWIP_IPV6 && LWIP_6LOWPAN


#endif /* __HCSIM_IF_H__ */
