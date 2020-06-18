CC = g++
CXX = g++ 
	   
CXXFLAGS = -DMAX_MSG_PER_PROC=8
CXXFLAGS += -Wall -O3 -Iface/include -Iface/include/opencv/include -Iface/include/opencv/include/opencv

LDFLAGS = -pthread

LDFLAGS += -L./face/bin -lTHFaceImage -lTHFeature -lTHFacialPos -lopencv_core -lopencv_highgui \
	   -lmkl_core -lmkl_def -lmkl_intel_lp64 -lmkl_sequential -lmkl_vml_avx2 -lprotobuf

all : testtasks testface testsrv teststorage testlisten listen testc

.c : .o
	$(CC) -c $(CFLAGS) -o $@

.cpp : .o
	$(CXX) -c $(CFLAGS) $^


testtasks : tasks.o testtasks.o
	$(CC) $^ -o $@ $(LDFLAGS)

testface : testface.o
	$(CXX) $^ -o $@ $(LDFLAGS)
	cp -v $@ ./face/bin

testsrv : testsrv.o tasks.o facesrv.o storage.o
	$(CXX) $^ -o $@ $(LDFLAGS)
	cp -v $@ ./face/bin

teststorage : teststorage.o storage.o
	$(CC) $^ -o $@ $(LDFLAGS)

testlisten : testlisten.o
	$(CC) $^ -o $@ $(LDFLAGS)
	cp -v $@ ./face/bin

listen : listen.o client.o tasks.o facesrv.o storage.o sort.o client.h
	$(CC) $^ -o $@ $(LDFLAGS)
	cp -v $@ ./face/bin


testc : testc.o client.o tasks.o facesrv.o storage.o sort.o client.h
	$(CC) $^ -o $@ $(LDFLAGS)
	cp -v $@ ./face/bin

.PHONY:clean

clean :
	rm -f *.o testtasks testface testsrv teststorage client listen testc testlisten
	cd ./face/bin && rm -f *.o testtasks testface testsrv teststorage client listen testc testlisten
