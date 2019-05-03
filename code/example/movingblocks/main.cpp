#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>
#include <random>
#include <forward_list>
#include "../../ecs/RealTimeEcs.h"

using EcsCore::Entity_Id;

static const int TILE_SCALING = 1;

static const int TILE_SIZE = 32 * TILE_SCALING;

static const int WORLD_WIDTH = 512 / TILE_SCALING;
static const int WORLD_HEIGHT = 512 / TILE_SCALING;

static const int SCREEN_WIDTH = 1024;
static const int SCREEN_HEIGHT = 768;

static const int TREE_SPAWN = (WORLD_WIDTH * WORLD_HEIGHT) * ((TILE_SCALING*TILE_SCALING) / 8.0f);

static const double PLAYER_SPEED = 200;
static const double PLAYER_ACCELERATION = 400;
static const int PLAYER_VISION = 640;

static const int FOLLOWER_SPAWN = (WORLD_WIDTH * WORLD_HEIGHT) * ((TILE_SCALING*TILE_SCALING) / 128.0f);
static const int FOLLOWER_PERCEPTION = 320;

static EcsCore::uint32 MAX_ENTITIES = 10000000;
static EcsCore::uint32 MAX_COMPONENTS = 256;

static bool quit = false;

static SDL_Window* window;
static SDL_Renderer* renderer;
static RtEcs::RtEcs* rtEcs;


///////////////////////////////////////////////////////////////////
/////////////////       HELPER CLASSES       //////////////////////
///////////////////////////////////////////////////////////////////

class RandomNumberGenerator {
public:
    RandomNumberGenerator(int start, int end) : mt(rd()) {
        random = std::uniform_int_distribution<>(start, end);
    }
    int get() {
        return random(mt);
    }
private:
    std::random_device rd;
    std::mt19937 mt;
    std::uniform_int_distribution<> random;
};


///////////////////////////////////////////////////////////////////
////////////////         STRUCTURES          //////////////////////
///////////////////////////////////////////////////////////////////

struct Position;

struct World {

    World (int width, int height, int tileSize) : width(width), height(height), tileSize(tileSize) {
        entities.reserve(width * height);
    }

    int width;
    int height;
    int tileSize;

    int pixWidth() {
        return width * tileSize;
    }

    int pixHeight() {
        return width * tileSize;
    }

    std::forward_list<Entity_Id>* getEntities(int tileX, int tileY) {
        return &entities[tileY * width + tileX % width];
    }

private:
    std::vector<std::forward_list<Entity_Id>> entities;

};
World* world;


///////////////////////////////////////////////////////////////////
////////////////         COMPONENTS          //////////////////////
///////////////////////////////////////////////////////////////////

struct Player {

};


struct Position {

public:
    Position()= default;;
    Position (Entity_Id entityId, int x, int y) : entityId(entityId), x_(x), y_(y) {
        corral();
        world->getEntities(xInt() / world->tileSize, yInt() / world->tileSize)->push_front(entityId);
    }

    void move(double xMove, double yMove) {
        int tileX = xInt() / world->tileSize;
        int tileY = yInt() / world->tileSize;
        x_ += xMove;
        y_ += yMove;
        corral();
        int newTileX = xInt() / world->tileSize;
        int newTileY = yInt() / world->tileSize;
        if (tileX != newTileX || tileY != newTileY) {
            world->getEntities(tileX, tileY)->remove(entityId);
            world->getEntities(newTileX, newTileY)->push_front(entityId);
        }
    }

    bool inRange(Position *otherPosition, float distance) {
        return (x() - otherPosition->x()) * (x() - otherPosition->x()) +
               (y() - otherPosition->y()) * (y() - otherPosition->y()) <= distance * distance;
    }

    float directionTo(Position *target) {
        return atan2(target->x() - x(), target->y() - y());
    }

    std::vector<Entity_Id> getPotentiallyNearbyEntities(RtEcs::System *system, float distance = 0) {

        int tilesRadius = (int)distance / world->tileSize + 1;

        int startTileX = xTile() - tilesRadius;
        int startTileY = yTile() - tilesRadius;
        int endTileX = xTile() + tilesRadius;
        int endTileY = yTile() + tilesRadius;

        if (startTileX < 0) startTileX = 0;
        if (startTileY < 0) startTileY = 0;
        if (endTileX >= world->width) endTileX = world->width - 1;
        if (endTileY >= world->height) endTileY = world->height - 1;

        std::vector<Entity_Id> entities;
        float d2 = distance * distance;
        for (int x = startTileX; x <= endTileX; x++) {
            for (int y = startTileY; y <= endTileY; y++) {
                for (Entity_Id oEntityId : *world->getEntities(x, y)) {
                    entities.push_back(oEntityId);
                }
            }
        }
        return entities;
    }


    double x() const { return x_; }
    double y() const { return y_; }

    int xInt() const { return (int)x_; }
    int yInt() const { return (int)y_; }

    int xTile() const { return (int)x_ / world->tileSize; }
    int yTile() const { return (int)y_ / world->tileSize; }

    Entity_Id getEntityId() const { return entityId; }

private:
    double x_ = 0;
    double y_ = 0;
    Entity_Id entityId = 0;

    void corral() {
        if (x_ < 0)
            x_ = 0;
        else if (x_ >= world->width * world->tileSize)
            x_ = world->width * world->tileSize - 1;
        if (y_ < 0)
            y_ = 0;
        else if (y_ >= world->height * world->tileSize)
            y_ = world->height * world->tileSize - 1;
    }

};


struct Movement {

    Movement()= default;
    explicit Movement(float maxVelocity) : maxVelocity(maxVelocity) {}

    void accelerate(float acceleration, float direction, double delta) {
        x_ += std::sin(direction) * acceleration * delta;
        y_ += std::cos(direction) * acceleration * delta;

        float velocity2 = x_ * x_ + y_ * y_;
        if (velocity2 > maxVelocity * maxVelocity) {
            float rate = (maxVelocity) / std::sqrt(velocity2);
            x_ *= rate;
            y_ *= rate;
        }
    }

    void brake(float value, double delta) {
        float velocity2 = x_ * x_ + y_ * y_;
        if (velocity2 < (value * delta) * (value * delta)) {
            x_ = 0;
            y_ = 0;
        }
        else {
            float rate = (value * delta) / std::sqrt(velocity2);
            if (rate > 0) {
                x_ -= rate * x_;
                y_ -= rate * y_;
            }
        }
    }

    float x() const { return x_; }
    float y() const { return y_; }

    float getSpeed2() {
        return x_ * x_ + y_ * y_;
    }

private:
    float x_ = 0;
    float y_ = 0;
    float maxVelocity = 0;
};


struct Body {

    Body()= default;;
    Body (int size, Uint8 red, Uint8 green, Uint8 blue)
    : size(size), red(red), green(green), blue(blue) {}

    int size = 0;
    Uint8 red = 0;
    Uint8 green = 0;
    Uint8 blue = 0;

};


struct Perception {

    Perception()= default;
    explicit Perception(int visionDistance)
    : visionDistance(visionDistance) {}

    int visionDistance = 0;
    std::vector<Entity_Id> inVision;

};


struct KI {

    enum TYPE { NONE, FOLLOWER };

    KI()= default;
    explicit KI(TYPE type) : type(type) {}

    TYPE type = NONE;

};


///////////////////////////////////////////////////////////////////
//////////////////         SYSTEMS          ///////////////////////
///////////////////////////////////////////////////////////////////

class PlayerSystem : public RtEcs::System {

public:
    explicit PlayerSystem (RtEcs::Entity player) : player(player) {}

    RtEcs::Entity player;

    void update(double delta) override {
        //Handle events on queue
        SDL_Event e;
        while( SDL_PollEvent( &e ) != 0 ) {
            //User requests quit
            if( e.type == SDL_QUIT ) {
                std::cout << "Quit";
                quit = true;
            }
        }

        // Get Player Components
        auto* p = player.getComponent<Position>();
        auto* b = player.getComponent<Body>();
        auto* perception = player.getComponent<Perception>();
        auto* movement = player.getComponent<Movement>();

        // Move Player based on current keystate
        const Uint8* currentKeyStates = SDL_GetKeyboardState( NULL );
        if( currentKeyStates[ SDL_SCANCODE_UP ] )
        {
            movement->accelerate(PLAYER_ACCELERATION, M_PI, delta);
        }
        if( currentKeyStates[ SDL_SCANCODE_DOWN ] )
        {
            movement->accelerate(PLAYER_ACCELERATION, 0, delta);
        }
        if( currentKeyStates[ SDL_SCANCODE_LEFT ] )
        {
            movement->accelerate(PLAYER_ACCELERATION, 1.5 * M_PI, delta);
        }
        if( currentKeyStates[ SDL_SCANCODE_RIGHT ] )
        {
            movement->accelerate(PLAYER_ACCELERATION, .5 * M_PI, delta);
        }
        movement->brake(PLAYER_ACCELERATION / 2, delta);

        int camX = p->xInt() - SCREEN_WIDTH / 2;
        int camY = p->yInt() - SCREEN_HEIGHT / 2;

        //Clear screen
        SDL_SetRenderDrawColor( renderer, 0xFF, 0xFF, 0xFF, 0xFF );
        SDL_RenderClear( renderer );

        // Draw Grid
        SDL_SetRenderDrawColor( renderer, 0, 0, 0, 0xFF );
        int startX = -camX; if (startX < 0) startX = 0;
        int startY = -camY; if (startY < 0) startY = 0;
        int endX = world->pixWidth() - camX; if (endX > SCREEN_WIDTH) endX = SCREEN_WIDTH;
        int endY = world->pixHeight() - camY; if (endY > SCREEN_HEIGHT) endY = SCREEN_HEIGHT;
        // Verticals
        for (int i = 0; i < SCREEN_WIDTH / world->tileSize + 2; i++) {
            int x = i * world->tileSize - camX % world->tileSize;
            if (x >= startX && x <= endX)
                SDL_RenderDrawLine(renderer, x, startY, x, endY);
        }
        // Horizontals
        for (int i = 0; i < SCREEN_HEIGHT / world->tileSize + 2; i++) {
            int y = i * world->tileSize - camY % world->tileSize;
            if (y >= startY && y <= endY)
                SDL_RenderDrawLine(renderer, startX, y, endX, y);
        }

        // Draw all Creatures in Vision (also itself)
        for (Entity_Id entityId : perception->inVision) {
            RtEcs::Entity other = getEntity(entityId);
            auto* oP = other.getComponent<Position>();
            auto* oB = other.getComponent<Body>();
            if (oP != nullptr && oB != nullptr) {
                SDL_Rect oFillRect = { oP->xInt() - oB->size/2 - camX, oP->yInt() - oB->size/2 - camY, oB->size, oB->size };
                SDL_SetRenderDrawColor( renderer, oB->red, oB->green, oB->blue, 0xFF );
                SDL_RenderFillRect( renderer, &oFillRect );
            }
        }

        SDL_RenderPresent( renderer );
    }
};


class MoveSystem : public RtEcs::IntervalSystem<Movement, Position> {

public:
    MoveSystem(EcsCore::uint32 intervals, double initDelta) : IntervalSystem(intervals, initDelta) {}

private:

    void update(RtEcs::Entity entity, double delta) override {

        auto* position = entity.getComponent<Position>();
        auto* movement = entity.getComponent<Movement>();

        position->move(movement->x() * delta, movement->y() * delta);
    }
};


class CollisionSystem : public RtEcs::IntervalSystem<Movement, Position, Body> {

public:
    CollisionSystem(EcsCore::uint32 intervals, double initDelta) : IntervalSystem(intervals, initDelta) {}

private:

    void update(RtEcs::Entity entity, double delta) override {

        auto* position = entity.getComponent<Position>();
        auto* body = entity.getComponent<Body>();

        for (Entity_Id entityId : position->getPotentiallyNearbyEntities(this)) {
            auto oPos = getComponent<Position>(entityId);
            auto oBody = getComponent<Body>(entityId);

            if (oPos != nullptr && oBody != nullptr && position != oPos) {
                double disX = oPos->x() - position->x();
                double disY = oPos->y() - position->y();
                double minDis = (body->size + oBody->size) / 2.0f;

                if (std::abs(disX) < minDis && std::abs(disY) < minDis) {
                    if (std::abs(disX) > std::abs(disY)) {
                        if (disX > 0) disX = -minDis + disX;
                        else disX = minDis + disX;
                        disY = 0;
                    }
                    else {
                        if (disY > 0) disY = -minDis + disY;
                        else disY = minDis + disY;
                        disX = 0;
                    }

                    double mul = 1;
                    if (getComponent<Movement>(entityId) != nullptr)
                        mul = 0.5;

                    position->move(disX * mul, disY * mul);
                }
            }
        }
    }
};


class PerceptionSystem : public RtEcs::IntervalSystem<Perception, Position> {

public:
    PerceptionSystem(EcsCore::uint32 intervals, double initDelta) : IntervalSystem(intervals, initDelta) {}

    void update(RtEcs::Entity entity, double delta) override {
        auto* position = entity.getComponent<Position>();
        auto* perception = entity.getComponent<Perception>();

        perception->inVision.clear();
        for (Entity_Id entityId : position->getPotentiallyNearbyEntities(this, perception->visionDistance)) {
            auto oPos = getEntity(entityId).getComponent<Position>();
            if (oPos != nullptr && position->inRange(oPos, perception->visionDistance))
                perception->inVision.push_back(entityId);
        }
    }
};


class KISystem : public RtEcs::IntervalSystem<KI, Perception, Position, Movement> {

public:
    KISystem(EcsCore::uint32 intervals, double initDelta) : IntervalSystem(intervals, initDelta) {}

public:

    void update(RtEcs::Entity entity, double delta) override {
        auto* ki = entity.getComponent<KI>();
        auto* perception = entity.getComponent<Perception>();
        auto* position = entity.getComponent<Position>();
        auto* movement = entity.getComponent<Movement>();

        switch (ki->type) {
            case KI::FOLLOWER:

                for (Entity_Id entityId : perception->inVision) {
                    auto* oMov = getComponent<Movement>(entityId);
                    auto* oPos = getComponent<Position>(entityId);
                    if (oMov != nullptr && oPos != nullptr && oMov->getSpeed2() > movement->getSpeed2()) {
                        movement->accelerate(100, position->directionTo(oPos), delta);
                    }
                }

                break;
            case KI::NONE:break;
        }
    }
};


///////////////////////////////////////////////////////////////////
//////////////////      INITIALIZATION       //////////////////////
///////////////////////////////////////////////////////////////////

void initSdl() {

    SDL_Init(SDL_INIT_EVERYTHING);

    window = SDL_CreateWindow(
            "TextWindow", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

}

void initRtEcs() {

    rtEcs = new RtEcs::RtEcs (MAX_ENTITIES, MAX_COMPONENTS);

    rtEcs->registerComponent<Player>("player");
    rtEcs->registerComponent<Position>("position");
    rtEcs->registerComponent<Body>("body");
    rtEcs->registerComponent<Perception>("perception");
    rtEcs->registerComponent<Movement>("movement");
    rtEcs->registerComponent<KI>("ki");

    world = new World(WORLD_WIDTH, WORLD_HEIGHT, TILE_SIZE);

    RtEcs::Entity player = rtEcs->createEntity();

    player.addComponent(new Player);
    player.addComponent(new Position(player.getId(), WORLD_WIDTH * 64, WORLD_HEIGHT * 64));
    player.addComponent(new Body(20, 0xFF, 0x00, 0x00));
    player.addComponent(new Perception(PLAYER_VISION));
    player.addComponent(new Movement(PLAYER_SPEED));

    RandomNumberGenerator randX(1, world->pixWidth() - 1);
    RandomNumberGenerator randY(1, world->pixHeight() - 1);

    for (int i = 0; i < TREE_SPAWN; i++) {
        RtEcs::Entity tree = rtEcs->createEntity();
        tree.addComponent(new Position(tree.getId(), randX.get(), randY.get()));
        tree.addComponent(new Body(20, 32, 128, 16));
    }

    for (int i = 0; i < FOLLOWER_SPAWN; i++) {
        RtEcs::Entity follower = rtEcs->createEntity();
        follower.addComponent(new Position(follower.getId(), randX.get(), randY.get()));
        follower.addComponent(new Body(20, 32, 32, 128));
        follower.addComponent(new Perception(FOLLOWER_PERCEPTION));
        follower.addComponent(new Movement(PLAYER_SPEED));
        follower.addComponent(new KI(KI::FOLLOWER));
    }

    rtEcs->addSystem(new PlayerSystem(player));
    rtEcs->addSystem(new PerceptionSystem(10, 0.1f));
    rtEcs->addSystem(new MoveSystem(1, 0.1f));
    rtEcs->addSystem(new CollisionSystem(1, 0.1f));
    rtEcs->addSystem(new KISystem(8, 0.1f));

}

void closeApp()
{
    //Destroy window
    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );

    //Quit SDL subsystems
    SDL_Quit();
}


///////////////////////////////////////////////////////////////////
//////////////////         MAIN LOOP         //////////////////////
///////////////////////////////////////////////////////////////////

double getDelta(Uint32 ticks) {
    static Uint32 lastTicks = ticks;
    double delta = (ticks - lastTicks) / 1000.;
    lastTicks = ticks;
    return delta;
}

int main() {

    initSdl();

    initRtEcs();

    while(!quit) {
        rtEcs->update(getDelta(SDL_GetTicks()));
    }

    closeApp();

}


