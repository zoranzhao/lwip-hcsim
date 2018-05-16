//***************************************************************                                                       
// * A (lwip) socket-based service API for IoT cluster middleware                                                                     
// * Zhuoran Zhao, UT Austin, zhuoran@utexas.edu                                                    
// * Last update: July 2018                                                                         
//***************************************************************

//Header file for sockets, drivers and OS APIs
#include "HCSim.h"
#include "lwip_ctxt.h"

#ifndef SERVICE_API__H
#define SERVICE_API__H

#define IPV4_TASK 0
#define IPV6_TASK !(IPV4_TASK)
#define LOWPAN6_TASK (IPV6_TASK && 0) 

typedef enum proto{
   TCP,
   UDP
} ctrl_proto;

typedef struct data_buffer{
   void* data;
   uint32_t size;
} raw_data;

raw_data* make_raw_data(uint32_t size){
   raw_data* blob = (raw_data*) malloc(sizeof(raw_data));
   if (blob == NULL) return NULL;
   blob->data = malloc(size);
   if(size > 0 && blob->data == NULL) {free(blob); return NULL;}
   blob->size = size;
   return blob;
}

void free_raw_data(raw_data* blob){
   free(blob->data);
   free(blob);
}

void copy_raw_data(raw_data* blob1, raw_data* blob2){
   blob1->data = blob2->data;
   blob1->size = blob2->size;
}

raw_data* pack_raw_data(void* data, uint32_t size){
   raw_data* blob  = (raw_data*) malloc(sizeof(raw_data));
   if (blob == NULL) return NULL;
   blob->data = data;
   return blob;
}

void extract_raw_data(raw_data* blob, void** data_ptr, uint32_t* size_ptr){
   *data_ptr = blob->data;
   *size_ptr = blob->size;
}


//Will allocate memory
raw_data* write_file_to_raw_data(const char *filename){
   uint32_t size = 0;
   raw_data* blob;
   FILE *f = fopen(filename, "rb");
   if (f == NULL){
      return NULL;
   }
   fseek(f, 0, SEEK_END);
   size = ftell(f);
   fseek(f, 0, SEEK_SET);
   blob = make_raw_data(size+1);
   if(blob == NULL) return NULL;
   if (size != fread(blob->data, sizeof(char), size, f)){
      free_raw_data(blob);
      return NULL;
   }
   fclose(f);
   ((uint8_t*)blob->data)[size] = 0;
   blob->size = size;
   return blob;
}


void write_raw_data_to_file(const char *filename, raw_data* blob){
   FILE *fp;
   fp = fopen( filename , "wb" );
   fwrite(blob->data , 1 ,  (blob->size)*sizeof(uint8_t) , fp );
   fclose(fp);
}

#define UDP_TRANS_SIZE 512
static inline void read_from_sock(int sock, ctrl_proto proto, uint8_t* buffer, uint32_t bytes_length, struct sockaddr *from, socklen_t *fromlen){
   uint32_t bytes_read = 0;
   int32_t n;
   while (bytes_read < bytes_length){
      if(proto == TCP){
         n = lwip_recv(sock, buffer + bytes_read, bytes_length - bytes_read, 0);
         if( n < 0 ) printf("ERROR reading socket");
      }else if(proto == UDP){
         if((bytes_length - bytes_read) < UDP_TRANS_SIZE) { n = bytes_length - bytes_read; }
         else { n = UDP_TRANS_SIZE; }
         if( lwip_recvfrom(sock, buffer + bytes_read, n, 0, from, fromlen) < 0) printf("ERROR reading socket");
      }else{printf("Protocol is not supported\n");}
      bytes_read += n;
   }
}

static inline void write_to_sock(int sock, ctrl_proto proto, uint8_t* buffer, uint32_t bytes_length, const struct sockaddr *to, socklen_t tolen){
   uint32_t bytes_written = 0;
   int32_t n;
   while (bytes_written < bytes_length) {
      if(proto == TCP){
         n = lwip_send(sock, buffer + bytes_written, bytes_length - bytes_written, 0);
         if( n < 0 ) printf("ERROR writing socket");
      }else if(proto == UDP){
         if((bytes_length - bytes_written) < UDP_TRANS_SIZE) { n = bytes_length - bytes_written; }
         else { n = UDP_TRANS_SIZE; }
         if(lwip_sendto(sock, buffer + bytes_written, n, 0, to, tolen)< 0) printf("ERROR writing socket\n");
      }else{printf("Protocol is not supported\n");}
      bytes_written += n;
   }
}

int service_init(int portno, ctrl_proto proto){
   int sockfd;
#if IPV4_TASK
   struct sockaddr_in serv_addr;
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);
#elif IPV6_TASK//IPV4_TASK
   struct sockaddr_in6 serv_addr;
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin6_family = AF_INET6;
   serv_addr.sin6_addr = in6addr_any;
   serv_addr.sin6_port = htons(portno);
#endif//IPV4_TASK
#if IPV4_TASK
   if (proto == UDP) sockfd = lwip_socket(AF_INET, SOCK_DGRAM, 0);
   else if (proto == TCP) sockfd = lwip_socket(AF_INET, SOCK_STREAM, 0);
#elif IPV6_TASK//IPV4_TASK
   if (proto == UDP) sockfd = lwip_socket(AF_INET6, SOCK_DGRAM, 0);
   else if (proto == TCP) sockfd = lwip_socket(AF_INET6, SOCK_STREAM, 0);
#endif//IPV4_TASK
   else {
	printf("Control protocol is not supported");
	exit(EXIT_FAILURE);
   }
   if (sockfd < 0) {
	printf("ERROR opening socket: %s", lwip_strerr(sockfd));
	exit(EXIT_FAILURE);
   }
   if (lwip_bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
	printf("ERROR on binding: %s", lwip_strerr(sockfd));
	exit(EXIT_FAILURE);
   }
   if (proto == TCP) lwip_listen(sockfd, 10);//back_log numbers 
   return sockfd;
}


void send_data(raw_data *blob, ctrl_proto proto, const char *dest_ip, int portno){
   void* data;
   uint32_t bytes_length;
   extract_raw_data(blob, &data, &bytes_length);
   ip_addr_t dstaddr;
   int sockfd;
#if IPV4_TASK
   struct sockaddr_in serv_addr;
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(portno);
   ip4addr_aton(dest_ip, ip_2_ip4(&dstaddr));
   inet_addr_from_ip4addr(&serv_addr.sin_addr, ip_2_ip4(&dstaddr));
#elif IPV6_TASK//IPV4_TASK
   struct sockaddr_in6 serv_addr;
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin6_family = AF_INET6;
   serv_addr.sin6_port = htons(portno);
   ip6addr_aton(dest_ip, ip_2_ip6(&dstaddr));
   inet6_addr_from_ip6addr(&serv_addr.sin6_addr, ip_2_ip6(&dstaddr));
#endif//IPV4_TASK

   if(proto == TCP) {
#if IPV4_TASK
      sockfd = lwip_socket(AF_INET, SOCK_STREAM, 0);
#elif IPV6_TASK//IPV4_TASK
      sockfd = lwip_socket(AF_INET6, SOCK_STREAM, 0);
#endif//IPV4_TASK
      if (lwip_connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
      printf("ERROR connecting");
   } else if (proto == UDP) {
#if IPV4_TASK
      sockfd = lwip_socket(AF_INET, SOCK_DGRAM, 0);
#elif IPV6_TASK//IPV4_TASK
      sockfd = lwip_socket(AF_INET6, SOCK_DGRAM, 0);
#endif//IPV4_TASK
   }
   else {printf("Control protocol is not supported");}
   if (sockfd < 0) printf("ERROR opening socket");

   write_to_sock(sockfd, proto, (uint8_t*)&bytes_length, sizeof(bytes_length), (struct sockaddr *) &serv_addr, sizeof(serv_addr));
   write_to_sock(sockfd, proto, (uint8_t*)data, bytes_length, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

   lwip_close(sockfd);
}

//sock is generated through service_init()
raw_data* recv_data(int sockfd, ctrl_proto proto){
   socklen_t clilen;
   struct sockaddr_in cli_addr;
   int newsockfd;
   clilen = sizeof(cli_addr);
   uint32_t bytes_length;
   uint8_t* buffer;

   if(proto == TCP){
      newsockfd = lwip_accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
      if (newsockfd < 0) printf("ERROR on accept");
      read_from_sock(newsockfd, TCP, (uint8_t*)&bytes_length, sizeof(bytes_length), NULL, NULL);
	//printf("bytes_length is = %d\n", bytes_length);
      buffer = (uint8_t*)malloc(bytes_length);
      read_from_sock(newsockfd, TCP, buffer, bytes_length, NULL, NULL);
      lwip_close(newsockfd);   
   }else if(proto == UDP){
      read_from_sock(sockfd, UDP, (uint8_t*)&bytes_length, sizeof(bytes_length), (struct sockaddr *) &cli_addr, &clilen);
      buffer = (uint8_t*)malloc(bytes_length);
      read_from_sock(sockfd, UDP, buffer, bytes_length, (struct sockaddr *) &cli_addr, &clilen);
   }else{ printf("Protocol is not supported\n"); }
   lwip_close(sockfd);

   return pack_raw_data(buffer, bytes_length);
}

int service_init(int portno, ctrl_proto proto);
raw_data* recv_data(int sockfd, ctrl_proto proto);
void send_data(raw_data *blob, ctrl_proto proto, const char *dest_ip, int portno);

raw_data* make_raw_data(uint32_t size);
void free_raw_data(raw_data* blob);
void copy_raw_data(raw_data* blob1, raw_data* blob2);
raw_data* pack_raw_data(void* data, uint32_t size);
void extract_raw_data(raw_data* blob, void** data_ptr, uint32_t* size_ptr);

raw_data* write_file_to_raw_data(const char *filename);
void write_raw_data_to_file(const char *filename, raw_data* blob);

#endif // SERVICE_API__H 
