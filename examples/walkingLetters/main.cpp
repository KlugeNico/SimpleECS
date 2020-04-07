#include <iostream>
#include <SimpleECS/TypeWrapper.h>


// The length of the map. It's a 1-dimensional, as a string represented map.
#define MAP_SIZE 80

///////////////////////////////////////////////////////////////////
////////////////         COMPONENTS          //////////////////////
///////////////////////////////////////////////////////////////////

struct Position {
    // IMPORTANT! We need an empty argument constructor.
    Position() : x(0) {}
    // This constructor helps us to make a inline construction.
    explicit Position(int x) : x((float)x) {}
    // We only have one coordinate, because it's a 1-dimensional world.
    float x;
};

// A component to make the Entity visible and assign a letter.
struct Appearance {
    Appearance() : character(' ') {}
    explicit Appearance(char character) : character(character) {}
    char character;
};

// I want some entities to be able to walk around.
struct Move {
    Move() : velocity(0) {}
    explicit Move(float velocity) : velocity(velocity) {}
    float velocity;
};


///////////////////////////////////////////////////////////////////
//////////////////         SYSTEMS          ///////////////////////
///////////////////////////////////////////////////////////////////

// A system to render all visible and located entities.
class RenderSystem : public sEcs::IntervalSystem<Appearance, Position> {

public:
    // intervals: defines over how many Frames the entities will be updated. An argument of 2 would mean half of
    // the entities will be updated every first frame and the rest every second frame. So every 2 frames every
    // entity gets updated. The order of the entities is NON-DETERMINISTIC! (ehmmm... yes it is, but you can not
    // get control over it.)
    explicit RenderSystem(sEcs::uint32 intervals) : IntervalSystem(intervals) {}

    // This method will be called first, before any entity gets updated. We use it here to clean the map string.
    void start(float delta) override {
        for (char & i : charMap)
            i = ' ';
    }

    // Then this method is called for every entity with a Appearance and Position component.
    // We use it here to add every entities letter to the map.
    void update(sEcs::Entity entity, float delta) override {
        charMap[(int)entity.getComponent<Position>()->x] = entity.getComponent<Appearance>()->character;
    }

    // This method will be called in the end, after all Entities got updated. We use it here to print our world.
    void end(float delta) override {
        std::cout << charMap << "\n";
    }

private:
    // This is the string, which represents our world.
    char charMap[MAP_SIZE]{};

};


// A system to move all movable entities.
class MoveSystem : public sEcs::IntervalSystem<Position, Move> {

public:
    explicit MoveSystem(sEcs::uint32 intervals) : IntervalSystem(intervals) {}

    // We don't need the start and end method.

    void update(sEcs::Entity entity, float delta) override {
        // At first we access the components of our entities.
        auto* pos = entity.getComponent<Position>();
        auto* move = entity.getComponent<Move>();

        // Move the entity
        pos->x += move->velocity;

        // and then check if it has reached the border of the map. In case, it changes direction.
        if (pos->x >= MAP_SIZE) {
            pos->x = MAP_SIZE * 2.f - pos->x -0.1;   // -0.1 for safety to avoid out of bounds. not important.
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

    // Here we init the ecs
    sEcs::EcsManager ecs;

    // Here we initialize the TypeWrapper so we have easy Type-Based access to the ecs
    sEcs::initTypeManaging(ecs);

    // Here we register our components. We could include them in an extern library and register them with the same name,
    // to access them from the external library. Registering different components with the same name in two libraries
    // ends up in undefined behavior.
    sEcs::registerComponent<Position>();
    sEcs::registerComponent<Appearance>();
    sEcs::registerComponent<Move>();

    // Here we add the systems to the ECS. They will be run in the order we add them.
    // We want them to run every frame completely, so we pass 1 as argument.
    sEcs::addSystem(std::make_shared<RenderSystem>(1));
    sEcs::addSystem(std::make_shared<MoveSystem>(1));

    // We add a tree at any x^2 position.
    for (int i = 1; i < MAP_SIZE; i = i * 2) {
        sEcs::Entity tree = sEcs::createEntity();
        tree.addComponents(Position(i), Appearance('T'));
    }

    // And a walking letter every 10th position.
    for (int i = 1; i < MAP_SIZE; i += 10) {
        sEcs::Entity guy = sEcs::createEntity();
        guy.addComponents(Position(i), Appearance((char)('A' + i / 10)), Move((float)i / 40));
    }

    // Update the system, press enter in the terminal to make next step.
    do {
        // Here we just use 1 as delta. In a real time game you should pass delta (1 / Frames per second).
        // The IntervalSystem will automatically calculate the correct delta.
        ecs.update(1);
    } while (getchar() == 10); // input any key and press enter to exit.

}

