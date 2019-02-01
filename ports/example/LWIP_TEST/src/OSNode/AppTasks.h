/*********************************************                                                       
 * ECG Tasks Based on Interrupt-Driven Task Model                                                                     
 * Zhuoran Zhao, UT Austin, zhuoran@utexas.edu                                                    
 * Last update: July 2018                                                                         
 ********************************************/

#include "HCSim.h"
#include "lwip_ctxt.h"

#include "netif/hcsim_if.h"
#include "annotation.h"
#include "OmnetIf_pkt.h"
#include "app_utils.h"
#include "service_api.h"

#ifndef SC_TASK_MODEL__H
#define SC_TASK_MODEL__H

#define PROTO TCP

#define PORTNO 11111 //Service for job stealing and sharing
#define SMART_GATEWAY 11112 //Service for a smart gateway 
#define START_CTRL 11113 //Control the start and stop of a service

void tcpip_init_done(void *arg);
void recv_with_sock(void *arg);
void send_with_sock(void *arg);

void send_data_all(raw_data *blob, ctrl_proto proto, int portno){
   ip_addr_t dstaddr;
   int dest_id;   
   OSModelCtxt* OSmodel = sim_ctxt.getTaskCtxt( sc_core::sc_get_current_process_handle() );
   if(OSmodel->NodeID == 1){dest_id = 0;}
   else if(OSmodel->NodeID == 0){dest_id = 1;}
#if IPV4_TASK
   IP_ADDR4(&dstaddr, 192, 168, 0, 2 + dest_id);
   send_data(blob, proto, ipaddr_ntoa(&dstaddr), portno);
#elif IPV6_TASK//IPV4_TASK
   IP_ADDR6(&dstaddr,  1, 2, 3, 4 + dest_id);
   send_data(blob, proto, ipaddr_ntoa(&dstaddr), portno);
#endif//IPV4_TASK
}

void send_task(void *arg){
   OSModelCtxt* OSmodel = sim_ctxt.getTaskCtxt( sc_core::sc_get_current_process_handle() );
   if(OSmodel->NodeID != 1){return;}
   lwip_context *ctxt = (lwip_context *)arg;

   ctrl_proto proto=PROTO;
   raw_data* blob = write_file_to_raw_data("IN.JPG");
   printf("blob->size: %d \n", blob->size);
   //send_data(blob,  proto, "192.168.0.2", PORTNO);
   send_data_all(blob,  proto, PORTNO);
}

void recv_task(void *arg){
   OSModelCtxt* OSmodel = sim_ctxt.getTaskCtxt( sc_core::sc_get_current_process_handle() );
   if(OSmodel->NodeID != 0){return;}
   lwip_context *ctxt = (lwip_context *)arg;

   ctrl_proto proto=PROTO;
   int sock1 = service_init(PORTNO, proto);
   raw_data* blob = recv_data(sock1, proto);
   if(OSmodel->NodeID == 0){
     write_raw_data_to_file("OUT0.JPG", blob);
   }else if(OSmodel->NodeID == 1){
     write_raw_data_to_file("OUT1.JPG", blob);
   }
   free_raw_data(blob);
}

class IntrDriven_Task :public sc_core::sc_module,virtual public HCSim::OS_TASK_INIT 
{
 public:

    sc_core::sc_vector< sc_core::sc_port< sys_call_recv_if > > recv_port;
    sc_core::sc_vector< sc_core::sc_port< sys_call_send_if > > send_port;
    sc_core::sc_port< HCSim::OSAPI > os_port;
    void* g_ctxt;
    OSModelCtxt* OSmodel;

    SC_HAS_PROCESS(IntrDriven_Task);
  	IntrDriven_Task(const sc_core::sc_module_name name, 
            sc_dt::uint64 exe_cost, sc_dt::uint64 period, 
            unsigned int priority, int id, uint8_t init_core,
            sc_dt::uint64 end_sim_time, int NodeID)
    :sc_core::sc_module(name)
    {
        this->exe_cost = exe_cost;
        this->period = period;
        this->priority = priority;
        this->id = id;
        this->end_sim_time = end_sim_time;
	this->NodeID = NodeID;
        this->init_core = 1;
	recv_port.init(2);
	send_port.init(2);
        SC_THREAD(run_jobs);
        g_ctxt=new lwip_context();
        OSmodel = new OSModelCtxt();
	OSmodel->NodeID = NodeID;
	OSmodel->flag_compute=0;
	OSmodel->os_port(this->os_port);
	OSmodel->recv_port[0](this->recv_port[0]);
	OSmodel->recv_port[1](this->recv_port[1]);
	OSmodel->send_port[0](this->send_port[0]);
	OSmodel->send_port[1](this->send_port[1]);	
    }
    
    ~IntrDriven_Task() {}

    void OSTaskCreate(void){
#if IPV4_TASK
	IP_ADDR4(&((lwip_context* )g_ctxt)->gw, 192,168,0,1);
	IP_ADDR4(&((lwip_context* )g_ctxt)->netmask, 255,255,255,0);
	IP_ADDR4(&((lwip_context* )g_ctxt)->ipaddr, 192,168,0,NodeID+2);
#elif IPV6_TASK//IPV4_TASK
	IP_ADDR6(&((lwip_context* )g_ctxt)->ipaddr,  1, 2, 3, (4 + NodeID));
#endif//IPV4_TASK
	printf("Setting up NodeID %d ...\n", NodeID);
	((lwip_context* )g_ctxt)->NodeID = NodeID;
        os_task_id = os_port->taskCreate(sc_core::sc_gen_unique_name("intrdriven_task"), 
                                HCSim::OS_RT_APERIODIC, priority, period, exe_cost, 
                                HCSim::DEFAULT_TS, HCSim::ALL_CORES, init_core);
    }

 private:
    int id;
    uint8_t init_core;
    sc_dt::uint64 exe_cost;
    sc_dt::uint64 period;
    unsigned int priority;
    sc_dt::uint64 end_sim_time;
    int os_task_id;
    int NodeID;

    void run_jobs(void)
    {
	os_port->taskActivate(os_task_id);
	os_port->timeWait(0, os_task_id);
	os_port->syncGlobalTime(os_task_id);
        sim_ctxt.registerTask(OSmodel, g_ctxt, os_task_id, sc_core::sc_get_current_process_handle());
	tcpip_init(tcpip_init_done, g_ctxt);
	printf("Applications started, NodeID is %d %d\n", ((lwip_context* )g_ctxt)->NodeID, sim_ctxt.getTaskID(sc_core::sc_get_current_process_handle()));
	printf("TCP/IP initialized.\n");
	sys_thread_new("send_with_sock", send_task, ((lwip_context* )g_ctxt), DEFAULT_THREAD_STACKSIZE, 0);
	sys_thread_new("recv_with_sock", recv_task, ((lwip_context* )g_ctxt), DEFAULT_THREAD_STACKSIZE, 1);
        os_port->taskTerminate(os_task_id);
    }
};


void tcpip_init_done(void *arg){
   lwip_context* ctxt = (lwip_context*)arg;
#if IPV4_TASK
   netif_set_default(
		netif_add( &(ctxt->netif), (ip_2_ip4(&(ctxt->ipaddr))),	(ip_2_ip4(&(ctxt->netmask))), (ip_2_ip4(&(ctxt->gw))), NULL, hcsim_if_init, tcpip_input)
   );
   netif_set_up(&(ctxt->netif));
#elif IPV6_TASK//IPV4_TASK
#if LOWPAN6_TASK
   netif_add(&(ctxt->netif), NULL, hcsim_if_init_6lowpan, tcpip_6lowpan_input);
   lowpan6_set_pan_id(1);
#else //LOWPAN6_TASK
   netif_add(&(ctxt->netif), NULL, hcsim_if_init, tcpip_input);
#endif //LOWPAN6_TASK
   (ctxt->netif).ip6_autoconfig_enabled = 1;
   netif_create_ip6_linklocal_address(&(ctxt->netif), 1);
   netif_add_ip6_address(&(ctxt->netif), ip_2_ip6(&(ctxt->ipaddr)), NULL);
   netif_set_default(&(ctxt->netif));
   netif_set_up(&(ctxt->netif));
   netif_ip6_addr_set_state(&(ctxt->netif), 0,  IP6_ADDR_PREFERRED);
   netif_ip6_addr_set_state(&(ctxt->netif), 1,  IP6_ADDR_PREFERRED);
#endif//IPV4_TASK   
}

#endif // SC_TASK_MODEL__H 

