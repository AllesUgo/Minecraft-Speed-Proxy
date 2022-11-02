CXX=gcc
LINKLIBS=-lpthread -lm -ldl
OUTPUT=minecraftspeedproxy

CARGVS=-c -DLOG_USE_COLOR -DVERSION=\"v$(VERSION)\"
ALLARGV=-O3 -std=gnu99
DEBUG= 
minecraftspeedproxy:CheckLogin.o cJSON.o client.o log.o main.o mempool.o online.o remote.o sending.o unpack.o varint.o websocket.o
	$(CXX) CheckLogin.o cJSON.o client.o log.o main.o mempool.o online.o remote.o sending.o unpack.o varint.o websocket.o $(DEBUG) $(LINKLIBS) $(DEBUG) $(ALLARGV) -o $(OUTPUT)
CheckLogin.o:CheckLogin.h CheckLogin.c
	$(CXX) CheckLogin.c $(ALLARGV) $(DEBUG) $(CARGVS) -o CheckLogin.o
cJSON.o:cJSON.c cJSON.h
	$(CXX) cJSON.c $(ALLARGV) $(DEBUG) $(CARGVS) -o cJSON.o
client.o:client.c log.h websocket.h RemoteClient.h unpack.h cJSON.h varint.h mempool.h
	$(CXX) client.c $(ALLARGV) $(DEBUG) $(CARGVS) -o client.o
log.o:log.c log.h
	$(CXX) log.c $(ALLARGV) $(DEBUG) $(CARGVS) -o log.o
main.o:main.c websocket.h cJSON.h log.h
	$(CXX) main.c $(ALLARGV) $(DEBUG) $(CARGVS) -o main.o
mempool.o:mempool.c mempool.h
	$(CXX) mempool.c $(ALLARGV) $(DEBUG) $(CARGVS) -o mempool.o
online.o:online.c cJSON.h CheckLogin.h
	$(CXX) online.c $(ALLARGV) $(DEBUG) $(CARGVS) -o online.o
remote.o:remote.c log.h RemoteClient.h websocket.h mempool.h
	$(CXX) remote.c $(ALLARGV) $(DEBUG) $(CARGVS) -o remote.o
sending.o:sending.c RemoteClient.h websocket.h mempool.h
	$(CXX) sending.c $(ALLARGV) $(DEBUG) $(CARGVS) -o sending.o
unpack.o:unpack.c websocket.h varint.h
	$(CXX) unpack.c $(ALLARGV) $(DEBUG) $(CARGVS) -o unpack.o
varint.o:varint.c varint.h
	$(CXX) varint.c $(ALLARGV) $(DEBUG) $(CARGVS) -o varint.o
websocket.o:websocket.c websocket.h
	$(CXX) websocket.c $(ALLARGV) $(DEBUG) $(CARGVS) -o websocket.o

clean:
	rm ./*.o
install:minecraftspeedproxy
	cp $(OUTPUT) /usr/bin/
	@echo "输入$(OUTPUT)以运行程序"
uninstall:/usr/bin/$(OUTPUT)
	rm /usr/bin/$(OUTPUT)
