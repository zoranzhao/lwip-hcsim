#include <systemc>
#include "HCSim.h"
#include <string>
#include <unordered_map>

#ifndef OS_CTXT__H
#define OS_CTXT__H

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



class app_context{
   std::unordered_map<std::string, void*> ctxt_list;  
public:
   /*For example, here we can have global data defined to hold application states*/
   app_context(void* ctxt){
      ctxt_list["lwIP"] = ctxt;
   }
   void add_context(std::string ctxt_name, void* ctxt){
      ctxt_list[ctxt_name] = ctxt;
   }
   void* get_context(std::string ctxt_name){
      return ctxt_list[ctxt_name];
   }
};



typedef struct AnnotationTrackerStruct {
  int FunID[10000];//depth of calling stack
  int LibID[10000];//depth of calling stack
  int NumFuncInExec;
} AnnotTracker;



class AnnotationCtxt{
public:
	long long cycles;
	long TotalBBs;
	long FuncBBs[100][10000];
	int CurFunID;
	int CurLibID;
	AnnotTracker AnnotationTracker;

	AnnotationCtxt(){
		cycles = 0;
		TotalBBs = 0;
		CurFunID = 0;
		CurLibID = 0;
		int ii;
		int jj;
		for(ii=0;ii<100;ii++)
		  for(jj=0;jj<10000;jj++){
			FuncBBs[ii][jj] = 0;	
		  }
		TotalBBs = 0;
		AnnotationTracker.NumFuncInExec=0;


	}
};



class os_model_context{
  public:
	int NodeID;

	//TODO Should be replaced by SystemC OS model channel
	int flag_compute;
        sc_core::sc_event comp_sig; // systemc channel
        int Blocking_taskID;
	//TODO Should be replaced by SystemC OS model channel

    	//sc_core::sc_port< lwip_send_if > dataCH;
    	//sc_core::sc_port< lwip_recv_if > intrCH;
	sc_core::sc_port<sys_call_recv_if> recv_port[2];
	sc_core::sc_port<sys_call_send_if> send_port[2]; 
	sc_core::sc_port< HCSim::OSAPI > os_port;

	//System Node configuration parameters
  	int CLI_NUM;
	int OFFLOAD_LEVEL;
	int CLI_TYPE;
	int SRV_TYPE;
	int CLI_CORE_NUM;
	int SRV_CORE_NUM;


};



class GlobalRecorder {
  public:
	std::vector< sc_core::sc_process_handle> taskHandlerList;  
	std::vector< int > taskIDList;  
	std::vector<os_model_context* > ctxtIDList;  
	std::vector<AnnotationCtxt* > annotList;
	std::vector<void* > lwipList;
        std::vector<app_context* > app_context_list;

	void register_task(os_model_context* ctxt, void* lwipCtxt, int taskID, sc_core::sc_process_handle taskHandler){
                

		lwipList.push_back(lwipCtxt);
		ctxtIDList.push_back(ctxt);
		taskIDList.push_back(taskID);
		taskHandlerList.push_back(taskHandler);
		annotList.push_back(new AnnotationCtxt());
  		app_context_list.push_back(new app_context(lwipCtxt));

		for(size_t i = 0; i < taskIDList.size(); i++)
			std::cout << taskIDList[i]  <<", ";
		std::cout << std::endl;

		for(size_t i = 0; i < ctxtIDList.size(); i++)
			std::cout << (ctxtIDList[i])->NodeID  <<", ";
		std::cout << std::endl;



	}


        app_context* get_app_ctxt(sc_core::sc_process_handle taskHandler){
		auto handlerIt = taskHandlerList.begin();
		auto idIt = app_context_list.begin();
		for(; (handlerIt!=taskHandlerList.end() && idIt!= app_context_list.end() ) ;handlerIt++, idIt++){
			if(*handlerIt == taskHandler)
				return *idIt;	
		}
		return NULL;
        } 

	int get_task_id(sc_core::sc_process_handle taskHandler){
		std::vector< sc_core::sc_process_handle >::iterator handlerIt = taskHandlerList.begin();
		std::vector< int >::iterator idIt = taskIDList.begin();
		for(; (handlerIt!=taskHandlerList.end() && idIt!=taskIDList.end() ) ;handlerIt++, idIt++){
			if(*handlerIt == taskHandler)
				return *idIt;	
		}
		return -1;
	} 



	AnnotationCtxt* getAnnotCtxt(sc_core::sc_process_handle taskHandler){
		std::vector< sc_core::sc_process_handle >::iterator handlerIt = taskHandlerList.begin();
		std::vector< AnnotationCtxt* >::iterator idIt = annotList.begin();
		for(; (handlerIt!=taskHandlerList.end() && idIt!= annotList.end() ) ;handlerIt++, idIt++){
			if(*handlerIt == taskHandler)
				return *idIt;	
		}
		//printf("Error: no annotation ctxt existing in the global recorder\n");
		return NULL;
	} 


	os_model_context* get_os_ctxt(sc_core::sc_process_handle taskHandler){
		std::vector< sc_core::sc_process_handle >::iterator handlerIt = taskHandlerList.begin();
		std::vector< os_model_context* >::iterator idIt = ctxtIDList.begin();
		for(; (handlerIt!=taskHandlerList.end() && idIt!=ctxtIDList.end() ) ;handlerIt++, idIt++)
		{
			if(*handlerIt == taskHandler)
				return *idIt;	
		}
		//printf("Error: no task ctxt existing in the global recorder\n");
		return NULL;
	} 


};


extern GlobalRecorder sim_ctxt;



#ifndef LWIP_HDR_SYS_H
typedef void (*thread_fn)(void *arg);
struct sys_thread;
typedef struct sys_thread* sys_thread_t;
/*multithreading APIs*/
extern "C" sys_thread_t sys_thread_new(const char *name, thread_fn function, void *arg, int stacksize, int prio);
extern "C" void sys_thread_join(sys_thread_t thread);

/*Semaphore APIs*/
struct sys_sem;
typedef struct sys_sem* sys_sem_t;
extern "C" void sys_sem_signal(sys_sem_t *s);
extern "C" uint32_t sys_arch_sem_wait(sys_sem_t *s, uint32_t timeout);
extern "C" void sys_sem_free(sys_sem_t *sem);
extern "C" void sys_sleep();
extern "C" uint32_t sys_now(void);
extern "C" double sys_now_in_sec(void);
#endif


#endif




