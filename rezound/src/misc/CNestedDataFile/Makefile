
CPPFLAGS=-c -I/home/ddurham/code/rezound/src/misc -I/home/ddurham/code/rezound/src/misc/missing



default: test_CNestedDataFile
	@echo
	


# cfg.y --bison--> cfg.tab.cc --c++--> cfg.tab.o
cfg.tab.cc: cfg.lex.c cfg.y CNestedDataFile.h
	bison -p cfg_ -b cfg cfg.y && mv cfg.tab.c cfg.tab.cc

cfg.tab.o: cfg.tab.cc
	c++ $(CPPFLAGS) cfg.tab.cc


# cfg.l --flex--> cfg.lex.c
cfg.lex.c: cfg.l CNestedDataFile.h
	flex -Pcfg_ -t cfg.l > cfg.lex.c




CNestedDataFile.o: CNestedDataFile.cpp CNestedDataFile.h
	c++ $(CPPFLAGS) CNestedDataFile.cpp





test_CNestedDataFile.o: test_CNestedDataFile.cpp CNestedDataFile.h
	c++ $(CPPFLAGS) test_CNestedDataFile.cpp

test_CNestedDataFile: test_CNestedDataFile.o cfg.tab.o CNestedDataFile.o
	c++ -o test_CNestedDataFile test_CNestedDataFile.o cfg.tab.o CNestedDataFile.o



clean:
	rm -f *.o test_CNestedDataFile cfg.tab.cc cfg.lex.c core


