// #define PECS_DEBUG
#include <chrono>
#include "pecs.hpp"

using namespace pecs;

// Define a bunch of component groups
enum {
	COMPONENT_ANY       = 0,
	COMPONENT_INFO      = 1 << 0,
	COMPONENT_RENDER    = 1 << 1,
	COMPONENT_PHYSICS   = 1 << 2,
	COMPONENT_TRANSFORM = 1 << 3,
	COMPONENT_ANIMATION = 1 << 4,
	COMPONENT_LIGHT     = 1 << 5,
	COMPONENT_CAMERA    = 1 << 6,
	COMPONENT_TRIGGER   = 1 << 7
};

struct component_info_t {
	std::string name;
};

struct component_render_t {
	void *model;
	bool wireframe;
	float color[4];
	std::vector<void*> textures;
	uint8_t visible;
};

struct component_physics_t {
	float force[3];
	float velocity[3];
};

struct component_transform_t {
	float position[3];
	float scale[3];
	float orientation[4];
	float direction[3];
};

struct component_animation_t {
	void *animation;
	std::vector<uint16_t> markers;
};

struct component_light_t {
	float intensity;
	float color[3];
};

struct component_camera_t {
	float fov;
	float near;
	float far;
	float exposure;
};

struct component_trigger_t {
	std::string callback;
	float size[3];
};

// templates and C++ are stupid
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
	std::vector<component_camera_t>    cameras;

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

	void set_component(entity_t *entity, component_camera_t component) {
		entity->mask |= COMPONENT_CAMERA;
		resize(this->cameras, entity->id);
		this->cameras[entity->id] = component;
	}

};

struct sys_test : system_t {
	std::string name;

	sys_test(std::string name) {
		this->name           = name;
		this->priority       = 0;
		this->mask = COMPONENT_INFO | COMPONENT_TRANSFORM;
	}

	void on_add(world_t *world) {
		#ifdef PECS_DEBUG
		printf("Added system!\n");
		#endif
	}

	void on_remove(world_t *world) {
		#ifdef PECS_DEBUG
		printf("Removed system!\n");
		#endif
	}

	void on_add(entity_t *entity) {
		#ifdef PECS_DEBUG
		printf("Added entity!\n");
		#endif
	}

	void on_remove(entity_t *entity) {
		#ifdef PECS_DEBUG
		printf("Removed entity!\n");
		#endif
	}

	void update(double dt) {
		test_world_t *world = (test_world_t*)this->world;

		for (auto &entity : world->entities) {
			PECS_SKIP_INVALID_ENTITY;

			auto &info = world->infos[entity.id];
			auto &transform = world->transforms[entity.id];
			transform.position[0] += transform.position[1];
			#ifdef PECS_DEBUG
			printf("%s\n", info.name.c_str());
			printf(
				"%f,%f,%f\n",
				transform.position[0],
				transform.position[1],
				transform.position[2]
			);
			#else
			(void)info;
			(void)transform;
			#endif
		}
	}
};

struct camera_system_t : system_t {
	std::string name;
	entity_t *camera;

	camera_system_t() {
		this->name     = "Camera System";
		this->camera   = nullptr;
		this->mask     = COMPONENT_INFO | COMPONENT_TRANSFORM | COMPONENT_CAMERA;
		this->priority = 0;
	}

	void on_add(entity_t *camera) {
		this->camera = camera;

		#ifdef PECS_DEBUG
		printf("CAMSYS: Added camera\n");
		#endif
	}

	void on_remove(entity_t *camera) {
		this->camera = nullptr;
		#ifdef PECS_DEBUG
		printf("CAMSYS: Removed camera\n");
		#endif
	}

	void update(double dt) {
		if (!this->camera) {
			return;
		}
		test_world_t *world = (test_world_t*)this->world;

		entity_t &entity = *this->camera;

		auto &info = world->infos[entity.id];
		auto &transform = world->transforms[entity.id];

		#ifdef PECS_DEBUG
		printf("%s\n", info.name.c_str());
		printf(
			"%f,%f,%f\n",
			transform.position[0],
			transform.position[1],
			transform.position[2]
		);
		#else
		// Silence warnings.
		(void)info;
		(void)transform;
		#endif
	}
};

using std::chrono::steady_clock;
double to_seconds(std::chrono::steady_clock::duration tdiff) {
	return tdiff.count() * steady_clock::period::num / static_cast<double>(steady_clock::period::den);
}

/**
 * TEST PROGRAM
 */
int main(int argc, char* argv[]) {
	// Create new world
	test_world_t world;

	// Create new system
	sys_test a = sys_test("Test System");
	world.add(&a);

	// Create new entities
	const size_t num_entities = 2000000;
	printf("Adding %ld entities...\n", num_entities);
	auto as = steady_clock::now();
	for (size_t i = 0; i < num_entities; i++) {
		entity_t entity = world.get_entity();
		world.set_component(&entity,
			component_info_t { "Hello" }
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

	auto ae = steady_clock::now();
	printf("ADD: %fms\n", to_seconds(ae - as) * 1000);

	// Run a few update cycles
	auto rs = steady_clock::now();
	world.refresh();
	auto re = steady_clock::now();
	printf("REFRESH: %fms\n", to_seconds(re - rs) * 1000);

	for (int i = 0; i < 10; i++) {
		auto us = steady_clock::now();
		world.update(1.0);

		if (i == 5) {
			auto base_size = world.entities.size();
			int num_murders = 500;
			printf("Killing %d entities...\n", num_murders);
			for (int j = 0; j < num_murders; j++) {
				world.kill(world.entities[j]);
			}
			printf("Spawning new entities in their place... (dead: %u)\n", world.dead_entities);
			for (int j = 0; j < num_murders; j++) {
				auto e = world.get_entity();
				world.set_component(&e, component_info_t { "ayy lmao" });
				world.set_component(&e, component_transform_t { { 1.0, 2.0, 3.0 } } );
				world.add(e);
			}
			world.refresh();
			auto new_size = world.entities.size();
			printf("Entity count: %lu (dead: %u)\n", new_size, world.dead_entities);
			if (new_size > base_size) {
				printf("WARNING: Entity list grew by %lu. It's not supposed to.\n", new_size - base_size);
				for (auto &e : world.entities) {
					if (!e.alive) {
						printf("Entity %u is, for some reason, dead!\n", e.id);
					}
				}
			}
		}

		auto ue = steady_clock::now();
		printf("UPDATE: %fms (SKIPPED: %ld)\n", to_seconds(ue - us) * 1000, pecs::SKIPS);
	}

	world.refresh();

	return 0;
}
