#include <thread>
#include "../../ecs/RealTimeEcs.h"
#include "gtest/gtest.h"

using std::cout;
using std::endl;
using std::vector;

using namespace RtEcs;

u_int32_t MAX_ENTITIES = 5000;
u_int32_t MAX_COMPONENTS_MANAGER = 50;

class SomeEvent {};
class SomeOtherEvent {};
class WrongEvent {};

struct SomeComponent {
    int x;
};

class SomeReceiver :
        public Listener <SomeEvent>,
        public Listener <SomeOtherEvent>,
        public Listener <RtEcs::ComponentAddedEvent<SomeComponent>>,
        public Listener <RtEcs::ComponentDeletedEvent<SomeComponent>>,
        public Listener <RtEcs::EntityCreatedEvent>,
        public Listener <RtEcs::EntityErasedEvent> {

public:
    int someEventReceived = 0;
    int someOtherEventReceived = 0;
    int compAddedReceived = 0;
    int compDeletedReceived = 0;
    int entityCreated = 0;
    int entityErased = 0;

    void  receive(const SomeEvent& event) override {
        someEventReceived++;
        cout << "SomeEvent received! Nr. " << someEventReceived << endl;
    };

    void  receive(const SomeOtherEvent& event) override {
        someOtherEventReceived++;
        cout << "SomeOtherEvent received! Nr. " << someOtherEventReceived << endl;
    };

    void  receive(const RtEcs::ComponentAddedEvent<SomeComponent>& event) override {
        compAddedReceived++;
        cout << "SomeComponent added! Nr. " << compAddedReceived << endl;
    };

    void  receive(const RtEcs::ComponentDeletedEvent<SomeComponent>& event) override {
        compDeletedReceived++;
        cout << "SomeComponent deleted! Nr. " << compDeletedReceived << endl;
    };

    void  receive(const RtEcs::EntityCreatedEvent& event) override {
        entityCreated++;
        cout << "Entity created! Nr. " << compAddedReceived << endl;
    };

    void  receive(const RtEcs::EntityErasedEvent& event) override {
        entityErased++;
        cout << "Entity erased! Nr. " << compDeletedReceived << endl;
    };

};

TEST (RtManagerTest, TestRtManagerEventHandler) {

    cout << "Create RtManager" << endl;

    RtManager manager(MAX_ENTITIES);

    cout << "register Events" << endl;

    manager.registerEvent<SomeEvent>("SomeEvent");
    manager.registerEvent<SomeOtherEvent>("SomeOtherEvent");
    manager.registerEvent<WrongEvent>("WrongEvent");

    cout << "add Listener" << endl;

    SomeReceiver receiver;
    manager.subscribeEvent<SomeEvent>(&receiver);
    manager.subscribeEvent<SomeOtherEvent>(&receiver);

    manager.registerComponent<SomeComponent>("comp");

    manager.subscribeEvent<RtEcs::ComponentAddedEvent<SomeComponent>>(&receiver);
    manager.subscribeEvent<RtEcs::ComponentDeletedEvent<SomeComponent>>(&receiver);
    manager.subscribeEvent<RtEcs::EntityCreatedEvent>(&receiver);
    manager.subscribeEvent<RtEcs::EntityErasedEvent>(&receiver);

    ASSERT_EQ(receiver.someEventReceived, 0);

    SomeEvent someEvent = SomeEvent();
    manager.emitEvent(someEvent);

    SomeOtherEvent someOtherEvent = SomeOtherEvent();
    manager.emitEvent(someOtherEvent);

    WrongEvent wrongEvent = WrongEvent();
    manager.emitEvent(wrongEvent);

    ASSERT_EQ(receiver.someEventReceived, 1);
    ASSERT_EQ(receiver.someOtherEventReceived, 1);

    manager.unsubscribeEvent<SomeEvent>(&receiver);
    manager.emitEvent(someEvent);

    ASSERT_EQ(receiver.someEventReceived, 1);

    ASSERT_EQ(receiver.entityCreated, 0);
    Entity entity = manager.createEntity();
    ASSERT_EQ(receiver.entityCreated, 1);

    entity.addComponent(SomeComponent());
    ASSERT_EQ(receiver.compAddedReceived, 1);

    entity.addComponent(SomeComponent());
    ASSERT_EQ(receiver.compAddedReceived, 2);
    ASSERT_EQ(receiver.compDeletedReceived, 1);

    entity.deleteComponent<SomeComponent>();
    ASSERT_EQ(receiver.compDeletedReceived, 2);

    entity.addComponent(SomeComponent());   // + 1
    ASSERT_EQ(receiver.compAddedReceived, 3);

    ASSERT_EQ(receiver.entityErased, 0);
    entity.erase();
    ASSERT_EQ(receiver.compDeletedReceived, 3);
    ASSERT_EQ(receiver.entityErased, 1);

    ASSERT_EQ(receiver.someEventReceived, 1);
    ASSERT_EQ(receiver.someOtherEventReceived, 1);
    ASSERT_EQ(receiver.compAddedReceived, 3);
    ASSERT_EQ(receiver.entityCreated, 1);

}