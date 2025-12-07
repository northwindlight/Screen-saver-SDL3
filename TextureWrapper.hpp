#pragma once
#include <SDL3/SDL.h>

class TextureWrapper {
private:
    SDL_Texture* texture_ = nullptr;
    SDL_FRect rect_ = {0, 0, 0, 0};
    SDL_FPoint center_ = {0, 0};
    bool rotate_ = true;
public:
    // 获取旋转中心
    SDL_FPoint* getCenter() {
        return &center_;
    }
    // 获取矩形
    SDL_FRect* getFRect() {
        return &rect_;
    }
    void setRotate(bool b) {
        rotate_ = b;
    }
    // 设置位置
    void setPosition(float x, float y) {
        if (rotate_) {
            rect_.x = x - rect_.w / 2.0f - rect_.h / 2.0f;
            rect_.y = y - rect_.h / 2.0f + rect_.w / 2.0f;
        } else {
            rect_.x = x;
            rect_.y = y;
        }
    }
    
    // 设置大小
    void setSize(float w, float h) {
        rect_.w = w;
        rect_.h = h;
    }

    void destroy() {
        if (texture_) {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
        }
    }
    
    void setTexture(SDL_Texture* texture) {
        texture_ = texture;
        if (!texture_) {
            SDL_Log("Couldn't create text: %s\n", SDL_GetError());
            return;
        }
        SDL_GetTextureSize(texture_, &rect_.w, &rect_.h);
        center_.x = rect_.w / 2.0f;
        center_.y = rect_.h / 2.0f;
    }
    // 获取原始纹理
    SDL_Texture* getTexture() const { return texture_; }
    
    // 判断是否有效
    bool isValid() const { return texture_ != nullptr; }

    ~TextureWrapper() {
        destroy();
    }
    
};