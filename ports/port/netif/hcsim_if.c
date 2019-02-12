/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#include "lwip/debug.h"

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/ip.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "netif/hcsim_if.h"
#include "lwip_ctxt.h"
#include "netif/etharp.h"
#include "lwip/ethip6.h"

#if defined(LWIP_DEBUG) && defined(LWIP_TCPDUMP)
#include "netif/tcpdump.h"
#endif /* LWIP_DEBUG && LWIP_TCPDUMP */
#ifndef HCSIM_IF_DEBUG
#define HCSIM_IF_DEBUG LWIP_DBG_OFF
#endif
#include <systemc>
#include "os_ctxt.h"


struct hcsim_if {
  struct eth_addr *ethaddr;
  /* Add whatever per-interface state that is needed here. */
  int fd;
};

/* Forward declarations. */
static void  hcsim_if_input(struct netif *netif);

static void hcsim_if_thread(void *arg);



/*--------------------Code added as the virtual interface-----------------------*/



static int server_write(const void *buf, size_t len) {

    int err=len;

    int ii;
/*
    static char this_str[16];
    ipaddr_ntoa_r( &(((lwip_context* )(ctxt))->ipaddr_dest), this_str, 16);

    static char dest_str[16];
    static char src_str[16];
    ipaddr_ntoa_r( &(((lwip_context* )(ctxt))->current_iphdr_dest), dest_str, 16);
    ipaddr_ntoa_r( &(((lwip_context* )(ctxt))->current_iphdr_src), src_str, 16);
*/
    int taskID =  ( sim_ctxt.get_task_id( sc_core::sc_get_current_process_handle() ));
    ( sim_ctxt.get_os_ctxt( sc_core::sc_get_current_process_handle() ))->send_port[0]->set_size(len, taskID);
    ( sim_ctxt.get_os_ctxt( sc_core::sc_get_current_process_handle() ))->send_port[0]->set_data(len, (char*)buf, taskID);

    return err;

}

static int server_read(void *buf, size_t len) {

    int pkt_size;
    int ii;
    int err=0;
    int taskID = sim_ctxt.get_task_id( sc_core::sc_get_current_process_handle() );
    pkt_size =   ( sim_ctxt.get_os_ctxt( sc_core::sc_get_current_process_handle() ))->recv_port[0]->get_size(taskID);
     ( sim_ctxt.get_os_ctxt( sc_core::sc_get_current_process_handle() ))->recv_port[0]->get_data(pkt_size, (char*)buf, taskID);
    err = pkt_size;
    return err;

}





/*-----------------------------------------------------------------------------------*/
static void
low_level_init(struct netif *netif)
{

  //struct hcsim_if *hcsim_if;


  //hcsim_if = (struct hcsim_if *)netif->state;

  /* Obtain MAC address from network interface. */

  /* (We just fake an address...) */
  //hcsim_if->ethaddr->addr[0] = 0x1;
  //hcsim_if->ethaddr->addr[1] = 0x2;
  //hcsim_if->ethaddr->addr[2] = 0x3;
  //hcsim_if->ethaddr->addr[3] = 0x4;
  //hcsim_if->ethaddr->addr[4] = 0x5;
  //hcsim_if->ethaddr->addr[5] = 0x6;
  netif->hwaddr_len = 6;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
  netif_set_link_up(netif);
  sys_thread_new("hcsim_if_thread", hcsim_if_thread, netif, 100, 0);

  //Necessary steps of initializing a IPv6 interface.
  //Pre-defined variables needed for IPv6 initialization
  //static ip6_addr_t addr6; 
  /*
    IP6_ADDR2(&addr6, a, b, c, d, e, f, g, h);                      
    interface6.mtu = _mtu;
    interface6.name[0] = 't';
    interface6.name[1] = 'p';
    interface6.hwaddr_len = 6;
    interface6.linkoutput = low_level_output;
    interface6.ip6_autoconfig_enabled = 1;

    netif_create_ip6_linklocal_address(&interface6, 1);
    netif_add(&interface6, NULL, tapif_init, ethernet_input);
    netif_set_default(&interface6);
    netif_set_up(&interface6);      

    netif_ip6_addr_set_state(&interface6, 1, IP6_ADDR_TENTATIVE); 
    _mac.copyTo(interface6.hwaddr, interface6.hwaddr_len);
    ip6_addr_copy(ip_2_ip6(interface6.ip6_addr[1]), addr6);

    interface6.output_ip6 = ethip6_output;//Route the outgoing packets to proper interface function.
    interface6.state = this;
    interface6.flags = NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;

  */




}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
/*-----------------------------------------------------------------------------------*/

static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{

  struct pbuf *q;
  char buf[1514];
  char *bufptr;
  struct hcsim_if *hcsim_if;
//  ((lwip_context* )(ctxt));

  hcsim_if = (struct hcsim_if *)netif->state;
#if 0
    if(((double)rand()/(double)RAND_MAX) < 0.2) {
    printf("drop output\n");
    return ERR_OK;
    }
#endif
  /* initiate transfer(); */

  bufptr = &buf[0];

  for(q = p; q != NULL; q = q->next) {
    /* Send the data from the pbuf to the interface, one pbuf at a
       time. The size of the data in each pbuf is kept in the ->len
       variable. */
    /* send data from(q->payload, q->len); */
    memcpy(bufptr, q->payload, q->len);
    bufptr += q->len;
  }

  /* signal that packet should be sent(); */
  //if(server_write(buf, p->tot_len) == -1) {
    //perror("hcsim_if: write");
  //}
  server_write(buf, p->tot_len);

  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
/*-----------------------------------------------------------------------------------*/
static struct pbuf *
low_level_input(struct netif *netif)
{
  struct pbuf *p, *q;
  u16_t len;
  char buf[1514];
  char *bufptr;

  len = server_read(buf, sizeof(buf));


  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

  if(p != NULL) {
    /* We iterate over the pbuf chain until we have read the entire
       packet into the pbuf. */
    bufptr = &buf[0];
    for(q = p; q != NULL; q = q->next) {
      /* Read enough bytes to fill this pbuf in the chain. The
         available data in the pbuf is given by the q->len
         variable. */
      /* read data into(q->payload, q->len); */
      memcpy(q->payload, bufptr, q->len);
      bufptr += q->len;
    }
    /* acknowledge that packet has been read(); */
  } else {
    printf("Could not allocate pbufs, or nothing is read out of the device\n");
    /* drop packet(); */
  }

  return p;
}
/*-----------------------------------------------------------------------------------*/
static void
hcsim_if_thread(void *arg)
{
  struct netif *netif;
  struct hcsim_if *hcsim_if;
  int ret;

  netif = (struct netif *)arg;
  hcsim_if = (struct hcsim_if *)netif->state;

  while(1) {
     //printf("hcsim_if_thread reading\n");
    /* Wait for a packet to arrive. */
    /* Handle incoming packet. */
      hcsim_if_input(netif);
    /*if nothing arrives, the server_read() in low_level_input() will be blocked*/

  }
}
/*-----------------------------------------------------------------------------------*/
/*
 * hcsim_if_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
/*-----------------------------------------------------------------------------------*/

static void
hcsim_if_input(struct netif *netif)
{
  struct pbuf *p = low_level_input(netif);

  if (p == NULL) {
#if LINK_STATS
    LINK_STATS_INC(link.recv);
#endif /* LINK_STATS */
    LWIP_DEBUGF(TAPIF_DEBUG, ("tapif_input: low_level_input returned NULL\n"));
    return;
  }
  if (netif->input(p, netif) != ERR_OK) {
    LWIP_DEBUGF(NETIF_DEBUG, ("tapif_input: netif input error\n"));
    pbuf_free(p);
  }
}
/*-----------------------------------------------------------------------------------*/
/*
 * hcsim_if_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
/*-----------------------------------------------------------------------------------*/
err_t
hcsim_if_init(struct netif *netif)
{

  void* ctxt;
  ctxt = sim_ctxt.get_app_ctxt(sc_core::sc_get_current_process_handle())->get_context("lwIP");//HCSim

 
  struct hcsim_if *hcsim_if; // A holder for ethernet address and device number
  hcsim_if = (struct hcsim_if *)mem_malloc(sizeof(struct hcsim_if));
  if (!hcsim_if) {
    return ERR_MEM;
  }

  netif->state = hcsim_if;   // 
#if LWIP_IPV4
  netif->output = etharp_output;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = low_level_output;
  netif->mtu = 1500;
  /* hardware address length */

  netif->hwaddr[0] = 0x00;
  netif->hwaddr[1] = 0x00;
  netif->hwaddr[2] = 0x00;
  netif->hwaddr[3] = 0x00;
  netif->hwaddr[4] = 0x00;
  netif->hwaddr[5] = 0x00 +  (((lwip_context*)ctxt)->node_id);;


  netif->hwaddr_len = 6;
  //netif->hwaddr_len = 6;
  //hcsim_if->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
  //netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;

  low_level_init(netif);

  return ERR_OK;
}
#if LWIP_IPV6 && LWIP_6LOWPAN
err_t
hcsim_if_init_6lowpan(struct netif *netif)
{

  void* ctxt;
  ctxt = sim_ctxt.get_app_ctxt(sc_core::sc_get_current_process_handle())->get_context("lwIP");//HCSim

 
  struct hcsim_if *hcsim_if; // A holder for ethernet address and device number
  hcsim_if = (struct hcsim_if *)mem_malloc(sizeof(struct hcsim_if));
  if (!hcsim_if) {
    return ERR_MEM;
  }
  netif->state = hcsim_if;
  lowpan6_if_init(netif);
  //lowpan6_set_pan_id(1);
  netif->linkoutput = low_level_output;

  /* hardware address length */

  netif->hwaddr[0] = 0x00;
  netif->hwaddr[1] = 0x00;
  netif->hwaddr[2] = 0x00;
  netif->hwaddr[3] = 0x00;
  netif->hwaddr[4] = 0x00;
  netif->hwaddr[5] = 0x00 + (((lwip_context*)ctxt)->NodeID);;
  netif->hwaddr_len = 6;
  //netif->hwaddr_len = 6;
  //hcsim_if->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
  //netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;

  low_level_init(netif);

  return ERR_OK;
}
#endif//LWIP_IPV6 && LWIP_6LOWPAN
/*-----------------------------------------------------------------------------------*/
