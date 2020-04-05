//
// Created by kreischenderdepp on 04.04.20.
//

#ifndef SIMPLEECS_TYPEREGISTER_H
#define SIMPLEECS_TYPEREGISTER_H

#include "Typedef.h"

namespace sEcs {

    class Register : std::unordered_map<Key, Id> {

    public:
        Id getId(const Key& key) {
            auto idField = find(key);
            if (idField != end())
                return idField->second;
            else
                return 0;
        }

        void set(const std::string& key, ComponentId id) {
            (*this)[key] = id;
        }
    };

}


#endif //SIMPLEECS_TYPEREGISTER_H
