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
struct Listener : public Receiver<Listener> {
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

/*
TEST_F(BenchmarkFixture, TestEntityIterationUnpackTwo) {
  int count = COUNT;
  for (int i = 0; i < count; i++) {
    auto e = em.create();
    e.assign<Position>();
    e.assign<Direction>();
  }

  AutoTimer t;
  cout << "iterating over " << count << " entities, unpacking two components" << endl;

  ComponentHandle<Position> position;
  ComponentHandle<Direction> direction;
  for (auto e : em.entities_with_components(position, direction)) {
    (void)e;
  }
}

TEST_F(BenchmarkFixture, TestEntityIterationUnpackTen) {
    int count = COUNT;
    for (int i = 0; i < count; i++) {
        auto e = em.create();
        e.assign<Position>();
        e.assign<Direction>();
        e.assign<C3>();
        e.assign<C4>();
        e.assign<C5>();
        e.assign<C6>();
        e.assign<C7>();
        e.assign<C8>();
        e.assign<C9>();
        e.assign<C10>();
    }

    AutoTimer t;
    cout << "iterating over " << count << " entities, unpacking ten components" << endl;

    ComponentHandle<Position> position;
    ComponentHandle<Direction> direction;
    ComponentHandle<C3> C3;
    ComponentHandle<C4> C4;
    ComponentHandle<C5> C5;
    ComponentHandle<C6> C6;
    ComponentHandle<C7> C7;
    ComponentHandle<C8> C8;
    ComponentHandle<C9> C9;
    ComponentHandle<C10> C10;
    for (auto e : em.entities_with_components(position, direction, C3, C4, C5, C6, C7, C8, C9, C10)) {
        (void)e;
    }
}
    */
