#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <entt/entt.hpp>
#include "TextureManager.h"
#include <iostream>

// Definiciones de constantes
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

// Componente para almacenar la posición del fondo
struct BackgroundPosition {
    float x, y;
};

// Componente para almacenar la textura del fondo
struct BackgroundTexture {
    SDL_Texture* texture;
    int width;
    int height;
};

// Componente para manejar el parallax
struct ParallaxEffect {
    float speedX;
    float speedY;
};

// Sistema de renderizado para el fondo
void RenderBackgroundSystem(entt::registry& registry, SDL_Renderer* renderer) {
    auto view = registry.view<BackgroundPosition, BackgroundTexture>();

    for (auto entity : view) {
        auto& pos = view.get<BackgroundPosition>(entity);
        auto& tex = view.get<BackgroundTexture>(entity);

        // Escalar la textura al tamaño completo de la ventana
        SDL_Rect dstRect = { static_cast<int>(pos.x), static_cast<int>(pos.y), SCREEN_WIDTH, SCREEN_HEIGHT };
        SDL_RenderCopy(renderer, tex.texture, NULL, &dstRect);
    }
}


// Sistema de actualización para el parallax
void UpdateParallaxSystem(entt::registry& registry, float dT) {
    auto view = registry.view<BackgroundPosition, ParallaxEffect>();

    for (auto entity : view) {
        auto& pos = view.get<BackgroundPosition>(entity);
        auto& parallax = view.get<ParallaxEffect>(entity);

        pos.x += parallax.speedX * dT;
        pos.y += parallax.speedY * dT;

        // Puedes añadir lógica aquí para hacer que el fondo se repita o se detenga
    }
}

int main() {
    // Inicialización de SDL y creación de la ventana y el renderer
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) {
        std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error creating renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    entt::registry registry;

    // Cargar la textura del fondo utilizando el TextureManager
    auto bgTexture = TextureManager::LoadTexture("background.bmp", renderer);
    if (!bgTexture) {
        std::cerr << "Error loading texture: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Crear una entidad para el fondo
    auto bgEntity = registry.create();
    registry.emplace<BackgroundPosition>(bgEntity, 0.0f, 0.0f);
    registry.emplace<BackgroundTexture>(bgEntity, bgTexture->sdlTexture, bgTexture->width, bgTexture->height);

    // Opcional: añadir parallax
    //registry.emplace<ParallaxEffect>(bgEntity, 50.0f, 0.0f);

    bool running = true;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Procesar eventos
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // Actualizar sistemas
        UpdateParallaxSystem(registry, deltaTime);

        // Cambiar el color de fondo a verde oscuro
        SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255); // Verde oscuro
        SDL_RenderClear(renderer);

        // Renderizar el fondo
        RenderBackgroundSystem(registry, renderer);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
