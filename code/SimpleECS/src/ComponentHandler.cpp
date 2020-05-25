/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */

#include "../ComponentHandler.h"

namespace sEcs {

    PointingComponentHandle::PointingComponentHandle(size_t typeSize, void(* deleteFunc)(void*)) :
        deleteFunc(deleteFunc), typeSize(typeSize),
        components(std::vector<void*>(MAX_ENTITY_AMOUNT + 1)) {}

    PointingComponentHandle::~PointingComponentHandle() {
        for (auto& component : components) {
            deleteFunc(component);
            component = nullptr;
        }
    }

    void* PointingComponentHandle::getComponent(sEcs::EntityIndex entityIndex) {
        return components[entityIndex];
    }

    void* PointingComponentHandle::createComponent(sEcs::EntityIndex entityIndex) {
        return components[entityIndex] = malloc(typeSize);
    }

    void PointingComponentHandle::destroyComponentIntern(sEcs::EntityIndex entityIndex) {
        deleteFunc(getComponent(entityIndex));
        components[entityIndex] = nullptr;
    }


    ValuedComponentHandle::ValuedComponentHandle(size_t typeSize, void(* destroyFunc)(void*)) :
            destroyFunc(destroyFunc), typeSize(typeSize) {
        data = std::vector<char>((MAX_ENTITY_AMOUNT + 1) * typeSize);
    }

    void* ValuedComponentHandle::getComponent(sEcs::EntityIndex entityIndex) {
        return &data[entityIndex * typeSize];
    }

    void* ValuedComponentHandle::createComponent(sEcs::EntityIndex entityIndex) {
        return &data[entityIndex * typeSize];
    }

    void ValuedComponentHandle::destroyComponentIntern(sEcs::EntityIndex entityIndex) {
        destroyFunc(getComponent(entityIndex));
    }

}