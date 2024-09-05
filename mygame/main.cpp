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

struct AnimationComponent {
    int currentFrame;       // Frame actual de la animación
    int frameCount;         // Número total de frames
    int frameWidth;         // Ancho de cada frame en la hoja de sprites
    int frameHeight;        // Alto de cada frame en la hoja de sprites
    float frameTime;        // Duración de cada frame (en segundos)
    float currentTime;      // Tiempo transcurrido desde el último cambio de frame
    SDL_Texture* texture;   // Textura de la hoja de sprites
};

// Sistema de renderizado para el fondo
void RenderBackgroundSystem(entt::registry& registry, SDL_Renderer* renderer) {
    auto view = registry.view<BackgroundPosition, BackgroundTexture>();

    for (auto entity : view) {
        auto& pos = view.get<BackgroundPosition>(entity);
        auto& tex = view.get<BackgroundTexture>(entity);

        // Escalar el fondo al tamaño de la ventana
        SDL_Rect dstRect = { static_cast<int>(pos.x), static_cast<int>(pos.y), SCREEN_WIDTH, SCREEN_HEIGHT };
        SDL_RenderCopy(renderer, tex.texture, NULL, &dstRect);
    }
}

// Sistema de actualización de la animación
void UpdateAnimationSystem(entt::registry& registry, float deltaTime) {
    auto view = registry.view<AnimationComponent>();

    for (auto entity : view) {
        auto& anim = view.get<AnimationComponent>(entity);

        // Actualizar el tiempo actual de la animación
        anim.currentTime += deltaTime;

        // Si ha pasado suficiente tiempo, cambiamos de frame
        if (anim.currentTime >= anim.frameTime) {
            anim.currentFrame = (anim.currentFrame + 1) % anim.frameCount; // Cicla entre los frames
            anim.currentTime = 0.0f; // Reinicia el tiempo
        }
    }
}

// Sistema de renderizado de la animación
void RenderAnimationSystem(entt::registry& registry, SDL_Renderer* renderer) {
    auto view = registry.view<AnimationComponent>();

    for (auto entity : view) {
        auto& anim = view.get<AnimationComponent>(entity);

        // Definir el rectángulo de origen (el frame actual)
        SDL_Rect srcRect = {
            anim.currentFrame * anim.frameWidth, // X del frame actual en la hoja de sprites
            0,                                   // Y (asumimos que la animación está en una fila)
            anim.frameWidth,                     // Ancho del frame
            anim.frameHeight                     // Alto del frame
        };

        // Definir el rectángulo de destino, escalando la animación
        int scaleFactor = 8; // Cambia este valor para ajustar el tamaño de la animación
        SDL_Rect dstRect = {
            (SCREEN_WIDTH - anim.frameWidth * scaleFactor) / 2,  // Centrar en pantalla
            (SCREEN_HEIGHT - anim.frameHeight * scaleFactor) / 2,
            anim.frameWidth * scaleFactor, // Escalar el ancho
            anim.frameHeight * scaleFactor // Escalar el alto
        };

        // Renderizar el frame actual con el nuevo tamaño
        SDL_RenderCopy(renderer, anim.texture, &srcRect, &dstRect);
    }
}

int main() {
    // Inicialización de SDL y creación de la ventana y el renderer
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Snake Animation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error creating renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    entt::registry registry;

    // Cargar la textura del fondo
    auto bgTexture = TextureManager::LoadTexture("background.bmp", renderer); // Cambia a tu archivo BMP
    if (!bgTexture) {
        std::cerr << "Error loading background texture: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Crear una entidad para el fondo
    auto bgEntity = registry.create();
    registry.emplace<BackgroundPosition>(bgEntity, 0.0f, 0.0f);
    registry.emplace<BackgroundTexture>(bgEntity, bgTexture->sdlTexture, bgTexture->width, bgTexture->height);

    // Cargar la hoja de sprites de la animación
    auto animTexture = TextureManager::LoadTexture("snake_head_blink.bmp", renderer); // Cambia a tu archivo BMP
    if (!animTexture) {
        std::cerr << "Error loading animation texture: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Crear una entidad para la animación
    auto animEntity = registry.create();
    registry.emplace<AnimationComponent>(animEntity,
        0,                // Frame inicial
        4,                // Número total de frames (asume 4 frames en la animación)
        8,                // Ancho de cada frame (ajusta según tu hoja de sprites)
        8,                // Alto de cada frame
        0.15f,            // Tiempo por frame (0.15 segundos por frame)
        0.0f,             // Tiempo actual de la animación
        animTexture->sdlTexture  // La textura de la hoja de sprites
    );

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
        UpdateAnimationSystem(registry, deltaTime);

        // Renderizar
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Fondo negro
        SDL_RenderClear(renderer);

        RenderBackgroundSystem(registry, renderer);  // Renderizar el fondo primero
        RenderAnimationSystem(registry, renderer);   // Luego la animación

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
