# Practical Entity Component System

PECS is a practical, powerful, and provocative ECS designed to be easy to use,
easy to understand, and **FAST**! Build PECS into your game and let it do all
the heavy lifting for you!

## Example

```
#include "pecs.hpp"

using namespace pecs;

// Define a bunch of component groups
enum {
	COMPONENT_ANY       = 0,
	COMPONENT_INFO      = 1 << 0,
	COMPONENT_TRANSFORM = 1 << 1
};

struct component_info_t {
	std::string name;
};

struct component_transform_t {
	float position[3];
	float scale[3];
	float orientation[4];
	float direction[3];
};

// Resize components as more entities flood in
template <typename T>
void resize(std::vector<T> &components, uint32_t id) {
	size_t c = components.capacity();

	while (c <= id) {
		c = std::max<size_t>(c, 1);
		c = c << 1;
	}

	if (components.size() <= id) {
		components.reserve(c);
		components.resize(id+1);
	}
}

struct test_world_t : world_t {
	std::vector<component_info_t>      infos;
	std::vector<component_transform_t> transforms;

	void set_component(entity_t *entity, component_info_t component) {
		entity->mask |= COMPONENT_INFO;
		resize(this->infos, entity->id);
		this->infos[entity->id] = component;
	}

	void set_component(entity_t *entity, component_transform_t component) {
		entity->mask |= COMPONENT_TRANSFORM;
		resize(this->transforms, entity->id);
		this->transforms[entity->id] = component;
	}
};

struct test_system_t : system_t {
	std::string name;

	test_system_t(std::string name) {
		this->name     = name;
		this->priority = 0;
		this->mask     = COMPONENT_INFO | COMPONENT_TRANSFORM;
	}

	void on_add(world_t *world) {
		printf("Added system to world!\n");
	}

	void on_remove(world_t *world) {
		printf("Removed system from world!\n");
	}

	void on_add(entity_t *entity) {
		printf("Added entity to system!\n");
	}

	void on_remove(entity_t *entity) {
		printf("Removed entity form system!\n");
	}

	void update(double dt) {
		test_world_t *world = (test_world_t*)this->world;

		for (auto &entity : world->entities) {
			PECS_SKIP_INVALID_ENTITY;

			auto &info = world->infos[entity.id];
			printf("%s\n", info.name.c_str());

			auto &transform = world->transforms[entity.id];
			transform.position[0] += transform.position[1];

			(void)info;
			(void)transform;
		}
	}
};

int main(int argc, char* argv[]) {
	// Create new world
	test_world_t world;

	// Create new system
	test_system_t a = test_system_t("Test System");
	world.add(&a);

	// Create new entities
	const size_t num_entities = 5;
	printf("Adding %ld entities...\n", num_entities);

	for (size_t i = 0; i < num_entities; i++) {
		entity_t entity = world.get_entity();
		world.set_component(&entity,
			component_info_t { "I am an entity!" }
		);
		world.set_component(&entity,
			component_transform_t {
				{ 5.0, 6.0, 7.0 },
				{ 1.0, 1.0, 1.0 },
				{ 0.0, 0.0, 0.0, 1.0 },
				{ 0.0, 0.0, 1.0 }
			}
		);
		world.add(entity);
	}

	// Run a few update cycles
	for (int i = 0; i < 10; i++) {
		world.update(1.0);
	}

	return 0;
}
```