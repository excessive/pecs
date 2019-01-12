/**
 * Practical Entity Component System
 * @version 0.0.6
 * @author Colby Klein <shakesoda@gmail.com>
 * @author Landon Manning <lmanning17@gmail.com>
 * @license MIT/X11
 */
#pragma once
#include <algorithm>
#include <unordered_set>
#include <vector>
#include <cstdio>
#include <cstdint>

namespace pecs {

#ifdef PECS_SKIP_COUNTER
uint64_t SKIPS = 0;
#define PECS_SKIP_INVALID_ENTITY if (!entity.alive || (entity.mask & this->mask) != this->mask) { pecs::SKIPS++; continue; }
#else
#define PECS_SKIP_INVALID_ENTITY if (!entity.alive || (entity.mask & this->mask) != this->mask) { continue; }
#endif

#define PECS_FILTER_ENTITY(MASK) if (!entity.alive || (entity.mask & MASK) != MASK) { continue; }

/**
 * Entity
 */
struct entity_t {
	bool alive;  // OPPORTUNITY: this could be part of the mask
	uint32_t id; // OPPORTUNITY: this doesn't need to be stored
	uint64_t mask;

	entity_t(uint32_t _id):
		alive(false),
		id(_id),
		mask(0)
	{}
};

/**
 * System
 */
struct world_t; // good 'ol circular dependencies.
struct system_t {
	int priority;
	bool active;
	uint64_t mask;
	world_t *world;

	system_t():
		priority(0),
		active(true),
		mask(0),
		world(nullptr)
	{}

	virtual void on_add(entity_t *entity) { (void)entity; }
	virtual void on_add(world_t *_world)   { (void)_world; }
	virtual void on_remove(entity_t *entity) { (void)entity; }
	virtual void on_remove(world_t *_world)   { (void)_world; }
	virtual void update(double dt) { (void)dt; }
};

/**
 * World
 */
struct world_t {
	std::vector<system_t*> systems;
	std::vector<system_t*> queue_systems;
	std::vector<system_t*> rm_queue_systems;

	std::vector<entity_t> entities;
	std::vector<uint32_t> queue_entities;
	std::vector<uint32_t> rm_queue_entities;

	uint32_t dead_entities;
	uint32_t next_id;

	world_t():
		dead_entities(0),
		next_id(0)
	{}

	/* Internal, use get_entity instead! */
	entity_t _spawn_entity() {
		entity_t e(this->next_id);
		this->next_id++;
		return e;
	}

	entity_t get_entity() {
		if (this->dead_entities == 0) {
			return this->_spawn_entity();
		}
		auto it = this->rm_queue_entities.begin();
		for ( ; it != this->rm_queue_entities.end(); it++) {
			this->dead_entities--;
			this->rm_queue_entities.erase(it);
			auto &e = this->entities[*it];
			e.mask = 0;
			return e;
		}
		for (auto &e : this->entities) {
			if (!e.alive) {
				this->dead_entities--;
				e.mask = 0;
				return e;
			}
		}
		#ifdef ECS_DEBUG
		printf("BUG: world_t::dead_entities was non-zero, but nothing was dead!\n");
		#endif
		// This shouldn't happen, but just in case...
		return this->_spawn_entity();
	}

	/**
	 * Add entity to the world
	 * @param entity A reference to an entity
	 * @return The referenced entity
	 */
	entity_t& add(entity_t &entity) {
		if (entity.id >= this->entities.size()) {
			this->entities.push_back(entity);
		}
		this->entities[entity.id] = entity;
		this->queue_entities.push_back(entity.id);
		#ifdef ECS_DEBUG
		printf("Add to add queue: entity\n");
		#endif
		return entity;
	}

	/**
	 * Flag an entity as no longer active.
	 * @param entity A reference to an entity
	 */
	void kill(entity_t &entity) {
		// this->entity_lookup.erase(&entity);
		this->dead_entities++;
		this->rm_queue_entities.push_back(entity.id);
		#ifdef ECS_DEBUG
		printf("Add to remove queue: entity\n");
		#endif
	}

	/**
	 * Remove all entities from the world
	 */
	void clear_entities() {
		for (auto &entity : this->entities) {
			this->kill(entity);
		}
	}

	/**
	 * Add system to the world
	 * @param system A reference to a system
	 * @return The referenced system
	 */
	system_t* add(system_t *system) {
		this->queue_systems.push_back(system);
		#ifdef ECS_DEBUG
		printf("Added system to add queue.\n");
		#endif
		return system;
	}

	/**
	 * Remove system from the world
	 * @param system A reference to a system
	 */
	void remove(system_t *system) {
		this->rm_queue_systems.push_back(system);
		#ifdef ECS_DEBUG
		printf("Added system to remove queue.\n");
		#endif
	}

	/**
	 * Remove all systems fro mthe world
	 */
	void clear_systems() {
		for (auto &system : this->systems) {
			this->remove(system);
		}
	}

	/**
	 * Process add/remove queues, calling on_add/on_remove as needed.
	 */
	void refresh() {
		// Process systems queued for removal
		for (const auto &system : this->rm_queue_systems) {
			auto it = std::find(this->systems.begin(), this->systems.end(), system);

			if (it != this->systems.end()) {
				this->systems.erase(it);
				system->on_remove(this);
				system->world = nullptr;
				#ifdef ECS_DEBUG
				printf("Remove system from queue.\n");
				#endif
			}
		}
		this->rm_queue_systems.clear();

		// Process systems queued for addition
		for (const auto &system : this->queue_systems) {
			auto it = std::find(this->systems.begin(), this->systems.end(), system);

			if (it == this->systems.end()) {
				this->systems.push_back(system);
				system->world = this;
				system->on_add(this);
				#ifdef ECS_DEBUG
				printf("Add system from queue.\n");
				#endif
			}
		}
		this->queue_systems.clear();

		// Process entities queued for removal
		for (const auto id : this->rm_queue_entities) {
			auto &entity = this->entities[id];
			entity.alive = false;
			for (const auto &system : this->systems) {
				if ((entity.mask & system->mask) == system->mask) {
					system->on_remove(&entity);
				}
			}
			#ifdef ECS_DEBUG
			printf("Remove from queue: entity\n");
			#endif
		}
		this->rm_queue_entities.clear();

		// Process entities queued for addition
		for (const auto id : this->queue_entities) {
			auto &entity = this->entities[id];
			entity.alive = true;
			for (const auto &system : this->systems) {
				if ((entity.mask & system->mask) == system->mask) {
					system->on_add(&entity);
				}
			}
			#ifdef ECS_DEBUG
			printf("Add from queue: entity\n");
			#endif
		}
		this->queue_entities.clear();
	}

	/**
	 * Update all systems within the world in order of priority (low to high)
	 */
	void update(double dt) {
		this->refresh();

		// Sort systems by priority (low to high)
		std::sort(
			this->systems.begin(),
			this->systems.end(),
			[](system_t *a, system_t *b) {
				return a->priority < b->priority;
			}
		);

		// Process system updates
		for (const auto &system : this->systems) {
			if (system->active) {
				system->update(dt);
			}
		}
	}
};

} // ecs
