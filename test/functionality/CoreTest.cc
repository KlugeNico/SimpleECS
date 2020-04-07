#include <thread>
#include <SimpleECS/Core.h>
#include <SimpleECS/TypeWrapper.h>
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

    Core core;

    std::vector<EntityId> entities = vector<EntityId>(40);

    cout << "Create Manager" << endl;

    ComponentHandle* ph = new PointingComponentHandle( sizeof(Position), [](void *p) { delete reinterpret_cast<Position *>(p); });
    ComponentHandle* sh = new ValuedComponentHandle( sizeof(Size), [](void *p) { reinterpret_cast<Size *>(p)->~Size(); });

    ComponentId p_Id = core.registerComponent(ph);
    ComponentId s_Id = core.registerComponent(sh);

    auto s1_c = std::vector<ComponentId>();
    auto s2_c = std::vector<ComponentId>();

    s1_c.push_back(p_Id);
    s2_c.push_back(s_Id);

    SetIteratorId s1 = core.createSetIterator(s1_c);
    SetIteratorId s2 = core.createSetIterator(s2_c);

    ASSERT_EQ(core.getEntityAmount(), 0);

    for (int i = 0; i < 40; i++) {
        entities[i] = core.createEntity();
    }

    ASSERT_EQ(core.getEntityAmount(), 40);

    for (int i = 0; i < 20; i++) {
        auto position = reinterpret_cast<Position*>( core.addComponent(entities[i], p_Id) );
        position->x = i * 10;
        position->y = 10;
    }

    for (int i = 10; i < 30; i++) {
        auto size = reinterpret_cast<Size*>( core.addComponent(entities[i], s_Id) );
        size->size = i;
    }

    for (int i = 0; i < 20; i++) {
        auto position = reinterpret_cast<Position*>( core.getComponent(entities[i], p_Id) );
        ASSERT_EQ(position->x, i * 10);
        ASSERT_EQ(position->y, 10);
    }

    for (int i = 20; i < 40; i++) {
        ASSERT_TRUE(core.getComponent(entities[i], p_Id) == nullptr);
    }

    for (int i = 10; i < 30; i++) {
        auto size = reinterpret_cast<Size*>( core.getComponent(entities[i], s_Id) );
        ASSERT_EQ(size->size, i);
    }

    core.eraseEntity(entities[10]);
    core.eraseEntity(entities[11]);
    core.eraseEntity(entities[12]);

    ASSERT_EQ(core.getEntityAmount(), 37);

    ASSERT_TRUE(core.getComponent(entities[14], p_Id) != nullptr);
    ASSERT_TRUE(core.getComponent(entities[10], p_Id) == nullptr);
    ASSERT_TRUE(core.getComponent(entities[11], p_Id) == nullptr);
    ASSERT_TRUE(core.getComponent(entities[12], p_Id) == nullptr);

    uint32 amount = 0;
    while (core.nextEntity(s1).index != INVALID) {
        amount++;
    }
    ASSERT_EQ(amount, 7);

    SetIteratorId s3 = core.createSetIterator(s2_c);

    auto size = reinterpret_cast<Size*>( core.addComponent(core.createEntity(), s_Id) );
    size->size = 1;
    core.addComponent(core.createEntity(), p_Id);

    amount = 0;
    while (core.nextEntity(s3).index != INVALID) {
        amount++;
    }
    ASSERT_EQ(amount, 18);

    ASSERT_EQ(core.getEntityAmount(), 39);

}
