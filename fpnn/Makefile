EXES_SERVER = fpnnServer
EXES_CLIENT = fpnnClient
EXES_CLIENT2 = fpnnOneClient

FPNN_DIR = ../../infra-fpnn
DEPLOYMENT_DIR = ../bin
CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -I$(FPNN_DIR)/core -I$(FPNN_DIR)/proto -I$(FPNN_DIR)/base -I$(FPNN_DIR)/proto/msgpack -I$(FPNN_DIR)/proto/rapidjson
LIBS += -L$(FPNN_DIR)/core -L$(FPNN_DIR)/proto -L$(FPNN_DIR)/base -lfpnn

OBJS_SERVER = fpnnServer.o
OBJS_CLIENT = fpnnClient.o
OBJS_CLIENT2 = fpnnOneClient.o

all: $(EXES_SERVER) $(EXES_CLIENT) $(EXES_CLIENT2)

deploy:
	-mkdir -p $(DEPLOYMENT_DIR)
	cp -rf $(EXES_SERVER) $(DEPLOYMENT_DIR)
	cp -rf $(EXES_CLIENT) $(DEPLOYMENT_DIR)
	cp -rf $(EXES_CLIENT2) $(DEPLOYMENT_DIR)
	cp -rf $(EXES_SERVER).conf $(DEPLOYMENT_DIR)

clean:
	$(RM) *.o $(EXES_SERVER) $(EXES_CLIENT) $(EXES_CLIENT2)
include $(FPNN_DIR)/def.mk