CXX=clang++ -fsanitize=address
LD=clang++ -fsanitize=address
LLVMCXXFLAGS=`llvm-config --cxxflags`
LLVMLDFLAGS=`llvm-config --ldflags --system-libs --libs`
CPPFLAGS=-DDEBUG
CXXFLAGS=-std=c++11 -Wall -g $(LLVMCXXFLAGS)
LDFLAGS=$(LLVMLDFLAGS)

test: reader.o
	(cd tests && $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c test_reader.cpp -o test_reader.o && $(LD) test_reader.o ../reader.o $(LDFLAGS) -o test && cat test_reader.in | ./test)

reader.o: reader.cpp reader.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c reader.cpp -o reader.o

clean:
	rm *.o
	rm tests/test
