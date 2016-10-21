CPP = g++
CPPOPTS = -O3

lib:
	$(CPP) -g *.cpp ../thirdparty/src/*.cpp -I../include -I../thirdparty/include -L../thirdparty/lib -lLinear -lblas -lICTCLAS50 -glog -lpthread $(CPPOPTS)
	ar cr libClassifier.a *.o
	cp ./libClassifier.a ../build
clean:
	rm -r libFilter.a *.o

