/*********************************************                                                       
 * ECG Tasks Based on Interrupt-Driven Task Model                                                                     
 * Zhuoran Zhao, UT Austin, zhuoran@utexas.edu                                                    
 * Last update: July 2017                                                                           
 ********************************************/

#include <systemc>
#include "OmnetIf_pkt.h"


#ifndef SC_OMNETIF__H
#define SC_OMNETIF__H


class cSimpleModule
{

  public:
    virtual void get_pkt(OmnetIf_pkt* pkt) = 0;


};





#endif // SC_OMNETIF__H 
