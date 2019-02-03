#
# Copyright (c) 2001, 2002 Swedish Institute of Computer Science.
# All rights reserved. 
# 
# Redistribution and use in source and binary forms, with or without modification, 
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
# SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
# OF SUCH DAMAGE.
#
# Makefile for the HCSim lwIP TCP/IP stack.
# 
# Author: Zhuoran Zhao <zhuoran@utexas.edu>
#

# Include SystemC lib.
SYSTEMC_LIB=/home/slam/OMNET/systemc-2.3.1/lib-linux64
SYSTEMC_INC=/home/slam/OMNET/systemc-2.3.1
HCSIM_DIR=/home/slam/OMNET/HCSim

#INS_LIB = ../InstrumentLLVM/build/FunctionInstr/libLLVMFunctionInstr.so
#LLVM_INSTR_PASS = -functionInstr
#INS_STAGE=annot#preprof#prof#annot 
#LIB_ID=-libname=12


CCDEP=g++
CC=g++
COPTS=-emit-llvm
LLVM_CC=clang++
LLVM_LINK=llvm-link-6.0
LLVM_LLC=llc-6.0
ARFLAGS=rs
LFLAGS=-L$(SYSTEMC_LIB) -lsystemc -L$(HCSIM_DIR)/lib -lhcsim -Wl,-rpath,$(SYSTEMC_LIB) 



CFLAGS+=-g -Wall -DNDEBUG -DSC_INCLUDE_DYNAMIC_PROCESSES \
	-DLWIP_DEBUG -pedantic -Werror \
	-Wparentheses -Wsequence-point -Wswitch-default \
	-Wextra -Wundef -Wshadow -Wpointer-arith \
	-Wcast-qual \
	-Wwrite-strings  -Wcast-align \
	-Wredundant-decls  -Wno-address \
	-Wunreachable-code -Wuninitialized \
	-Wwrite-strings -Wredundant-decls -fPIC -w

ifeq (,$(findstring clang,$(CC)))
CFLAGS+= -Wlogical-op
# if GCC is newer than 4.8/4.9 you may use:
#CFLAGS:=$(CFLAGS) -fsanitize=address -fstack-protector -fstack-check -fsanitize=undefined -fno-sanitize=alignment
else
# we cannot sanitize alignment on x86-64 targets because clang wants 64 bit alignment
CFLAGS+= -fsanitize=address -fsanitize=undefined -fno-sanitize=alignment -Wdocumentation
endif



LWIPPORT=./port
APPLWIP=./hcsim
LWIPDIR=../../lwip-hcsim/src
HELPER_DIR = ./hcsim/context

CFLAGS:=$(CFLAGS) \
	-I$(LWIPDIR)/include -I$(LWIPPORT)/include -I$(LWIPDIR)/include/ipv4 \
	-I$(LWIPDIR) -I$(LWIPPORT)/include -I$(APPLWIP) \
	-I$(SYSTEMC_INC)/include \
	-I$(HCSIM_DIR)/include -I$(HELPER_DIR)



COREFILES=$(LWIPDIR)/core/init.c \
	$(LWIPDIR)/core/def.c \
	$(LWIPDIR)/core/inet_chksum.c \
	$(LWIPDIR)/core/ip.c \
	$(LWIPDIR)/core/mem.c \
	$(LWIPDIR)/core/memp.c \
	$(LWIPDIR)/core/netif.c \
	$(LWIPDIR)/core/pbuf.c \
	$(LWIPDIR)/core/stats.c \
	$(LWIPDIR)/core/sys.c \
	$(LWIPDIR)/core/tcp.c \
	$(LWIPDIR)/core/tcp_in.c \
	$(LWIPDIR)/core/tcp_out.c \
	$(LWIPDIR)/core/timeouts.c \
	$(LWIPDIR)/core/udp.c 
	#$(LWIPDIR)/core/raw.c 

CORE4FILES=$(LWIPDIR)/core/ipv4/autoip.c \
	$(LWIPDIR)/core/ipv4/etharp.c \
	$(LWIPDIR)/core/ipv4/ip4_frag.c \
	$(LWIPDIR)/core/ipv4/ip4.c \
	$(LWIPDIR)/core/ipv4/ip4_addr.c \
	$(LWIPDIR)/core/ipv4/icmp.c \
	$(LWIPDIR)/core/ipv4/igmp.c

CORE6FILES=$(LWIPDIR)/core/ipv6/ethip6.c \
	$(LWIPDIR)/core/ipv6/icmp6.c \
	$(LWIPDIR)/core/ipv6/inet6.c \
	$(LWIPDIR)/core/ipv6/ip6.c \
	$(LWIPDIR)/core/ipv6/ip6_addr.c \
	$(LWIPDIR)/core/ipv6/ip6_frag.c \
	$(LWIPDIR)/core/ipv6/nd6.c \
	$(LWIPDIR)/core/ipv6/mld6.c

APIFILES=$(LWIPDIR)/api/api_lib.c \
	$(LWIPDIR)/api/api_msg.c \
	$(LWIPDIR)/api/err.c \
	$(LWIPDIR)/api/netbuf.c \
	$(LWIPDIR)/api/netdb.c \
	$(LWIPDIR)/api/netifapi.c \
	$(LWIPDIR)/api/tcpip.c \
	$(LWIPDIR)/api/sockets.c


NETIFFILES=$(LWIPDIR)/netif/ethernet.c 


# SIXLOWPAN: 6LoWPAN
SIXLOWPAN=$(LWIPDIR)/netif/lowpan6.c \
	  $(LWIPDIR)/netif/lowpan6_common.c

ARCHFILES=$(wildcard $(LWIPPORT)/*.c $(LWIPPORT)/netif/hcsim_if.c)
APPFILES=$(APPLWIP)/app_utils.c 
LWIPFILES=$(COREFILES) $(CORE4FILES) $(CORE6FILES) $(APIFILES) $(NETIFFILES) $(SIXLOWPAN)


LWIPLIB=liblwip4.a
APPLIB=liblwipapps.a
LWIPALL=liblwip4
LWIPFILESW=$(wildcard $(LWIPFILES))
LWIPBCFILES=$(notdir $(LWIPFILES:.c=.bc))
ARCHFILESW=$(wildcard $(ARCHFILES))
APPOBJS=$(notdir $(APPFILES:.c=.o))
LWIPOBJ=$(LWIPALL).o
LWIPOBJS=$(notdir $(LWIPFILESW:.c=.o))
ARCHOBJS=$(notdir $(ARCHFILESW:.c=.o))



all: $(LWIPLIB) $(APPLIB)
.PHONY: all

clean:
	rm -f *.bc *.ll *.a *.o $(LWIPLIB) $(APPLIB) *.s .depend* *.core core *~

depend dep: .depend

include .depend

$(LWIPOBJ): $(LWIPFILES)
	$(LLVM_CC) $(COPTS) $(CFLAGS) -c $(LWIPFILES) 
	$(LLVM_LINK) $(LWIPBCFILES) -o $(LWIPALL).all.bc 
	#opt -load $(INS_LIB) $(LLVM_INSTR_PASS) -libname=12 -InstruProf=$(INS_STAGE) < $(LWIPALL).all.bc > $(LWIPALL).instru.bc
	#llc -filetype=obj $(LWIPALL).instru.bc -o $(LWIPALL).o
	$(LLVM_LLC) -filetype=obj $(LWIPALL).all.bc -o $(LWIPALL).o

#$(): 
%.o:
	$(LLVM_CC) $(CFLAGS) -c $(<:.o=.c) $(LFLAGS)

$(APPLIB): $(APPOBJS)
	$(AR) $(ARFLAGS) $(APPLIB) $?

$(LWIPLIB): $(LWIPOBJ) $(ARCHOBJS)
	echo $(AR) $(ARFLAGS) $(LWIPLIB) $?
	$(AR) $(ARFLAGS) $(LWIPLIB) $?

.depend: $(LWIPFILES) $(ARCHFILES) $(APPFILES)
	$(CCDEP) $(CFLAGS) -MM $^ > .depend || rm -f .depend










