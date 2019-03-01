#include "../ecs/Manager.h"
#include "gtest/gtest.h"

using std::cout;
using std::endl;
using std::vector;

uint32 COMPONENT_COUNT = 300;

struct Numbered {
    int id;
};


TEST (BitsetTest, TestSetBitset) {

    Bitset bitset = Bitset(COMPONENT_COUNT);

    cout << "Set Bitset" << endl;

    bitset.set(10);
    bitset.set(222);

    std::vector<uint32> v = {22, 150 , 144};
    bitset.set(&v);

    bitset.set(140);

    bitset.unset(144);

    ASSERT_TRUE(bitset.isSet(222));
    ASSERT_TRUE(bitset.isSet(22));
    ASSERT_TRUE(bitset.isSet(150));
    ASSERT_TRUE(bitset.isSet(140));

    ASSERT_TRUE(bitset.isSet(10));
    ASSERT_TRUE(bitset.isSet(222));
    ASSERT_TRUE(bitset.isSet(22));
    ASSERT_TRUE(bitset.isSet(150));
    ASSERT_TRUE(bitset.isSet(140));

    ASSERT_FALSE(bitset.isSet(20));
    ASSERT_FALSE(bitset.isSet(125));
    ASSERT_FALSE(bitset.isSet(210));
    ASSERT_FALSE(bitset.isSet(144));
    ASSERT_FALSE(bitset.isSet(141));

}


TEST (BitsetTest, TestBitsetContains) {

    Bitset bitsetOne = Bitset(COMPONENT_COUNT);
    Bitset bitsetTwo = Bitset(COMPONENT_COUNT);

    cout << "Bitset contains Bitset" << endl;

    std::vector<uint32> v1 = {22, 150, 144, 205, 4, 94, 267};
    bitsetOne.set(&v1);

    std::vector<uint32> v2 = {22, 144, 4, 94};
    bitsetTwo.set(&v2);

    ASSERT_TRUE(bitsetOne.contains(&bitsetOne));
    ASSERT_TRUE(bitsetOne.contains(&bitsetTwo));
    ASSERT_FALSE(bitsetTwo.contains(&bitsetOne));

    bitsetTwo.set(23);

    ASSERT_FALSE(bitsetOne.contains(&bitsetTwo));

}