//
// Created by kreischenderdepp on 06.05.19.
//

#ifndef SIMPLE_EVENT_HANDLER_H
#define SIMPLE_EVENT_HANDLER_H

namespace SimpleEH {

    template<typename T>
    class Receiver {
    public:
        virtual void receive(T* event) = 0;
    };

    class SimpleEventHandler {

    public:
        template<typename T>
        void subscribe(Receiver<T>* receiver) {
            list<T>()->push_back(receiver);
        }

        template<typename T>
        bool unsubscribe(Receiver<T>* toRemove) {
            std::vector<Receiver<T>*>* rList = list<T>();
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
        void emit(T event) {
            std::vector<Receiver<T>*>* rList = list<T>();
            for (int i = 0; i < rList->size(); i++) {
                (*rList)[i]->receive(&event);
            }
        }

        template<typename T>
        std::vector<Receiver<T>*>* list() {
            static std::vector<Receiver<T>*> list = std::vector<Receiver<T>*>();
            return &list;
        }

    };

}

#endif //SIMPLE_EVENT_HANDLER_H
