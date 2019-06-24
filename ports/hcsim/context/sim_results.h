#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#ifndef SIM_RESULTS_H
#define SIM_RESULTS_H
/*
{
   "edge_0":{
      "latency": 7.81,
      "lwip_core_util": 0.1,
      "lwip_energy": 0.24,
      "app_core_util": 0.1,
      "app_energy": 0.24,
      "edge_0_task_number": 8.2,
      "edge_1_task_number": 8.2
   }, 
   "gateway":{
      "latency": 7.81,
      "task_number": 8.1,
      "lwip_core_util": 0.1,
      "lwip_energy": 0.24,
      "app_core_util": 0.1,
      "app_energy": 0.24
   }, 
}
*/
class sim_results{
public:
   std::vector< std::unordered_map<std::string, double> > edges;
   std::unordered_map<std::string, double> gateway;
   
   sim_results(){
   }

   sim_results(int total_edge_number){   
      std::unordered_map<std::string, double> edge;
      for(int i = 0; i < total_edge_number; i++)
         edges.push_back(edge);
   }
   void set_gateway_result(std::string key, double value){
      gateway[key] = value;
   }
   void set_edge_result(int device_id, std::string key, double value){
      assert(device_id<(edges.size()));
      edges[device_id][key] = value;
   }
   void accumulate_edge_result(int device_id, std::string key, double value){
      assert(device_id<(edges.size()));
      if(edges[device_id].find(key) != edges[device_id].end()){
          double new_value = edges[device_id][key] + value;
          edges[device_id][key] = new_value;
      }else{
          edges[device_id][key] = value;
      } 
      //std::cout<<"update value: "<< edges[device_id][key] << std::endl;
   }

   void accumulate_gateway_result(std::string key, double value){
      if(gateway.find(key) != gateway.end()){
          double new_value = gateway[key] + value;
          gateway[key] = new_value;
      }else{
          gateway[key] = value;
      } 
      //std::cout<<"update value gateway: "<< gateway[key] << std::endl;
   }

   std::unordered_map<std::string, double> get_gateway_result(){
      return gateway;
   }
   std::unordered_map<std::string, double> get_edge_result(int device_id){
      assert(device_id<(edges.size()));
      return edges[device_id];
   }
   ~sim_results(){
      
   }
};
#ifdef __cplusplus
extern "C" {
#endif
void record_static(char* function_name, char* input, char* record_name);
#ifdef __cplusplus
}
#endif
#endif
