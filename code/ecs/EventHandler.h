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

namespace SimpleEH {

    typedef std::string Event_Key;

    template<typename T>
    class Listener {
    public:
        virtual void receive(const T& event) = 0;
    };

    class SimpleEventHandler {

    public:
        template<typename T>
        void subscribeEvent(Listener<T>* listener) {
            getReceiverList<T>().push_back(listener);
        }

        template<typename T>
        void unsubscribeEvent(Listener<T>* toRemove) {
            std::vector<void*>& v = getReceiverList<T>();
            v.erase(std::remove(v.begin(), v.end(), toRemove), v.end());
        }

        template<typename T>
        void emitEvent(const T& event) {
            std::vector<void*>& rList = getReceiverList<T>();
            for (void* p : rList) {
                Listener<T>& receiver = *reinterpret_cast<Listener<T>*>(p);
                receiver.receive(event);
            }
        }

        template<typename T>
        void registerEvent(const Event_Key& eventKey) {
            getReceiverList<T>(eventKey);
        }

    private:
        std::vector<std::vector<void*>> receiverLists;
        std::unordered_map<Event_Key, uint32_t> receiverListsIdMap;

        template<typename T>
        std::vector<void*>& getReceiverList(const Event_Key& eventKey = Event_Key()) {
            static uint32_t id = linkReceiverList(eventKey);
            return receiverLists[id];
        }

        uint32_t linkReceiverList(const Event_Key& eventKey) {
            if (eventKey.empty())
                throw std::invalid_argument("Tried to use unregistered event!");
            if (!receiverListsIdMap.count(eventKey)) {     // Eventtype doesn't exist yet
                receiverListsIdMap[eventKey] = receiverLists.size();
                receiverLists.emplace_back();
            }
            return receiverListsIdMap[eventKey];
        }

    };

}

#endif //SIMPLE_EVENT_HANDLER_H
