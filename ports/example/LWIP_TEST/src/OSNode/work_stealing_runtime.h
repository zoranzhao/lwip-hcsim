#include "darkiot.h"
#include "configure.h"
#include <string.h>
#ifndef WORK_STEALING_RUNTIME_H
#define WORK_STEALING_RUNTIME_H

extern const char* addr_list[MAX_EDGE_NUM];
/*
void test_gateway(uint32_t total_number){
   device_ctxt* ctxt = init_gateway(total_number, addr_list);
   set_gateway_local_addr(ctxt, GATEWAY_LOCAL_ADDR);
   set_gateway_public_addr(ctxt, GATEWAY_PUBLIC_ADDR);
   set_total_frames(ctxt, FRAME_NUM);
   set_batch_size(ctxt, BATCH_SIZE);

   sys_thread_t t3 = sys_thread_new("work_stealing_thread", work_stealing_thread, ctxt, 0, 0);
   sys_thread_t t1 = sys_thread_new("collect_result_thread", collect_result_thread, ctxt, 0, 0);
   sys_thread_t t2 = sys_thread_new("merge_result_thread", merge_result_thread, ctxt, 0, 0);
   exec_barrier(START_CTRL, TCP, ctxt);

   sys_thread_join(t1);
   sys_thread_join(t2);
   sys_thread_join(t3);
}
*/


#ifndef UDP_TRANS_SIZE
#define UDP_TRANS_SIZE 512
#endif
static inline void read_from_sock(int sock, ctrl_proto proto, uint8_t* buffer, uint32_t bytes_length, struct sockaddr *from, socklen_t *fromlen){
   uint32_t bytes_read = 0;
   int32_t n = 0;
   while (bytes_read < bytes_length){
      if(proto == TCP){
         n = lwip_recv(sock, buffer + bytes_read, bytes_length - bytes_read, 0);
         if( n < 0 ) printf("ERROR reading socket\n");
      }else if(proto == UDP){
         if((bytes_length - bytes_read) < UDP_TRANS_SIZE) { n = bytes_length - bytes_read; }
         else { n = UDP_TRANS_SIZE; }
         if( lwip_recvfrom(sock, buffer + bytes_read, n, 0, from, fromlen) < 0) printf("ERROR reading socket\n");
      }else{printf("Protocol is not supported\n");}
      bytes_read += n;
   }
}
static inline void write_to_sock(int sock, ctrl_proto proto, uint8_t* buffer, uint32_t bytes_length, const struct sockaddr *to, socklen_t tolen){
   uint32_t bytes_written = 0;
   int32_t n = 0;
   while (bytes_written < bytes_length) {
      if(proto == TCP){
         n = lwip_send(sock, buffer + bytes_written, bytes_length - bytes_written, 0);
         if( n < 0 ) printf("ERROR writing socket\n");
      }else if(proto == UDP){
         if((bytes_length - bytes_written) < UDP_TRANS_SIZE) { n = bytes_length - bytes_written; }
         else { n = UDP_TRANS_SIZE; }
         if(lwip_sendto(sock, buffer + bytes_written, n, 0, to, tolen)< 0) 
	   printf("ERROR writing socket\n");
      }else{printf("Protocol is not supported\n"); return;}
      bytes_written += n;
   }
}

static void process_task(blob* temp, device_ctxt* ctxt){
   blob* result;
   char data[20] = "output_data";
   result = new_blob_and_copy_data(temp->id, 20, (uint8_t*)data);
   copy_blob_meta(result, temp);

   /*We must have certain amount of delay*/
   os_model_context* os_model = sim_ctxt.get_os_ctxt( sc_core::sc_get_current_process_handle() );
   os_model->os_port->timeWait(3000000000000, sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle()));
   os_model->os_port->syncGlobalTime(sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle()));
   /*We must have certain amount of delay*/

   enqueue(ctxt->result_queue, result); 
   free_blob(result);
}

void generate_and_process_thread_no_gateway(void *arg){
   device_ctxt* ctxt = (device_ctxt*)arg;
   uint32_t task;
   blob* temp;
   char data[20] = "input_data";
   uint32_t frame_num;
   for(frame_num = 0; frame_num < ctxt->total_frames; frame_num ++){
      /*register_client(ctxt);*/
      for(task = 0; task < ctxt->batch_size; task ++){
         temp = new_blob_and_copy_data((int32_t)task, 20, (uint8_t*)data);
         annotate_blob(temp, get_this_client_id(ctxt), frame_num, task);
         enqueue(ctxt->task_queue, temp);
         free_blob(temp);
      }
      while(1){
         temp = try_dequeue(ctxt->task_queue);
         if(temp == NULL) break;
         process_task(temp, ctxt);
         printf("Process task locally, frame %d, task is %d\n", frame_num, temp->id);
         free_blob(temp);
      }
   }
}


void send_data_without_conn(int sockfd, blob *temp){
   void* data;
   uint32_t bytes_length;
   void* meta;
   uint32_t meta_size;
   int32_t id;
   data = temp->data;
   bytes_length = temp->size;
   meta = temp->meta;
   meta_size = temp->meta_size;
   id = temp->id;
   write_to_sock(sockfd, TCP, (uint8_t*)&meta_size, sizeof(meta_size), NULL, NULL);
   if(meta_size > 0)
      write_to_sock(sockfd, TCP, (uint8_t*)meta, meta_size, NULL, NULL);
   write_to_sock(sockfd, TCP, (uint8_t*)&id, sizeof(id), NULL, NULL);
   write_to_sock(sockfd, TCP, (uint8_t*)&bytes_length, sizeof(bytes_length), NULL, NULL);
   write_to_sock(sockfd, TCP, (uint8_t*)data, bytes_length, NULL, NULL);
}

void steal_and_process_thread_no_gateway(void *arg){
   /*Check gateway for possible stealing victims*/
   device_ctxt* ctxt = (device_ctxt*)arg;
   service_conn* conn;
   blob* temp;
   uint32_t dest_id = 0;
   while(1){
      conn = connect_service(TCP, addr_list[dest_id], WORK_STEAL_PORT);
      char* a = "Okay, it is a pure test case, first...";
      write_to_sock(conn->sockfd, TCP, (uint8_t*)a, 60, NULL, NULL);
      write_to_sock(conn->sockfd, TCP, (uint8_t*)a, 60, NULL, NULL);
   os_model_context* os_model = sim_ctxt.get_os_ctxt( sc_core::sc_get_current_process_handle() );
   os_model->os_port->timeWait(3000000000000, sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle()));
   os_model->os_port->syncGlobalTime(sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle()));
      char buffer[60];
      read_from_sock(conn->sockfd, TCP, (uint8_t*)buffer, 60, NULL, NULL);

   os_model->os_port->timeWait(3000000000000, sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle()));
   os_model->os_port->syncGlobalTime(sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle()));

      printf("Recv 1 piece of data on client side, %s\n", buffer);
      write_to_sock(conn->sockfd, TCP, (uint8_t*)a, 60, NULL, NULL);
      write_to_sock(conn->sockfd, TCP, (uint8_t*)a, 60, NULL, NULL);
      write_to_sock(conn->sockfd, TCP, (uint8_t*)a, 60, NULL, NULL);



      close_service_connection(conn);
      //if(temp->id == -1){
      //   free_blob(temp);
      //   sys_sleep(100);
      //   continue;
      //}
      //process_task(temp, ctxt);
      //free_blob(temp);
      break;
   }
}

void test_stealer_client(uint32_t edge_id){
   device_ctxt* ctxt = init_client(edge_id);
   set_gateway_local_addr(ctxt, GATEWAY_LOCAL_ADDR);
   set_gateway_public_addr(ctxt, GATEWAY_PUBLIC_ADDR);
   set_total_frames(ctxt, FRAME_NUM);
   set_batch_size(ctxt, BATCH_SIZE);

   /*exec_barrier(START_CTRL, TCP, ctxt);*/
   /*TODO some sort of barrier must exist here*/

   sys_thread_t t1 = sys_thread_new("steal_and_process_thread_no_gateway", steal_and_process_thread_no_gateway, ctxt, 0, 0);
   /*sys_thread_t t2 = sys_thread_new("send_result_thread", send_result_thread, ctxt, 0, 0);*/

   sys_thread_join(t1);
   /*sys_thread_join(t2);*/

}

void test_victim_client(uint32_t edge_id){
   device_ctxt* ctxt = init_client(edge_id);
   set_gateway_local_addr(ctxt, GATEWAY_LOCAL_ADDR);
   set_gateway_public_addr(ctxt, GATEWAY_PUBLIC_ADDR);
   set_total_frames(ctxt, FRAME_NUM);
   set_batch_size(ctxt, BATCH_SIZE);

   /*exec_barrier(START_CTRL, TCP, ctxt);*/
   /*TODO some sort of barrier must exist here*/
   sys_thread_t t3 = sys_thread_new("serve_stealing_thread", serve_stealing_thread, ctxt, 0, 0);
   //sys_thread_t t1 = sys_thread_new("generate_and_process_thread_no_gateway", generate_and_process_thread_no_gateway, ctxt, 0, 0);
   /*sys_thread_t t2 = sys_thread_new("send_result_thread", send_result_thread, ctxt, 0, 0);*/

   //sys_thread_join(t1);
   sys_thread_join(t3);
   /*sys_thread_join(t3);*/
}

/*

void test_wst(int argc, char **argv)
{

   printf("this_cli_id %d\n", atoi(argv[1]));

   uint32_t cli_id = atoi(argv[1]);
      
   if(0 == strcmp(argv[2], "start")){  
      printf("start\n");
      exec_start_gateway(START_CTRL, TCP, GATEWAY_PUBLIC_ADDR);
   }else if(0 == strcmp(argv[2], "wst")){
      printf("Work stealing\n");
      if(0 == strcmp(argv[3], "non_data_source")){
         printf("non_data_source\n");
         test_stealer_client(cli_id);
      }else if(0 == strcmp(argv[3], "data_source")){
         printf("data_source\n");
         test_victim_client(cli_id);
      }else if(0 == strcmp(argv[3], "gateway")){
         printf("gateway\n");
         test_gateway(cli_id);
      }
   }
}

*/
//sock is generated through service_init()




blob* recv_data_without_conn_tcp(int sockfd){
   uint32_t bytes_length;
   uint8_t* meta = NULL;
   uint32_t meta_size = 0;
   int32_t id;

   socklen_t clilen;

#if IPV4_TASK
   struct sockaddr_in cli_addr;
#elif IPV6_TASK//IPV4_TASK
   struct sockaddr_in6 cli_addr;
#endif//IPV4_TASK

   int newsockfd;
   clilen = sizeof(cli_addr);


   newsockfd = lwip_accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
   if (newsockfd < 0) printf("ERROR on accept");


   char buffer[60];
   read_from_sock(newsockfd, TCP, (uint8_t*)buffer, 60, NULL, NULL);
   printf("Recv 1 piece of data on server side, %s\n", buffer);
   read_from_sock(newsockfd, TCP, (uint8_t*)buffer, 60, NULL, NULL);
   printf("Recv 2 piece of data on server side, %s\n", buffer);
   os_model_context* os_model = sim_ctxt.get_os_ctxt( sc_core::sc_get_current_process_handle() );
   os_model->os_port->timeWait(3000000000000, sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle()));
   os_model->os_port->syncGlobalTime(sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle()));

   char* a = "Okay, it is a pure test case...";
   write_to_sock(newsockfd, TCP, (uint8_t*)a, 60, NULL, NULL);

   os_model->os_port->timeWait(3000000000000, sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle()));
   os_model->os_port->syncGlobalTime(sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle()));

   read_from_sock(newsockfd, TCP, (uint8_t*)buffer, 60, NULL, NULL);
   printf("Recv 3 piece of data on server side, %s\n", buffer);
   read_from_sock(newsockfd, TCP, (uint8_t*)buffer, 60, NULL, NULL);
   printf("Recv 4 piece of data on server side, %s\n", buffer);
   read_from_sock(newsockfd, TCP, (uint8_t*)buffer, 60, NULL, NULL);
   printf("Recv 5 piece of data on server side, %s\n", buffer);


   lwip_close(newsockfd);    
   lwip_close(sockfd);    
 
   blob* tmp = new_blob_and_copy_data(id, 60, (uint8_t*)buffer);
   return tmp;
}

void test_server_thread(void *arg){
   int wst_service = service_init(WORK_STEAL_PORT, TCP);
   blob* temp = recv_data_without_conn_tcp(wst_service);
   printf("Received request %s\n!", temp->data);
}

void test_server(uint32_t edge_id){
   device_ctxt* ctxt = init_client(edge_id);
   set_gateway_local_addr(ctxt, GATEWAY_LOCAL_ADDR);
   set_gateway_public_addr(ctxt, GATEWAY_PUBLIC_ADDR);
   set_total_frames(ctxt, FRAME_NUM);
   set_batch_size(ctxt, BATCH_SIZE);
   sys_thread_t t0 = sys_thread_new("test_server_thread", test_server_thread, ctxt, 0, 0);
   sys_thread_join(t0);
}




#endif /*WORK_STEALING_RUNTIME_H*/

