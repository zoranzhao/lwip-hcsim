#include <systemc.h>
#include "omnet_dummy.h"

double power_cli[10];
double CliEnergy[10];
int node_choice = 0;
unsigned char debug_flags=LWIP_DBG_OFF;

//const char* addr_list[MAX_EDGE_NUM] = EDGE_ADDR_LIST;

const char* addr_list[MAX_EDGE_NUM] = {"100:0:200:0:300:0:400:", "100:0:200:0:300:0:500:", "100:0:200:0:300:0:600:", "100:0:200:0:300:0:700:", "100:0:200:0:300:0:800:", "100:0:200:0:300:0:900:"};

class top : public sc_module
{
   public:
     sc_fifo<OmnetIf_pkt*> fifo_inst1;
     sc_fifo<OmnetIf_pkt*> fifo_inst2;

     cSimpleModuleWrapper *prod_inst;
     cSimpleModuleWrapper *cons_inst;

     top(sc_module_name name) : sc_module(name), fifo_inst1(100), fifo_inst2(100){
       prod_inst = new cSimpleModuleWrapper("Producer1", 0);
       prod_inst->data_out(fifo_inst1);
       prod_inst->data_in(fifo_inst2);

       cons_inst = new cSimpleModuleWrapper("Consumer1", 1);
       cons_inst->data_out(fifo_inst2);
       cons_inst->data_in(fifo_inst1);
     }
};

int sc_main (int argc , char *argv[]) {
   top top1("Top1");
   sc_start();
   return 0;
}
