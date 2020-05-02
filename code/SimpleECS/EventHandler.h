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

#include "Typedef.h"

namespace sEcs {
    namespace Events {

        class Listener {
        public:
            virtual void receive(EventId, const void* event) = 0;
        };


        class EventHandler {

        public:
            EventHandler() {
                listeners.emplace_back();
            }

            EventId generateEvent() {
                listeners.emplace_back();
                return listeners.size() - 1;
            }

            void subscribeEvent(uint32_t eventId, Listener* listener) {
                listeners[eventId].push_back(listener);
            }

            void unsubscribeEvent(uint32_t eventId, Listener* toRemove) {
                std::vector<Listener*>& v = listeners[eventId];
                v.erase(std::remove(v.begin(), v.end(), toRemove), v.end());
            }

            void emitEvent(uint32_t eventId, const void* event) {
                std::vector<Listener*>& eventListeners = listeners[eventId];
                for (Listener* listener : eventListeners) {
                    listener->receive(eventId, event);
                }
            }

        private:
            std::vector<std::vector<Listener*>> listeners;

        };

    }
}

#endif //SIMPLE_EVENT_HANDLER_H
