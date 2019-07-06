#include "../ecs/Core.h"
#include "Timer.h"
#include "gtest/gtest.h"

using std::cout;
using std::endl;
using std::vector;

using namespace EcsCore;

uint32 COUNT = 10000000L;

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
    BenchmarkFixture() : manager(Manager(COUNT, 64)) {}
    Manager manager;
};


TEST_F(BenchmarkFixture, TestCreateEntities) {
    AutoTimer t;

    cout << "creating " << COUNT << " entities" << endl;

    for (uint64 i = 0; i < COUNT; i++) {
        manager.createEntity();
    }
}


TEST_F(BenchmarkFixture, TestDestroyEntities) {
  vector<uint64> entities;
  for (uint64 i = 0; i < COUNT; i++) {
    entities.push_back(manager.createEntity());
  }

  AutoTimer t;
  cout << "destroying " << COUNT << " entities" << endl;

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
    int count = COUNT;
    manager.registerComponent<Position>(Component_Key("Position"));
    vector<uint64> entities;

    for (int i = 0; i < count; i++) {
        auto e = manager.createEntity();
        manager.addComponents<Position>(e, new Position());
        entities.push_back(e);
    }

    AutoTimer t;
    cout << "iterating over " << count << " entities, unpacking one component" << endl;

    for (auto e : entities) {
        manager.getComponent<Position>(e);
    }
}

TEST_F(BenchmarkFixture, TestEntityIterationUnpackTwo) {
    int count = COUNT;
    manager.registerComponent<Position>(Component_Key("Position"));
    manager.registerComponent<Direction>(Component_Key("Direction"));
    vector<uint64> entities;

    for (int i = 0; i < count; i++) {
        auto e = manager.createEntity();
        manager.addComponents<Position>(e, new Position());
        manager.addComponents<Direction>(e, new Direction());
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
    int count = COUNT;
    manager.registerComponent<Position>(Component_Key("Position"));
    manager.registerComponent<Direction>(Component_Key("Direction"));
    manager.registerComponent<C3>(Component_Key("c3"));
    manager.registerComponent<C4>(Component_Key("c4"));
    manager.registerComponent<C5>(Component_Key("c5"));
    manager.registerComponent<C6>(Component_Key("c6"));
    manager.registerComponent<C7>(Component_Key("c7"));
    manager.registerComponent<C8>(Component_Key("c8"));
    manager.registerComponent<C9>(Component_Key("c9"));
    manager.registerComponent<C10>(Component_Key("c10"));
    vector<uint64> entities;

    for (int i = 0; i < count; i++) {
        auto e = manager.createEntity();
        manager.addComponents<Position>(e, new Position());
        manager.addComponents<Direction>(e, new Direction());
        manager.addComponents(e, new C3());
        manager.addComponents(e, new C4());
        manager.addComponents(e, new C5());
        manager.addComponents(e, new C6());
        manager.addComponents(e, new C7());
        manager.addComponents(e, new C8());
        manager.addComponents(e, new C9());
        manager.addComponents(e, new C10());
        entities.push_back(e);
    }

    AutoTimer t;
    cout << "iterating over " << count << " entities, unpacking two components" << endl;

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
