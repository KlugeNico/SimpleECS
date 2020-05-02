//
// Created by kreischenderdepp on 28.03.20.
//

#ifndef SIMPLEECS_TYPEDEF_H
#define SIMPLEECS_TYPEDEF_H

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
        bool operator == (const EntityId& other) const {
            return index == other.index && version == other.version;
        }
        bool operator != (const EntityId& other) const {
            return index != other.index || version != other.version;
        }
        explicit operator uint64() const {
            return (((uint64) version) * POW_2_32 | index);
        }
    };

}

#endif //SIMPLEECS_TYPEDEF_H
