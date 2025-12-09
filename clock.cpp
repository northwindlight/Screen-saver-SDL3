#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include <mutex>
#include <condition_variable>
#include <fstream>

#include "EventThread.h"
#include "TextureWrapper.hpp"

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static SDL_Texture* battery_100_texture = nullptr;
static SDL_Texture* battery_80_texture = nullptr;
static SDL_Texture* battery_60_texture = nullptr;
static SDL_Texture* battery_40_texture = nullptr;
static SDL_Texture* battery_20_texture = nullptr;
static SDL_Texture* battery_0_texture = nullptr;
static SDL_Texture* battery_charging_texture = nullptr;
static TTF_Font* font_200 = nullptr;
static TTF_Font* font_55 = nullptr;
static TTF_Font* font_40 = nullptr;
static SDL_Color color = { 255, 255, 255, SDL_ALPHA_OPAQUE };

static EventThread* eventthread = nullptr;

static TextureWrapper sshd, sshd_status, ifaddr, fuck, battery, level, temp; //我生气了，不让我用clock变量名，因为EventThread.h中使用了#include ctime 重定义了，没有命名空间标准库头文件污染人，能不能死一死啊
static SDL_Texture* ChangeLevel(const int& level)
{
    if (level >= 90) {
        return battery_100_texture;
    }
    if (level >= 70) {
        return battery_80_texture;
    }
    if (level >= 50) {
        return battery_60_texture;
    }
    if (level >= 30) {
        return battery_40_texture;
    }
    if (level >= 10) {
        return battery_20_texture;
    } else {
        return battery_0_texture;
    }
}
static bool CreateBatteryTexture(SDL_Texture*& texture, const char* file, SDL_Renderer*& renderer)
{
    std::string path = std::string("/home/northwind/sdl3/PNG/128px/") + file;
    SDL_IOStream* pngfile = SDL_IOFromFile(path.c_str(), "rb");
    if (!pngfile) {
        SDL_Log("Couldn't open png: %s\n", SDL_GetError());
        return false;
    } else {
        texture = IMG_LoadTexture_IO(renderer, pngfile, true);
        if (!texture) {
            SDL_Log("Couldn't load png: %s\n", SDL_GetError());
            return false;
        }
        return true;
    }
}

const static bool* keystate = nullptr;
const static double rotate = 90.0;
static int frameCount = 10;
static int frameCount_OFF = 5;
static int screen_w = 0, screen_h = 0;;
static void redraw() {
    frameCount = 5;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    
    //timerThread = new std::thread(timerFunction);
    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("Clock", 0, 0, SDL_WINDOW_FULLSCREEN, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!TTF_Init()) {
        SDL_Log("Couldn't initialize SDL_ttf: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!CreateBatteryTexture(battery_100_texture, "Battery Icons - White 128px (1).png", renderer)) return SDL_APP_FAILURE;
    if (!CreateBatteryTexture(battery_80_texture, "Battery Icons - White 128px (2).png", renderer)) return SDL_APP_FAILURE;
    if (!CreateBatteryTexture(battery_60_texture, "Battery Icons - White 128px (3).png", renderer)) return SDL_APP_FAILURE;
    if (!CreateBatteryTexture(battery_40_texture, "Battery Icons - White 128px (4).png", renderer)) return SDL_APP_FAILURE;
    if (!CreateBatteryTexture(battery_20_texture, "Battery Icons - White 128px (5).png", renderer)) return SDL_APP_FAILURE;
    if (!CreateBatteryTexture(battery_0_texture, "Battery Icons - White 128px (6).png", renderer)) return SDL_APP_FAILURE;
    if (!CreateBatteryTexture(battery_charging_texture, "Battery Icons - White 128px (13).png", renderer)) return SDL_APP_FAILURE;
    /* Open the font */
    SDL_IOStream* fontfile = SDL_IOFromFile("/usr/share/fonts/truetype/roboto/Roboto-Regular.ttf", "rb");
    if (!fontfile) {
        SDL_Log("Couldn't open font: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    font_200 = TTF_OpenFontIO(fontfile, false, 200.0f);
    if (!font_200) {
        SDL_Log("Couldn't open font: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SeekIO(fontfile, 0, SDL_IO_SEEK_SET);
    font_40 = TTF_OpenFontIO(fontfile, false, 40.0f);
    if (!font_40) {
        SDL_Log("Couldn't open font: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SeekIO(fontfile, 0, SDL_IO_SEEK_SET);
    font_55 = TTF_OpenFontIO(fontfile, true, 55.0f);
    if (!font_55) {
        SDL_Log("Couldn't open font: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderVSync(renderer, 1);
    SDL_GetRenderOutputSize(renderer, &screen_w, &screen_h);
    SDL_Surface* text = TTF_RenderText_Blended(font_40, "sshd", strlen("sshd"), color);
    if (text) {
        sshd.setTexture(SDL_CreateTextureFromSurface(renderer, text));
    }
    if (!sshd.isValid()) {
        SDL_Log("Couldn't create text: %s\n", SDL_GetError());
    }
    SDL_DestroySurface(text);
    sshd.setPosition(screen_w - screen_w / 15.0f, screen_h / 3.0f);
    eventthread = new EventThread();
    std::unique_lock<std::mutex> lock(eventthread->mtx);
    if(!eventthread->start()) {
        SDL_Log("Event thread start failed");
        return SDL_APP_FAILURE;
    };
    while(!eventthread->status) {
        eventthread->cv.wait(lock);
    }
    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    if (event->type == SDL_EVENT_KEY_DOWN) {
        eventthread->light_turns = 0;
        SDL_Log("keycode %d, scancode %d", event->key.key, event->key.scancode);
        keystate = SDL_GetKeyboardState(nullptr);
        if(keystate[129] && keystate[128]) {
            return SDL_APP_SUCCESS;
        }
        if(event->key.scancode == 102) {
            if(eventthread->lightScreen) {
                frameCount_OFF = 5;
                eventthread->lightScreen = false;
            } else {
                eventthread->lightScreen = true;
            }
            redraw();
        }
    }
    if (event->type == eventthread->event_code) {
        redraw();
        char current_time[8] = {0};
        snprintf(current_time, 8, "%02d:%02d", event->user.code / 60, event->user.code % 60);
        SDL_Surface* text = TTF_RenderText_Blended(font_200, current_time, strlen(current_time), color);
        if (text) {
            fuck.destroy();
            fuck.setTexture(SDL_CreateTextureFromSurface(renderer, text));
        }
        SDL_DestroySurface(text);
        fuck.setRotate(false); //特殊设计，手动处理纹理
        fuck.setPosition(screen_w / 2.0f - fuck.getFRect()->w / 2, screen_h / 2.0f - fuck.getFRect()->h / 2.0f);
    }

    if (event->type == eventthread->event_code + 1) {
        redraw();
        if(event->user.code == 1) {
            battery.setTexture(battery_charging_texture);
        }
        else {
            battery.setTexture(ChangeLevel((intptr_t)event->user.data1));
        }
        battery.setRotate(false);
        battery.setPosition(screen_w - screen_w / 9.5f, screen_h - screen_h / 10.0f);
    }

    if (event->type == eventthread->event_code + 2) {
        redraw();
        if(battery.getTexture() != battery_charging_texture) {
            battery.setTexture(ChangeLevel((intptr_t)event->user.data1));
        }
        battery.setRotate(false);
        battery.setPosition(screen_w - screen_w / 9.5f, screen_h - screen_h / 10.0f);
        char level_str[6] = {0};
        snprintf(level_str, 6, "%3d%%", (intptr_t)event->user.data1);
        SDL_Surface* text = TTF_RenderText_Blended(font_55, level_str, strlen(level_str), color);
        if (text) {
            level.destroy();
            level.setTexture(SDL_CreateTextureFromSurface(renderer, text));
        }
        level.setPosition(screen_w - screen_w / 80.0f, battery.getFRect()->y - screen_h / 15.0f);
        SDL_DestroySurface(text);

    }

    if (event->type == eventthread->event_code + 3) {
        redraw();
        char temp1_str[30] = {0};
        char temp2_str[30] = {0};
        snprintf(temp1_str, 30, "%s  %3.2f", ((Temp*)event->user.data1)->type1, ((Temp*)event->user.data1)->temp1);
        snprintf(temp2_str, 30, "%s  %3.2f", ((Temp*)event->user.data1)->type2, ((Temp*)event->user.data1)->temp2);
        delete (Temp*)event->user.data1;
        SDL_Surface* text1 = TTF_RenderText_Blended(font_40, temp1_str, strlen(temp1_str), color);
        SDL_Surface* text2 = TTF_RenderText_Blended(font_40, temp2_str, strlen(temp2_str), color);
        SDL_Surface* text = [&]() -> SDL_Surface* {
            if(text1 && text2) {
                SDL_Surface* surf = SDL_CreateSurface(
                    [&](){
                        if(text1->w >= text2->w){
                            return text1->w;
                        } else {
                            return text2->w;
                        }          
                    }(),
                    text1->h + text2->h,
                    SDL_PIXELFORMAT_RGBA8888
                );
                SDL_Rect dst = {0, 0, 0, 0};
                if(SDL_BlitSurface(text1, nullptr, surf, &dst)) {
                    dst.y = text1->h;
                    if(SDL_BlitSurface(text2, nullptr, surf, &dst)) {
                        return surf;
                    }
                }
                return nullptr;
            } else { 
                return nullptr;
            }
        }();
        SDL_DestroySurface(text1);
        SDL_DestroySurface(text2);
        if (text) {
            temp.destroy();
            temp.setTexture(SDL_CreateTextureFromSurface(renderer, text));
            temp.setPosition(screen_w - screen_w / 15.0f, screen_h / 20.0f);
        }
        SDL_DestroySurface(text);

    }

    if (event->type == eventthread->event_code + 4) {
        SDL_Surface* text = nullptr;
        redraw();
        if (event->user.code == 1) {
            text = TTF_RenderText_Blended(font_40, "active", strlen("active"), { 0, 255, 0, SDL_ALPHA_OPAQUE });
        } else {
            text = TTF_RenderText_Blended(font_40, "died", strlen("died"), { 255, 0, 0, SDL_ALPHA_OPAQUE });
        }
        if (text) {
            sshd_status.destroy();
            sshd_status.setTexture(SDL_CreateTextureFromSurface(renderer, text));
        }
        SDL_DestroySurface(text);
        sshd_status.setPosition(screen_w - screen_w / 15.0f, screen_h / 3.0f + screen_h / 20.0f);

    }

    if (event->type == eventthread->event_code + 5) {
        redraw();
        std::vector<SDL_Surface*> surfaces;
        for(const auto& name : *(std::vector<std::string>*)event->user.data1) {
            surfaces.push_back(TTF_RenderText_Blended(font_55, name.c_str(), name.length(), color));
        }
        delete (std::vector<std::string>*)event->user.data1;
        std::sort(surfaces.begin(), surfaces.end(), [](SDL_Surface* a, SDL_Surface* b){
            return a->w > b->w;
        });
        SDL_Surface* text = SDL_CreateSurface(
                    surfaces[0]->w,
                    surfaces[0]->h * surfaces.size(),
                    SDL_PIXELFORMAT_RGBA8888
                );
        SDL_Rect dst = {0, 0, 0, 0};
        for(const auto& surf : surfaces) {
            if(SDL_BlitSurface(surf, nullptr, text, &dst)) {
                dst.y += surf->h;
            }
            SDL_DestroySurface(surf);
        }
        if (text) {
            ifaddr.destroy();
            ifaddr.setTexture(SDL_CreateTextureFromSurface(renderer, text));
        }
        SDL_DestroySurface(text);
        ifaddr.setPosition(screen_w / 2.0f, screen_h / 18.0f);
        
    }
    if (event->type == eventthread->event_code + 6) {
        frameCount_OFF = 5;
        SDL_Log("Close screen");

    }

    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate)
{
    if(eventthread->lightScreen) //一直亮屏会烧屏。针对oled
    {
        if (frameCount)
        {
            /* 旋转中心：text_dstrect 的中心点（相对） */
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            if (!SDL_RenderTextureRotated(renderer, fuck.getTexture(), nullptr, fuck.getFRect(), rotate, fuck.getCenter(), SDL_FLIP_NONE)) {
                SDL_Log("Failed to render rotated clock_texture: %s\n", SDL_GetError());
            }

            if (!SDL_RenderTextureRotated(renderer, level.getTexture(), nullptr, level.getFRect(), rotate, level.getCenter(), SDL_FLIP_NONE)) {
                SDL_Log("Failed to render rotated level_texture: %s\n", SDL_GetError());
            }

            if (!SDL_RenderTextureRotated(renderer, temp.getTexture(), nullptr, temp.getFRect(), rotate, temp.getCenter(), SDL_FLIP_NONE)) {
                SDL_Log("Failed to render rotated temp_texture: %s\n", SDL_GetError());
            }

            if (!SDL_RenderTexture(renderer, battery.getTexture(), nullptr, battery.getFRect()))
            {
                SDL_Log("Failed to render rotated battery_texture: %s\n", SDL_GetError());
            }
            
            if (!SDL_RenderTextureRotated(renderer, sshd.getTexture(), nullptr, sshd.getFRect(), rotate, sshd.getCenter(), SDL_FLIP_NONE)) {
                SDL_Log("Failed to render rotated sshd_texture: %s\n", SDL_GetError());
            }

            if (!SDL_RenderTextureRotated(renderer, sshd_status.getTexture(), nullptr, sshd_status.getFRect(), rotate, sshd_status.getCenter(), SDL_FLIP_NONE)) {
                SDL_Log("Failed to render rotated sshd_status_texture: %s\n", SDL_GetError());
            }

            if (!SDL_RenderTextureRotated(renderer, ifaddr.getTexture(), nullptr, ifaddr.getFRect(), rotate, ifaddr.getCenter(), SDL_FLIP_NONE)) {
                SDL_Log("Failed to render rotated ifaddr_texture: %s\n", SDL_GetError());
            }
            //需要保证渲染足够的新帧提交给drm后端，以用于三缓冲刷新，否则会存在旧帧进入缓冲区
            //SDL_Log("frameCount %d", frameCount);
            frameCount--;
        }
    } else {
        if(frameCount_OFF) {
            SDL_RenderClear(renderer);
            frameCount_OFF--;
        }
    }
    SDL_RenderPresent(renderer); //drm后端需要一直提交帧保证独占 为什么不一秒1帧，对于不支持可变刷新率的屏幕而言，没有意义，最多节省一点点cpu
    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    delete eventthread;
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
}