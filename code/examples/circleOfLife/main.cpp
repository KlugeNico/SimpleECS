#if defined(__GNUC__) // GNU compiler
    #include <unistd.h>
#elif defined(_MSC_VER) // Microsoft compiler
    #include <windows.h>
#else
    error define your copiler
#endif

#include <iostream>

#include "../../ecs/RealTimeEcs.h"

void sleep(int sleepMs) {
#if defined(__GNUC__)
    usleep(sleepMs * 1000);   // usleep takes sleep time in us (1 millionth of a second)
#elif defined(_MSC_VER)
    Sleep(sleepMs);
#endif
}

#define MAP_SIZE 80
std::vector<RtEcs::Entity> map[MAP_SIZE];

///////////////////////////////////////////////////////////////////
////////////////         COMPONENTS          //////////////////////
///////////////////////////////////////////////////////////////////

struct Position {
    Position() : x(0) {}
    explicit Position(int x) : x(x) {}
    float x;
};

struct Appearance {
    Appearance() : character(' ') {}
    explicit Appearance(int character) : character(character) {}
    char character;
};

struct Move {
    float velocity;
};


///////////////////////////////////////////////////////////////////
//////////////////         SYSTEMS          ///////////////////////
///////////////////////////////////////////////////////////////////

class RenderSystem : public RtEcs::IntervalSystem<Appearance, Position> {

public:
    RenderSystem(EcsCore::uint32 intervals) : IntervalSystem(intervals) {}

    void start(float delta) override {
        for (char & i : charMap)
            i = ' ';
    }

    void update(RtEcs::Entity entity, float delta) override {
        charMap[(int)entity.getComponent<Position>()->x] = entity.getComponent<Appearance>()->character;
    }

    void end(float delta) override {
        std::cout << charMap << "\n";
    }

private:
    char charMap[MAP_SIZE];

};


class MoveSystem : public RtEcs::IntervalSystem<Position, Move> {

public:
    MoveSystem(EcsCore::uint32 intervals) : IntervalSystem(intervals) {}

    void update(RtEcs::Entity entity, float delta) override {
        auto* pos = entity.getComponent<Position>();
        auto* move = entity.getComponent<Move>();
        pos->x += move->velocity;
        if (pos->x >= MAP_SIZE) {
            pos->x = 2 * MAP_SIZE - pos->x - 0.1;
            move->velocity *= -1;
        } else if (pos->x < 0) {
            pos->x *= -1;
            move->velocity *= -1;
        }
    }

};


///////////////////////////////////////////////////////////////////
///////////////////   SETUP AND MAIN LOOP    //////////////////////
///////////////////////////////////////////////////////////////////

int main() {

    RtEcs::INIT_DELTA = 1;
    RtEcs::RtEcs ecs = RtEcs::RtEcs(1000, 10);

    ecs.registerComponent<Position>("position");
    ecs.registerComponent<Appearance>("appearance");
    ecs.registerComponent<Move>("move");

    ecs.addSystem(new RenderSystem(1));
    ecs.addSystem(new MoveSystem(1));

    for (int i = 1; i < MAP_SIZE; i = i * 2) {
        RtEcs::Entity tree = ecs.createEntity();
        tree.addComponents(new Position(i), new Appearance('T'));
    }

    while(true) {
        ecs.update(1);
        sleep(1000);
    }

}

