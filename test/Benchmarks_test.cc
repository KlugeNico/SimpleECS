#define MAX_ENTITY_AMOUNT 10000000
#define MAX_COMPONENT_AMOUNT 31

#include <SimpleECS/TypeWrapper.h>
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
    EcsManager manager;
};


TEST_F(BenchmarkFixture, TestCreateEntities) {
    sEcs::initTypeManaging(manager);

    AutoTimer t;

    cout << "creating " << MAX_ENTITY_AMOUNT << " entities" << endl;

    for (uint64 i = 0; i < MAX_ENTITY_AMOUNT; i++) {
        sEcs::createEntity();
    }
}


TEST_F(BenchmarkFixture, TestDestroyEntities) {
    sEcs::initTypeManaging(manager);
    vector<sEcs::Entity> entities;
  for (uint64 i = 0; i < MAX_ENTITY_AMOUNT; i++) {
    entities.push_back(sEcs::createEntity());
  }

  AutoTimer t;
  cout << "destroying " << MAX_ENTITY_AMOUNT << " entities" << endl;

  for (auto e : entities) {
      sEcs::eraseEntity(e);
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
    sEcs::initTypeManaging(manager);
    int count = MAX_ENTITY_AMOUNT;
    sEcs::registerComponent<Position>();
    vector<sEcs::Entity> entities;

    for (int i = 0; i < count; i++) {
        auto e = sEcs::createEntity();
        e.addComponent(Position());
        entities.push_back(e);
    }

    AutoTimer t;
    cout << "iterating over " << count << " entities, unpacking one component" << endl;

    for (auto e : entities) {
        e.getComponent<Position>();
    }
}

TEST_F(BenchmarkFixture, TestEntityIterationUnpackTwo) {
    sEcs::initTypeManaging(manager);
    int count = MAX_ENTITY_AMOUNT;
    sEcs::registerComponent<Position>();
    sEcs::registerComponent<Direction>();
    vector<sEcs::Entity> entities;

    for (int i = 0; i < count; i++) {
        auto e = sEcs::createEntity();
        e.addComponent( Position() );
        e.addComponent( Direction());
        entities.push_back(e);
    }

    AutoTimer t;
    cout << "iterating over " << count << " entities, unpacking two components" << endl;

    for (auto e : entities) {
        e.getComponent<Position>();
        e.getComponent<Direction>();
    }
}

TEST_F(BenchmarkFixture, TestEntityIterationUnpackTen) {
    sEcs::initTypeManaging(manager);
    int count = MAX_ENTITY_AMOUNT;
    sEcs::registerComponent<Position>( );
    sEcs::registerComponent<Direction>( );
    sEcs::registerComponent<C3>( );
    sEcs::registerComponent<C4>( );
    sEcs::registerComponent<C5>( );
    sEcs::registerComponent<C6>( );
    sEcs::registerComponent<C7>( );
    sEcs::registerComponent<C8>( );
    sEcs::registerComponent<C9>( );
    sEcs::registerComponent<C10>( );
    vector<sEcs::Entity> entities;

    for (int i = 0; i < count; i++) {
        auto e = sEcs::createEntity();
        e.addComponents(Position(),Direction(),C3(),C4(),C5(),C6(),C7(),C8(),C9(),C10());
        entities.push_back(e);
    }

    AutoTimer t;
    cout << "iterating over " << count << " entities, unpacking 10 components" << endl;

    for (auto e : entities) {
        e.getComponent<Position>();
        e.getComponent<Direction>();
        e.getComponent<C3>();
        e.getComponent<C4>();
        e.getComponent<C5>();
        e.getComponent<C6>();
        e.getComponent<C7>();
        e.getComponent<C8>();
        e.getComponent<C9>();
        e.getComponent<C10>();
    }
}
