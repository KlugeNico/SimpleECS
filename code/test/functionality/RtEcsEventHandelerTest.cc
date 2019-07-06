#include "../../ecs/RealTimeEcs.h"
#include "gtest/gtest.h"

using std::cout;
using std::endl;
using std::vector;

using namespace RtEcs;

uint32 MAX_ENTITIES = 5000;
uint32 MAX_COMPONENTS_MANAGER = 50;

class SomeEvent {};
class SomeOtherEvent {};
class WrongEvent {};

class SomeReceiver : public Receiver <SomeEvent>, public Receiver <SomeOtherEvent> {
public:
    int amountReceived = 0;

    void  receive(SomeEvent* event) override {
        amountReceived++;
        cout << "SomeEvent received! Nr. " << amountReceived << endl;
    };

    void  receive(SomeOtherEvent* event) override {
        amountReceived++;
        cout << "SomeOtherEvent received! Nr. " << amountReceived << endl;
    };
};

TEST (RtManagerTest, TestRtManagerEventHandler) {

    cout << "Create RtManager" << endl;

    RtManager manager(MAX_ENTITIES, MAX_COMPONENTS_MANAGER);

    cout << "register Events" << endl;

    manager.registerEvent<SomeEvent>("SomeEvent");
    manager.registerEvent<SomeOtherEvent>("SomeOtherEvent");
    manager.registerEvent<WrongEvent>("WrongEvent");

    cout << "add Listener" << endl;

    SomeReceiver receiver;
    manager.subscribeEvent<SomeEvent>(&receiver);
    manager.subscribeEvent<SomeOtherEvent>(&receiver);

    ASSERT_EQ(receiver.amountReceived, 0);

    SomeEvent someEvent = SomeEvent();
    manager.emitEvent(someEvent);

    SomeOtherEvent someOtherEvent = SomeOtherEvent();
    manager.emitEvent(someOtherEvent);

    WrongEvent wrongEvent = WrongEvent();
    manager.emitEvent(wrongEvent);

    ASSERT_EQ(receiver.amountReceived, 2);

    manager.unsubscribeEvent<SomeEvent>(&receiver);
    manager.emitEvent(someEvent);

    ASSERT_EQ(receiver.amountReceived, 2);

}