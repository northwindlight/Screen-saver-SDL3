#pragma once
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

class ThermalMonitor {
private:
    static std::vector<std::pair<std::string, int>> sensors;
    
    static void ensureDataLoaded();

public:
    // 获取最热传感器的类型
    static const char* getHottestType();
    
    // 获取最热传感器的温度
    static int getHottestTemp();
    // 获取次热传感器的类型
    static const char* getSecondHottestType(); 
    
    // 获取次热传感器的温度
    static int getSecondHottestTemp();
    
    // 可选：重新加载数据（如果需要更新温度）
    static void reloadData();
    
    // 可选：打印所有传感器信息（调试用）
    static void printAllSensors();
};

// 静态成员初始化
