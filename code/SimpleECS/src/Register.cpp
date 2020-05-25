/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */


#include "../Register.h"

namespace sEcs {

    Id Register::getId(const Key& key) {
        auto idField = find(key);
        if (idField != end())
            return idField->second;
        else
            return 0;
    }

    void Register::set(const std::string& key, ComponentId id) {
        (*this)[key] = id;
    }

}