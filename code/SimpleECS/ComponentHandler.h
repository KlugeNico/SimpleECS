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
        explicit PointingComponentHandle(size_t typeSize, void(* deleteFunc)(void*)) :
                deleteFunc(deleteFunc), typeSize(typeSize),
                components(std::vector<void*>(MAX_ENTITY_AMOUNT + 1)) {}

        ~PointingComponentHandle() override {
            for (auto& component : components) {
                deleteFunc(component);
                component = nullptr;
            }
        }

        void* getComponent(sEcs::EntityIndex entityIndex) override {
            return components[entityIndex];
        }

        void* createComponent(sEcs::EntityIndex entityIndex) override {
            return components[entityIndex] = malloc(typeSize);
        }

        void destroyComponentIntern(sEcs::EntityIndex entityIndex) override {
            deleteFunc(getComponent(entityIndex));
            components[entityIndex] = nullptr;
        }

    private:
        size_t typeSize;

        void (* deleteFunc)(void*);

        std::vector<void*> components;

    };


    class ValuedComponentHandle : public sEcs::ComponentHandle {

    public:
        explicit ValuedComponentHandle(size_t typeSize, void(* destroyFunc)(void*)) :
                destroyFunc(destroyFunc), typeSize(typeSize) {
            data = std::vector<char>((MAX_ENTITY_AMOUNT + 1) * typeSize);
        }

        void* getComponent(sEcs::EntityIndex entityIndex) override {
            return &data[entityIndex * typeSize];
        }

        void* createComponent(sEcs::EntityIndex entityIndex) override {
            return &data[entityIndex * typeSize];
        }

        void destroyComponentIntern(sEcs::EntityIndex entityIndex) override {
            destroyFunc(getComponent(entityIndex));
        }

    private:
        size_t typeSize;

        void (* destroyFunc)(void*);

        std::vector<char> data;

    };

}

#endif //SIMPLEECS_COMPONENTHANDLER_H