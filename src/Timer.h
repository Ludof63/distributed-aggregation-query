#pragma once

#include <chrono>
#include <stdexcept>

class Timer {
public:
    //starts the timer
    void start();
    //get elepsed time from last check, without stopping clock
    int64_t check();
    //get elepsed time, stopping clock
    int64_t stop();
    //check if the timer is started
    bool is_started() { return started; }

private:
    bool started = false;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point last_check;
};