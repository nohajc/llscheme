test: reader.o
	(cd tests && clang++ -std=c++11 -c test_reader.cpp -o test_reader.o -Wall && clang++ test_reader.o ../reader.o -o test && echo "((a b) c)" | ./test)

reader.o: reader.cpp reader.hpp
	clang++ -std=c++11 -c reader.cpp -o reader.o -Wall
