/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */


#ifndef SIMPLEECS_COMPONENTHANDLER_H
#define SIMPLEECS_COMPONENTHANDLER_H


#include "Core.h"


namespace sEcs {

    class PointingComponentHandle : public sEcs::ComponentHandle {

    public:
        explicit PointingComponentHandle(size_t typeSize, void(* deleteFunc)(void*));

        ~PointingComponentHandle() override;

        void* getComponent(sEcs::EntityIndex entityIndex) override;

        void* createComponent(sEcs::EntityIndex entityIndex) override;

        void destroyComponentIntern(sEcs::EntityIndex entityIndex) override;

    private:
        size_t typeSize;
        void (* deleteFunc)(void*);
        std::vector<void*> components;

    };


    class ValuedComponentHandle : public sEcs::ComponentHandle {

    public:
        explicit ValuedComponentHandle(size_t typeSize, void(* destroyFunc)(void*));

        void* getComponent(sEcs::EntityIndex entityIndex) override;

        void* createComponent(sEcs::EntityIndex entityIndex) override;

        void destroyComponentIntern(sEcs::EntityIndex entityIndex) override;

    private:
        size_t typeSize;
        void (* destroyFunc)(void*);
        std::vector<char> data;

    };

}

#endif //SIMPLEECS_COMPONENTHANDLER_H