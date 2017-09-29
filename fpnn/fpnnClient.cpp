#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include "TCPClient.h"

using namespace std;
using namespace fpnn;

FPQuestPtr buildQuest()
{
    FPQWriter qw(6, "pressure");
    qw.param("message", "quest");
    qw.param("int", 2); 
    qw.param("double", 3.3);
    qw.param("boolean", true);

    qw.paramArray("array", 2);
    qw.param(32);
    qw.param(4);

    qw.paramMap("map", 5);
    qw.param("message", "first_map");
    qw.param("bool", true);
    qw.param("int", 5);
    qw.param("double", 5.7);
    qw.param("desc", "中文");

	return qw.take();
}

class Tester;

class PressureCallback: public AnswerCallback
{
public:
	virtual void onAnswer(FPAnswerPtr);
	virtual void onException(FPAnswerPtr answer, int errorCode);

	int64_t send_time;
	Tester* tester;
};

class Tester
{
	std::string _ip;
	int _port;
	int _thread_num;
	int _qps;

	std::atomic<int64_t> _send;
	std::atomic<int64_t> _recv;
	std::atomic<int64_t> _sendError;
	std::atomic<int64_t> _recvError;
	std::atomic<int64_t> _timecost;

	std::vector<std::thread> _threads;

	void test_worker(int qps);

public:
	Tester(const char* ip, int port, int thread_num, int qps): _ip(ip), _port(port), _thread_num(thread_num), _qps(qps),
		_send(0), _recv(0), _sendError(0), _recvError(0), _timecost(0)
	{
	}

	~Tester()
	{
		stop();
	}

	inline void incRecv() { _recv++; }
	inline void incRecvError() { _recvError++; }
	inline void addTimecost( int64_t cost) { _timecost.fetch_add(cost); }

	void launch()
	{
		int pqps = _qps / _thread_num;
		if (pqps == 0)
			pqps = 1;
		int remain = _qps - pqps * _thread_num;

		for(int i = 0 ; i < _thread_num; i++)
			_threads.push_back(std::thread(&Tester::test_worker, this, pqps));

		if (remain > 0)
			_threads.push_back(std::thread(&Tester::test_worker, this, remain));
	}

	void stop()
	{
		for(size_t i = 0; i < _threads.size(); i++)
			_threads[i].join();
	}

	void showStatistics()
	{
		const int sleepSeconds = 3;

		int64_t send = _send;
		int64_t recv = _recv;
		int64_t sendError = _sendError;
		int64_t recvError = _recvError;
		int64_t timecost = _timecost;


		while (true)
		{
			int64_t start = exact_real_usec();

			sleep(sleepSeconds);

			int64_t s = _send;
			int64_t r = _recv;
			int64_t se = _sendError;
			int64_t re = _recvError;
			int64_t tc = _timecost;

			int64_t ent = exact_real_usec();

			int64_t ds = s - send;
			int64_t dr = r - recv;
			int64_t dse = se - sendError;
			int64_t dre = re - recvError;
			int64_t dtc = tc - timecost;

			send = s;
			recv = r;
			sendError = se;
			recvError = re;
			timecost = tc;

			int64_t real_time = ent - start;

			ds = ds * 1000 * 1000 / real_time;
			dr = dr * 1000 * 1000 / real_time;
			//dse = dse * 1000 * 1000 / real_time;
			//dre = dre * 1000 * 1000 / real_time;
			if (dr)
				dtc = dtc / dr;

			cout<<"time interval: "<<(real_time / 1000.0)<<" ms, send error: "<<dse<<", recv error: "<<dre<<endl;
			cout<<"[QPS] send: "<<ds<<", recv: "<<dr<<", per quest time cost: "<<dtc<<" usec"<<endl;
		}
	}
};

void Tester::test_worker(int qps)
{
	int usec = 1000 * 1000 / qps;
	Tester* ins = this;

	cout<<"-- qps: "<<qps<<", interval usec: "<<usec<<endl;

	TCPClientPtr client = TCPClient::createClient(_ip, _port);

	while (true)
	{
		int64_t begin_time = exact_real_usec();

		PressureCallback* callback = new PressureCallback;
		FPQuestPtr quest = buildQuest();

		callback->tester = this;
		callback->send_time = exact_real_usec();

		client->sendQuest(quest, callback);
		_send++;

		int64_t sent_time = exact_real_usec();
		int64_t real_usec = usec - (sent_time - begin_time);
		if (real_usec > 0)
			usleep(real_usec);
	}
}

void PressureCallback::onAnswer(FPAnswerPtr answer)
{
	int64_t recv_time = exact_real_usec();
	int64_t diff = recv_time - send_time;
	
	tester->incRecv();
	tester->addTimecost(diff);
}

void PressureCallback::onException(FPAnswerPtr answer, int errorCode)
{
	tester->incRecvError();
	if (errorCode == FPNN_EC_CORE_TIMEOUT)
		cout<<"Timeouted occurred when recving."<<endl;
	else
		cout<<"error occurred when recving."<<endl;
}

int main(int argc, char* argv[])
{
	if (argc < 5 || argc > 6)
	{
		cout<<"Usage: "<<argv[0]<<" ip port connections total-qps [client_work_thread]"<<endl;
		return 0;
	}

	if (argc == 6)
	{
		int count = atoi(argv[5]);
		ClientEngine::configAnswerCallbackThreadPool(count, 1, count, count);
	}

	Tester tester(argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));

	tester.launch();
	tester.showStatistics();

	return 0;
}
