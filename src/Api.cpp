#include "Api.h"


TaskApi::TaskApi() {

}

TaskApi::TaskApi(Operation op, int n_part, SourceInfo input, SourceInfo output) :
    task(""),
    op(op),
    n_part(n_part),
    input(input),
    output(output) {
}

TaskApi::TaskApi(Operation op, SourceInfo input, SourceInfo output) :TaskApi(op, 0, input, output) {}

TaskApi::TaskApi(Operation op, SourceInfo output) :TaskApi(op, { Mode::blob , "x" }, output) {}

TaskApi::TaskApi(std::string s) {
    std::string tmp;
    std::stringstream ss(s);
    std::vector<std::string> parts;

    while (getline(ss, tmp, ',')) {
        parts.push_back(tmp);
    }

    if (parts.size() != 7)
        throw std::runtime_error("Expects 7 parts");

    if (!code_to_op.contains(parts[1]))
        throw std::runtime_error("Operation not known");

    if (!code_to_mode.contains(parts[3]))
        throw std::runtime_error("Mode of input not known");

    if (!code_to_mode.contains(parts[5]))
        throw std::runtime_error("Mode of output not known");

    task = parts[0];
    op = code_to_op.at(parts[1]);
    n_part = atoi(parts[2].data());
    input = { code_to_mode.at(parts[3]), parts[4] };
    output = { code_to_mode.at(parts[5]), parts[6] };
}

std::string TaskApi::to_str() const {
    return std::string{
        task +                           // 0 -> task
        "," + op_to_code.at(op) +              // 1 -> operation
        "," + std::to_string(n_part) +         // 2 -> n_part (if not needed, 0)
        "," + mode_to_code.at(input.first) +   // 3 -> mode of input
        "," + input.second +                   // 4  -> input (str)      
        "," + mode_to_code.at(output.first) +  // 5 -> mode of ouput
        "," + output.second };                 // 6 -> output (str)

}