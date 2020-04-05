#define MAX_ENTITY_AMOUNT 10000000
#define MAX_COMPONENT_AMOUNT 31

#include <SimpleECS/Core.h>
#include "Timer.h"
#include "gtest/gtest.h"

using std::cout;
using std::endl;
using std::vector;

using namespace sEcs;

struct AutoTimer {
    ~AutoTimer() {
        cout << timer_.elapsed() << " seconds elapsed" << endl;
}

private:
    Timer timer_;
};

/*
struct Listener : public Listener<Listener> {
    void receive(const EntityCreatedEvent &event) { ++created; }
    void receive(const EntityDestroyedEvent &event) { ++destroyed; }

    int created = 0;
    int destroyed = 0;
};
*/

struct Position {
};

struct Direction {
};

struct C3 {
};

struct C4 {
};

struct C5 {
};

struct C6 {
};

struct C7 {
};

struct C8 {
};

struct C9 {
};

struct C10 {
};

class BenchmarkFixture : public ::testing::Test {
protected:
    BenchmarkFixture() : manager() {
    }
    Core manager;
};


TEST_F(BenchmarkFixture, TestCreateEntities) {
    AutoTimer t;

    cout << "creating " << MAX_ENTITY_AMOUNT << " entities" << endl;

    for (uint64 i = 0; i < MAX_ENTITY_AMOUNT; i++) {
        manager.createEntity();
    }
}


TEST_F(BenchmarkFixture, TestDestroyEntities) {
  vector<sEcs::EntityId> entities;
  for (uint64 i = 0; i < MAX_ENTITY_AMOUNT; i++) {
    entities.push_back(manager.createEntity());
  }

  AutoTimer t;
  cout << "destroying " << MAX_ENTITY_AMOUNT << " entities" << endl;

  for (auto e : entities) {
      manager.eraseEntity(e);
  }
}

/*
TEST_F(BenchmarkFixture, TestCreateEntitiesWithListener) {
  Listener listen;
  ev.subscribe<EntityCreatedEvent>(listen);

  int count = COUNT;

  AutoTimer t;
  cout << "creating " << count << " entities while notifying a single EntityCreatedEvent listener" << endl;

  vector<Entity> entities;
  for (int i = 0; i < count; i++) {
    entities.push_back(em.create());
  }

  REQUIRE(entities.size() == count);
  REQUIRE(listen.created == count);
}

TEST_F(BenchmarkFixture, TestDestroyEntitiesWithListener) {
  int count = COUNT;
  vector<Entity> entities;
  for (int i = 0; i < count; i++) {
    entities.push_back(em.create());
  }

  Listener listen;
  ev.subscribe<EntityDestroyedEvent>(listen);

  AutoTimer t;
  cout << "destroying " << count << " entities while notifying a single EntityDestroyedEvent listener" << endl;

  for (auto &e : entities) {
    e.destroy();
  }

  REQUIRE(entities.size() == count);
  REQUIRE(listen.destroyed == count);
}
*/


TEST_F(BenchmarkFixture, TestEntityIteration) {
    int count = MAX_ENTITY_AMOUNT;
    manager.registerComponent<Position>(Key("Position"));
    vector<sEcs::EntityId> entities;

    for (int i = 0; i < count; i++) {
        auto e = manager.createEntity();
        manager.addComponents<Position>(e, Position());
        entities.push_back(e);
    }

    AutoTimer t;
    cout << "iterating over " << count << " entities, unpacking one component" << endl;

    for (auto e : entities) {
        manager.getComponent<Position>(e);
    }
}

TEST_F(BenchmarkFixture, TestEntityIterationUnpackTwo) {
    int count = MAX_ENTITY_AMOUNT;
    manager.registerComponent<Position>(Key("Position"));
    manager.registerComponent<Direction>(Key("Direction"));
    vector<sEcs::EntityId> entities;

    for (int i = 0; i < count; i++) {
        auto e = manager.createEntity();
        manager.addComponents<Position>(e, Position());
        manager.addComponents<Direction>(e, Direction());
        entities.push_back(e);
    }

    AutoTimer t;
    cout << "iterating over " << count << " entities, unpacking two components" << endl;

    for (auto e : entities) {
        manager.getComponent<Position>(e);
        manager.getComponent<Direction>(e);
    }
}

TEST_F(BenchmarkFixture, TestEntityIterationUnpackTen) {
    int count = MAX_ENTITY_AMOUNT;
    manager.registerComponent<Position>(Key("Position"));
    manager.registerComponent<Direction>(Key("Direction"));
    manager.registerComponent<C3>(Key("c3"));
    manager.registerComponent<C4>(Key("c4"));
    manager.registerComponent<C5>(Key("c5"));
    manager.registerComponent<C6>(Key("c6"));
    manager.registerComponent<C7>(Key("c7"));
    manager.registerComponent<C8>(Key("c8"));
    manager.registerComponent<C9>(Key("c9"));
    manager.registerComponent<C10>(Key("c10"));
    vector<sEcs::EntityId> entities;

    for (int i = 0; i < count; i++) {
        auto e = manager.createEntity();
        manager.addComponents<Position>(e, Position());
        manager.addComponents<Direction>(e, Direction());
        manager.addComponents(e, C3());
        manager.addComponents(e, C4());
        manager.addComponents(e, C5());
        manager.addComponents(e, C6());
        manager.addComponents(e, C7());
        manager.addComponents(e, C8());
        manager.addComponents(e, C9());
        manager.addComponents(e, C10());
        entities.push_back(e);
    }

    AutoTimer t;
    cout << "iterating over " << count << " entities, unpacking 10 components" << endl;

    for (auto e : entities) {
        manager.getComponent<Position>(e);
        manager.getComponent<Direction>(e);
        manager.getComponent<C3>(e);
        manager.getComponent<C4>(e);
        manager.getComponent<C5>(e);
        manager.getComponent<C6>(e);
        manager.getComponent<C7>(e);
        manager.getComponent<C8>(e);
        manager.getComponent<C9>(e);
        manager.getComponent<C10>(e);
    }
}
