SYSTEMC=/home/slam/OMNET/systemc-2.3.1

OS_MODEL=/home/slam/OMNET/HCSim
LWIP=/home/slam/OMNET/lwip-hcsim
LWIP_LIB=/home/slam/OMNET/lwip-hcsim/ports
HELPER=/home/slam/OMNET/lwip-hcsim/ports/hcsim/context
DISTRIOT=/home/slam/OMNET/NoSSim_contrib/DeepThings/port/hcsim_lwip
DEEPTHINGS=/home/slam/OMNET/NoSSim_contrib/DeepThings/DeepThings
DEEPTHINGS_LIB=/home/slam/OMNET/NoSSim_contrib/DeepThings
DEEPTHINGS_TEST=/home/slam/OMNET/NoSSim_contrib/DeepThings
DARKNET=/home/slam/OMNET/NoSSim_contrib/DeepThings/darknet-nnpack

DEFINES	= -g -DNDEBUG -DSC_INCLUDE_DYNAMIC_PROCESSES
# --- Package
MODULE 	= simple_fifo
OBJS	= $(MODULE).o
EXE	= $(MODULE)

# --- Macros
INCDIR = -I. -I$(SYSTEMC)/include  -I./src/OSNode 
INCDIR += -I$(OS_MODEL)/include -I$(LWIP)/src/include -I$(LWIP)/ports/port/include -I$(LWIP)/src/include/ipv4 -I$(LWIP)/lwip/src  -I$(LWIP)/ports/hcsim -I$(HELPER) -I$(DISTRIOT)/include -I$(DISTRIOT)/src -I$(DISTRIOT)/hcsim -I$(DISTRIOT)/lwip -I$(DEEPTHINGS)/include -I$(DEEPTHINGS)/src -I$(DEEPTHINGS_TEST)/include -I$(DEEPTHINGS_TEST)/src -I$(DARKNET)/include -I$(DARKNET)/src 

EXTRA_LIBDIR = -L$(LWIP_LIB)  -L$(HELPER)  -L$(OS_MODEL)/lib -L$(DISTRIOT) -L$(DEEPTHINGS_LIB) -L$(DARKNET)
EXTRA_LIBDIR += -Wl,-rpath,$(LWIP_LIB) -Wl,-rpath,$(HELPER) -Wl,-rpath,$(OS_MODEL)/lib -Wl,-rpath,$(DISTRIOT) -Wl,-rpath,$(DEEPTHINGS_LIB) -Wl,-rpath,$(DARKNET)

LIBDIR = -L. -L$(SYSTEMC)/lib-linux64 -Wl,-rpath,$(SYSTEMC)/lib-linux64 $(EXTRA_LIBDIR)

EXTRA_LIBS = -ldeepthings -ldarknet -ldistriot -llwipapps -llwip4  -lhcsim -lsystemc 
LIBS   =  -lm $(EXTRA_LIBS)

# --- Tools
CC	= clang++ $(DEFINES)

# --- Rules
.SUFFIXES: .cc .cpp .o

$(EXE): $(OBJS) $(SYSTEMC)/lib-linux64/libsystemc.a 
	$(CC) $(CFLAGS) $(INCDIR) $(LIBDIR) -o $@ $(OBJS) $(LIBS) 

.cpp.o:
	$(CC) $(CFLAGS) $(INCDIR) -c $<

.cc.o:
	$(CC) $(CFLAGS) $(INCDIR) -c $<

test:
	./$(MODULE)

clean::
	rm -f $(OBJS) *.log *~ $(EXE) core


