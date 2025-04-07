#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_rect.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

constexpr float WINDOW_WIDTH = 1024;
constexpr float WINDOW_HEIGHT = 720;

constexpr SDL_Color BACKGROUND_COLOR = { 0x18, 0x18, 0x18, 0xFF };
constexpr SDL_Color ENTITY_COLOR = { 0x00, 0x00, 0xFF, 0xFF };
constexpr SDL_Color TARGET_COLOR = { 0x00, 0xFF, 0x00, 0xFF };
constexpr SDL_Color PATH_COLOR = {0xFF, 0xFF, 0xFF, 0xFF };

constexpr float ENTITY_WIDTH = 20.0f;
constexpr float TARGET_WIDTH = 24.0f;
constexpr size_t PATH_RESOLUTION = 100;
constexpr float PATH_DELTA = 1.0f / (float)PATH_RESOLUTION;

constexpr float TARGET_TIME = 1.0f;
constexpr float T_MULTIPLIER = 1.0f / TARGET_TIME;

bool Draw(
    SDL_Renderer* renderer,
    const SDL_FPoint entityPos,
    const SDL_FPoint targetPos,
    SDL_FPoint *path,
    const size_t pathSize
) {
    if (!SDL_SetRenderDrawColor(renderer,
        BACKGROUND_COLOR.r,
        BACKGROUND_COLOR.g,
        BACKGROUND_COLOR.b,
        BACKGROUND_COLOR.a
    ))
        return false;
    if (!SDL_RenderClear(renderer)) return false;

    const SDL_FRect entityRect = {
        entityPos.x - ENTITY_WIDTH / 2.0f,
        entityPos.y - ENTITY_WIDTH / 2.0f,
        ENTITY_WIDTH,
        ENTITY_WIDTH
    };

    const SDL_FRect targetRect = {
        targetPos.x - TARGET_WIDTH / 2.0f,
        targetPos.y - TARGET_WIDTH / 2.0f,
        TARGET_WIDTH,
        TARGET_WIDTH
    };

    if (!SDL_SetRenderDrawColor(renderer,
        TARGET_COLOR.r,
        TARGET_COLOR.g,
        TARGET_COLOR.b,
        TARGET_COLOR.a
    ))
        return false;

    if (!SDL_RenderFillRect(renderer, &targetRect)) return false;

    if (!SDL_SetRenderDrawColor(renderer,
        PATH_COLOR.r,
        PATH_COLOR.g,
        PATH_COLOR.b,
        PATH_COLOR.a
    ))
        return false;
    if (!SDL_RenderLines(renderer, path, pathSize)) return false;

    if (!SDL_SetRenderDrawColor(renderer,
        ENTITY_COLOR.r,
        ENTITY_COLOR.g,
        ENTITY_COLOR.b,
        ENTITY_COLOR.a
    ))
        return false;
    if (!SDL_RenderFillRect(renderer, &entityRect)) return false;

    if (!SDL_RenderPresent(renderer)) return false;
    return true;
}

void CalculateCoefs(float *coefsOut, const float initialPos, const float targetPos, const float initialVel) {
    coefsOut[0] = initialPos;
    coefsOut[1] = initialVel;
    coefsOut[2] = -3.0f * initialPos + 3.0f * targetPos - 2.0f * initialVel;
    coefsOut[3] = 2.0f * initialPos - 2.0f * targetPos + initialVel;
}

float Cubic(const float *coefs, const float t) {
    return coefs[3] * t * t * t + coefs[2] * t * t + coefs[1] * t + coefs[0];
}

float Derivative(const float *coefs, const float t) {
    return 3.0f * coefs[3] * t * t + 2.0f * coefs[2] * t + coefs[1];
}

void SetPath(SDL_FPoint *points, float *xCoefs, float* yCoefs) {
    float t = 0.0f;
    for (size_t i = 0; i < PATH_RESOLUTION; ++i) {
        points[i].x = Cubic(xCoefs, t);
        points[i].y = Cubic(yCoefs, t);
        t += PATH_DELTA;
    }
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Cubic Solver", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_FPoint entityPos = { WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f };
    SDL_FPoint targetPos = entityPos;

    float xCoefs[4] = {entityPos.x, 0.0f, 0.0f, 0.0f }, yCoefs[4] = { entityPos.y, 0.0f, 0.0f, 0.0f};
    SDL_FPoint path[PATH_RESOLUTION];
    SetPath(path, xCoefs, yCoefs);
    size_t pathIndex = 0;

    bool shouldQuit = false;
    
    struct timespec initialTime;
    struct timespec currentTime;
    if (!timespec_get(&initialTime, TIME_UTC)) shouldQuit = true;
    while (!shouldQuit) {
        if (!timespec_get(&currentTime, TIME_UTC)) shouldQuit = true;
        const float sDiff = (float)difftime(currentTime.tv_sec, initialTime.tv_sec);
        const float nsDiff = (float)(currentTime.tv_nsec - initialTime.tv_nsec) / 1'000'000'000.0f;
        const float tDelta = sDiff + nsDiff;
        float t;
        if (tDelta >= TARGET_TIME) {
            initialTime = currentTime;
            t = 0.0f;
            xCoefs[0] = targetPos.x;
            yCoefs[0] = targetPos.y;
            for (size_t i = 1; i < 4; ++i) {
                xCoefs[i] = 0.0f;
                yCoefs[i] = 0.0f;
            }
            SetPath(path, xCoefs, yCoefs);
            pathIndex = 0;
        }
        else
            t = T_MULTIPLIER * tDelta;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    shouldQuit = true;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    (void)SDL_GetMouseState(&targetPos.x, &targetPos.y);
                    CalculateCoefs(xCoefs, entityPos.x, targetPos.x, Derivative(xCoefs, t));
                    CalculateCoefs(yCoefs, entityPos.y, targetPos.y, Derivative(yCoefs, t));
                    SetPath(path, xCoefs, yCoefs);
                    pathIndex = 0;
                    initialTime = currentTime;
                    t = 0.0f;
                    break;
                default:
                    break;
            }
        }

        entityPos.x = Cubic(xCoefs, t);
        entityPos.y = Cubic(yCoefs, t);

        while ((float)pathIndex * PATH_DELTA < t && pathIndex + 1 < PATH_RESOLUTION) {
            ++pathIndex;
        }
        path[pathIndex] = entityPos;

        if (!Draw(renderer, entityPos, targetPos, path + pathIndex, PATH_RESOLUTION - pathIndex))
            shouldQuit = true;
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
