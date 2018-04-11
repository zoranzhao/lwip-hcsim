/*********************************************                                                       
 * ECG Tasks Based on Interrupt-Driven Task Model                                                                     
 * Zhuoran Zhao, UT Austin, zhuoran@utexas.edu                                                    
 * Last update: July 2018                                                                         
 ********************************************/

#include "HCSim.h"
#include "lwip_ctxt.h"

#include "lwip/opt.h"
#include "lwip/init.h"
#include "lwip/ip_addr.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "lwip/inet_chksum.h"
#include "lwip/tcpip.h"
#include "lwip/sys.h"
#include "lwip/api.h"



#include "netif/hcsim_if.h"



#include "arch/perf.h"

#include "annotation.h"
#include "OmnetIf_pkt.h"
#include "app_utils.h"


#ifndef SC_VISION_TASK__H
#define SC_VISION_TASK__H




void tcpip_init_done(void *arg);
void recv_dats(void *arg);
void send_dats(void *arg);



class IntrDriven_Task
    :public sc_core::sc_module
  	,virtual public HCSim::OS_TASK_INIT 
{
 public:

    sc_core::sc_vector< sc_core::sc_port< lwip_recv_if > > recv_port;
    sc_core::sc_vector< sc_core::sc_port< lwip_send_if > > send_port;

    //sc_core::sc_port<lwip_recv_if> recv_port[2];
    //sc_core::sc_port<lwip_send_if> send_port[2];  
    sc_core::sc_port< HCSim::OSAPI > os_port;
    void* g_ctxt;

   

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

        g_ctxt=new LwipCntxt();
	((LwipCntxt* )g_ctxt) -> OSmodel = (new OSModelCtxt());

	((OSModelCtxt*)(((LwipCntxt* )g_ctxt)->OSmodel))->NodeID = NodeID;
	((OSModelCtxt*)(((LwipCntxt* )g_ctxt)->OSmodel))->flag_compute=0;
	((OSModelCtxt*)(((LwipCntxt* )g_ctxt)->OSmodel))->os_port(this->os_port);
	((OSModelCtxt*)(((LwipCntxt* )g_ctxt)->OSmodel))->recv_port[0](this->recv_port[0]);
	((OSModelCtxt*)(((LwipCntxt* )g_ctxt)->OSmodel))->recv_port[1](this->recv_port[1]);
	((OSModelCtxt*)(((LwipCntxt* )g_ctxt)->OSmodel))->send_port[0](this->send_port[0]);
	((OSModelCtxt*)(((LwipCntxt* )g_ctxt)->OSmodel))->send_port[1](this->send_port[1]);


		
    }
    
    ~IntrDriven_Task() {}

    void OSTaskCreate(void)
    {
//IPV4 program
	IP_ADDR4(&((LwipCntxt* )g_ctxt)->gw, 192,168,0,1);
	IP_ADDR4(&((LwipCntxt* )g_ctxt)->netmask, 255,255,255,0);
	IP_ADDR4(&((LwipCntxt* )g_ctxt)->ipaddr, 192,168,0,NodeID+2);
	printf("Setting up NodeID %d ............... \n", NodeID);


	((LwipCntxt* )g_ctxt)->NodeID = NodeID;

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

        taskManager.registerTask( (OSModelCtxt*)(((LwipCntxt*)(g_ctxt))->OSmodel ), g_ctxt, os_task_id, sc_core::sc_get_current_process_handle());


	//netif_init();
	tcpip_init(tcpip_init_done, g_ctxt);
	printf("Applications started, NodeID is %d %d\n", ((LwipCntxt* )g_ctxt)->NodeID, taskManager.getTaskID(sc_core::sc_get_current_process_handle()));
	printf("TCP/IP initialized.\n");
	sys_thread_new("send_dats", send_dats, ((LwipCntxt* )g_ctxt), DEFAULT_THREAD_STACKSIZE, init_core);
	recv_dats(g_ctxt);


        os_port->taskTerminate(os_task_id);





    }



};



void tcpip_init_done(void *arg)
{

  LwipCntxt* ctxt = (LwipCntxt*)arg;//LWIP_IPV4
  netif_set_default(
		netif_add( &(ctxt->netif), (ip_2_ip4(&(ctxt->ipaddr))), (ip_2_ip4(&(ctxt->netmask))), (ip_2_ip4(&(ctxt->gw))), NULL, hcsim_if_init, tcpip_input)
  );//LWIP_IPV4


  netif_set_up(&(ctxt->netif));//LWIP_IPV4

  


}



//int load_file_to_memory(const char *filename, char **result);
//void dump_mem_to_file(unsigned int size, const char *filename, char **result);



void send_dats(void *arg)
{
  OSModelCtxt* OSmodel = taskManager.getTaskCtxt( sc_core::sc_get_current_process_handle() );
  if(OSmodel->NodeID != 1){return;}
  struct netconn *conn;
  err_t err;
  LwipCntxt *ctxt = (LwipCntxt *)arg;
  /* Create a new connection identifier. */
  conn = netconn_new(NETCONN_TCP);
  /* Bind connection to well known port number 7. */
  //char this_str[16];
  //ipaddr_ntoa_r(&(((LwipCntxt* )ctxt)->ipaddr_dest), this_str, 16);
  unsigned int buf_size;
  char* buf;
  //buf = (char*)malloc( buf_size );
  //printf("Size is %d \n", buf_size);
  //buf[buf_size-1] = 'c';

  buf_size = load_file_to_memory("./IN.JPG", &buf);

  char this_str[16];
  IP_ADDR4(&((LwipCntxt* )arg)->ipaddr_dest, 192, 168, 0, (2));
  ipaddr_ntoa_r(&(((LwipCntxt* )arg)->ipaddr_dest), this_str, 16);
  printf("dest ip is %s\n", this_str);
  err = netconn_connect(conn, &(((LwipCntxt* )ctxt)->ipaddr_dest), 7);

  //std::cout << "Connected in node " << (((LwipCntxt* )ctxt)->NodeID) << " connect to "<< this_str << std::endl;
  printf("%d, ", err);
  if (err != ERR_OK) {
    std::cout << "Connection refused, "  << "error code is: "  << std::endl;
    return;
  }


  size_t *bytes_written=NULL; 
/*
  char dat[30];   
  sprintf(dat, "send data %d", ((LwipCntxt* )ctxt)->NodeID);//large
*/
  //std::cout << "Before netconn_write_partly: "<< (((LwipCntxt* )ctxt)->NodeID)<<"  "<<buf_size<<" time: " << sc_core::sc_time_stamp().value() << std::endl;
  while (1) {

    printf(" ====================== netconn_write_partly ======================\n");
    err = netconn_write_partly(conn, buf, (buf_size), NETCONN_COPY, bytes_written);
    //err = netconn_write_partly(ctxt, conn, dat, 30, NETCONN_COPY, bytes_written);
    if (err == ERR_OK) 
    {
      netconn_close(conn);
      netconn_delete(conn);
      break;
    }
  }
  printf(" ====================== netconn_write_partly done======================\n");




  //std::cout << "After netconn_write_partly: "<< (((LwipCntxt* )ctxt)->NodeID)<<"  "<<buf_size<<" time: " << sc_core::sc_time_stamp().value() << std::endl;
  free(buf);
}



void recv_dats(void *arg)
{
  OSModelCtxt* OSmodel = taskManager.getTaskCtxt( sc_core::sc_get_current_process_handle() );
  if(OSmodel->NodeID != 0){return;}
  char this_str[16];
  ipaddr_ntoa_r(&(((LwipCntxt* )arg)->ipaddr_dest), this_str, 16);

  struct netconn *conn, *newconn;
  err_t err;
  LwipCntxt *ctxt = (LwipCntxt *)arg;
  conn = netconn_new_with_proto_and_callback(NETCONN_TCP, 0, NULL);
  /* Bind connection to well known port number 7. */
  netconn_bind(conn, NULL, 7);
  /* Tell connection to go into listening mode. */
  netconn_listen(conn);

  int taskID = taskManager.getTaskID( sc_core::sc_get_current_process_handle());
  OSmodel->Blocking_taskID = taskID;
  int push;

  while(1){
    std::cout << "====================== netconn_accept ======================"  << " time is: " << sc_core::sc_time_stamp().value() << std::endl;
    err = netconn_accept(conn, &newconn);
    if (err == ERR_OK) {
      struct netbuf *buf;
      void *data;
      u16_t len;
      int ii;
      char *img = (char*)malloc( 4000000 );
      int img_ii = 0;
      while ((err = netconn_recv(newconn, &buf)) == ERR_OK) {
        do {
             netbuf_data(buf, &data, &len);

             for(ii=0;ii<(len);ii++)
		img[img_ii+ii]=((char*)data)[ii];
             img_ii+=len;
        } while (netbuf_next(buf) >= 0);
        if((buf->p->flags) & PBUF_FLAG_PUSH){

		push++; 
		std::cout<< img_ii << "   "<< push<<std::endl;

	}

        netbuf_delete(buf);
      }
      //std::cout<< img_ii << "   "<< img[img_ii -1]<<std::endl;

      dump_mem_to_file(img_ii, "./OUT.JPG", &img);
      free(img);

      netconn_close(newconn);
      netconn_delete(newconn);

    }
  }

}








#endif // SC_VISION_TASK__H 
