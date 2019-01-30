#include <systemc>
#include "HCSim.h"
#include "lwipOS_if.h"
#include <string>
#include <unordered_map>

#ifndef OS_CTXT__H
#define OS_CTXT__H


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



class OSModelCtxt{
  public:
	int NodeID;

	//TODO Should be replaced by SystemC OS model channel
	int flag_compute;
        sc_core::sc_event comp_sig; // systemc channel
        int Blocking_taskID;
	//TODO Should be replaced by SystemC OS model channel

    	//sc_core::sc_port< lwip_send_if > dataCH;
    	//sc_core::sc_port< lwip_recv_if > intrCH;
	sc_core::sc_port<lwip_recv_if> recv_port[2];
	sc_core::sc_port<lwip_send_if> send_port[2]; 
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
	std::vector<OSModelCtxt* > ctxtIDList;  
	std::vector<AnnotationCtxt* > annotList;
	std::vector<void* > lwipList;
        std::vector<app_context* > app_context_list;

	void registerTask(OSModelCtxt* ctxt, void* lwipCtxt, int taskID, sc_core::sc_process_handle taskHandler){
                

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


	void* getLwipCtxt(sc_core::sc_process_handle taskHandler){
		//printf("Getting lwip context ... .... .....\n");

		std::vector< sc_core::sc_process_handle >::iterator handlerIt = taskHandlerList.begin();
		std::vector< void* >::iterator idIt = lwipList.begin();
		for(; (handlerIt!=taskHandlerList.end() && idIt!= lwipList.end() ) ;handlerIt++, idIt++){
			if(*handlerIt == taskHandler)
				return *idIt;	
		}

		return NULL;
	} 
	

	int getTaskID(sc_core::sc_process_handle taskHandler){
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




	OSModelCtxt* getTaskCtxt(sc_core::sc_process_handle taskHandler){
		std::vector< sc_core::sc_process_handle >::iterator handlerIt = taskHandlerList.begin();
		std::vector< OSModelCtxt* >::iterator idIt = ctxtIDList.begin();
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
#endif



