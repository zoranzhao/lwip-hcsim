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

class Nic_dummy: public sc_core::sc_module {
  public:
	cSimpleModule* OmnetWrapper;

   	sc_core::sc_event recvd; 
   	sc_core::sc_event sent; 
	char* data_send;
	int size_send;
	char* data_recv;
	int size_recv;
	int NodeID;

	SC_HAS_PROCESS(Nic_dummy);

	Nic_dummy(sc_module_name name, int NodeID) : sc_module(name)
	{
		this->NodeID = NodeID;
		SC_THREAD(NicRecv);
		SC_THREAD(NicSend);
	}



	void NicSend()
	{


		cSimpleModule* wrapper = (cSimpleModule*)(OmnetWrapper);
		//while(1){
		size_send = 10;
		data_send = new char[size_send];
		data_send[0] = '0';
		data_send[1] = '1';	
		data_send[2] = '2';
		data_send[3] = '\0';				

			OmnetIf_pkt* pkt = new OmnetIf_pkt();
			pkt->setFileBufferArraySize(size_send);
			for(int ii=0; ii<size_send; ii++){
				pkt->setFileBuffer(ii, ((char*)data_send)[ii]);
			}
			wrapper -> get_pkt(pkt);
			sc_core::wait(sent);
		//}




	}
	void NicRecv()
	{
		while (1) {
			sc_core::wait(recvd);
			std::cout << NodeID << std::endl;
			std::cout << data_recv << std::endl;

	    	}   
	}



	void notify_sending(){  
		sent.notify(sc_core::SC_ZERO_TIME); 
	}
	void notify_receiving(char* fileBuffer, unsigned int size){  
        	data_recv = fileBuffer;
		size_recv = size;
		recvd.notify(sc_core::SC_ZERO_TIME); 
	}

};

class artificial_example_dummy : public sc_core::sc_module
{
  public:
	Nic_dummy* NetworkInterfaceCard1;
	Nic_dummy* NetworkInterfaceCard2;

	artificial_example_dummy(const sc_core::sc_module_name name, int NodeID)
        :sc_core::sc_module(name)
	{
		NetworkInterfaceCard1 =  new Nic_dummy("nic1", NodeID); 
		NetworkInterfaceCard2 =  new Nic_dummy("nic2", NodeID); 
	}

};

class cSimpleModuleWrapper:public sc_core::sc_module,virtual public cSimpleModule
{
  public:



    sc_port< sc_fifo_out_if<OmnetIf_pkt*> > data_out;
    sc_port< sc_fifo_in_if<OmnetIf_pkt*> > data_in;


    int NodeID;
    artificial_example *System; 

    SC_HAS_PROCESS(cSimpleModuleWrapper);
    cSimpleModuleWrapper(const sc_core::sc_module_name name, int NodeID)
    :sc_core::sc_module(name)
    {

	this->NodeID = NodeID;
	System = new artificial_example ("mix_taskset_cli", NodeID); 
	System -> NetworkInterfaceCard1 -> OmnetWrapper = this;
	System -> NetworkInterfaceCard2 -> OmnetWrapper = this;

        SC_THREAD(linkin);		

    }
    
    ~cSimpleModuleWrapper() {}

    void get_pkt(OmnetIf_pkt* pkt){

	data_out -> write(pkt);
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

		if(NodeID==0) {
		        std::cout << NodeID << " ------------- in <<<<<<<< size is: " << pkt->fileBuffer_arraysize<< " count is:" <<count  << " time is: " << sc_core::sc_time_stamp().value() << std::endl;
			//if(count == 2) continue;
			//if(count == 5) continue;
			//if(count == 30) continue;
			//if(count == 31) continue;

		}

		//if(NodeID==1) {
		//        std::cout << NodeID << " ------------- in <<<<<<<< size is: " << pkt->fileBuffer_arraysize<< " count is:" <<count  << " time is: " << sc_core::sc_time_stamp().value() << std::endl;

		//}

      		System -> NetworkInterfaceCard1 -> notify_receiving(pkt->fileBuffer, pkt->fileBuffer_arraysize);
	}
    }







};









#endif // SC_OMNET__H 
