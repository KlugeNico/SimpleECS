# SimpleECS

A small and simple [Entity Component System](https://en.wikipedia.org/wiki/Entity_component_system) written with CLion in C++ 11.

This project is orientated to the [EntityX Project](https://github.com/alecthomas/entityx).

I made my own ECS, to get a better understanding for ECS and to get full control to implement features I need.

## Differences to EntityX

As far as I understood the code of EntityX, it is designed in a way I, which makes it very difficult (or impossible) to add components and systems from an external library. Because I want my games to be easy modifiable I need this feature. Also I think it's necessary to split the execution of expensive systems over multiple frames, because they can not be processed in one frame.

### Additional Features of SimpleECS

- Core API very near to functionality (But there is also a handy wrapper)
- Registered Components to make usage of Libraries (.dll .so) for mods possible.
- Systems can be updated over multiple Frames.

### Advantages of EntityX

- Seems to be faster for now. I took the EntityX example (the exploding circles) and changed it to use SimpleECS. I build both with gcc version 6.3.0 on Debian.
  - EntityX: 40 fps
  - SimpleECS: 30 fps.

## Planed Features:

- more CPU cache friendliness (performance boost). Maybe I have to change the component storage and entity iteration fundamentally.

## Implementation details

### Iterating over entities

Each used combination of components (usually in Systems) organizes its own list of related entities. Each time a component is added or removed from an entity, every list gets updated. In this way I don't need to iterate over all entities and check for relation while running the systems. But it consumes a lot of memory to organize the lists. Also it makes adding and removing components more expensive.

### Component storage

Components are stored in ComponentHandles. The ComponentHandles are containing vectors of pointers to components.

## Usage

The project contains a [Core](code/ecs/Core.h) file, which is a standalone header file with all main functionality. Because using the core directly is a little bit unhandy there is also a [Wrapper for real time applications](code/ecs/RealTimeEcs.h) (supports fps and comfortable systems). Additional there is an external [EventHandler](code/ecs/EventHandler.h).

The ECS takes care about the deletion of removed components. Also if the entity gets deleted. So you should not assign one component object to multiple entities.

There are three examples which demonstrate the usage of the real time wrapper.

- [A very small example](code/examples/walkingLetters/main.cpp) which explains the the fundamental usage. (Recommended to start with.)
- [The SFML example from EntityX](code/examples/exampleFromEntityx/example.cc) changed to use SimpleECS.
- [Some small "game"](code/examples/movingblocks/main.cpp) with SDL2.