$APP-NAME := kcc

CPPFLAGS+=-O2

kcc: preprocessor.o lexical.o syntax.o semantic.o assembly.o kcc.o
	g++ -o kcc preprocessor.o lexical.o syntax.o semantic.o assembly.o kcc.o

preprocessor.o: preprocessor.cpp kcc.h
	g++ $(CPPFLAGS) -c -o preprocessor.o preprocessor.cpp

lexical.o: lexical.cpp kcc.h
	g++ $(CPPFLAGS) -c -o lexical.o lexical.cpp

syntax.o: syntax.cpp kcc.h
	g++ $(CPPFLAGS) -c -o syntax.o syntax.cpp

semantic.o: semantic.cpp kcc.h
	g++ $(CPPFLAGS) -c -o semantic.o semantic.cpp

assembly.o: assembly.cpp assembly_32.cpp assembly_64.cpp kcc.h
	g++ $(CPPFLAGS) -c -o assembly.o assembly.cpp

kcc.o: kcc.cpp kcc.h
	g++ $(CPPFLAGS) -c -o kcc.o kcc.cpp


clean:
	rm -rf kcc preprocessor.o lexical.o syntax.o semantic.o assembly.o kcc.o
