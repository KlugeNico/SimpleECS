/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */


#include "../EventHandler.h"

#include <algorithm>

namespace sEcs {
    namespace Events {

        EventHandler::EventHandler() {
            listeners.emplace_back();
        }

        EventId EventHandler::generateEvent() {
            listeners.emplace_back();
            return listeners.size() - 1;
        }

        void EventHandler::subscribeEvent(uint32_t eventId, Listener* listener) {
            listeners[eventId].push_back(listener);
        }

        void EventHandler::unsubscribeEvent(uint32_t eventId, Listener* toRemove) {
            std::vector<Listener*>& v = listeners[eventId];
            v.erase(std::remove(v.begin(), v.end(), toRemove), v.end());    // Erase–remove idiom
        }

        void EventHandler::emitEvent(uint32_t eventId, const void* event) {
            std::vector<Listener*>& eventListeners = listeners[eventId];
            for (Listener* listener : eventListeners) {
                listener->receive(eventId, event);
            }
        }

    }
}