/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */


#ifndef SIMPLEECS_TYPEREGISTER_H
#define SIMPLEECS_TYPEREGISTER_H

#include <unordered_map>
#include "Typedef.h"

namespace sEcs {

    class Register : std::unordered_map<Key, Id> {

    public:
        Id getId(const Key& key);

        void set(const std::string& key, ComponentId id);
    };

}


#endif //SIMPLEECS_TYPEREGISTER_H
