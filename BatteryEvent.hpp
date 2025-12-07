#pragma once

#include <SDL3/SDL.h>
#include <systemd/sd-bus.h>

class BatteryEvent
{
private:
    static int battery_changed(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
        const char *interface;
        
        
        return 0;
    }
public:
    static bool init(sd_bus*& bus) {
        int r = sd_bus_open_system(&bus);
        if (r < 0) {
            SDL_Log("Failed to connect to D-Bus.");
            return false;
        }
        r = sd_bus_add_match(bus, nullptr,
        "type='signal',"
        "interface='org.freedesktop.DBus.Properties',"
        "member='PropertiesChanged',"
        "path='/org/freedesktop/UPower/devices/DisplayDevice'",
        battery_changed, nullptr);

        if (r < 0) {
            SDL_Log("Failed to add match.");
            return false;
        }
        SDL_Log("OK");
        return true;

    }


    

};