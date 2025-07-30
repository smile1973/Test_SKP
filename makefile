CC:= g++
OBJS:= SKProxy.o FprintfStderrLog.o SKGlobal.o pevents.o SKClient.o SKClients.o SKThread.o SKHeartbeat.o SKServerSocket.o SKConn.o SKLogger.o SKLogin.o SKSession.o SKServers.o SKTSOrder.o SKTSAlter.o SKTFOrder.o SKTFAlter.o SKOFOrder.o SKOSOrder.o SKOSBOrder.o HttpRequest.o json.o Utility.o SKPreTFAlter.o SKPreTSAlter.o SKPreOFOrder.o SKPreOSOrder.o
CFLAGS:= -std=c++11 -c
LFLAGS:= -lpthread -lrt -lssl -lcrypto -L/usr/local/curl/lib -lcurl
OUTPUT_PATH:= ./build
FILENAME:=

ifeq ($(RELEASE),0)
# debug
    CFLAGS += -g
else
# release
    CFLAGS += -O3
endif

ifeq ($(MODE),0)
# cpu
	CFLAGS += -DCPUMODE
    FILENAME += ${OUTPUT_PATH}/sk_proxy_cpu
else ifeq ($(MODE),1)
# safe
    FILENAME += ${OUTPUT_PATH}/sk_proxy
endif

.PHONY: clean all
all: SKProxy

clean:
	rm -f ${OUTPUT_PATH}/sk_proxy*
	rm -f ${OBJS}

SKProxy: ${OBJS}
	mkdir -p ${OUTPUT_PATH}
	${CC} `pkg-config --cflags --libs glib-2.0` ${LFLAGS} -o ${FILENAME} ${OBJS}

SKProxy.o: SKProxy.cpp
	${CC} ${CFLAGS} `pkg-config --cflags --libs glib-2.0` SKProxy.cpp

FprintfStderrLog.o: FprintfStderrLog.cpp
	${CC} ${CFLAGS} FprintfStderrLog.cpp

SKGlobal.o: SKGlobal.cpp
	${CC} ${CFLAGS} SKGlobal.cpp

pevents.o: pevents.cpp
	${CC} ${CFLAGS} -DWFMO pevents.cpp

SKClient.o: SKClient.cpp
	${CC} ${CFLAGS} SKClient.cpp

SKClients.o: SKClients.cpp
	${CC} ${CFLAGS} SKClients.cpp

SKThread.o: SKThread.cpp
	${CC} ${CFLAGS} SKThread.cpp

SKHeartbeat.o: SKHeartbeat.cpp
	${CC} ${CFLAGS} -DWFMO SKHeartbeat.cpp

SKServerSocket.o: ./SKNet/SKServerSocket.cpp
	${CC} ${CFLAGS} ./SKNet/SKServerSocket.cpp

SKConn.o: ./SKGWOther/SKConn.cpp
	${CC} ${CFLAGS} ./SKGWOther/SKConn.cpp

SKLogger.o: ./SKGWLogger/SKLogger.cpp
	${CC} ${CFLAGS} ./SKGWLogger/SKLogger.cpp

SKLogin.o: ./SKGWOther/SKLogin.cpp
	${CC} ${CFLAGS} ./SKGWOther/SKLogin.cpp

SKSession.o: ./SKGWOther/SKSession.cpp
	${CC} ${CFLAGS} ./SKGWOther/SKSession.cpp

SKServers.o: SKServers.cpp
	${CC} ${CFLAGS} SKServers.cpp
	
SKTSOrder.o: ./SKGWOrder/SKTSOrder.cpp
	${CC} ${CFLAGS} ./SKGWOrder/SKTSOrder.cpp

SKTSAlter.o: ./SKGWOrder/SKTSAlter.cpp
	${CC} ${CFLAGS} ./SKGWOrder/SKTSAlter.cpp

SKTFOrder.o: ./SKGWOrder/SKTFOrder.cpp
	${CC} ${CFLAGS} ./SKGWOrder/SKTFOrder.cpp

SKTFAlter.o: ./SKGWOrder/SKTFAlter.cpp
	${CC} ${CFLAGS} ./SKGWOrder/SKTFAlter.cpp

SKOFOrder.o: ./SKGWOrder/SKOFOrder.cpp
	${CC} ${CFLAGS} ./SKGWOrder/SKOFOrder.cpp

SKOSOrder.o: ./SKGWOrder/SKOSOrder.cpp
	${CC} ${CFLAGS} ./SKGWOrder/SKOSOrder.cpp

SKOSBOrder.o: ./SKGWOrder/SKOSBOrder.cpp
	${CC} ${CFLAGS} ./SKGWOrder/SKOSBOrder.cpp

SKPreTSAlter.o: ./SKGWOrder/SKPreTSAlter.cpp
	${CC} ${CFLAGS} ./SKGWOrder/SKPreTSAlter.cpp

SKPreTFAlter.o: ./SKGWOrder/SKPreTFAlter.cpp
	${CC} ${CFLAGS} ./SKGWOrder/SKPreTFAlter.cpp

SKPreOFOrder.o: ./SKGWOrder/SKPreOFOrder.cpp
	${CC} ${CFLAGS} ./SKGWOrder/SKPreOFOrder.cpp

SKPreOSOrder.o: ./SKGWOrder/SKPreOSOrder.cpp
	${CC} ${CFLAGS} ./SKGWOrder/SKPreOSOrder.cpp

HttpRequest.o: HttpRequest.cpp
	${CC} ${CFLAGS} -I/usr/local/curl/include HttpRequest.cpp

json.o: json.cpp
	${CC} ${CFLAGS} json.cpp

Utility.o: Utility.cpp
	${CC} ${CFLAGS} Utility.cpp
