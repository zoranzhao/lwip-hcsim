#include <systemc>
#include "HCSim.h"
#ifndef LWIP_OS__H
#define LWIP_OS__H
class lwip_recv_if: virtual public sc_core::sc_interface
{
public:
    virtual int GetNode(int os_task_id) = 0;
    virtual int GetWeight(int os_task_id) = 0;
    virtual int GetSize(int os_task_id) = 0;
    virtual bool GetData(unsigned size, char* data, int os_task_id) = 0;
};

class lwip_send_if: virtual public sc_core::sc_interface
{
public:
    virtual void SetNode(int NodeID, int os_task_id) = 0;
    virtual void SetWeight(int weight, int os_task_id) = 0;
    virtual void SetSize(int size, int os_task_id) = 0;
    virtual void SetData(unsigned size, char* data, int os_task_id) = 0;
};

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


#endif
