
#ifndef __HCSIM_IF_H__
#define __HCSIM_IF_H__

#include "lwip/netif.h"
#include "netif/lowpan6.h"

err_t hcsim_if_init(struct netif *netif);
err_t hcsim_if_init_6lowpan(struct netif *netif);

#endif /* __HCSIM_IF_H__ */
