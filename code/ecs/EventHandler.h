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
        virtual void receive(T* event) = 0;
    };

    class SimpleEventHandler {

    public:
        template<typename T>
        void subscribe(Listener<T>* listener) {
            list<T>()->push_back(listener);
        }

        template<typename T>
        bool unsubscribe(Listener<T>* toRemove) {
            std::vector<Listener<T>*>* rList = list<T>();
            for (int i = 0; i < rList->size(); i++) {
                if ((*rList)[i] == toRemove) {
                    (*rList)[i] = (*rList)[rList->size() - 1];
                    rList->pop_back();
                    return true;
                }
            }
            return false;
        }

        template<typename T>
        void emit(T& event) {
            std::vector<Listener<T>*>* rList = list<T>();
            for (int i = 0; i < rList->size(); i++) {
                (*rList)[i]->receive(&event);
            }
        }

        template<typename T>
        void registerEvent(Event_Key eventKey) {
            list<T>(eventKey);
        }

    private:
        std::unordered_map<Event_Key, void*> receiverLists;

        template<typename T>
        std::vector<Listener<T>*>* list(Event_Key eventKey = Event_Key()) {
            static std::vector<Listener<T>*>* receiverList = linkReceiverList<T>(&eventKey);
            return receiverList;
        }

        template<typename T>
        std::vector<Listener<T>*>* linkReceiverList(Event_Key *eventKey) {
            if (eventKey->empty())
                throw std::invalid_argument("Tried to use unregistered event!");
            if (receiverLists[*eventKey] == nullptr) {
                auto *listPointer = new std::vector<Listener<T>*>;
                receiverLists[*eventKey] = reinterpret_cast<void *>(listPointer);
            }
            auto *listPointer = reinterpret_cast<std::vector<Listener<T>*>*>(receiverLists[*eventKey]);
            return listPointer;
        }

    };

}

#endif //SIMPLE_EVENT_HANDLER_H
