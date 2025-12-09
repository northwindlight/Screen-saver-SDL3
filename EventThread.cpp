#include "EventThread.h"

void getIp(std::vector<std::string>& ifname) {
    struct ifaddrs *ifaddr, *ifa;
    char ip[60] = { 0 };
    ifname.clear();
    if (getifaddrs(&ifaddr) == -1) {
        ifname.push_back("network error");
        return;
    }
    
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        //if (!strcmp(ifa->ifa_name, "lo")) 
            //continue;
            // 如果这个条目没有地址信息，跳过
        if (ifa->ifa_addr == nullptr )
            continue;
        
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
            //if(strcmp(ip, "127.0.0.1")){
                ifname.push_back(std::string(ifa->ifa_name) + ": " + std::string(ip));
                SDL_Log("Interface: %s\tAddress: %s\n", ifa->ifa_name, ip);
            //}
        }
    }
    freeifaddrs(ifaddr);
    if(ifname.empty()) {
        ifname.push_back("no network");
    }
    return;
}

void loadData(std::vector<std::pair<std::string, int>>& sensors,const std::vector<std::string>& sensor_names)
{
    sensors.clear();
    for (const auto& path : sensor_names) {
        std::ifstream type_file(path + "/type");
        std::ifstream temp_file(path + "/temp");
        
        if (type_file && temp_file) {
            std::string type;
            int temp;
            std::getline(type_file, type);
            temp_file >> temp;
            sensors.push_back({type, temp});
        } else {
            SDL_Log("Check thermal failed");
        }
    }
    
    // 按温度降序排序
    std::sort(sensors.begin(), sensors.end(), 
                [](const auto& a, const auto& b) { 
                    return a.second > b.second; 
                });
}


void EventThread::run() {
    std::unique_lock<std::mutex> lock(mtx);

    unsigned int turns = 0;
    int old_time = 0;
    intptr_t old_level = 0;
    uint8_t old_charge = 0;
    uint8_t old_sshd = 0;
    time_t now = 0;
    struct tm* local_time = nullptr;
    std::string status_str;
    bool is_charging = 0;
    bool is_sshd = 0;
    std::vector<std::pair<std::string, int>> sensors;
    std::vector<std::string> sensor_names;
    std::vector<std::string> ifname;
    std::vector<std::string> old_ifname;
    std::string path = "/sys/class/thermal/";
    if (std::filesystem::exists(path)) {
        for(const auto& entry : std::filesystem::directory_iterator(path)) {
            std::string name = entry.path().filename().string();
            if (name.find("thermal_zone") != std::string::npos) {
                sensor_names.push_back(path + name);
            }
        }
    }
    while (running) {
        // 时间
        now = time(0);
        local_time = localtime(&now);
        cur_time = local_time->tm_hour * 60 + local_time->tm_min;
        is_sshd = std::filesystem::exists("/var/run/sshd.pid");
        std::ifstream batterylevel("/sys/class/power_supply/qcom-battery/capacity");
        std::ifstream batterystatus("/sys/class/power_supply/qcom-battery/status");
        if (is_sshd) {
            sshd_status_ = 1;
        } else {
            sshd_status_ = 2;
        }
        if(batterylevel && batterystatus){
            batterylevel >> cur_level;
            batterystatus >> status_str;
        } else {
            SDL_Log("Check battery failed");
        }


        // 充电状态
        is_charging = (status_str == "Charging");
        if (is_charging) {
            charge_status = 1;
        } else {
            charge_status = 2;
        }

        if (cur_time != old_time) {
            old_time = cur_time;
            SDL_Event event{ .type = event_code + 0 };
            event.user.code = cur_time;
            SDL_PushEvent(&event);
        }

        if (cur_level != old_level) {
            old_level = cur_level;
            SDL_Event event{ .type = event_code + 2 };
            event.user.data1 = (void*)cur_level;
            SDL_PushEvent(&event);
        }

        if (old_charge == 0 || (is_charging && old_charge != 1) || (!is_charging && old_charge != 2)) {
            old_charge = charge_status;
            SDL_Event event{ .type = event_code + 1 };
            event.user.code = charge_status;
            event.user.data1 = (void*)cur_level;
            SDL_PushEvent(&event);
        }

        if (turns % 8 == 0 && !sensor_names.empty()) {

            loadData(sensors, sensor_names);
            Temp* temp = new Temp;
            strcpy(temp->type1, [&sensors](){
                if (sensors.empty()) return "Unknown";
                return sensors[0].first.c_str();
            }());
            strcpy(temp->type2, [&sensors](){
                if (sensors.size() < 2) return "Unknown";
                return sensors[1].first.c_str();
            }());
            temp->temp1 = [&sensors]() -> float {
                if (sensors.empty()) return -1;
                return sensors[0].second / 1000.0;
            }();
            temp->temp2 = [&sensors]() -> float {
                if (sensors.size() < 2) return -1;
                return sensors[1].second / 1000.0;
            }();
            SDL_Event event{ .type = event_code + 3 };
            event.user.data1 = temp;
            if(!SDL_PushEvent(&event)){
                SDL_Log("Failed to push: %s\n", SDL_GetError());
                delete temp;
            }
            
        }
        
        if (old_sshd == 0 || (is_sshd && old_sshd != 1) || (!is_sshd && old_sshd != 2)) {
            old_sshd = sshd_status_;
            SDL_Event event{ .type = event_code + 4 };
            event.user.code = sshd_status_;
            SDL_PushEvent(&event);
        }

        if (turns % 6 == 0) {
            getIp(ifname);
            if(ifname != old_ifname) {
                old_ifname = ifname;
                SDL_Event event{ .type = event_code + 5 };
                std::vector<std::string>* push_ifname = new std::vector<std::string>(ifname);
                event.user.data1 = push_ifname;
                if(!SDL_PushEvent(&event)) {
                    SDL_Log("Failed to push: %s\n", SDL_GetError());
                    delete push_ifname;
                }
            }  
        }

        if ((light_turns + 1) % 600 == 0 ) {
            SDL_Event event{ .type = event_code + 6 };
            lightScreen = false;
            SDL_PushEvent(&event);
            light_turns = 0;
        }

        if (!status) {
            status = true;
            cv.notify_all();
            lock.unlock();
            
            SDL_Log("OK");
        }
        turns++;
        if(lightScreen) light_turns++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
    }
    return;
}

EventThread::EventThread() {
    event_code = SDL_RegisterEvents(event_nums);
    if (event_code) {
        SDL_Log("Successfully created event_code");
        
    }
    else {
        SDL_Log("Fail created event_code");
    }
}

bool EventThread::start()
{
    running = true;
    if(worker) return false;
    worker = new std::thread(&EventThread::run, this);
    if(worker) return true;
    else return false;
}
void EventThread::stop()
{
    running = false;
}

EventThread::~EventThread() {
    stop();
    if(worker && worker->joinable()) worker->join();
    if(worker) delete worker;
}