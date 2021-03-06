/**
 *
 * ORIGINAL INTRODUCTION for EntityX version of the example (not completely accurate anymore):
 *
 * This is an example of using EntityX.
 *
 * It is an SFML2 application that spawns 100 random circles on a 2D plane
 * moving in random directions. If two circles collide they will explode and
 * emit particles.
 *
 * This illustrates a bunch of EC/EntityX concepts:
 *
 * - Separation of data via components.
 * - Separation of logic via systems.
 * - Use of events (colliding bodies trigger a CollisionEvent).
 *
 * Compile with:
 *
 *    c++ -I.. -O3 -std=c++11 -Wall -lsfml-system -lsfml-window -lsfml-graphics -lentityx example.cc -o example
 *
 *
 * UPDATED INTRODUCTION:
 *
 *      If i got it right it doesn't spawn 100 random circles, but 500
 *
 *     I converted the example from the EntityX project to make a performance/FPS comparison between SimpleEcs and EntityX.
 *     I used the example version from 7 Jan 2017.
 *     This example now uses SimpleEcs.
 *
 *     You can find the EntityX Project on GitHub.
 *
 *     To get a valid comparision you should compile it the same way you compile the EntityX version.
 *     You should also use the same screen, because depending on the screen resolution there will be a
 *     different amount of collisions. This results in a different amount of particles.
 *
 *     Compile with:
 *
 *       c++ -I.. -O3 -std=c++11 -Wall -lsfml-system -lsfml-window -lsfml-graphics example.cc -o ../../../out/example
 *
 *       and then run "./example" in "out" folder.
 *
 */

#include <cmath>
#include <memory>
#include <unordered_set>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <SimpleECS/TypeWrapper.h>
#include <SimpleECS/Systems.h>

using namespace sEcs;

using std::cerr;
using std::cout;
using std::endl;

static const std::string FONT_FILE = "VeraMono-Bold.ttf";

float r(int a, float b = 0) {
    return static_cast<float>(std::rand() % (a * 1000) + b * 1000) / 1000.0;
}


struct Body {
    Body()= default;

    Body(const sf::Vector2f &position, const sf::Vector2f &direction, float rotationd = 0.0)
            : position(position), direction(direction), rotationd(rotationd), alpha(0.0) {}

    sf::Vector2f position;
    sf::Vector2f direction;
    float rotation = 0.0, rotationd, alpha;
};


using Renderable = std::shared_ptr<sf::Shape>;


struct Particle {
    Particle()= default;;

    explicit Particle(sf::Color colour, float radius, float duration)
            : colour(colour), radius(radius), alpha(colour.a), d(colour.a / duration) {}

    sf::Color colour;
    float radius, alpha, d;
};


struct Collideable {
    Collideable()= default;

    explicit Collideable(float radius) : radius(radius) {}

    float radius;
};


// Emitted when two entities collide.
struct CollisionEvent {
    CollisionEvent(sEcs::EntityId left, sEcs::EntityId right) : left(left), right(right) {}

    sEcs::EntityId left, right;
};


class SpawnSystem : public sEcs::System {
public:

    explicit SpawnSystem(sf::RenderTarget &target, int count) : size(target.getSize()), count(count) {}

    void update(sEcs::DELTA_TYPE delta) override {

        int c = countEntities<Collideable>();

        for (int i = 0; i < count - c; i++) {
            sEcs::Entity entity = createEntity();

            entity.addComponents(
                Collideable(r(10, 5)),
                Body(sf::Vector2f(r(size.x), r(size.y)),sf::Vector2f(r(100, -50), r(100, -50)))
            );

            auto* collideable = entity.getComponent<Collideable>();

            // Shape to apply to entity.
            Renderable shape(new sf::CircleShape(collideable->radius));
            shape->setFillColor(sf::Color(r(128, 127), r(128, 127), r(128, 127), 0));
            shape->setOrigin(collideable->radius, collideable->radius);

            entity.addComponent(Renderable(shape));

        }
    }

private:
    sf::Vector2u size;
    int count;
};


// Updates a body's position and rotation.
struct BodySystem : public sEcs::IntervalSystem<Body> {
    BodySystem(sEcs::uint32 intervals) : IntervalSystem(intervals) {}

    void update(sEcs::Entity entity, sEcs::DELTA_TYPE delta) override {

        Body* body = entity.getComponent<Body>();
        body->position += body->direction * delta;
        body->rotation += body->rotationd * delta;
        body->alpha = std::min(1.0f, body->alpha + delta);

    };
};



// Bounce bodies off the edge of the screen.
class BounceSystem : public sEcs::IntervalSystem<Body> {
public:
    explicit BounceSystem(sEcs::uint32 intervals, sf::RenderTarget &target) : IntervalSystem(
            intervals), size(target.getSize()) {}

    void update(sEcs::Entity entity, sEcs::DELTA_TYPE delta) override {
        auto* body = entity.getComponent<Body>();
        if (body->position.x + body->direction.x < 0 ||
            body->position.x + body->direction.x >= size.x)
            body->direction.x = -body->direction.x;
        if (body->position.y + body->direction.y < 0 ||
            body->position.y + body->direction.y >= size.y)
            body->direction.y = -body->direction.y;
    }

private:
    sf::Vector2u size;
};


// Determines if two Collideable bodies have collided. If they have it emits a
// CollisionEvent. This is used by ExplosionSystem to create explosion
// particles, but it could be used by a SoundSystem to play an explosion
// sound, etc..
//
// Uses a fairly rudimentary 2D partition system, but performs reasonably well.
class CollisionSystem : public sEcs::IntervalSystem<Body, Collideable> {
    static const int PARTITIONS = 200;

    struct Candidate {
        sf::Vector2f position;
        float radius;
        sEcs::EntityId entity;
    };

public:
    explicit CollisionSystem(sEcs::uint32 intervals, sf::RenderTarget &target)
            : IntervalSystem(intervals), size(target.getSize()) {
        size.x = size.x / PARTITIONS + 1;
        size.y = size.y / PARTITIONS + 1;
    }

    void start(sEcs::DELTA_TYPE delta) override {
        reset();
    }

    void update(sEcs::Entity entity, sEcs::DELTA_TYPE delta) override {
        collect(entity);
    };

    void end(sEcs::DELTA_TYPE delta) override {
        collide();
    }

private:
    std::vector<std::vector<Candidate>> grid;
    sf::Vector2u size;

    void reset() {
        grid.clear();
        grid.resize(size.x * size.y);
    }

    void collect(sEcs::Entity entity) {
        auto* body = entity.getComponent<Body>();
        auto* collideable = entity.getComponent<Collideable>();
        unsigned int
                left = static_cast<int>(body->position.x - collideable->radius) / PARTITIONS,
                top = static_cast<int>(body->position.y - collideable->radius) / PARTITIONS,
                right = static_cast<int>(body->position.x + collideable->radius) / PARTITIONS,
                bottom = static_cast<int>(body->position.y + collideable->radius) / PARTITIONS;
        Candidate candidate {body->position, collideable->radius, entity.id()};
        unsigned int slots[4] = {
                left + top * size.x,
                right + top * size.x,
                left  + bottom * size.x,
                right + bottom * size.x,
        };
        grid[slots[0]].push_back(candidate);
        if (slots[0] != slots[1]) grid[slots[1]].push_back(candidate);
        if (slots[1] != slots[2]) grid[slots[2]].push_back(candidate);
        if (slots[2] != slots[3]) grid[slots[3]].push_back(candidate);
    }

    void collide() {
        for (const std::vector<Candidate> &candidates : grid) {
            for (const Candidate &left : candidates) {
                for (const Candidate &right : candidates) {
                    if (left.entity == right.entity) continue;
                    if (collided(left, right)) {
                        CollisionEvent collisionEvent = CollisionEvent(left.entity, right.entity);
                        emitEvent(collisionEvent);
                    }
                }
            }
        }
    }

    float length(const sf::Vector2f &v) {
        return std::sqrt(v.x * v.x + v.y * v.y);
    }

    bool collided(const Candidate &left, const Candidate &right) {
        return length(left.position - right.position) < left.radius + right.radius;
    }
};


// Fade out and then remove particles.
class ParticleSystem : public sEcs::IntervalSystem<Particle> {
public:
    ParticleSystem(sEcs::uint32 intervals) : IntervalSystem(intervals) {}

    void update(sEcs::Entity entity, sEcs::DELTA_TYPE delta) override {
        auto* particle = entity.getComponent<Particle>();
        particle->alpha -= particle->d * delta;
        if (particle->alpha <= 0) {
            entity.erase();
        } else {
            particle->colour.a = particle->alpha;
        }
    }
};


// Renders all explosion particles efficiently as a quad vertex array.
class ParticleRenderSystem : public sEcs::IntervalSystem<Particle, Body> {
public:
    explicit ParticleRenderSystem(sEcs::uint32 intervals, sf::RenderTarget &target)
            : IntervalSystem(intervals), target(target) {}

    void start(sEcs::DELTA_TYPE delta) override {
        vertices = sf::VertexArray(sf::Quads);
    }

    void update(sEcs::Entity entity, sEcs::DELTA_TYPE delta) override {
        auto* particle = entity.getComponent<Particle>();
        auto* body = entity.getComponent<Body>();
        const float r = particle->radius;
        // Spin the particles.
        sf::Transform transform;
        transform.rotate(body->rotation);
        vertices.append(sf::Vertex(body->position + transform.transformPoint(sf::Vector2f(-r, -r)), particle->colour));
        vertices.append(sf::Vertex(body->position + transform.transformPoint(sf::Vector2f(r, -r)), particle->colour));
        vertices.append(sf::Vertex(body->position + transform.transformPoint(sf::Vector2f(r, r)), particle->colour));
        vertices.append(sf::Vertex(body->position + transform.transformPoint(sf::Vector2f(-r, r)), particle->colour));
    }

    void end(sEcs::DELTA_TYPE delta) override {
        target.draw(vertices);
    }

private:
    sf::RenderTarget &target;
    sf::VertexArray vertices;
};


// For any two colliding bodies, destroys the bodies and emits a bunch of bodgy explosion particles.
class ExplosionSystem : public sEcs::System, public sEcs::Listener<CollisionEvent> {

public:
    ExplosionSystem() {
        subscribeEvent(this);
    }

    void update(sEcs::DELTA_TYPE delta) override {
        for (uint64_t entityIdAsLong : collided) {
            emit_particles(sEcs::EntityId(entityIdAsLong));
            getEntity(sEcs::EntityId(entityIdAsLong)).erase();
        }
        collided.clear();
    }

    void emit_particles(sEcs::EntityId entityId) {
        sEcs::Entity entity = getEntity(entityId);
        auto* body = entity.getComponent<Body>();
        auto* renderable = entity.getComponent<Renderable>();
        auto* collideable = entity.getComponent<Collideable>();
        sf::Color colour = (*renderable)->getFillColor();
        colour.a = 200;

        float area = (M_PI * collideable->radius * collideable->radius) / 3.0;
        for (int i = 0; i < area; i++) {
            sEcs::Entity particle = createEntity();

            float rotationd = r(720, 180);
            if (std::rand() % 2 == 0) rotationd = -rotationd;

            float radius = r(3, 1);

            float offset = r(collideable->radius, 1);
            float angle = r(360) * M_PI / 180.0;

            particle.addComponents(
                    Body(
                        body->position + sf::Vector2f(offset * cos(angle), offset * sin(angle)),
                        body->direction + sf::Vector2f(offset * 2 * cos(angle), offset * 2 * sin(angle)),
                        rotationd),
                    Particle(colour, radius, radius / 2));
        }
    }

    void receive(const CollisionEvent& collisionEvent) override {
        // Events are immutable, so we can't destroy the entities here. We defer
        // the work until the update loop.
        collided.insert((uint64_t) collisionEvent.left);
        collided.insert((uint64_t) collisionEvent.right);
    }

private:
    std::unordered_set<uint64_t> collided;

};


// Render all Renderable entities and draw some informational text.
class RenderSystem : public sEcs::IntervalSystem<Body, Renderable> {
public:
    explicit RenderSystem(sEcs::uint32 intervals, sf::RenderTarget &target,
                          sf::Font &font) : IntervalSystem(intervals), target(target) {
        text.setFont(font);
        text.setPosition(sf::Vector2f(2, 2));
        text.setCharacterSize(18);
        text.setColor(sf::Color::White);
    }

    void update(sEcs::Entity entity, sEcs::DELTA_TYPE delta) override {
        auto* body = entity.getComponent<Body>();
        auto* renderable = entity.getComponent<Renderable>();

        sf::Color fillColor = renderable->get()->getFillColor();
        fillColor.a = sf::Uint8(body->alpha * 255);
        renderable->get()->setFillColor(fillColor);
        renderable->get()->setPosition(body->position);
        renderable->get()->setRotation(body->rotation);
        target.draw(*renderable->get());
    }

    void end(float delta) {
        last_update += delta;
        overall_delta += delta;
        frame_count++;
        overall_frame_count++;
        if (last_update >= 0.5) {
            std::ostringstream out;
            const double fps = frame_count / last_update;
            const double overall_fps = overall_frame_count / overall_delta;
            out << countEntities() << " entities FPS: " << static_cast<int>(fps) << " => " << static_cast<int>(overall_fps);
            text.setString(out.str());
            last_update = 0.0;
            frame_count = 0.0;
        }
        target.draw(text);
    }

private:
    double last_update = 0.0;
    double frame_count = 0.0;
    double overall_frame_count = 0;
    double overall_delta = 0;
    sf::RenderTarget &target;
    sf::Text text;
};


class Application : public sEcs::EcsManager {
public:
    explicit Application(sEcs::uint32 maxEntities, sEcs::uint32 maxComponents, sf::RenderTarget &target,
                         sf::Font &font) {
        ECS_MANAGER_INSTANCE = this;

        ::registerComponent<Body>();
        ::registerComponent<Particle>();
        ::registerComponent<Collideable>();
        ::registerComponent<Renderable>();

    }

};


int main() {
    std::srand(std::time(nullptr));

    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "EntityX Example run with SimpleEcs", sf::Style::Fullscreen);
    sf::Font font;
    if (!font.loadFromFile(FONT_FILE)) {
        cerr << "error: failed to load " << FONT_FILE << endl;
        return 1;
    }

    Application app(200000, 20, window, font);

    ::addSystem(std::make_shared<SpawnSystem>(window, 500));
    ::addSystem(std::make_shared<BodySystem>(1));
    ::addSystem(std::make_shared<BounceSystem>(1, window));
    ::addSystem(std::make_shared<CollisionSystem>(1, window));
    ::addSystem(std::make_shared<ExplosionSystem>());
    ::addSystem(std::make_shared<ParticleSystem>(1));
    ::addSystem(std::make_shared<RenderSystem>(1, window, font));
    ::addSystem(std::make_shared<ParticleRenderSystem>(1, window));

    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                case sf::Event::KeyPressed:
                    window.close();
                    break;

                default:
                    break;
            }
        }

        window.clear();
        sf::Time elapsed = clock.restart();
        app.update(elapsed.asSeconds());
        window.display();
    }
}
