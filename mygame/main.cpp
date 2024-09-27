#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <entt/entt.hpp>
#include "TextureManager.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

// Definiciones de constantes
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int TILE_SIZE = 32;
const float MOVE_DELAY = 0.15f;  // Delay entre movimientos en segundos

// Direcciones de movimiento
enum Direction { UP, DOWN, LEFT, RIGHT };

// Componente para almacenar la textura del fondo
struct BackgroundTexture {
    SDL_Texture* texture;
    int width;
    int height;
};

// Componente para los segmentos de la serpiente
struct SnakeSegment {
    SDL_Texture* texture;   // Textura del segmento (cabeza, cuerpo, curva, cola)
    SDL_Rect srcRect;       // Subparte del sprite
};

// Componente para manejar la serpiente
struct SnakeBody {
    SDL_Point position;     // Posición actual de la cabeza
    Direction direction;    // Dirección de movimiento (UP, DOWN, LEFT, RIGHT)
    int speed = TILE_SIZE;  // Velocidad de la serpiente (mueve un tile por tick)
    float moveTimer = 0.0f; // Temporizador para controlar el tiempo entre movimientos
};

// Componente para la manzana
struct Apple {
    SDL_Point position;     // Posición actual de la manzana
    SDL_Texture* texture;   // Textura de la manzana
};

// Sistema de actualización del movimiento de la serpiente
void UpdateSnakeMovement(entt::registry& registry, float deltaTime) {
    auto view = registry.view<SnakeBody>();

    for (auto entity : view) {
        auto& snake = view.get<SnakeBody>(entity);

        // Actualizar el temporizador de movimiento
        snake.moveTimer += deltaTime;

        // Si ha pasado suficiente tiempo, mover la serpiente
        if (snake.moveTimer >= MOVE_DELAY) {
            // Mover la cabeza según la dirección
            switch (snake.direction) {
                case UP:
                    snake.position.y -= snake.speed;
                    break;
                case DOWN:
                    snake.position.y += snake.speed;
                    break;
                case LEFT:
                    snake.position.x -= snake.speed;
                    break;
                case RIGHT:
                    snake.position.x += snake.speed;
                    break;
            }

            // Asegurar que la serpiente no salga de los bordes de la pantalla
            if (snake.position.x < 0) snake.position.x = SCREEN_WIDTH - TILE_SIZE;
            if (snake.position.x >= SCREEN_WIDTH) snake.position.x = 0;
            if (snake.position.y < 0) snake.position.y = SCREEN_HEIGHT - TILE_SIZE;
            if (snake.position.y >= SCREEN_HEIGHT) snake.position.y = 0;

            // Reiniciar el temporizador de movimiento
            snake.moveTimer = 0.0f;
        }
    }
}

// Sistema de renderizado para la serpiente
void RenderSnakeSystem(entt::registry& registry, SDL_Renderer* renderer) {
    auto view = registry.view<SnakeSegment, SnakeBody>();

    for (auto entity : view) {
        auto& segment = view.get<SnakeSegment>(entity);
        auto& snake = view.get<SnakeBody>(entity);

        // Renderizar la cabeza de la serpiente con rotación según la dirección
        double angle = 0.0;
        SDL_Point center = { TILE_SIZE / 2, TILE_SIZE / 2 };

        // Corregimos el ángulo de la cabeza para reflejar las direcciones correctas
        switch (snake.direction) {
            case UP:
                angle = 180.0;
                break;
            case DOWN:
                angle = 0.0;
                break;
            case LEFT:
                angle = 90.0;
                break;
            case RIGHT:
                angle = 270.0;
                break;
        }

        SDL_Rect dstRect = { snake.position.x, snake.position.y, TILE_SIZE, TILE_SIZE };
        SDL_RenderCopyEx(renderer, segment.texture, &segment.srcRect, &dstRect, angle, &center, SDL_FLIP_NONE);
    }
}

// Sistema de renderizado para la manzana
void RenderAppleSystem(entt::registry& registry, SDL_Renderer* renderer) {
    auto view = registry.view<Apple>();

    for (auto entity : view) {
        auto& apple = view.get<Apple>(entity);

        // Renderizar la manzana
        SDL_Rect dstRect = { apple.position.x, apple.position.y, TILE_SIZE, TILE_SIZE };
        SDL_RenderCopy(renderer, apple.texture, NULL, &dstRect);
    }
}

// Sistema para verificar la colisión entre la serpiente y la manzana
void CheckCollisionWithApple(entt::registry& registry, int& appleCounter) {
    auto snakeView = registry.view<SnakeBody>();
    auto appleView = registry.view<Apple>();

    for (auto snakeEntity : snakeView) {
        auto& snake = snakeView.get<SnakeBody>(snakeEntity);

        for (auto appleEntity : appleView) {
            auto& apple = appleView.get<Apple>(appleEntity);

            // Verificar si la cabeza de la serpiente está en la misma posición que la manzana
            if (snake.position.x == apple.position.x && snake.position.y == apple.position.y) {
                std::cout << "¡Manzana comida! Contador: " << ++appleCounter << std::endl;

                // Generar nueva manzana en una posición aleatoria
                apple.position.x = (rand() % (SCREEN_WIDTH / TILE_SIZE)) * TILE_SIZE;
                apple.position.y = (rand() % (SCREEN_HEIGHT / TILE_SIZE)) * TILE_SIZE;
            }
        }
    }
}

// Sistema para renderizar el fondo
void RenderBackgroundSystem(entt::registry& registry, SDL_Renderer* renderer) {
    auto view = registry.view<BackgroundTexture>();

    for (auto entity : view) {
        auto& bg = view.get<BackgroundTexture>(entity);

        SDL_Rect dstRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
        SDL_RenderCopy(renderer, bg.texture, NULL, &dstRect);
    }
}

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));  // Inicializar la semilla para números aleatorios

    // Inicialización de SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error creating renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    entt::registry registry;

    // Cargar el fondo
    auto bgTexture = TextureManager::LoadTexture("background.bmp", renderer);
    if (!bgTexture) {
        std::cerr << "Error loading background texture: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Crear una entidad para el fondo
    auto bgEntity = registry.create();
    registry.emplace<BackgroundTexture>(bgEntity, bgTexture->sdlTexture, bgTexture->width, bgTexture->height);

    // Cargar las texturas de la serpiente
    auto snakeTexture = TextureManager::LoadTexture("snake_sprites.bmp", renderer);  // Asegúrate de que snake_sprites.bmp esté en la carpeta correcta
    if (!snakeTexture) {
        std::cerr << "Error loading snake sprite texture: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Crear una entidad para la cabeza de la serpiente
    auto snakeEntity = registry.create();
    registry.emplace<SnakeBody>(snakeEntity, SDL_Point{SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2}, RIGHT);  // Empieza en el centro y se mueve a la derecha
    registry.emplace<SnakeSegment>(snakeEntity, snakeTexture->sdlTexture, SDL_Rect{0, 0, 8, 8});  // Sprite de la cabeza

    // Cargar la textura de la manzana
    auto appleTexture = TextureManager::LoadTexture("apple.bmp", renderer);
    if (!appleTexture) {
        std::cerr << "Error loading apple texture: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Crear una entidad para la manzana
    auto appleEntity = registry.create();
    registry.emplace<Apple>(appleEntity, SDL_Point{160, 160}, appleTexture->sdlTexture);  // La manzana empieza en una posición fija

    int appleCounter = 0;  // Contador de manzanas comidas

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
            // Cambiar la dirección de la serpiente según las teclas de flecha
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        registry.get<SnakeBody>(snakeEntity).direction = UP;
                        break;
                    case SDLK_DOWN:
                        registry.get<SnakeBody>(snakeEntity).direction = DOWN;
                        break;
                    case SDLK_LEFT:
                        registry.get<SnakeBody>(snakeEntity).direction = LEFT;
                        break;
                    case SDLK_RIGHT:
                        registry.get<SnakeBody>(snakeEntity).direction = RIGHT;
                        break;
                }
            }
        }

        // Actualizar el movimiento de la serpiente
        UpdateSnakeMovement(registry, deltaTime);

        // Verificar colisión con la manzana
        CheckCollisionWithApple(registry, appleCounter);

        // Renderizar
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Fondo negro
        SDL_RenderClear(renderer);

        RenderBackgroundSystem(registry, renderer);  // Renderizar el fondo
        RenderSnakeSystem(registry, renderer);       // Renderizar la serpiente
        RenderAppleSystem(registry, renderer);       // Renderizar la manzana

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
