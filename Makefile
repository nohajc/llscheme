#ASAN=
ASAN=-fsanitize=address
CXX=clang++ $(ASAN)
LD=clang++ $(ASAN)
LLVMCXXFLAGS=`llvm-config --cxxflags`
LLVMLDFLAGS=`llvm-config --ldflags --system-libs --libs`
CPPFLAGS=-DDEBUG
CXXFLAGS=-std=c++11 -Wall -g $(LLVMCXXFLAGS)
LDFLAGS=$(LLVMLDFLAGS)

OBJS=\
	types.o \
	parser.o \
	reader.o \
	driver.o

schemec: $(OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): types.hpp parser.hpp reader.hpp driver.hpp

test: reader.o
	(cd tests && $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c test_reader.cpp -o test_reader.o && $(LD) test_reader.o ../reader.o $(LDFLAGS) -o test && cat test_reader.in | ./test)

reader.o: reader.cpp reader.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c reader.cpp -o reader.o

clean:
	rm *.o
	rm tests/test
