/*********************************************                                                       
 * ECG Tasks Based on Interrupt-Driven Task Model                                                                     
 * Zhuoran Zhao, UT Austin, zhuoran@utexas.edu                                                    
 * Last update: July 2018                                                                         
 ********************************************/

#include "HCSim.h"
#include "lwip_ctxt.h"
/*
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
*/


#include "netif/hcsim_if.h"
#include "annotation.h"
#include "OmnetIf_pkt.h"
#include "app_utils.h"

#include "service_api.h"

#ifndef SC_TASK_MODEL__H
#define SC_TASK_MODEL__H




void tcpip_init_done(void *arg);
void recv_dats(void *arg);
void send_dats(void *arg);
void recv_with_sock(void *arg);
void send_with_sock(void *arg);


class IntrDriven_Task
    :public sc_core::sc_module
  	,virtual public HCSim::OS_TASK_INIT 
{
 public:

    sc_core::sc_vector< sc_core::sc_port< lwip_recv_if > > recv_port;
    sc_core::sc_vector< sc_core::sc_port< lwip_send_if > > send_port;

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
	tcpip_init(tcpip_init_done, g_ctxt);
	printf("Applications started, NodeID is %d %d\n", ((LwipCntxt* )g_ctxt)->NodeID, taskManager.getTaskID(sc_core::sc_get_current_process_handle()));
	printf("TCP/IP initialized.\n");

	sys_thread_new("send_with_sock", send_with_sock, ((LwipCntxt* )g_ctxt), DEFAULT_THREAD_STACKSIZE, init_core);
        recv_with_sock(g_ctxt);


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


void read_sock_tcp(int sock, char* buffer, unsigned int bytes_length){
    size_t bytes_read = 0;
    int n;
    while (bytes_read < bytes_length){
        n = lwip_recv(sock, buffer + bytes_read, bytes_length - bytes_read, 0);
        if( n < 0 ) printf("ERROR reading socket");
        bytes_read += n;
    }

};

void write_sock_tcp(int sock, char* buffer, unsigned int bytes_length){
    size_t bytes_written = 0;
    int n;
    while (bytes_written < bytes_length) {
        n = lwip_send(sock, buffer + bytes_written, bytes_length - bytes_written, 0);
        if( n < 0 ) printf("ERROR writing socket");
        bytes_written += n;
    }
};



#define UDP_TRANS_SIZE 512

void read_sock_udp(int sock, char* buffer, unsigned int bytes_length, struct sockaddr *from, socklen_t *fromlen){  
    size_t bytes_read = 0;
    int n;
    printf("read_sock_udp size is %u\n", bytes_length);
    while (bytes_read < bytes_length){
        if((bytes_length - bytes_read) < UDP_TRANS_SIZE) { n = bytes_length - bytes_read; }
	else { n = UDP_TRANS_SIZE; }
        printf("read_sock_udp n size is %d\n", n);
        lwip_recvfrom(sock, buffer + bytes_read, n, 0, from, fromlen);
        bytes_read += n;
    }

};

void write_sock_udp(int sock, char* buffer, unsigned int bytes_length, const struct sockaddr *to, socklen_t tolen){
    size_t bytes_written = 0;
    int n;
    printf("write_sock_udp size is %u\n", bytes_length);
    while (bytes_written < bytes_length) {
        if((bytes_length - bytes_written) < UDP_TRANS_SIZE) { n = bytes_length - bytes_written; }
	else { n = UDP_TRANS_SIZE; }
        printf("write_sock_udp n size is %d\n", n);
        lwip_sendto(sock, buffer + bytes_written, n, 0, to, tolen);
        bytes_written += n;
    }
};

void send_data_tcp(char *blob_buffer, unsigned int bytes_length, const char *dest_ip, int portno)
{
     int sockfd;
     struct sockaddr_in serv_addr;
     sockfd = lwip_socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        printf("ERROR opening socket");
     memset(&serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = inet_addr(dest_ip) ;
     serv_addr.sin_port = htons(portno);
     if (lwip_connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
	printf("ERROR connecting");
     write_sock_tcp(sockfd, (char*)&bytes_length, sizeof(bytes_length));
     write_sock_tcp(sockfd, blob_buffer, bytes_length);
     lwip_close(sockfd);
}

void recv_data_tcp(int portno)
{
   int sockfd, newsockfd;
   socklen_t clilen;

   struct sockaddr_in serv_addr, cli_addr;
   sockfd = lwip_socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd < 0) 
	printf("ERROR opening socket");
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);
   if (lwip_bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
	printf("ERROR on binding");
   lwip_listen(sockfd, 10);//back_log numbers 
   clilen = sizeof(cli_addr);

   unsigned int bytes_length;
   char* blob_buffer;
   int job_id;
   unsigned int id;
   blob_buffer = (char*)malloc(48000000);
   while(1){
     	newsockfd = lwip_accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0) printf("ERROR on accept");
	read_sock_tcp(newsockfd, (char*)&bytes_length, sizeof(bytes_length));
	printf("bytes_length is = %d\n", bytes_length);
	read_sock_tcp(newsockfd, blob_buffer, bytes_length); 
        dump_mem_to_file(bytes_length, "./OUT.JPG", &blob_buffer);
     	lwip_close(newsockfd);
   }
   lwip_close(sockfd);

}

void send_data_udp(char *blob_buffer, unsigned int bytes_length, const char *dest_ip, int portno)
{
   int taskID = taskManager.getTaskID(sc_core::sc_get_current_process_handle());
   OSModelCtxt* OSmodel = taskManager.getTaskCtxt( sc_core::sc_get_current_process_handle() );

   int sockfd;
   struct sockaddr_in serv_addr;
   sockfd = lwip_socket(AF_INET, SOCK_DGRAM, 0);
   if (sockfd < 0) 
        printf("ERROR opening socket");
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = inet_addr(dest_ip) ;
   serv_addr.sin_port = htons(portno);

   write_sock_udp(sockfd, (char*)&bytes_length, sizeof(bytes_length), (struct sockaddr *) &serv_addr, sizeof(serv_addr));
   write_sock_udp(sockfd, blob_buffer, bytes_length, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
   lwip_close(sockfd);
}

void recv_data_udp(int portno)
{
   int taskID = taskManager.getTaskID(sc_core::sc_get_current_process_handle());
   OSModelCtxt* OSmodel = taskManager.getTaskCtxt( sc_core::sc_get_current_process_handle() );

   int sockfd, newsockfd;
   socklen_t clilen;
   struct sockaddr_in serv_addr, cli_addr;
   sockfd = lwip_socket(AF_INET, SOCK_DGRAM, 0);
   if (sockfd < 0) 
	printf("ERROR opening socket");
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);
   if (lwip_bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
	printf("ERROR on binding");
   clilen = sizeof(cli_addr);
   unsigned int bytes_length;
   char* blob_buffer;
   int job_id;
   unsigned int id;
   blob_buffer = (char*)malloc(48000000);
   read_sock_udp(sockfd, (char*)&bytes_length, sizeof(bytes_length), (struct sockaddr *) &cli_addr, &clilen);
   read_sock_udp(sockfd, blob_buffer, bytes_length, (struct sockaddr *) &cli_addr, &clilen);
   printf("bytes_length = %u   \n;", bytes_length);
   dump_mem_to_file(bytes_length, "./OUT.JPG", &blob_buffer);
   lwip_close(sockfd);
}

void send_with_sock(void *arg)
{
  OSModelCtxt* OSmodel = taskManager.getTaskCtxt( sc_core::sc_get_current_process_handle() );
  if(OSmodel->NodeID != 1){return;}
  LwipCntxt *ctxt = (LwipCntxt *)arg;
  char dest_ip[16];
  IP_ADDR4(&((LwipCntxt* )arg)->ipaddr_dest, 192, 168, 0, (2));
  ipaddr_ntoa_r(&(((LwipCntxt* )arg)->ipaddr_dest), dest_ip, 16);
  printf("dest ip is %s\n", dest_ip);
  unsigned int buf_size;
  char* buf;
  buf_size = load_file_to_memory("./IN.JPG", &buf);
  send_data_udp(buf, buf_size, dest_ip, 7);
  free(buf);
}

void recv_with_sock(void *arg)
{
  OSModelCtxt* OSmodel = taskManager.getTaskCtxt( sc_core::sc_get_current_process_handle() );
  if(OSmodel->NodeID != 0){return;}
  LwipCntxt *ctxt = (LwipCntxt *)arg;
  recv_data_udp(7);
}


/*
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
  netconn_bind(conn, NULL, 7);
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
      dump_mem_to_file(img_ii, "./OUT.JPG", &img);
      free(img);
      netconn_close(newconn);
      netconn_delete(newconn);
    }
  }

}



void send_dats(void *arg)
{
  OSModelCtxt* OSmodel = taskManager.getTaskCtxt( sc_core::sc_get_current_process_handle() );
  if(OSmodel->NodeID != 1){return;}
  struct netconn *conn;
  err_t err;
  LwipCntxt *ctxt = (LwipCntxt *)arg;
  conn = netconn_new(NETCONN_TCP);
  unsigned int buf_size;
  char* buf;

  buf_size = load_file_to_memory("./IN.JPG", &buf);
  char this_str[16];
  IP_ADDR4(&((LwipCntxt* )arg)->ipaddr_dest, 192, 168, 0, (2));
  ipaddr_ntoa_r(&(((LwipCntxt* )arg)->ipaddr_dest), this_str, 16);
  printf("dest ip is %s\n", this_str);
  err = netconn_connect(conn, &(((LwipCntxt* )ctxt)->ipaddr_dest), 7);

  printf("%d, ", err);
  if (err != ERR_OK) {
    std::cout << "Connection refused, "  << "error code is: "  << std::endl;
    return;
  }
  size_t *bytes_written=NULL; 
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
  free(buf);
}
*/
#endif // SC_TASK_MODEL__H 

