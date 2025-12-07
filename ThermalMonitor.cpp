#include "ThermalMonitor.h"
void ThermalMonitor::ensureDataLoaded()
{
    if (!sensors.empty()) return;
    
    // 读取所有thermal zone (0-27)
    for (int i = 0; i <= 27; i++) {
        std::string path = "/sys/class/thermal/thermal_zone" + std::to_string(i);
        
        std::ifstream type_file(path + "/type");
        std::ifstream temp_file(path + "/temp");
        
        if (type_file && temp_file) {
            std::string type;
            int temp;
            std::getline(type_file, type);
            temp_file >> temp;
            sensors.push_back({type, temp});
        }
    }
    
    // 按温度降序排序
    std::sort(sensors.begin(), sensors.end(), 
                [](const auto& a, const auto& b) { 
                    return a.second > b.second; 
                });
}

const char* ThermalMonitor::getHottestType()
{
    ensureDataLoaded();
    if (sensors.empty()) return "Unknown";
    return sensors[0].first.c_str();
}

int ThermalMonitor::getHottestTemp()
{
    ensureDataLoaded();
    if (sensors.empty()) return -1;
    return sensors[0].second;
}

const char* ThermalMonitor::getSecondHottestType()
{
    ensureDataLoaded();
    if (sensors.size() < 2) return "Unknown";
    return sensors[1].first.c_str();
}

int ThermalMonitor::getSecondHottestTemp()
{
    ensureDataLoaded();
    if (sensors.size() < 2) return -1;
    return sensors[1].second;
}

void ThermalMonitor::reloadData() {
    sensors.clear();
    ensureDataLoaded();
}

void ThermalMonitor::printAllSensors() {
    ensureDataLoaded();
    std::cout << "所有传感器温度:" << std::endl;
    for (size_t i = 0; i < sensors.size(); i++) {
        std::cout << (i + 1) << ". " << sensors[i].first 
                    << " - " << sensors[i].second / 1000.0 << "°C" << std::endl;
    }
}
