#################################################################
#Helper files for instrumentation, profiling and back-annotation
#################################################################
HELPER=helper
ANNOTATOR=annotation
HELPER_INC =  -Wno-c++11-compat-deprecated-writable-strings -Wno-write-strings  -I. 

#################################################################
# SystemC Options
#################################################################
# SystemC path
include ../../../../HCSim/Makefile.macros
HCSIM=../../../../HCSim


LFLAGS=-L$(SYSTEMC_DIR)/lib-linux64 -Wl,-rpath=$(SYSTEMC_DIR)/lib-linux64  -lsystemc -L$(HCSIM)/lib -lhcsim 
HELPER_INC:=$(HELPER_INC) -DNDEBUG -DSC_INCLUDE_DYNAMIC_PROCESSES -I$(SYSTEMC_DIR)/include -I$(HCSIM)/include 

all: $(ANNOTATOR).cpp
	g++ -O3 $(HELPER_INC) -c  $(ANNOTATOR).cpp -o $(ANNOTATOR).o 
	/usr/bin/ar cr lib${HELPER}.a $(ANNOTATOR).o 
	/usr/bin/ranlib lib${HELPER}.a

clean:
	rm -f *.o *.a *.ll *.log *.bc *~
