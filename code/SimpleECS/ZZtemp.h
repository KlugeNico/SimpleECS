//
// Created by kreischenderdepp on 27.03.20.
//

#ifndef SIMPLEECS_TYPEWRAPPER_H
#define SIMPLEECS_TYPEWRAPPER_H

#endif //SIMPLEECS_TYPEWRAPPER_H



template<typename ... Ts>
void insertComponentIds(std::vector<ComponentId> *ids) {
    std::vector<EcsCoreIntern::ComponentHandle*> chs = std::vector<EcsCoreIntern::ComponentHandle*>(sizeof...(Ts));
    insertComponentHandles<void, Ts...>(&chs, 0);
    for (size_t i = 0; i < chs.size(); i++) {
        (*ids)[i] = chs[i]->getComponentId();
    }
}

template<typename V>
void insertComponentHandles(std::vector<EcsCoreIntern::ComponentHandle *> *chs, int index) {
}

template<typename V, typename T, typename... Ts>
void insertComponentHandles(std::vector<EcsCoreIntern::ComponentHandle *> *chs, int index) {
    (*chs)[index] = getSetComponentHandle<T>(this, nullptr, VALUE);
    insertComponentHandles<V, Ts...>(chs, ++index);
}

template<typename T>
void addComponentsToComponentHandles(EntityIndex index, T component) {
    void* p = getSetComponentHandle<T>(this, nullptr, VALUE)->createComponent(index);
    new(p) T(std::move(component));
#if CORE_ECS_EVENTS==1
    emitEvent(ComponentAddedEvent<T>(entities[index].id(index)));
#endif
}

template<typename T, typename... Ts>
void addComponentsToComponentHandles(EntityIndex index, T&& component, Ts&&... components) {
    addComponentsToComponentHandles<T>(index, component);
    addComponentsToComponentHandles<Ts...>(index, std::forward<Ts>(components)...);
}



template<typename S_T>
inline void makeAvailable(S_T* object) {
    getSetSingleton(object);
}

template<typename S_T>
inline S_T* access() {
    return getSetSingleton<S_T>();
}
std::unordered_map<Key, void*> singletons;



template<typename T>
EcsCoreIntern::ComponentHandle *linkComponentHandle(const Key* componentKey,
                                                    EcsCoreIntern::ComponentHandle* (*getSetComponentHandleFunc) (Manager*, const Key*),
                                                    EcsCoreIntern::ComponentHandle* chp) {

    if (!componentKey) {
        std::ostringstream oss;
        oss << "Tried to access unregistered component: " << typeid(T).name();
        throw std::invalid_argument(oss.str());
    }

    if (componentMap[*componentKey] == nullptr) {

        EcsCoreIntern::ComponentInfo componentInfo = EcsCoreIntern::ComponentInfo();
        componentInfo.id = ++lastComponentIndex;
        componentInfo.typeSize = sizeof(T);
#if CORE_ECS_EVENTS==1
        componentInfo.emitDeleteEvent = &emitDeleteEvent<T>;
#endif

        componentMap[*componentKey] = chp;
        componentHandles[chp->getComponentId()] = chp;
        getSetComponentHandleFuncList.emplace_back(getSetComponentHandleFunc);
    }
    EcsCoreIntern::ComponentHandle *ch = componentMap[*componentKey];
    return ch;
}

template<typename T>
static EcsCoreIntern::ComponentHandle* getSetComponentHandle(
        Manager* instance, const Key* componentKey = nullptr, Storing storing = DEFAULT_STORING) {

    static EcsCoreIntern::ComponentHandle* componentHandle =
            instance->linkComponentHandle<T>(componentKey, storing, &getSetComponentHandle<T>);

    if (componentKey)
        if (*componentKey == ECS_RESET_CODE)
            componentHandle = nullptr;
        else
            componentHandle = instance->linkComponentHandle<T>(componentKey, storing, &getSetComponentHandle<T>);

    else if (!componentHandle) {
        std::ostringstream oss;
        oss << "Tried to access unregistered component: " << typeid(T).name();
        throw std::invalid_argument(oss.str());
    }

    return componentHandle;
}

void updateAllMemberships(EntityId entityId, EcsCoreIntern::ComponentBitset *previous, EcsCoreIntern::ComponentBitset *recent) {
    for (EcsCoreIntern::EntitySet *set : entitySets) {
        set->updateMembership(entityId.index, previous, recent);
    }
}

#if CORE_ECS_EVENTS==1
template<typename T>
        static void emitDeleteEvent(SimpleEH::EventHandler& eventHandler, EntityId entityId) {
            eventHandler.emitEvent(ComponentDeletedEvent<T>(entityId));
        }
#endif
