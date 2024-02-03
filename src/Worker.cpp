#include "Worker.h"


#define EXECUTABLE_NAME "WORKER"

//UTILITIES------------------------------------------------------
std::string_view Worker::getDomain(std::string_view url) {
    using namespace std::literals;
    auto pos = url.find("://"sv);
    if (pos != std::string::npos) {
        auto afterProtocol = std::string_view(url).substr(pos + 3);
        auto endDomain = afterProtocol.find('/');
        return afterProtocol.substr(0, endDomain);
    }
    return url;
}

bool Worker::cmp(std::pair<std::string, int>& a,
    std::pair<std::string, int>& b) {
    return a.second > b.second;
}



//TASKS -------------------------------------------------------------

int Worker::task_aggregate(const Task& task, const int& n_part, AzureBlobClient& input, AzureBlobClient& output) {
    LOG("Starting aggregation on: " << task);


    auto csvData = input.downloadStringStream(task);
    std::unordered_map<std::string, int> domain_count;

    for (std::string row; std::getline(csvData, row, '\n');) {
        std::string url = row.substr(row.find("\t") + 1, row.size());
        auto domain = std::string{ getDomain(url) };

        if (domain_count.contains(domain))
            domain_count[domain]++;
        else
            domain_count[domain] = 1;
    }

    LOG("Aggregate count done, different domains= " << domain_count.size());

    //producing subpartitions stringstream
    std::unordered_map <int, std::stringstream> subparts;
    for (const auto& kv : domain_count) {
        int n = (int)(std::hash<std::string>{}(kv.first) % n_part);
        subparts[n] << kv.second << "\t" << kv.first << std::endl;
    }

    LOG("Aggregate different subpartions: " << subparts.size());

    for (auto& kv : subparts) {
        std::string part_name = std::to_string(kv.first) + "_" + task;
        output.uploadStringStream(part_name, kv.second);
    }

    return 0;
}



int Worker::task_merge(const Task& task, AzureBlobClient& input, AzureBlobClient& output) {
    LOG("Merge on: " << task);

    auto blobs = input.listBlobs();
    std::unordered_map<std::string, int> domain_count;

    for (const auto& bl : blobs) {
        std::string bl_idx = bl.substr(0, bl.find("_"));
        if (bl_idx == task) {
            //I consider this blob in the merge
            auto csvData = input.downloadStringStream(bl);
            for (std::string row; std::getline(csvData, row, '\n');) {

                std::string domain = row.substr(row.find("\t") + 1, row.size());
                int count = atoi(row.substr(0, row.find("\t")).data());

                if (domain_count.contains(domain))
                    domain_count[domain] += count;
                else
                    domain_count[domain] = count;
            }
        }
    }

    std::vector<std::pair<std::string, int>> aux;
    for (const auto& kv : domain_count)
        aux.push_back(kv);

    LOG("Merge: size of mrg of " << task << " : " << aux.size());

    std::sort(aux.begin(), aux.end(), cmp);
    std::stringstream res;
    for (size_t i = 0; i < aux.size(); i++) {
        //just return the first 25
        if (i >= 25 && aux[i] != aux[i - 1])
            break;

        res << aux[i].second << "\t" << aux[i].first << std::endl;
    }

    output.uploadStringStream(task + "_top25", res);

    return 0;
}

int Worker::task_upload(const Task& task, AzureBlobClient& output) {
    auto curl = CurlEasyPtr::easyInit();

    std::string blob_name{ task.substr(62, 13) };
    curl.setUrl(task);
    auto csvData = curl.performToStringStream();
    auto dataStream = std::stringstream(std::move(csvData));
    output.uploadStringStream(blob_name, dataStream);

    return 0;
}


int Worker::solve_task(const TaskApi& info, const std::string& account, const std::string& token) {
    auto in_cli = AzureBlobClient(account, token, info.input.second);
    auto out_cli = AzureBlobClient(account, token, info.output.second);
    switch (info.op) {
    case Operation::aggregate:
        return task_aggregate(info.task, info.n_part, in_cli, out_cli);
    case Operation::merge:
        return task_merge(info.task, in_cli, out_cli);
    case Operation::upload:
        return task_upload(info.task, out_cli);
    }
    return -1;
}