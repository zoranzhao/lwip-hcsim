#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <list>
#ifndef CLUSTER_CONFIG_H
#define CLUSTER_CONFIG_H
/*
   "edge": {
      "total_number": 6,
      "edge_id": [0, 1, 2, 3, 4, 5],
      "edge_type": ["victim", "stealer", "stealer", "stealer", "stealer", "stealer"],
      "edge_core_number": [1, 1, 1, 1, 1, 1],
      "edge_ipv4_address": ["192.168.4.9", "192.168.4.8", "192.168.4.4", "192.168.4.14", "192.168.4.15", "192.168.4.16"],
      "edge_ipv6_address": ["100:0:200:0:300:0:400:", "100:0:200:0:300:0:500:", "100:0:200:0:300:0:600:", "100:0:200:0:300:0:700:", "100:0:200:0:300:0:800:", "100:0:200:0:300:0:900:"],
      "edge_mac_address": ["00:01:00:00:00:00", "00:02:00:00:00:00", "00:03:00:00:00:00", "00:04:00:00:00:00", "00:05:00:00:00:00", "00:06:00:00:00:00"]
   },
   "gateway": {
      "total_number": 6,
      "gateway_id": 6,
      "gateway_core_number": 1,
      "gateway_ipv4_address": "192.168.4.1",
      "gateway_ipv6_address": "100:0:200:0:300:0:300:",
      "gateway_mac_address": "00:07:00:00:00:00"
   },
   "ap_mac_address": "00:10:00:00:00:00",
   "network":{
      "type": "802.11",
      "mode": "n", 
      "bitrate": "600Mbps"
   }
*/
#define BROAD_CAST_MAC_ADDR -1
#define UNKNOWN_MAC_ADDR -2
class cluster_config{
public:
   int total_number;
   std::vector<int> edge_id;
   std::unordered_map<int, std::string> edge_type;
   std::unordered_map<int, int> edge_core_number;
   std::unordered_map<int, std::string> edge_ipv4_address;
   std::unordered_map<int, std::string> edge_ipv6_address;
   std::unordered_map<int, std::string> edge_mac_address;
   std::unordered_map<std::string,  int> edge_mac_address_to_id;
   std::unordered_map<std::string,  int> ipv6_address_to_id;
   std::unordered_map<int,  int> dest_id_map;
   std::unordered_map<int,  std::list<int>> dest_id_queues;

   int gateway_id;
   int gateway_core_number;
   std::string gateway_ipv4_address;
   std::string gateway_ipv6_address;
   std::string gateway_mac_address;

   std::string ap_mac_address;
   
   cluster_config(){

   }
   ~cluster_config(){
      
   }
   void print(){
      std::cout << "total_number:" << total_number << std::endl;
      for(int& it : edge_id){
         std::cout << "edge_id is: " << it << std::endl;
      }
      for(int& it : edge_id){
         std::cout << "edge_type is: " << edge_type[it] << std::endl;
      }
      for(int& it : edge_id){
         std::cout << "edge_core_number is: " << edge_core_number[it] << std::endl;
      }
      for(int& it : edge_id){
         std::cout << "edge_ipv4_address is: " << edge_ipv4_address[it] << std::endl;
      }
      for(int& it : edge_id){
         std::cout << "edge_ipv6_address is: " << edge_ipv6_address[it] << std::endl;
         std::cout << "Device ID is: " << ipv6_address_to_id[edge_ipv6_address[it]] << ", ipv6 address is: " << edge_ipv6_address[it] << std::endl; 
      }
      for(int& it : edge_id){
         std::cout << "edge_mac_address is: " << edge_mac_address[it] << std::endl;
         std::cout << get_mac_address_byte(edge_mac_address[it], 0) << std::endl;
         std::cout << get_mac_address_byte(edge_mac_address[it], 1) << std::endl;
         std::cout << get_mac_address_byte(edge_mac_address[it], 2) << std::endl;
         std::cout << get_mac_address_byte(edge_mac_address[it], 3) << std::endl;
         std::cout << get_mac_address_byte(edge_mac_address[it], 4) << std::endl;
         std::cout << get_mac_address_byte(edge_mac_address[it], 5) << std::endl;
         char mac_addr[64];
         sprintf(mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X", 
                 get_mac_address_byte(it, 0) , 
                 get_mac_address_byte(it, 1) , 
                 get_mac_address_byte(it, 2) , 
                 get_mac_address_byte(it, 3) , 
                 get_mac_address_byte(it, 4) , 
                 get_mac_address_byte(it, 5) 
                );
         std::cout << "Device ID is: " << edge_mac_address_to_id[std::string(mac_addr)] << ", mac address is: " << std::string(mac_addr) << std::endl; 

      }
      std::cout << "gateway_id:" << gateway_id << std::endl;
      std::cout << "gateway_core_number:" << gateway_core_number << std::endl;
      std::cout << "gateway_ipv4_address:" << gateway_ipv4_address << std::endl;
      std::cout << "gateway_ipv6_address:" << gateway_ipv6_address << std::endl;
      std::cout << "gateway_mac_address:" << gateway_mac_address << std::endl;
      std::cout << "ap_mac_address:" << ap_mac_address << std::endl;
      
   }

   int get_device_id_from_mac_address(char* mac_address){
      if(std::string(mac_address) == gateway_mac_address) return gateway_id;
      if(edge_mac_address_to_id.find(std::string(mac_address)) != edge_mac_address_to_id.end()) return edge_mac_address_to_id[std::string(mac_address)];
      if(std::string(mac_address) == "FF:FF:FF:FF:FF:FF") return BROAD_CAST_MAC_ADDR;
      return UNKNOWN_MAC_ADDR;
   }

   int get_device_id_from_ipv6_address(char* ip_address){
      if(std::string(ip_address) == gateway_ipv6_address) return gateway_id;
      if(ipv6_address_to_id.find(std::string(ip_address)) != ipv6_address_to_id.end()) return ipv6_address_to_id[std::string(ip_address)];
      return BROAD_CAST_MAC_ADDR;
   }

   char* get_mac_address_from_device_id(int device_id){
      if(device_id == gateway_id) return (char*)gateway_mac_address.c_str();
      if(edge_mac_address.find(device_id) != edge_mac_address.end()) return (char*)edge_mac_address[device_id].c_str();
      if(device_id== BROAD_CAST_MAC_ADDR) return (char*)"FF:FF:FF:FF:FF:FF";
      return NULL;
   }

   int get_mac_address_byte(int device_id, int pos){
      if(device_id == gateway_id) 
         return get_mac_address_byte(gateway_mac_address, pos);
      return get_mac_address_byte(edge_mac_address[device_id], pos);
      
   }

   int get_mac_address_byte(std::string mac_address, int pos){
         std::stringstream ss(mac_address);
         std::string s;
         int count = 0;
         while (getline(ss, s, ':')) {
            if (count == pos) break;
            count++;
         }
         return std::stoi("0x"+s, NULL, 0);
   }

   int get_dest_id(int this_id){
      if(dest_id_map.find(this_id) != dest_id_map.end()) return dest_id_map[this_id];
      else return BROAD_CAST_MAC_ADDR;
   }

   void set_dest_id(int this_id, int dest_id){
      dest_id_map[this_id] = dest_id;
   }

   int dequeue_dest_id(int this_id){
      int front = dest_id_queues[this_id].front();
      dest_id_queues[this_id].pop_front();
      return front;
   }

   void enqueue_dest_id(int this_id){
      int dest_id = get_dest_id(this_id);
      dest_id_queues[this_id].push_back(dest_id);
   }

};

#endif
