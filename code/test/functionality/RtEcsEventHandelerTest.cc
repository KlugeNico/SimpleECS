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

class SomeReceiver : public Listener <SomeEvent>, public Listener <SomeOtherEvent> {
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