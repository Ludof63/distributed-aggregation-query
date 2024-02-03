#pragma once

#include "Api.h"
#include "CurlEasyPtr.h"

#define EXECUTABLE_NAME "WORKER"
// #define INFO
// #define ERRORS

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


class  Worker {
    //utility function to getDomain from url
    static std::string_view getDomain(std::string_view url);

    //utility function to compare pairs domaain, count (used to sort)
    static bool cmp(std::pair<std::string, int>& a, std::pair<std::string, int>& b);

    static int task_aggregate(const Task& task, const int& n_part, AzureBlobClient& input, AzureBlobClient& output);

    static int task_merge(const Task& task, AzureBlobClient& input, AzureBlobClient& output);

    //uploads task (a chunk) to a blob (similar name) in output
    static int task_upload(const Task& task, AzureBlobClient& output);

public:

    static int solve_task(const TaskApi& info, const std::string& account, const std::string& token);

    Worker() = delete;
};