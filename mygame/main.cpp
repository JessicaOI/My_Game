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
const float ROCK_TIMER = 15.0f;  // Tiempo para cambiar las rocas de lugar

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
    SDL_Texture* texture;
    SDL_Rect srcRect;
};

// Componente para manejar la serpiente
struct SnakeBody {
    std::vector<SDL_Point> segments;  // Posiciones de todos los segmentos
    std::vector<Direction> directions; // Direcciones de cada segmento
    Direction direction;              // Dirección de movimiento de la cabeza
    int speed = TILE_SIZE;            // Velocidad de la serpiente
    float moveTimer = 0.0f;           // Temporizador para el movimiento
    bool grow = false;                // Indica si la serpiente debe crecer
};

// Componente para la manzana
struct Apple {
    SDL_Point position;
    SDL_Texture* texture;
};

// Componente para las rocas
struct Rock {
    std::vector<SDL_Point> positions; // Posiciones de las rocas
    SDL_Texture* texture;
    float timer = 0.0f; // Temporizador para cambiar la posición de las rocas
};

// Sistema de generación de rocas
void GenerateRock(entt::registry& registry, SDL_Texture* rockTexture) {
    auto view = registry.view<Rock>();
    if (view.empty()) {
        auto rockEntity = registry.create();
        registry.emplace<Rock>(rockEntity, std::vector<SDL_Point>(), rockTexture);
    }

    auto rockEntity = view.front();
    auto& rock = registry.get<Rock>(rockEntity);

    // Limitar las posiciones aleatorias para asegurarse de que no se salgan de la pantalla
    int startX = (rand() % ((SCREEN_WIDTH - 2 * TILE_SIZE) / TILE_SIZE)) * TILE_SIZE;
    int startY = (rand() % ((SCREEN_HEIGHT - 2 * TILE_SIZE) / TILE_SIZE)) * TILE_SIZE;

    // Colocar los tres tiles consecutivos en dirección horizontal o vertical
    rock.positions.clear();
    if (rand() % 2 == 0) {  // Horizontal
        rock.positions.push_back({startX, startY});
        rock.positions.push_back({startX + TILE_SIZE, startY});
        rock.positions.push_back({startX + 2 * TILE_SIZE, startY});
    } else {  // Vertical
        rock.positions.push_back({startX, startY});
        rock.positions.push_back({startX, startY + TILE_SIZE});
        rock.positions.push_back({startX, startY + 2 * TILE_SIZE});
    }

    rock.texture = rockTexture;
}


// Sistema de actualización para cambiar las rocas de lugar cada 15 segundos
void UpdateRockMovement(entt::registry& registry, float deltaTime, SDL_Texture* rockTexture) {
    auto view = registry.view<Rock>();

    for (auto entity : view) {
        auto& rock = view.get<Rock>(entity);
        rock.timer += deltaTime;

        if (rock.timer >= ROCK_TIMER) {
            // Cambiar la posición de las rocas después de 15 segundos
            GenerateRock(registry, rockTexture);
            rock.timer = 0.0f;  // Reiniciar el temporizador
        }
    }
}

// Sistema de renderizado para la roca
void RenderRockSystem(entt::registry& registry, SDL_Renderer* renderer) {
    auto view = registry.view<Rock>();

    for (auto entity : view) {
        auto& rock = view.get<Rock>(entity);

        for (auto& pos : rock.positions) {
            SDL_Rect dstRect = { pos.x, pos.y, TILE_SIZE, TILE_SIZE };
            SDL_RenderCopy(renderer, rock.texture, NULL, &dstRect);
        }
    }
}

// Verificar colisión entre la serpiente y las rocas
bool CheckCollisionWithRock(const SnakeBody& snake, const Rock& rock) {
    SDL_Point head = snake.segments[0];

    for (const auto& pos : rock.positions) {
        // Verificamos si la cabeza está dentro de las dimensiones de la roca (32x32 px)
        if (head.x < pos.x + TILE_SIZE && head.x + TILE_SIZE > pos.x &&
            head.y < pos.y + TILE_SIZE && head.y + TILE_SIZE > pos.y) {
            return true;
            }
    }
    return false;
}

// Sistema de actualización del movimiento de la serpiente
void UpdateSnakeMovement(entt::registry& registry, float deltaTime) {
    auto view = registry.view<SnakeBody>();

    for (auto entity : view) {
        auto& snake = view.get<SnakeBody>(entity);

        // Actualizar el temporizador de movimiento
        snake.moveTimer += deltaTime;

        // Si ha pasado suficiente tiempo, mover la serpiente
        if (snake.moveTimer >= MOVE_DELAY) {
            SDL_Point prevPosition = snake.segments[0];  // Posición anterior de la cabeza
            Direction prevDirection = snake.direction;  // Dirección anterior de la cabeza

            // Mover la cabeza según la dirección
            switch (snake.direction) {
                case UP:
                    snake.segments[0].y -= snake.speed;
                    break;
                case DOWN:
                    snake.segments[0].y += snake.speed;
                    break;
                case LEFT:
                    snake.segments[0].x -= snake.speed;
                    break;
                case RIGHT:
                    snake.segments[0].x += snake.speed;
                    break;
            }

            // Asegurar que la serpiente no salga de los bordes de la pantalla
            if (snake.segments[0].x < 0) snake.segments[0].x = SCREEN_WIDTH - TILE_SIZE;
            if (snake.segments[0].x >= SCREEN_WIDTH) snake.segments[0].x = 0;
            if (snake.segments[0].y < 0) snake.segments[0].y = SCREEN_HEIGHT - TILE_SIZE;
            if (snake.segments[0].y >= SCREEN_HEIGHT) snake.segments[0].y = 0;

            // Mover el resto del cuerpo de la serpiente
            for (size_t i = snake.segments.size() - 1; i > 0; --i) {
                snake.segments[i] = snake.segments[i - 1];
                snake.directions[i] = snake.directions[i - 1];
            }

            // El primer segmento del cuerpo sigue a la cabeza (la antigua posición de la cabeza)
            snake.segments[1] = prevPosition;
            snake.directions[1] = prevDirection;

            // Crecer si es necesario
            if (snake.grow) {
                SDL_Point newSegment = snake.segments.back();  // Posición del último segmento
                snake.segments.push_back(newSegment);
                snake.directions.push_back(snake.directions.back());  // Mantener la misma dirección
                snake.grow = false;
            }

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

        // Renderizar la cabeza y los segmentos del cuerpo
        for (size_t i = 0; i < snake.segments.size(); ++i) {
            SDL_Rect dstRect = { snake.segments[i].x, snake.segments[i].y, TILE_SIZE, TILE_SIZE };
            SDL_Point center = { TILE_SIZE / 2, TILE_SIZE / 2 };
            double angle = 0.0;

            if (i == 0) {
                // Renderizar la cabeza con rotación según la dirección
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
                SDL_Rect headRect = { 0, 0, 8, 8 };  // Sprite de la cabeza en la posición 1
                SDL_RenderCopyEx(renderer, segment.texture, &headRect, &dstRect, angle, &center, SDL_FLIP_NONE);
            } else {
                // Renderizar los segmentos del cuerpo con rotación
                SDL_Rect bodyRect = { 8, 0, 8, 8 };  // Usar el sprite del cuerpo (posición 2)

                // Aplicar la rotación solo si el cuerpo se mueve en dirección horizontal
                switch (snake.directions[i]) {
                    case UP:
                    case DOWN:
                        angle = 0.0;  // No rotar para el movimiento vertical
                        break;
                    case LEFT:
                    case RIGHT:
                        angle = 90.0;  // Rotar 90 grados para el movimiento horizontal
                        break;
                }
                SDL_RenderCopyEx(renderer, segment.texture, &bodyRect, &dstRect, angle, &center, SDL_FLIP_NONE);
            }
        }
    }
}


// Sistema de renderizado para la manzana
void RenderAppleSystem(entt::registry& registry, SDL_Renderer* renderer) {
    auto view = registry.view<Apple>();

    for (auto entity : view) {
        auto& apple = view.get<Apple>(entity);

        SDL_Rect dstRect = { apple.position.x, apple.position.y, TILE_SIZE, TILE_SIZE };
        SDL_RenderCopy(renderer, apple.texture, NULL, &dstRect);
    }
}

void PlayBackgroundMusic(const char* filePath) {
    SDL_AudioSpec wavSpec;
    Uint32 wavLength;
    Uint8* wavBuffer;

    if (SDL_LoadWAV(filePath, &wavSpec, &wavBuffer, &wavLength) == NULL) {
        std::cerr << "Error al cargar archivo WAV de música de fondo: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);
    if (deviceId == 0) {
        std::cerr << "Error al abrir el dispositivo de audio: " << SDL_GetError() << std::endl;
        SDL_FreeWAV(wavBuffer);
        return;
    }

    SDL_QueueAudio(deviceId, wavBuffer, wavLength);
    SDL_PauseAudioDevice(deviceId, 0); // Reproducir audio

    // No bloqueamos con SDL_Delay; dejamos que el audio se reproduzca en segundo plano
}


// Función para reproducir un efecto de sonido
void PlaySoundEffect(const char* filePath) {
    SDL_AudioSpec wavSpec;
    Uint32 wavLength;
    Uint8* wavBuffer;

    if (SDL_LoadWAV(filePath, &wavSpec, &wavBuffer, &wavLength) == NULL) {
        std::cerr << "Error al cargar archivo WAV de efecto de sonido: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);
    if (deviceId == 0) {
        std::cerr << "Error al abrir el dispositivo de audio: " << SDL_GetError() << std::endl;
        SDL_FreeWAV(wavBuffer);
        return;
    }

    SDL_QueueAudio(deviceId, wavBuffer, wavLength);
    SDL_PauseAudioDevice(deviceId, 0); // Reproducir audio

    // No bloqueamos con SDL_Delay; el sonido se reproduce en segundo plano
    SDL_FreeWAV(wavBuffer);
}


// Sistema para verificar la colisión entre la serpiente y la manzana
void CheckCollisionWithApple(entt::registry& registry, int& appleCounter) {
    auto snakeView = registry.view<SnakeBody>();
    auto appleView = registry.view<Apple>();

    for (auto snakeEntity : snakeView) {
        auto& snake = snakeView.get<SnakeBody>(snakeEntity);

        for (auto appleEntity : appleView) {
            auto& apple = appleView.get<Apple>(appleEntity);

            int snakeX = (snake.segments[0].x / TILE_SIZE) * TILE_SIZE;
            int snakeY = (snake.segments[0].y / TILE_SIZE) * TILE_SIZE;
            int appleX = (apple.position.x / TILE_SIZE) * TILE_SIZE;
            int appleY = (apple.position.y / TILE_SIZE) * TILE_SIZE;

            if (snakeX == appleX && snakeY == appleY) {
                std::cout << "¡Manzana comida! Contador: " << ++appleCounter << std::endl;
                snake.grow = true;

                // Reproducir efecto de sonido al comer la manzana
                PlaySoundEffect("comiendoManzana.wav");

                // Generar nueva manzana en una posición aleatoria
                apple.position.x = (rand() % (SCREEN_WIDTH / TILE_SIZE)) * TILE_SIZE;
                apple.position.y = (rand() % (SCREEN_HEIGHT / TILE_SIZE)) * TILE_SIZE;
            }
        }
    }
}

// Sistema para verificar colisión con el cuerpo de la serpiente
bool CheckSelfCollision(const SnakeBody& snake) {
    SDL_Point head = snake.segments[0];

    // Verificar si la cabeza colisiona con cualquier segmento del cuerpo, excepto el primero que sigue a la cabeza
    for (size_t i = 2; i < snake.segments.size(); ++i) {
        if (head.x == snake.segments[i].x && head.y == snake.segments[i].y) {
            return true;
        }
    }
    return false;
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
    srand(static_cast<unsigned int>(time(nullptr)));

    // Inicializar SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Reproducir música de fondo al iniciar el juego
    PlayBackgroundMusic("fondo.wav");

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

    auto bgEntity = registry.create();
    registry.emplace<BackgroundTexture>(bgEntity, bgTexture->sdlTexture, bgTexture->width, bgTexture->height);

    // Cargar las texturas de la serpiente
    auto snakeTexture = TextureManager::LoadTexture("snake_sprites.bmp", renderer);
    if (!snakeTexture) {
        std::cerr << "Error loading snake sprite texture: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Crear entidad para la serpiente
    auto snakeEntity = registry.create();
    registry.emplace<SnakeBody>(snakeEntity, std::vector<SDL_Point>{{SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2}}, std::vector<Direction>{RIGHT}, RIGHT);
    registry.emplace<SnakeSegment>(snakeEntity, snakeTexture->sdlTexture, SDL_Rect{0, 0, 8, 8});

    // Cargar la textura de la manzana
    auto appleTexture = TextureManager::LoadTexture("apple.bmp", renderer);
    if (!appleTexture) {
        std::cerr << "Error loading apple texture: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    auto appleEntity = registry.create();
    registry.emplace<Apple>(appleEntity, SDL_Point{160, 160}, appleTexture->sdlTexture);

    // Cargar la textura de las rocas
    auto rockTexture = TextureManager::LoadTexture("roca.bmp", renderer);
    if (!rockTexture) {
        std::cerr << "Error loading rock texture: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Crear la entidad para las rocas
    GenerateRock(registry, rockTexture->sdlTexture);  // Inicializar las primeras rocas

    int appleCounter = 0;
    bool running = true;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYDOWN) {
                auto& snake = registry.get<SnakeBody>(snakeEntity);
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        if (snake.direction != DOWN) snake.direction = UP;
                        break;
                    case SDLK_DOWN:
                        if (snake.direction != UP) snake.direction = DOWN;
                        break;
                    case SDLK_LEFT:
                        if (snake.direction != RIGHT) snake.direction = LEFT;
                        break;
                    case SDLK_RIGHT:
                        if (snake.direction != LEFT) snake.direction = RIGHT;
                        break;
                }
            }
        }

        // Actualizar el movimiento de la serpiente
        UpdateSnakeMovement(registry, deltaTime);

        // Verificar colisión con la manzana
        CheckCollisionWithApple(registry, appleCounter);

        // Actualizar y mover las rocas cada 15 segundos
        UpdateRockMovement(registry, deltaTime, rockTexture->sdlTexture);

        // Verificar colisión con el cuerpo
        auto& snake = registry.get<SnakeBody>(snakeEntity);
        if (CheckSelfCollision(snake)) {
            std::cout << "Colisión con el cuerpo. ¡Juego terminado!" << std::endl;
            running = false;
        }

        // Verificar colisión con las rocas
        auto& rock = registry.get<Rock>(registry.view<Rock>().front());
        if (CheckCollisionWithRock(snake, rock)) {
            std::cout << "Colisión con la roca. ¡Juego terminado!" << std::endl;
            running = false;
        }

        // Renderizar todo
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        RenderBackgroundSystem(registry, renderer);
        RenderSnakeSystem(registry, renderer);
        RenderAppleSystem(registry, renderer);
        RenderRockSystem(registry, renderer);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
