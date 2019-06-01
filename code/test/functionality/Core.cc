#include "../../ecs/Core.h"
#include "gtest/gtest.h"

using std::cout;
using std::endl;
using std::vector;

using namespace EcsCore;

uint32 MAX_ENTITIES = 5000;
uint32 MAX_COMPONENTS_MANAGER = 50;

struct Numbered {
    int id;
};

struct Position {
    int x = 0;
    int y_ = 0;
};

struct Size {
    int size = 0;
};


TEST (ManagerTest, TestManagerCreate) {

    Manager manager = Manager(MAX_ENTITIES, MAX_COMPONENTS_MANAGER);

    std::vector<Entity_Id> entities = vector<Entity_Id>(40);

    cout << "Create Manager" << endl;

    manager.registerComponent<Position>("p");
    manager.registerComponent<Size>("s");

    SetIterator_Id s1 = manager.createSetIterator(new Position, new Size);
    SetIterator_Id s2 = manager.createSetIterator(new Position);

    ASSERT_EQ(manager.getEntityAmount(), 0);

    for (int i = 0; i < 40; i++) {
        entities[i] = manager.createEntity();
    }

    ASSERT_EQ(manager.getEntityAmount(), 40);

    for (int i = 0; i < 20; i++) {
        ASSERT_TRUE(manager.addComponents(entities[i], new Position));
        manager.getComponent<Position>(entities[i])->x = i * 10;
        manager.getComponent<Position>(entities[i])->y_ = 10;
    }

    for (int i = 10; i < 30; i++) {
        ASSERT_TRUE(manager.addComponents(entities[i], new Size));
    }

    for (int i = 0; i < 20; i++) {
        auto* position = manager.getComponent<Position>(entities[i]);
        ASSERT_EQ(position->x, i * 10);
        ASSERT_EQ(position->y_, 10);
    }

    for (int i = 20; i < 40; i++) {
        ASSERT_TRUE(manager.getComponent<Position>(entities[i]) == nullptr);
    }

    manager.eraseEntity(entities[15]);
    manager.eraseEntity(entities[16]);
    manager.eraseEntity(entities[17]);

    ASSERT_EQ(manager.getEntityAmount(), 37);

    ASSERT_TRUE(manager.getComponent<Position>(entities[14]) != nullptr);
    ASSERT_TRUE(manager.getComponent<Position>(entities[15]) == nullptr);
    ASSERT_TRUE(manager.getComponent<Position>(entities[16]) == nullptr);
    ASSERT_TRUE(manager.getComponent<Position>(entities[17]) == nullptr);

    uint32 amount = 0;
    while (manager.nextEntity(s1) != INVALID) {
        amount++;
    }
    ASSERT_EQ(amount, 7);

    SetIterator_Id s3 = manager.createSetIterator(new Size);

    manager.addComponent(manager.createEntity(), new Size);
    manager.addComponent(manager.createEntity(), new Position);

    amount = 0;
    while (manager.nextEntity(s3) != INVALID) {
        amount++;
    }
    ASSERT_EQ(amount, 18);

    ASSERT_EQ(manager.getEntityAmount(), 39);

}