
#include "world.h"

#include "../ecs.h"

EntityBuilder::EntityBuilder(World *p_world) :
		world(p_world) {
}

const EntityBuilder &EntityBuilder::with(uint32_t p_component_id, const Dictionary &p_data) const {
	world->add_component(entity, p_component_id, p_data);
	return *this;
}

World::World() {
	// Add self as databag, so that the `Systems` can obtain it.
	databags.resize(World::get_databag_id() + 1);
	for (uint32_t i = 0; i <= World::get_databag_id(); i += 1) {
		databags[i] = nullptr;
	}
	databags[World::get_databag_id()] = this;
}

EntityID World::create_entity_index() {
	return entity_count++;
}

const EntityBuilder &World::create_entity() {
	entity_builder.entity = create_entity_index();
	return entity_builder;
}

void World::destroy_entity(EntityID p_entity) {
	// Removes the components assigned to this entity.
	for (uint32_t i = 0; i < storages.size(); i += 1) {
		if (storages[i] != nullptr) {
			storages[i]->remove(p_entity);
		}
	}

	// TODO consider to reuse this ID.
}

EntityID World::get_last_entity_id() const {
	if (entity_count == 0) {
		return EntityID();
	} else {
		return EntityID(entity_count - 1);
	}
}

void World::add_component(EntityID p_entity, uint32_t p_component_id, const Dictionary &p_data) {
	create_storage(p_component_id);
	Storage *storage = get_storage(p_component_id);
	ERR_FAIL_COND(storage == nullptr);
	storage->insert_dynamic(p_entity, p_data);
}

void World::remove_component(EntityID p_entity, uint32_t p_component_id) {
	Storage *storage = get_storage(p_component_id);
	ERR_FAIL_COND(storage == nullptr);
	storage->remove(p_entity);
}

bool World::has_component(EntityID p_entity, uint32_t p_component_id) const {
	const Storage *storage = get_storage(p_component_id);
	if (unlikely(storage == nullptr)) {
		return false;
	}
	return storage->has(p_entity);
}

const Storage *World::get_storage(uint32_t p_storage_id) const {
	ERR_FAIL_COND_V_MSG(p_storage_id == UINT32_MAX, nullptr, "The component is not registered.");

	if (unlikely(p_storage_id >= storages.size())) {
		return nullptr;
	}

	return storages[p_storage_id];
}

Storage *World::get_storage(uint32_t p_storage_id) {
	if (unlikely(p_storage_id >= storages.size())) {
		return nullptr;
	}

	return storages[p_storage_id];
}

void World::create_storage(uint32_t p_component_id) {
	// Using crash because this function is not expected to fail.
	ERR_FAIL_COND_MSG(ECS::verify_component_id(p_component_id) == false, "The component id " + itos(p_component_id) + " is not registered.");

	if (p_component_id >= storages.size()) {
		const uint32_t start = storages.size();
		storages.resize(p_component_id + 1);
		for (uint32_t i = start; i < storages.size(); i += 1) {
			storages[i] = nullptr;
		}
	} else {
		if (storages[p_component_id] != nullptr) {
			// Nothing to do.
			return;
		}
	}

	storages[p_component_id] = ECS::create_storage(p_component_id);
}

void World::destroy_storage(uint32_t p_component_id) {
	// Using crash because this function is not expected to fail.
	ERR_FAIL_UNSIGNED_INDEX_MSG(p_component_id, storages.size(), "The component storage id " + itos(p_component_id) + " is unknown; so can't be destroyed.");

	if (storages[p_component_id] == nullptr) {
		// Nothing to do.
		return;
	}

	memdelete(storages[p_component_id]);
	storages[p_component_id] = nullptr;
}

void World::add_databag(godex::databag_id p_id) {
	ERR_FAIL_COND_MSG(ECS::verify_databag_id(p_id) == false, "The databag is not registered.");
	ERR_FAIL_COND_MSG(p_id == World::get_databag_id(), "The databag `World` is an internal type automatically register by the `World` itself. You can't add it again.");

	if (p_id >= databags.size()) {
		const uint32_t start = databags.size();
		databags.resize(p_id + 1);
		for (uint32_t i = start; i < databags.size(); i += 1) {
			databags[i] = nullptr;
		}
	}

	if (databags[p_id] == nullptr) {
		databags[p_id] = ECS::create_databag(p_id);
	}
}

void World::remove_databag(godex::databag_id p_id) {
	ERR_FAIL_COND_MSG(ECS::verify_databag_id(p_id) == false, "The databag is not registered.");
	ERR_FAIL_COND_MSG(p_id == World::get_databag_id(), "The databag `World` is an internal type automatically register by the `World` itself. You can't remove it.");

	if (unlikely(p_id >= databags.size() || databags[p_id] == nullptr)) {
		// Nothing to do.
		return;
	}

	memdelete(databags[p_id]);
	databags[p_id] = nullptr;
}

godex::Databag *World::get_databag(godex::databag_id p_id) {
	CRASH_COND_MSG(p_id == UINT32_MAX, "The databag is not registered.");

	if (unlikely(p_id >= databags.size())) {
		return nullptr;
	}

	return databags[p_id];
}

const godex::Databag *World::get_databag(godex::databag_id p_id) const {
	CRASH_COND_MSG(p_id == UINT32_MAX, "The databag is not registered.");

	if (unlikely(p_id >= databags.size())) {
		return nullptr;
	}

	return databags[p_id];
}
