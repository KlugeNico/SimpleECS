//
// Created by kreischenderdepp on 02.05.20.
//

#include <thread>

#include <SimpleECS/Core.h>
#include <SimpleECS/TypeWrapper.h>

#include <gtest/gtest.h>


struct Numbered {
    int id;
};


#include "BitsetTest.cc"
#include "CoreTest.cc"
#include "EventsTest.cc"