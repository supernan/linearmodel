g++ -g *.cpp ../thirdparty/src/*.cpp -L../thirdparty/lib/ -lLinear -lblas -lICTCLAS50 -lglog -lpthread -I../include/ -I../thirdparty/include/ -fPIC -shared -o ../build/libclassifier.so
