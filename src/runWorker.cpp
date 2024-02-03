#include "Client.h"
#include "Worker.h"

/// Worker process that receives a list of URLs and reports the result
int main(int argc, char* argv[]) {
    LOG(argc);
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <host> <port> <account name> <access token>" << std::endl;
        return 1;
    }

    static const std::string accountName{ argv[3] };
    static const std::string accountToken{ argv[4] };

    LOG("Started with " << argv[1] << " " << argv[2]);
    auto client = Client(argv[1], argv[2]);
    LOG("Connected to coordinator");


    while (true) {
        std::string payload;

        //receive until error
        if (!client.receive(payload, BUFFER_SIZE)) {
            LOG("Received error omn socket...quitting");
            break;
        }

        TaskApi info;
        LOG("Received task: " << payload);
        try {
            info = TaskApi(payload);
        }
        catch (const std::exception& e) {
            std::cerr << "Error during payload parsing :" << e.what() << std::endl;
            return 1;
        }

        int res;
        LOG("operation to do " << op_to_code.at(info.op));
        try {
            res = Worker::solve_task(info, accountName, accountToken);
        }
        catch (const std::exception& e) {
            EL("Error solving task :" << e.what());
            return 1;
        }

        std::cout << "TAKS_DONE_" << op_to_code.at(info.op) << " " << payload << std::endl;


        if (!client.send_message(std::to_string(res))) {
            EL("Error sending msg...killing myself");
            return 1;
        }
    }

    LOG("terminating");
    return 0;
}
