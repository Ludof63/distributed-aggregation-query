#pragma once

#include "AzureBlobClient.h"

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <string>

#define BUFFER_SIZE 300

enum class Operation { aggregate, merge, upload };

//originally created like this to support fstream for local shared states storing...lack of time
enum class Mode { blob };

using Task = std::string;
using SourceInfo = std::pair<Mode, std::string>;





struct TaskApi {
    Task task; //the  task payload
    Operation op; //the operation we want to execute
    int n_part; //number of subpartitions
    SourceInfo input; //where to read input
    SourceInfo output;  //where to write output

    //default constructor
    TaskApi();
    //constructor for base info struct for operation (no task) (aggregate)
    TaskApi(Operation op, int n_part, SourceInfo input, SourceInfo output);
    //constructor for merge
    TaskApi(Operation op, SourceInfo input, SourceInfo output);
    //constructor for upload
    TaskApi(Operation op, SourceInfo output);
    //constructor from a string (parsing)
    TaskApi(std::string s);
    //to send task info via socket
    std::string to_str() const;
};


//intialize common API references
const std::unordered_map<Operation, std::string> op_to_code{
    {Operation::aggregate, "a"},
    {Operation::merge, "m"},
    {Operation::upload, "u"}
};

const std::unordered_map<std::string, Operation> code_to_op{
    {op_to_code.at(Operation::aggregate), Operation::aggregate},
    {op_to_code.at(Operation::merge), Operation::merge},
    {op_to_code.at(Operation::upload), Operation::upload}
};

const std::unordered_map<Mode, std::string> mode_to_code{
    {Mode::blob, "b"}
};

const std::unordered_map<std::string, Mode> code_to_mode{
    {mode_to_code.at(Mode::blob), Mode::blob}
};
