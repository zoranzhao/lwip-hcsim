#include <systemc>
#include "HCSim.h"
#include <string>
#include "profile_data.h"
#include "cluster_config.h"
#include "sim_results.h"

#ifndef OS_CTXT__H
#define OS_CTXT__H

#define MAX_CORE_NUM 2

class sys_call_recv_if: virtual public sc_core::sc_interface{
public:
   virtual int get_node(int os_task_id) = 0;
   virtual int get_weight(int os_task_id) = 0;
   virtual int get_size(int os_task_id) = 0;
   virtual bool get_data(unsigned size, char* data, int os_task_id) = 0;
};

class sys_call_send_if: virtual public sc_core::sc_interface{
public:
   virtual void set_node(int NodeID, int os_task_id) = 0;
   virtual void set_weight(int weight, int os_task_id) = 0;
   virtual void set_size(int size, int os_task_id) = 0;
   virtual void set_data(unsigned size, char* data, int os_task_id) = 0;
};


class sys_call_recv_driver
    :public sc_core::sc_module
     ,virtual public sys_call_recv_if
{
public:
    /*---------------------------------------------------------
       OS/HAL interface
     ----------------------------------------------------------*/
   sc_core::sc_port< HCSim::IAmbaAhbBusMasterMacLink > mac_link_port;
   sc_core::sc_port< HCSim::receive_os_if > intr_ch;

   sys_call_recv_driver(const sc_core::sc_module_name name, unsigned long long addr)
    :sc_core::sc_module(name)
   {
      this->address = addr;
   }
   ~sys_call_recv_driver() { }

   virtual int get_node(int os_task_id){
      int tmp;
      mac_link_port->masterRead(address+2, &tmp, sizeof(int));
      return tmp;
   }

   virtual int get_weight(int os_task_id){
      int tmp;
      intr_ch->receive(os_task_id);
      mac_link_port->masterRead(address+1, &tmp, sizeof(int));
      return tmp;
   }

   virtual int get_size(int os_task_id){
      int tmp;
      intr_ch->receive(os_task_id);
      mac_link_port->masterRead(address+1, &tmp, sizeof(int));
      return tmp;
    }

   virtual bool get_data(unsigned size, char* data, int os_task_id){
      intr_ch->receive(os_task_id);
      mac_link_port->masterRead(address+1, data, size*sizeof(char));
      return true;
   }
private:
   unsigned long long address;
};



class sys_call_send_driver
    :public sc_core::sc_module
     ,virtual public sys_call_send_if
{
public:
    /*---------------------------------------------------------
       OS/HAL interface
     ----------------------------------------------------------*/
   sc_core::sc_port< HCSim::IAmbaAhbBusMasterMacLink > mac_link_port;
   sc_core::sc_port< HCSim::receive_os_if > intr_ch;

   sys_call_send_driver(const sc_core::sc_module_name name, unsigned long long addr)
    :sc_core::sc_module(name)
   {
      this->address = addr;
   }
   ~sys_call_send_driver() { }

   virtual void set_node(int NodeID, int os_task_id){
      mac_link_port->masterWrite(address, &NodeID, sizeof(int));
   }

   virtual void set_weight(int weight, int os_task_id){
      mac_link_port->masterWrite(address, &weight, sizeof(int));
   }

   virtual void set_size(int size, int os_task_id){
      mac_link_port->masterWrite(address, &size, sizeof(int));
   }

   virtual void set_data(unsigned size, char* data, int os_task_id){
      mac_link_port->masterWrite(address, data, size*sizeof(char));
   }
private:
   unsigned long long address;
};


class os_model_context{
public:
   int node_id;
   profile_data *profile;


   sc_core::sc_port< sc_core::sc_fifo_out_if<int> > ctrl_out1; 
   sc_core::sc_port< sc_core::sc_fifo_out_if<int> > ctrl_out2; 

   sc_core::sc_port<sys_call_recv_if> recv_port[MAX_CORE_NUM];
   sc_core::sc_port<sys_call_send_if> send_port[MAX_CORE_NUM]; 
   sc_core::sc_port< HCSim::OSAPI > os_port;

   os_model_context(){
      profile = new profile_data();
   }
   os_model_context(int node_id,
                    sc_core::sc_vector< sc_core::sc_port< sys_call_recv_if > >& recv_port,
                    sc_core::sc_vector< sc_core::sc_port< sys_call_send_if > >& send_port,
                    sc_core::sc_port< HCSim::OSAPI >& os_port){
      this->node_id = node_id;
      this->os_port(os_port);
      for(int i = 0; i < MAX_CORE_NUM; i++){
         this->recv_port[i](recv_port[i]);
         this->send_port[i](send_port[i]);
      }
      profile = new profile_data();
   }
   ~os_model_context(){
      delete profile;
   }
};


/*We also need a application context in order to capture app/lib-specific context data*/
class app_context{
   /*TODO, probably we need use class derivation instead of void pointer to implement the polymorphism*/
   std::unordered_map<std::string, void*> ctxt_list;  
public:
   /*For example, here we can have global data defined to hold application states*/
   void add_context(std::string ctxt_name, void* ctxt){
      ctxt_list[ctxt_name] = ctxt;
   }
   void* get_context(std::string ctxt_name){
      return ctxt_list[ctxt_name];
   }
};

typedef struct sc_process_handler_context{
   os_model_context* os_ctxt;  
   app_context* app_ctxt;
   int task_id;  
} handler_context;

class simulation_context{
   std::vector< sc_core::sc_process_handle> handler_list;  
   std::vector<handler_context> handler_context_list;
public:
   cluster_config* cluster;
   sim_results* result;
   simulation_context(){
      std::cout << "Construct a new simulation context" << std::endl; 
   }
   void register_task(os_model_context* os_ctxt, app_context* app_ctxt, int task_id, sc_core::sc_process_handle handler){
      handler_context ctxt;
      ctxt.os_ctxt = os_ctxt;  
      ctxt.app_ctxt = app_ctxt;
      ctxt.task_id = task_id;
      handler_list.push_back(handler);
      handler_context_list.push_back(ctxt);
   }

   handler_context get_handler_context(sc_core::sc_process_handle handler){
      auto key = handler_list.begin();
      auto item =  handler_context_list.begin();
      for(; key!=handler_list.end() && item!= handler_context_list.end(); key++, item++){
         if(*key == handler) break;
      }
      return *item;
   }

   os_model_context* get_os_ctxt(sc_core::sc_process_handle handler){
      handler_context ctxt = get_handler_context(handler);
      return ctxt.os_ctxt;
   } 

   app_context* get_app_ctxt(sc_core::sc_process_handle handler){
      handler_context ctxt = get_handler_context(handler);
      return ctxt.app_ctxt;
   } 
	
   int get_task_id(sc_core::sc_process_handle handler){
      handler_context ctxt = get_handler_context(handler);
      return ctxt.task_id;
   } 
   ~simulation_context(){
   }
};


extern simulation_context sim_ctxt;




#endif




