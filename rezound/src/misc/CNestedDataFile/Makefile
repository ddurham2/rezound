

# use this to handle the .l and .y files
cfg.tab.cc: cfg.lex.c cfg.y CNestedDataFile.h
	bison -p cfg_ -b cfg cfg.y && mv cfg.tab.c cfg.tab.cc

cfg.lex.c: cfg.l CNestedDataFile.h
	flex -Pcfg_ -t cfg.l > cfg.lex.c



and these are the required -I flags for building the lib because it uses istring
     CNestedDataFile.cpp cfg.tab.cc -I/home/ddurham/code/rezound/src/misc -I/home/ddurham/code/rezound/src/misc/missing


