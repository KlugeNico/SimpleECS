#include <thread>
#include <SimpleECS/Core.h>
#include "gtest/gtest.h"

using std::cout;
using std::endl;
using std::vector;

using namespace sEcs;

uint32 MAX_ENTITIES_RT = 5000;
const size_t COMPONENT_AMOUNT = 50;

struct Numbered {
    int id;
};

struct Position {
    int x = 0;
    int y = 0;
};

struct Size {
    explicit Size(int size) : size(size) {}
    int size = 0;
};


TEST (ManagerTest, TestManagerCreate) {

    Core manager;

    std::vector<EntityId> entities = vector<EntityId>(40);

    cout << "Create Manager" << endl;

    manager.registerComponent<Position>("p", sEcs::Storing::POINTER);
    manager.registerComponent<Size>("s", sEcs::Storing::VALUE);

    SetIteratorId s1 = manager.createSetIterator<Position, Size>();
    SetIteratorId s2 = manager.createSetIterator<Position>();

    ASSERT_EQ(manager.getEntityAmount(), 0);

    for (int i = 0; i < 40; i++) {
        entities[i] = manager.createEntity();
    }

    ASSERT_EQ(manager.getEntityAmount(), 40);

    for (int i = 0; i < 20; i++) {
        ASSERT_TRUE(manager.addComponents(entities[i], Position()));
        manager.getComponent<Position>(entities[i])->x = i * 10;
        manager.getComponent<Position>(entities[i])->y = 10;
    }

    for (int i = 10; i < 30; i++) {
        ASSERT_TRUE(manager.addComponents(entities[i], Size(i)));
    }

    for (int i = 0; i < 20; i++) {
        auto position = manager.getComponent<Position>(entities[i]);
        ASSERT_EQ(position->x, i * 10);
        ASSERT_EQ(position->y, 10);
    }

    for (int i = 20; i < 40; i++) {
        ASSERT_TRUE(manager.getComponent<Position>(entities[i]) == nullptr);
    }

    for (int i = 10; i < 30; i++) {
        auto* size = manager.getComponent<Size>(entities[i]);
        ASSERT_EQ(size->size, i);
    }

    manager.eraseEntity(entities[10]);
    manager.eraseEntity(entities[11]);
    manager.eraseEntity(entities[12]);

    ASSERT_EQ(manager.getEntityAmount(), 37);

    ASSERT_TRUE(manager.getComponent<Position>(entities[14]) != nullptr);
    ASSERT_TRUE(manager.getComponent<Position>(entities[10]) == nullptr);
    ASSERT_TRUE(manager.getComponent<Position>(entities[11]) == nullptr);
    ASSERT_TRUE(manager.getComponent<Position>(entities[12]) == nullptr);

    uint32 amount = 0;
    while (manager.nextEntity(s1).index != INVALID) {
        amount++;
    }
    ASSERT_EQ(amount, 7);

    SetIteratorId s3 = manager.createSetIterator<Size>();

    manager.addComponent(manager.createEntity(), Size(1));
    manager.addComponent(manager.createEntity(), Position());

    amount = 0;
    while (manager.nextEntity(s3).index != INVALID) {
        amount++;
    }
    ASSERT_EQ(amount, 18);

    ASSERT_EQ(manager.getEntityAmount(), 39);

}
