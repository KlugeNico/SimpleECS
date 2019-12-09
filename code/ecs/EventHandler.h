/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */

#ifndef SIMPLE_EVENT_HANDLER_H
#define SIMPLE_EVENT_HANDLER_H

#define NOT_REGISTERED UINT32_MAX
#define ECS_RESET_CODE "!=)Reset?§%§&Reset!=)?§Reset%§&"

namespace SimpleEH {

    typedef std::string Key;

    template<typename T>
    class Listener {
    public:
        virtual void receive(const T& event) = 0;
    };

    class SimpleEventHandler {

    public:

        ~SimpleEventHandler() {
            for (uint32_t (*getReceiverListIdFunc) (SimpleEventHandler*, bool) : getReceiverListIdFuncList) {
                Key resetCode(ECS_RESET_CODE);
                getReceiverListIdFunc(this, true);
            }
        }

        template<typename T>
        void subscribeEvent(Listener<T>* listener) {
            receiverLists[getReceiverListId<T>(this)].push_back(listener);
        }

        template<typename T>
        void unsubscribeEvent(Listener<T>* toRemove) {
            std::vector<void*>& v = receiverLists[getReceiverListId<T>(this)];
            v.erase(std::remove(v.begin(), v.end(), toRemove), v.end());
        }

        template<typename T>
        void emitEvent(const T& event) {
            std::vector<void*>& rList = receiverLists[getReceiverListId<T>(this)];
            for (void* p : rList) {
                Listener<T>& receiver = *reinterpret_cast<Listener<T>*>(p);
                receiver.receive(event);
            }
        }

        static std::string cleanClassName(const char* name) {
            char* strP = abi::__cxa_demangle(name,nullptr,nullptr,nullptr);
            std::string string(strP);
            free(strP);
            return std::move(string);
        }

    private:
        std::vector<std::vector<void*>> receiverLists;
        std::unordered_map<Key, uint32_t> receiverListsIdMap;
        std::vector<uint32_t (*) (SimpleEventHandler*, bool)> getReceiverListIdFuncList;

        template<typename T>
        static uint32_t getReceiverListId(SimpleEventHandler* instance, bool reset = false) {
            static uint32_t id = instance->linkReceiverList(cleanClassName(typeid(T).name()), &getReceiverListId<T>);

            if (reset)
                id = NOT_REGISTERED;
            else if (id == NOT_REGISTERED)
                id = instance->linkReceiverList(cleanClassName(typeid(T).name()), &getReceiverListId<T>);

            return id;
        }

        uint32_t linkReceiverList(const Key& eventKey, uint32_t (*getReceiverListIdFunc) (SimpleEventHandler*, bool)) {
            if (!receiverListsIdMap.count(eventKey)) {     // Eventtype doesn't exist yet
                receiverListsIdMap[eventKey] = receiverLists.size();
                receiverLists.emplace_back();
                getReceiverListIdFuncList.emplace_back(getReceiverListIdFunc);
            }
            return receiverListsIdMap[eventKey];
        }

    };

}

#endif //SIMPLE_EVENT_HANDLER_H
