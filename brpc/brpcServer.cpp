#include <gflags/gflags.h>
#include <butil/logging.h>
#include <brpc/server.h>
#include "PressureTest.pb.h"

DEFINE_bool(echo_attachment, false, "Echo attachment as well");
DEFINE_int32(port, 13012, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state "
             "(waiting for client to close connection before server stops)");

class PressureServiceImpl: public PressureTest::PressureService
{
public:
    virtual ~PressureServiceImpl() {};
	virtual void pressure(::google::protobuf::RpcController* controller,
							const ::PressureTest::PressureRequest* request,
							::PressureTest::PressureResponse* response,
							::google::protobuf::Closure* done)
	{
		brpc::ClosureGuard done_guard(done);

		int count = request->int_();
		count += request->map().int_();

		for (int i = 0; i < request->array_size(); i++)
			count += request->array(i);

		response->set_count(count);
	}
};

int main(int argc, char* argv[])
{
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

    brpc::Server server;
    PressureServiceImpl pressure_test_service_impl;

    if (server.AddService(&pressure_test_service_impl, brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
    {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    brpc::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    if (server.Start(FLAGS_port, &options) != 0)
    {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    server.RunUntilAskedToQuit();
    return 0;
}
