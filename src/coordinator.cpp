#include "Api.h"
#include "Timer.h"
#include "Server.h"
#include "CurlEasyPtr.h"

#include <queue>
#include <poll.h>
#include <random>

#define EXECUTABLE_NAME "COORDINATOR"
#define INFO
#define ERRORS

//logging
#ifdef INFO
#define LOG(x) std::cout << EXECUTABLE_NAME << " >> " << x << std::endl;
#else 
#define LOG(x)
#endif 

//error logging
#ifdef ERRORS
#define EL(x) std::cerr << EXECUTABLE_NAME << " >> " << x << std::endl;
#else
#define EL(x);
#endif 

//keep intermediate states in azure storage after execution?
#define KEEP_INT_STATES false

bool add_worker(const int srv_sockfd, std::vector<pollfd>& workers) {
    int newfd;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen = sizeof(remoteaddr);

    if (newfd = accept(srv_sockfd, (struct sockaddr*)&remoteaddr, &addrlen); newfd == -1) {
        EL("Failed to accept new worker");
        return false;
    }

    LOG("New worker accepted " << newfd);

    workers.push_back(pollfd{ newfd, POLLIN, 0 });
    return true;
}

void remove_worker(size_t index, std::vector<pollfd>& workers) {
    LOG("Removing worker " << workers[index].fd);
    close(workers[index].fd);
    workers.erase(workers.begin() + index);
}

void remove_all_workers(std::vector<pollfd>& workers) {
    LOG("Removing all workers");
    for (size_t i = 1; i < workers.size(); i++) {
        remove_worker(i, workers);
    }
}

bool send_task(const int worker_fd, std::queue<Task>& tasks, std::unordered_map<int, std::string>& in_progress, TaskApi& base_info) {
    base_info.task = tasks.front();
    auto payload = base_info.to_str();
    if (send(worker_fd, payload.c_str(), payload.length(), 0) == -1) {
        EL("Failed to send task to " << worker_fd);
        return false;
    }
    else {
        tasks.pop();
        in_progress[worker_fd] = base_info.task;
        LOG("Task sent to " << worker_fd);
        return true;
    }
}



void get_aggregate_tasks(AzureBlobClient& cli, std::queue<Task>& tasks) {
    LOG("Getting aggregate tasks");
    auto blobs = cli.listBlobs();

    for (const auto& s : blobs) {
        tasks.push(s);
    }
}

void get_merge_tasks(const int n_subpart, std::queue<Task>& tasks) {
    LOG("Getting merge tasks");
    // Iterate over all files
    for (int i = 0; i < n_subpart; i++) {
        tasks.push(std::to_string(i));
    }
}


void get_upload_tasks(const std::string url, std::queue<Task>& tasks) {
    LOG("Getting upload tasks");
    auto curlSetup = CurlGlobalSetup();
    auto curl = CurlEasyPtr::easyInit();
    curl.setUrl(url);
    auto fileList = curl.performToStringStream();

    for (std::string s; std::getline(fileList, s, '\n');) {
        tasks.push(s);
    }
}



std::vector<std::pair<std::string, int>> finalMerge(AzureBlobClient& cli) {
    auto blobs = cli.listBlobs();
    std::unordered_map<std::string, int> domain_count;

    for (const auto& bl : blobs) {
        auto csvData = cli.downloadStringStream(bl);
        for (std::string row; std::getline(csvData, row, '\n');) {

            std::string domain = row.substr(row.find("\t") + 1, row.size());
            int count = atoi(row.substr(0, row.find("\t")).data());
            domain_count[domain] = count;
        }

    }

    std::vector<std::pair<std::string, int>> aux;
    for (const auto& kv : domain_count)
        aux.push_back(kv);

    LOG("Size of final merge: " << aux.size());

    std::sort(aux.begin(), aux.end(), [](std::pair<std::string, int>& a, std::pair<std::string, int>& b) {return a.second > b.second;});
    aux.resize(25);
    return aux;
}





/// Leader process that coordinates workers. Workers connect on the specified port
/// and the coordinator distributes the work of the CSV file list.
int main(int argc, char* argv[]) {
    if (argc != 7) {
        std::cerr << "Usage: " << argv[0] << " <listen port> <account name> <access token> <cont_name> <u to upload | c to compute> <<n_subpart if c>|<url if u>>" << std::endl;
        return 1;
    }

    static const std::string accountName{ argv[2] };
    static const std::string accountToken{ argv[3] };
    static const std::string cont_data{ argv[4] };

    const bool do_compute = (argv[5] == std::string("c"));
    int n_subpart;
    std::string url;

    if (do_compute) {
        n_subpart = std::atoi(argv[6]);
        if (n_subpart < 1) {
            std::cerr << "Number of subpartions cannot be less than 1" << std::endl;
            return 1;
        }
    }
    else
        url = std::string(argv[6]);

    Timer timer = Timer();
    timer.start();
    int64_t first_worker_delay;
    double mean_conn_time = 0.0;
    int count = 0;
    bool first_arrived = false;
    LOG("Started");


    std::unordered_map<Operation, TaskApi> op_to_info;
    std::queue<Operation> operations;
    std::queue<Task> tasks;
    Operation actual_op;

    //used to test ops
    bool do_upload = !do_compute;

    bool do_aggregate = do_compute;
    bool do_merge = do_compute;
    bool do_final_merge = do_compute;

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(1, 10000);

    std::string aggr_container = "cbdp-5-aggr" + std::to_string(dist(rng));
    std::string merge_container = "cbdp-5-merge" + std::to_string(dist(rng));

    LOG("Using " << aggr_container << " as aggregation container");
    LOG("Using " << merge_container << " as merge container");


    op_to_info[Operation::aggregate] = TaskApi(Operation::aggregate, n_subpart, { Mode::blob, cont_data }, { Mode::blob, aggr_container });
    op_to_info[Operation::merge] = TaskApi(Operation::merge, { Mode::blob, aggr_container }, { Mode::blob, merge_container });
    op_to_info[Operation::upload] = TaskApi(Operation::upload, { Mode::blob, cont_data });

    //which task I have to do
    if (do_upload)
        operations.push(Operation::upload);
    if (do_aggregate)
        operations.push(Operation::aggregate);
    if (do_merge)
        operations.push(Operation::merge);

    //UTILITY FUNCTIONS
    auto get_task_for_op = [&](const Operation& op) {
        if (op == Operation::aggregate) {
            auto dataContainer = AzureBlobClient(accountName, accountToken, cont_data);
            get_aggregate_tasks(dataContainer, tasks);
            LOG("Aggregate tasks ready");
            AzureBlobClient(accountName, accountToken).createContainer(aggr_container);
            return;
        }
        if (op == Operation::merge) {
            AzureBlobClient(accountName, accountToken).createContainer(merge_container);
            get_merge_tasks(n_subpart, tasks);
            return;
        }
        if (op == Operation::upload) {
            AzureBlobClient(accountName, accountToken).createContainer(cont_data);
            get_upload_tasks(url, tasks);
            return;
        }
        };

    auto get_new_op = [&]() {
        actual_op = operations.front();
        operations.pop();
        //initialize tasks for operation
        get_task_for_op(actual_op);
        };



    //if I need to execute distributed operations (useful when testing only final merge)
    if (!operations.empty()) {
        LOG("I need to execute distributed operations...preparing server");


        int srv_sockfd = Server::startSever(argv[1]);
        if (srv_sockfd == -1) {
            EL("failed to listen on socket")
                return 1;
        }
        LOG("Listening OK");

        //PREPARING WORKERS DATA STRUCTURES
        std::unordered_map<int, Task> in_progress;
        auto workers = std::vector<pollfd>();
        workers.push_back(pollfd{ srv_sockfd, POLLIN,0 });


        //first operation
        get_new_op();
        LOG("First operation " << op_to_code.at(actual_op));
        //WORK
        while (true) {
            //wait for something to happen
            if (poll(workers.data(), workers.size(), -1) == -1) {
                perror("Coordinator: poll failed");
                break;
            }

            for (size_t i = 0; i < workers.size(); i++) {
                //check for socket responsible for the event
                if (workers[i].revents & POLLIN) {
                    if (workers[i].fd == srv_sockfd) {
                        LOG("New worker wants to be accepted");

                        if (!add_worker(srv_sockfd, workers)) {
                            EL("New worker not added");
                        }
                        else {
                            //just time prints
                            if (!first_arrived) {
                                first_worker_delay = timer.check();
                                first_arrived = true;
                                LOG("Time for first worker: " << first_worker_delay << " ms");
                            }
                            else {
                                auto interval = timer.check();
                                mean_conn_time = (double)(((count * mean_conn_time) + (double)interval) / (count + 1));
                                count++;
                                LOG("Time from last worker: " << interval << " ms");
                            }

                            //the new worker is the last
                            if (!send_task(workers.back().fd, tasks, in_progress, op_to_info[actual_op])) {
                                EL("Failed to send task...killing new worker");
                                remove_worker(workers.size() - 1, workers);
                            }
                        }
                    }
                    else {
                        LOG("We have received something from " << workers[i].fd);
                        std::array<char, BUFFER_SIZE> buffer{};
                        int nbytes = (int)recv(workers[i].fd, &buffer, buffer.size(), 0);
                        if (nbytes <= 0) {
                            EL("A socket problem...worker is probably dead");
                            if (in_progress.contains(workers[i].fd))
                                tasks.push(in_progress[workers[i].fd]);

                            in_progress.erase(workers[i].fd);
                            remove_worker(i, workers);
                        }
                        else {
                            LOG("Valid data received");

                            //aggregating result
                            std::string answer(buffer.data(), nbytes);
                            int ans = atoi(answer.c_str());

                            if (ans < 0) {
                                EL("Unexpected behaviour: " << workers[i].fd << "sent non zero reply")
                            }

                            LOG("Remaining tasks :" << tasks.size());

                            in_progress.erase(workers[i].fd);

                            if (!tasks.empty()) {
                                if (!send_task(workers[i].fd, tasks, in_progress, op_to_info[actual_op])) {
                                    EL("Failed to send task...killing the worker");
                                    remove_worker(i, workers);
                                }
                            }
                        }
                    }
                }
            }

            if (tasks.empty() && in_progress.size() == 0) {
                if (actual_op == Operation::aggregate)
                    LOG("Aggregate phase is over");

                if (actual_op == Operation::merge)
                    LOG("Merge phase is over");

                if (operations.empty()) {
                    LOG("All operations are done...quitting");
                    break;
                }

                //getting new operation
                get_new_op();
                LOG("Starting new operation " << op_to_code.at(actual_op));

                for (size_t i = 1; i < workers.size(); i++) {
                    if (tasks.empty())
                        break;

                    if (!send_task(workers[i].fd, tasks, in_progress, op_to_info[actual_op])) {
                        EL("Failed to send task...killing the worker");
                        remove_worker(i, workers);
                    }
                }

            }
        }

        LOG("Distributed operations done...releasing resources");
        remove_all_workers(workers);
        close(srv_sockfd);
    }


    std::vector<std::pair<std::string, int>> res;
    if (do_final_merge) {
        LOG("Starting final merge");
        auto merge_cont = AzureBlobClient(accountName, accountToken, merge_container);
        res = finalMerge(merge_cont);
    }

    LOG("Work done");
    auto elapsed_time = timer.stop();


    std::cout << "Time for first worker to arrive : " << first_worker_delay << " ms" << std::endl;
    std::cout << "Mean time for workers to arrive : " << mean_conn_time << " ms" << std::endl;
    std::cout << "Exectution time from first worker: " << (elapsed_time - first_worker_delay) << " ms" << std::endl;
    std::cout << "Execution time : " << elapsed_time << " ms" << std::endl;

    if (do_final_merge) {
        std::cout << "Final result:" << std::endl;

        for (const auto& [dom, count] : res)
            std::cout << dom << " " << count << std::endl;
    }



    if (!KEEP_INT_STATES) {
        AzureBlobClient(accountName, accountToken, aggr_container).deleteContainer();
        AzureBlobClient(accountName, accountToken, merge_container).deleteContainer();
    }

    return 0;
}
