/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */


#include <cxxabi.h>
#include "../TypeWrapper.h"

sEcs::EcsManager* sEcs::ECS_MANAGER_INSTANCE;

std::string sEcs::TypeWrapper_Intern::cleanClassName(const char* name) {
    char* strP = abi::__cxa_demangle(name,nullptr,nullptr,nullptr);
    std::string string(strP);
    free(strP);
    return std::move(string);
}


sEcs::Entity::Entity(sEcs::EntityId entityId)
    : entityId(entityId) {
}

sEcs::EcsManager* sEcs::manager() {
    return sEcs::ECS_MANAGER_INSTANCE;
}

void sEcs::setManager(sEcs::EcsManager& managerInstance) {
    sEcs::ECS_MANAGER_INSTANCE = &managerInstance;
}
