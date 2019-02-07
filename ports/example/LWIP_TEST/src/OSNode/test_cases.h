#include "darkiot.h"
#include "configure.h"
#include <string.h>
#ifndef TEST_CASES_H
#define TEST_CASES_H

#ifndef BATCH_SIZE
#define BATCH_SIZE 16
#endif

extern const char* addr_list[MAX_EDGE_NUM];

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


void test_socket_client_thread(void *arg){
   /*Check gateway for possible stealing victims*/
   device_ctxt* ctxt = (device_ctxt*)arg;
   service_conn* conn;
   blob* temp;
   uint32_t dest_id = 0;
   while(1){
      conn = connect_service(TCP, addr_list[dest_id], WORK_STEAL_PORT);
      char* a = "Okay, it is a pure test case, first...";

      int flags =1;
      lwip_setsockopt(conn->sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));

      write_to_sock(conn->sockfd, TCP, (uint8_t*)a, 60, NULL, NULL);
      write_to_sock(conn->sockfd, TCP, (uint8_t*)a, 60, NULL, NULL);

      char buffer[60];
      read_from_sock(conn->sockfd, TCP, (uint8_t*)buffer, 60, NULL, NULL);

      printf("Recv 1 piece of data on client side, %s\n", buffer);
      write_to_sock(conn->sockfd, TCP, (uint8_t*)a, 60, NULL, NULL);
      write_to_sock(conn->sockfd, TCP, (uint8_t*)a, 60, NULL, NULL);
      write_to_sock(conn->sockfd, TCP, (uint8_t*)a, 60, NULL, NULL);

      close_service_connection(conn);
      break;
   }
}

void test_socket_client(uint32_t edge_id){
   device_ctxt* ctxt = init_client(edge_id);
   set_gateway_local_addr(ctxt, GATEWAY_LOCAL_ADDR);
   set_gateway_public_addr(ctxt, GATEWAY_PUBLIC_ADDR);
   set_total_frames(ctxt, FRAME_NUM);
   set_batch_size(ctxt, BATCH_SIZE);


   sys_thread_t t1 = sys_thread_new("test_socket_client_thread", test_socket_client_thread, ctxt, 0, 0);
   sys_thread_join(t1);
}





blob* test_socket_server_impl(int sockfd){
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

   int flags =1;
   lwip_setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));

   char buffer[60];
   read_from_sock(newsockfd, TCP, (uint8_t*)buffer, 60, NULL, NULL);
   printf("Recv 1 piece of data on server side, %s\n", buffer);
   read_from_sock(newsockfd, TCP, (uint8_t*)buffer, 60, NULL, NULL);
   printf("Recv 2 piece of data on server side, %s\n", buffer);

   char* a = "Okay, it is a pure test case...";
   write_to_sock(newsockfd, TCP, (uint8_t*)a, 60, NULL, NULL);

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

void test_socket_server_thread(void *arg){
   int wst_service = service_init(WORK_STEAL_PORT, TCP);
   blob* temp = test_socket_server_impl(wst_service);
   printf("Received request %s\n!", temp->data);
}

void test_socket_server(uint32_t edge_id){
   device_ctxt* ctxt = init_client(edge_id);
   set_gateway_local_addr(ctxt, GATEWAY_LOCAL_ADDR);
   set_gateway_public_addr(ctxt, GATEWAY_PUBLIC_ADDR);
   set_total_frames(ctxt, FRAME_NUM);
   set_batch_size(ctxt, BATCH_SIZE);
   sys_thread_t t0 = sys_thread_new("test_socket_server_thread", test_socket_server_thread, ctxt, 0, 0);
   sys_thread_join(t0);
}




#endif /*TEST_CASES_H*/

