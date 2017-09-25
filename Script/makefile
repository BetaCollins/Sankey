CXX=g++

#if not set at command line default to release(DEBUG=0)
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CXXFLAGS =-g -std=gnu++11 -DGD -DCAIRO -I/usr/include -I/usr/include/cairo
else
	CXXFLAGS =-std=gnu++11 -DGD -DCAIRO -DNDEBUG -I/usr/include -I/usr/include/cairo
endif


LDFLAGS=
#remember that -l expands the name lib[name].a for example -lz looks for the library libz.a
LDLIBS=-lcairo -lz -lbz2

#these are the flags/libs i used for cygwin compilation
#LDFLAGS=-L/lib/perl5/5.22/x86_64-cygwin-threads/auto/Compress/Raw
#LDLIBS=-lgd -lcairo.dll -lz.dll -lbz2.dll

RM=rm -f

SRCDIR=./src

SRCS=System.cpp Utility.cpp Params.cpp FasReader.cpp Segment.cpp
STATS_SRCS=$(SRCS) PhyloStats.cpp
GRAPH_SRCS=$(SRCS) GraphPhylogeny.cpp
STATS_OBJS=$(subst .cpp,.o,$(STATS_SRCS))
GRAPH_OBJS=$(subst .cpp,.o,$(GRAPH_SRCS))
STATS_EXE=phylo_stats.exe
GRAPH_EXE=phylo_graph.exe
PARSE_EXE=parse_data.exe

all: phylo_graph parse_data
#phylo_stats is another program included in the original source.  I don't know what it does so I'll leave it out of the build
#all: phylo_stats phylo_graph parse_data

debug: phylo_graph parse_data

phylo_stats: $(STATS_OBJS)
	$(CXX) $(LDFLAGS) -o $(STATS_EXE) $(STATS_OBJS) $(LDLIBS)

phylo_graph: $(GRAPH_OBJS)
	$(CXX) $(LDFLAGS) -o $(GRAPH_EXE) $(GRAPH_OBJS) $(LDLIBS)
	
parse_data: System.o Utility.o ParseData.o Classification.o
	$(CXX) -o $(PARSE_EXE) ParseData.o Utility.o System.o Classification.o $(LDLIBS)
	
System.o: $(SRCDIR)/System.cpp $(SRCDIR)/System.h
	$(CXX) $(CXXFLAGS) -c $(SRCDIR)/System.cpp
Utility.o: $(SRCDIR)/Utility.cpp $(SRCDIR)/Utility.h $(SRCDIR)/System.h $(SRCDIR)/SmallPrimes.inc
	$(CXX) $(CXXFLAGS) -c $(SRCDIR)/Utility.cpp
Params.o: $(SRCDIR)/Params.cpp $(SRCDIR)/Params.h $(SRCDIR)/System.h $(SRCDIR)/Utility.h $(SRCDIR)/ParamDefines.inc
	$(CXX) $(CXXFLAGS) -c $(SRCDIR)/Params.cpp
FasReader.o: $(SRCDIR)/FasReader.cpp $(SRCDIR)/FasReader.h $(SRCDIR)/System.h $(SRCDIR)/Utility.h
	$(CXX) $(CXXFLAGS) -c $(SRCDIR)/FasReader.cpp
GraphPhylogeny.o: $(SRCDIR)/GraphPhylogeny.cpp $(SRCDIR)/System.h $(SRCDIR)/Utility.h $(SRCDIR)/Params.h $(SRCDIR)/DGNode.h
	$(CXX) $(CXXFLAGS) -c $(SRCDIR)/GraphPhylogeny.cpp
Segment.o: $(SRCDIR)/Segment.cpp $(SRCDIR)/Segment.h $(SRCDIR)/System.h
	$(CXX) $(CXXFLAGS) -c $(SRCDIR)/Segment.cpp
PhyloStats.o: $(SRCDIR)/PhyloStats.cpp $(SRCDIR)/System.h $(SRCDIR)/Utility.h $(SRCDIR)/Segment.h $(SRCDIR)/FasReader.h $(SRCDIR)/Params.h $(SRCDIR)/DGNode.h
	$(CXX) $(CXXFLAGS) -c $(SRCDIR)/PhyloStats.cpp
ParseData.o: $(SRCDIR)/DGNode.h
	$(CXX) $(CXXFLAGS) -c $(SRCDIR)/ParseData.cpp
Classification.o: $(SRCDIR)/Classification.cpp $(SRCDIR)/Classification.h
	$(CXX) $(CXXFLAGS) -c $(SRCDIR)/Classification.cpp

clean:
	$(RM) $(STATS_OBJS)
	$(RM) $(GRAPH_OBJS)
	$(RM) ParseData.o
	$(RM) Classification.o
	
dist-clean: clean
	$(RM) $(STATS_EXE)
	$(RM) $(GRAPH_EXE)
	$(RM) $(PARSE_EXE)
