#include <iostream>
#include "TCPEpollServer.h"
#include "IQuestProcessor.h"
#include "Setting.h"

using namespace fpnn;

class PressureQuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(PressureQuestProcessor)

public:
	FPAnswerPtr pressure(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		int count = args->wantInt("int");
		std::vector<int> array = args->want("array", std::vector<int>());
		OBJECT obj = args->wantObject("map");
		
		FPReader reader(obj);
		count += reader.wantInt("int");

		for (int v: array)
			count += v;

		return FPAWriter(1, quest)("count", count);
	}

	QuestProcessorClassBasicPublicFuncs
};

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cout<<"Usage: "<<argv[0]<<" config"<<std::endl;
		return 0;
	}
	if(!Setting::load(argv[1])){
		std::cout<<"Config file error:"<< argv[1]<<std::endl;
		return 1;
	}

	ServerPtr server = TCPEpollServer::create();
	server->setQuestProcessor(std::make_shared<PressureQuestProcessor>());
	if (server->startup())
		server->run();

	return 0;
}
