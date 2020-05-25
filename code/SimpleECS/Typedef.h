/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */


#ifndef SIMPLEECS_TYPEDEF_H
#define SIMPLEECS_TYPEDEF_H

#define POW_2_32 4294967296

#define BITSET_TYPE std::uint8_t
#define BITSET_TYPE_SIZE 8u

#ifndef USE_ECS_EVENTS
#define USE_ECS_EVENTS 1
#endif

#ifndef MAX_COMPONENT_AMOUNT
#define MAX_COMPONENT_AMOUNT 63
#endif

#ifndef MAX_ENTITY_AMOUNT
#define MAX_ENTITY_AMOUNT 100000
#endif

#include <string>

namespace sEcs {

    typedef std::uint32_t uint32;
    typedef std::uint64_t uint64;
    typedef uint32 Id;
    typedef uint32 EntityVersion;
    typedef uint32 EntityIndex;
    typedef Id ComponentId;
    typedef Id SetIteratorId;
    typedef Id EventId;
    typedef uint32 InternIndex;
    typedef std::string Key;

    struct EntityId {
        uint32 version;
        uint32 index;

        EntityId() : version(0), index(0) {};

        explicit EntityId(uint64 asUint64) {
            index = (uint32)asUint64;
            version = asUint64 >> 32u;
        };
        EntityId(EntityVersion version, EntityIndex index) : version(version), index(index) {};
        inline bool operator == (const EntityId& other) const {
            return index == other.index && version == other.version;
        }
        inline bool operator != (const EntityId& other) const {
            return index != other.index || version != other.version;
        }
        inline explicit operator uint64() const {
            return (((uint64) version) * POW_2_32 | index);
        }
    };

}

#endif //SIMPLEECS_TYPEDEF_H
