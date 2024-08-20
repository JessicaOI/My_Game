#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include <SDL.h>
#include <map>
#include <string>
#include <iostream>

class Texture {
public:
    SDL_Texture* sdlTexture;
    int width;
    int height;

    bool load(const std::string& filename, SDL_Renderer* renderer) {
        SDL_Surface* tempSurface = SDL_LoadBMP(filename.c_str());
        if (!tempSurface) {
            std::cerr << "Error: Could not load image " << filename << ". SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        sdlTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
        SDL_FreeSurface(tempSurface);

        if (!sdlTexture) {
            std::cerr << "Error: Could not create texture from " << filename << ". SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }

        SDL_QueryTexture(sdlTexture, NULL, NULL, &width, &height);
        return true;
    }

};

class TextureManager {
public:
    static Texture* LoadTexture(const std::string& filename, SDL_Renderer* renderer);
    static void UnloadTexture(const std::string& filename);
    static Texture* GetTexture(const std::string& filename);

private:
    static std::map<std::string, Texture*> textures;
};

#endif // TEXTUREMANAGER_H
