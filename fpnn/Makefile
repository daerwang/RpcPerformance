EXES_SERVER = fpnnServer
EXES_CLIENT = fpnnClient

FPNN_DIR = ../../infra-fpnn
DEPLOYMENT_DIR = ../bin
CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -I$(FPNN_DIR)/core -I$(FPNN_DIR)/proto -I$(FPNN_DIR)/base -I$(FPNN_DIR)/proto/msgpack -I$(FPNN_DIR)/proto/rapidjson
LIBS += -L$(FPNN_DIR)/core -L$(FPNN_DIR)/proto -L$(FPNN_DIR)/base -lfpnn

OBJS_SERVER = fpnnServer.o
OBJS_CLIENT = fpnnClient.o

all: $(EXES_SERVER) $(EXES_CLIENT)

deploy:
	-mkdir -p $(DEPLOYMENT_DIR)
	cp -rf $(EXES_SERVER) $(DEPLOYMENT_DIR)
	cp -rf $(EXES_CLIENT) $(DEPLOYMENT_DIR)
	cp -rf $(EXES_SERVER).conf $(DEPLOYMENT_DIR)

clean:
	$(RM) *.o $(EXES_SERVER) $(EXES_CLIENT)
include $(FPNN_DIR)/def.mk