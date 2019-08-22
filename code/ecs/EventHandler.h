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
#define RESET_CODE "!=)?§%§&ResetId!=)?§%§&"

namespace SimpleEH {

    typedef std::string Event_Key;

    template<typename T>
    class Listener {
    public:
        virtual void receive(const T& event) = 0;
    };

    class SimpleEventHandler;
    static SimpleEventHandler* instance = nullptr;

    class SimpleEventHandler {

    public:
        SimpleEventHandler() {
            if (instance)
                throw std::invalid_argument("SimpleEventHandler is a singleton! Only one instance allowed.");
            instance = this;
        }

        ~SimpleEventHandler() {
            for (uint32_t (*getReceiverListIdFunc) (const Event_Key*) : getReceiverListIdFuncList) {
                Event_Key resetCode(RESET_CODE);
                getReceiverListIdFunc(&resetCode);
            }
            instance = nullptr;
        }

        template<typename T>
        void subscribeEvent(Listener<T>* listener) {
            receiverLists[getReceiverListId<T>()].push_back(listener);
        }

        template<typename T>
        void unsubscribeEvent(Listener<T>* toRemove) {
            std::vector<void*>& v = receiverLists[getReceiverListId<T>()];
            v.erase(std::remove(v.begin(), v.end(), toRemove), v.end());
        }

        template<typename T>
        void emitEvent(const T& event) {
            std::vector<void*>& rList = receiverLists[getReceiverListId<T>()];
            for (void* p : rList) {
                Listener<T>& receiver = *reinterpret_cast<Listener<T>*>(p);
                receiver.receive(event);
            }
        }

        template<typename T>
        void registerEvent(const Event_Key& eventKey) {
            getReceiverListId<T>(&eventKey);
        }

    private:
        std::vector<std::vector<void*>> receiverLists;
        std::unordered_map<Event_Key, uint32_t> receiverListsIdMap;
        std::vector<uint32_t (*) (const Event_Key*)> getReceiverListIdFuncList;

        template<typename T>
        static uint32_t getReceiverListId(const Event_Key* eventKey = nullptr) {
            static uint32_t id = instance->linkReceiverList(eventKey, &getReceiverListId<T>);

            if (eventKey)
                if (*eventKey == RESET_CODE)
                    id = NOT_REGISTERED;
                else
                    id = instance->linkReceiverList(eventKey, &getReceiverListId<T>);

            else if (id == NOT_REGISTERED)
                throw std::invalid_argument("Tried to use unregistered event!");

            return id;
        }

        uint32_t linkReceiverList(const Event_Key* eventKey, uint32_t (*getReceiverListIdFunc) (const Event_Key*)) {
            if (eventKey->empty())
                throw std::invalid_argument("Tried to use unregistered event!");
            if (!receiverListsIdMap.count(*eventKey)) {     // Eventtype doesn't exist yet
                receiverListsIdMap[*eventKey] = receiverLists.size();
                receiverLists.emplace_back();
                getReceiverListIdFuncList.emplace_back(getReceiverListIdFunc);
            }
            return receiverListsIdMap[*eventKey];
        }

    };

}

#endif //SIMPLE_EVENT_HANDLER_H
