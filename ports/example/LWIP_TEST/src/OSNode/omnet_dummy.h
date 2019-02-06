/*********************************************                                                       
 * ECG Tasks Based on Interrupt-Driven Task Model                                                                     
 * Zhuoran Zhao, UT Austin, zhuoran@utexas.edu                                                    
 * Last update: July 2017                                                                           
 ********************************************/

#include <systemc>
#include "top_module.h"
#include "omnet_if.h"

#ifndef SC_OMNET__H
#define SC_OMNET__H

class cSimpleModuleWrapper:public sc_core::sc_module,virtual public cSimpleModule
{
  public:

    sc_port< sc_fifo_out_if<OmnetIf_pkt*> > data_out;
    sc_port< sc_fifo_in_if<OmnetIf_pkt*> > data_in;

    int NodeID;
    artificial_example *System; 
    int sent_count;

    SC_HAS_PROCESS(cSimpleModuleWrapper);
    cSimpleModuleWrapper(const sc_core::sc_module_name name, int NodeID)
    :sc_core::sc_module(name)
    {

	this->NodeID = NodeID;
	System = new artificial_example ("mix_taskset_cli", NodeID); 
	System -> NetworkInterfaceCard1 -> OmnetWrapper = this;
	System -> NetworkInterfaceCard2 -> OmnetWrapper = this;
        sent_count = 0;
        SC_THREAD(linkin);		

    }
    
    ~cSimpleModuleWrapper() {}

    void get_pkt(OmnetIf_pkt* pkt){
       
	data_out -> write(pkt);
        sent_count++;
	std::cout << NodeID << " ------------- out >>>>>>>> size is: " << pkt->fileBuffer_arraysize<< " count is:"<< pkt->pkt_number   << " time is: " << sc_core::sc_time_stamp().value() << std::endl;
	//if(NodeID==1) {
	//	        std::cout << NodeID << " ------------- out >>>>>>>> size is: " << pkt->fileBuffer_arraysize<< " count is:"   << " time is: " << sc_core::sc_time_stamp().value() << std::endl;
	//}
	System -> NetworkInterfaceCard1 -> notify_sending();

    }


 private:


  
    void linkin(){
	int count = 0;
	OmnetIf_pkt* pkt;
   	sc_core::sc_event test; 

	while(1){
		pkt = data_in -> read();
		count++;
		//if(NodeID==0) {
		std::cout << NodeID << " ------------- in <<<<<<<< size is: " << pkt->fileBuffer_arraysize<< " count is:" << pkt->pkt_number  << " time is: " << sc_core::sc_time_stamp().value() << std::endl;
		//if(count == 2) continue;
		//if(count == 5) continue;
		//if(count == 30) continue;
		//if(count == 31) continue;

		//}

		//if(NodeID==1) {
		//        std::cout << NodeID << " ------------- in <<<<<<<< size is: " << pkt->fileBuffer_arraysize<< " 
			//count is:" <<count  << " time is: " << sc_core::sc_time_stamp().value() << std::endl;

		//}

      		System -> NetworkInterfaceCard1 -> notify_receiving(pkt->fileBuffer, pkt->fileBuffer_arraysize);
	}
    }







};









#endif // SC_OMNET__H 
