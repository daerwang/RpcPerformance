
#include <iostream>
#include <gflags/gflags.h>
#include <butil/logging.h>
#include <butil/time.h>
#include <brpc/channel.h>
#include "PressureTest.pb.h"

DEFINE_string(protocol, "baidu_std", "Protocol type. Defined in src/brpc/options.proto");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(server, "0.0.0.0:13012", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 5000, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 1, "Max retries(not including the first RPC)"); 
DEFINE_string(http_content_type, "application/json", "Content type of http request");

DEFINE_int32(connection_count, 1000, "Clients count");
DEFINE_int32(total_qps, 50000, "Total QPS for all clients");

using namespace std;

int64_t exact_real_usec()
{
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	return (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
}

void buildQuest(PressureTest::PressureRequest &request)
{
	request.set_message("quest");
	request.set_int_(2);
	request.set_double_(3.3);
	request.set_bool_(true);

	request.add_array(32);
	request.add_array(4);

	PressureTest::PressureRequest_Map* Map = request.mutable_map();

	Map.set_message("first_map");
	request.set_bool_(true);
	Map.set_int_(5);
	Map.set_double_(5.7);
	Map.set_desc("中文");
}

class Tester;

class OnRPCDone: public google::protobuf::Closure
{
public:
	void Run();

	PressureTest::PressureResponse response;
	brpc::Controller cntl;
	int64_t send_time;
	Tester* tester;
};

class Tester
{
	std::string _endpoint;
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
	Tester(const std::string& endpoint, int thread_num, int qps): _endpoint(endpoint), _thread_num(thread_num), _qps(qps),
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

	brpc::Channel channel;
    
	brpc::ChannelOptions options;
	options.protocol = FLAGS_protocol;
	options.connection_type = FLAGS_connection_type;
	options.timeout_ms = FLAGS_timeout_ms;
	options.max_retry = FLAGS_max_retry;
	if (channel.Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(), &options) != 0)
	{
		LOG(ERROR) << "Fail to initialize channel";
		return;
	}

	PressureTest::PressureService_Stub stub(&channel);

	int log_id = 0;
	while (true)
	{
		int64_t begin_time = exact_real_usec();

		OnRPCDone* done = new OnRPCDone;
		PressureTest::PressureRequest request;

		buildQuest(request);

		done->cntl.set_log_id(log_id ++);
		done->cntl.set_timeout_ms(FLAGS_timeout_ms);
		done->tester = this;
		done->send_time = exact_real_usec();
		
        stub.pressure(&(done->cntl), &request, &(done->response), done);
        _send++;

        int64_t sent_time = exact_real_usec();
		int64_t real_usec = usec - (sent_time - begin_time);
		if (real_usec > 0)
			usleep(real_usec);
	}
}

void OnRPCDone::Run()
{
	int64_t recv_time = exact_real_usec();
	int64_t diff = recv_time - send_time;

	std::unique_ptr<OnRPCDone> self_guard(this);

	if (cntl->Failed())
	{
		tester->incRecvError();
		cout<<"RPC response error."<<endl;
	}
	
	tester->incRecv();
	tester->addTimecost(diff);
}

int main(int argc, char* argv[])
{
	GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
	Tester tester(FLAGS_server, FLAGS_connection_count, FLAGS_total_qps);

	tester.launch();
	tester.showStatistics();

	return 0;
}
