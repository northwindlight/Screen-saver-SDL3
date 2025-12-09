#pragma once

#include <SDL3/SDL.h>

#include <thread>
#include <string>
#include <atomic>
#include <ctime>
#include <chrono>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <filesystem>

#include <ifaddrs.h>
#include <arpa/inet.h>

struct Temp {
    char type1[20];
    char type2[20];
    float temp1;
    float temp2;
};

class EventThread {
private:
    std::thread* worker = nullptr;
    std::atomic<bool> running = true;
    void run();
    int cur_time = 0;
    intptr_t cur_level = 0;
    uint8_t charge_status = 0;
    uint8_t sshd_status_ = 0;

public:
    const int event_nums = 7;
    std::atomic<bool> lightScreen = true;
    std::mutex mtx;
    std::condition_variable cv;
    unsigned event_code;
    std::atomic<bool> status = false;
    std::atomic<unsigned> light_turns = 0;
    EventThread();
    bool start();
    void stop();
    ~EventThread();
    
};