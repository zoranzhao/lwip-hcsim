#include "darkiot.h"
#include "configure.h"
#include <string.h>
#include "test_utils.h"
#ifndef WORK_STEALING_RUNTIME_H
#define WORK_STEALING_RUNTIME_H

#ifndef BATCH_SIZE
#define BATCH_SIZE 8
#endif

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

/*
void all_in_one_device_test(void* arg){
   uint32_t total_cli_num = 1;
   uint32_t this_cli_id = 0;

   uint32_t partitions_h = 3;
   uint32_t partitions_w = 3;
   uint32_t fused_layers = 8;

   char network_file[30] = "models/yolo.cfg";
   char weight_file[30] = "models/yolo.weights";

   device_ctxt* client_ctxt = deepthings_edge_init(partitions_h, partitions_w, fused_layers, network_file, weight_file, this_cli_id);
   device_ctxt* gateway_ctxt = deepthings_gateway_init(partitions_h, partitions_w, fused_layers, network_file, weight_file, total_cli_num, addr_list);

   //partition_frame_and_perform_inference_thread_single_device(client_ctxt);
   //transfer_data_with_number(client_ctxt, gateway_ctxt, FRAME_NUM*partitions_h*partitions_h);
   //deepthings_merge_result_thread_single_device(gateway_ctxt);
   

   sys_thread_t t2 = sys_thread_new("deepthings_merge_result_thread_single_device", deepthings_merge_result_thread_single_device, gateway_ctxt, 49, 0);
   sys_thread_t t1 = sys_thread_new("partition_frame_and_perform_inference_thread_single_device",
                                     partition_frame_and_perform_inference_thread_single_device, client_ctxt, 49, 0);
   transfer_data(client_ctxt, gateway_ctxt);
   sys_thread_join(t1);
   sys_thread_join(t2);
}
*/

void partition_frame_and_perform_inference_thread_no_reuse_no_gateway(void *arg){
   device_ctxt* ctxt = (device_ctxt*)arg;
   cnn_model* model = (cnn_model*)(ctxt->model);
   blob* temp;
   uint32_t frame_num;
   bool* reuse_data_is_required;   
   for(frame_num = 0; frame_num < FRAME_NUM; frame_num ++){
      /*Wait for i/o device input*/
      /*recv_img()*/

      /*Load image and partition, fill task queues*/
      load_image_as_model_input(model, frame_num);
      partition_and_enqueue(ctxt, frame_num);

      /*Dequeue and process task*/
      while(1){
         temp = try_dequeue(ctxt->task_queue);
         if(temp == NULL) break;
         bool data_ready = false;
         process_task_single_device(ctxt, temp, data_ready);
         os_model_context* os_model = sim_ctxt.get_os_ctxt( sc_core::sc_get_current_process_handle() );
         os_model->os_port->timeWait(3000000000000, sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle()));
         os_model->os_port->syncGlobalTime(sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle()));
         free_blob(temp);

      }
   }

}



void steal_partition_and_perform_inference_thread_no_reuse_no_gateway(void *arg){
   device_ctxt* ctxt = (device_ctxt*)arg;
   service_conn* conn;
   blob* temp;
   while(1){
      conn = connect_service(TCP, (const char *)addr_list[0], WORK_STEAL_PORT);
      int flags =1;
      lwip_setsockopt(conn->sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
      send_request("steal_client", 20, conn);
      temp = recv_data(conn);
      close_service_connection(conn);
      if(temp->id == -1){
         free_blob(temp);
         sys_sleep(100);
         continue;
      }
      bool data_ready = true;
      process_task_single_device(ctxt, temp, data_ready);
      os_model_context* os_model = sim_ctxt.get_os_ctxt( sc_core::sc_get_current_process_handle() );
      os_model->os_port->timeWait(3000000000000, sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle()));
      os_model->os_port->syncGlobalTime(sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle()));
      free_blob(temp);
   }
}


void test_deepthings_stealer_edge(uint32_t edge_id){
   uint32_t N = 3;
   uint32_t M = 3;
   uint32_t fused_layers = 8;

   char network[30] = "models/yolo.cfg";
   char weights[30] = "models/yolo.weights";

   device_ctxt* ctxt = deepthings_edge_init(N, M, fused_layers, network, weights, edge_id);

   sys_thread_t t1 = sys_thread_new("steal_partition_and_perform_inference_thread_no_reuse_no_gateway", steal_partition_and_perform_inference_thread_no_reuse_no_gateway, ctxt, 0, 0);

   sys_thread_join(t1);
}

void test_deepthings_victim_edge(uint32_t edge_id){//edge_id == 0;
   uint32_t N = 3;
   uint32_t M = 3;
   uint32_t fused_layers = 8;

   char network[30] = "models/yolo.cfg";
   char weights[30] = "models/yolo.weights";

   device_ctxt* ctxt = deepthings_edge_init(N, M, fused_layers, network, weights, edge_id);

   sys_thread_t t1 = sys_thread_new("partition_frame_and_perform_inference_thread_no_reuse_no_gateway", partition_frame_and_perform_inference_thread_no_reuse_no_gateway, ctxt, 20, 0);
   sys_thread_t t3 = sys_thread_new("serve_stealing_thread", serve_stealing_thread, ctxt, 0, 1);

   sys_thread_join(t1);
   sys_thread_join(t3);
}



/*------------------------------------------------------*/
/*---Test functions for distributed work stealing runtime system---*/
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
         printf("Process task locally, frame %d, task is %d\n", get_blob_frame_seq(temp), get_blob_task_id(temp));
         free_blob(temp);
      }

   }
}

void steal_and_process_thread_no_gateway(void *arg){
   /*Check gateway for possible stealing victims*/
   device_ctxt* ctxt = (device_ctxt*)arg;
   service_conn* conn;
   blob* temp;
   while(1){      
      conn = connect_service(TCP, (const char *)addr_list[0], WORK_STEAL_PORT);
      int flags =1;
      lwip_setsockopt(conn->sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
      send_request("steal_client", 20, conn);
      temp = recv_data(conn);
      close_service_connection(conn);
      if(temp->id == -1){
         free_blob(temp);
         printf("Nothing is available in victim device!\n");
         sys_sleep(100);
         continue;
      }
      printf("Process task remotely, frame %d, task is %d\n", get_blob_frame_seq(temp), get_blob_task_id(temp));
      process_task(temp, ctxt);
      free_blob(temp);
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
   sys_thread_t t1 = sys_thread_new("generate_and_process_thread_no_gateway", generate_and_process_thread_no_gateway, ctxt, 20, 0);
   /*sys_thread_t t2 = sys_thread_new("send_result_thread", send_result_thread, ctxt, 0, 0);*/

   sys_thread_join(t1);
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




#endif /*WORK_STEALING_RUNTIME_H*/

