#pragma once

/* Author: AndreaCatania */

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"

#include "systems/system.h"
#include "systems/system_builder.h"

class World;
class Pipeline;
class DynamicComponentInfo;

namespace godex {
class Databag;
class DynamicSystemInfo;
} // namespace godex

/// These functions are implemented by the `COMPONENT` macro and assigned during
/// component registration.
struct ComponentInfo {
	LocalVector<PropertyInfo> *(*get_properties)();
	Variant (*get_property_default)(StringName p_property_name);
	Storage *(*create_storage)();
	DynamicComponentInfo *dynamic_component_info = nullptr;
};

/// These functions are implemented by the `DATABAG` macro and assigned during
/// component registration.
struct DatabagInfo {
	godex::Databag *(*create_databag)();
};

struct SystemInfo {
	String description;
	uint32_t dynamic_system_id = UINT32_MAX;
	func_get_system_exe_info exec_info;
};

class ECS : public Object {
	GDCLASS(ECS, Object)

	friend class Main;

	static ECS *singleton;
	static LocalVector<StringName> components;
	static LocalVector<ComponentInfo> components_info;

	static LocalVector<StringName> databags;
	static LocalVector<DatabagInfo> databags_info;

	static LocalVector<StringName> systems;
	static LocalVector<SystemInfo> systems_info;

	World *active_world = nullptr;
	Pipeline *active_world_pipeline = nullptr;
	bool dispatching = false;
	DataAccessorScriptInstance<godex::Databag> *world_accessor;
	Object world_access;

public:
	// ~~ Components ~~
	template <class C>
	static void register_component();

	// TODO specify the storage here?
	static uint32_t register_script_component(StringName p_name, const LocalVector<ScriptProperty> &p_properties, StorageType p_storage_type);

	static bool verify_component_id(uint32_t p_component_id);

	static Storage *create_storage(godex::component_id p_component_id);
	static const LocalVector<StringName> &get_registered_components();
	static godex::component_id get_component_id(StringName p_component_name);
	static StringName get_component_name(godex::component_id p_component_id);
	static const LocalVector<PropertyInfo> *get_component_properties(godex::component_id p_component_id);
	static Variant get_component_property_default(godex::component_id p_component_id, StringName p_property_name);

	// ~~ Databags ~~
	template <class C>
	static void register_databag();

	static bool verify_databag_id(godex::databag_id p_id);

	static godex::Databag *create_databag(godex::databag_id p_id);
	static uint32_t get_databag_count();
	static godex::databag_id get_databag_id(const StringName &p_name);
	static StringName get_databag_name(godex::databag_id p_databag_id);

	// ~~ Systems ~~
	static void register_system(func_get_system_exe_info p_func_get_exe_info, StringName p_name, String p_description = "");

// By defining the same name of the method, the IDE autocomplete shows the
// method name `register_system`, properly + it's impossible use the function
// directly by mistake.
#define register_system(func, name, desc)                           \
	register_system([](SystemExeInfo &r_info) {                     \
		SystemBuilder::get_system_info_from_function(r_info, func); \
		r_info.system_func = [](World *p_world) {                   \
			SystemBuilder::system_exec_func(p_world, func);         \
		};                                                          \
	},                                                              \
			name, desc)

	// Register the system and returns the ID.
	static godex::system_id register_dynamic_system(StringName p_name, const String &p_description = String());

	// This macro save the user the need to pass a `SystemExeInfo`, indeed it wraps
	// the passed function with a labda function that creates a `SystemExeInfo`.
	/// Returns the system id or UINT32_MAX if not found.
	static godex::system_id get_system_id(const StringName &p_name);
	static uint32_t get_systems_count();

	/// Returns the function that can be used to obtain the `SystemExeInfo`.
	static func_get_system_exe_info get_func_system_exe_info(godex::system_id p_id);

	/// Returns the `SystemExeInfo`.
	static void get_system_exe_info(godex::system_id p_id, SystemExeInfo &r_info);
	static StringName get_system_name(godex::system_id p_id);
	static String get_system_desc(godex::system_id p_id);

	static void set_dynamic_system_target(godex::system_id p_id, ScriptInstance *p_target);
	static godex::DynamicSystemInfo *get_dynamic_system_info(godex::system_id p_id);

	/// Returns `true` when the system dispatches a pipeline when executed.
	static bool is_system_dispatcher(godex::system_id p_id);
	/// Set the `SystemDispatcher` pipeline, does nothing if this system is not
	/// a `SystemDispatcher`.
	static void set_system_pipeline(godex::system_id p_id, Pipeline *p_pipeline);

	static bool verify_system_id(godex::system_id p_id);

protected:
	static void _bind_methods();

public:
	static void __set_singleton(ECS *p_singleton);
	static ECS *get_singleton();

public:
	ECS();

	virtual ~ECS();

	/// Set the active world. If there is already an active world an error
	/// is generated.
	void set_active_world(World *p_world);
	World *get_active_world() const;
	Object *get_active_world_gds();

	bool has_active_world() const;

	void set_active_world_pipeline(Pipeline *p_pipeline);
	Pipeline *get_active_world_pipeline() const;

	bool has_active_world_pipeline() const;

	godex::component_id get_component_id_obj(StringName p_component_name) const {
		return get_component_id(p_component_name);
	}

	bool verify_component_id_obj(godex::system_id p_id) const {
		return verify_component_id(p_id);
	}

	godex::databag_id get_databag_id_obj(StringName p_databag_name) const {
		return get_databag_id(p_databag_name);
	}

	bool verify_databag_id_obj(godex::databag_id p_id) const {
		return verify_databag_id(p_id);
	}

	godex::system_id get_system_id_obj(StringName p_system_name) const {
		return get_system_id(p_system_name);
	}

	bool verify_system_id_obj(godex::system_id p_id) const {
		return verify_system_id(p_id);
	}

private:
	void dispatch_active_world();
	void ecs_init();
};

template <class C>
void ECS::register_component() {
	ERR_FAIL_COND_MSG(C::get_component_id() != UINT32_MAX, "This component is already registered.");

	StringName component_name = C::get_class_static();
	C::component_id = components.size();
	C::_bind_properties();
	components.push_back(component_name);
	components_info.push_back(
			ComponentInfo{
					&C::get_properties_static,
					&C::get_property_default_static,
					&C::create_storage_no_type,
					nullptr });
}

template <class R>
void ECS::register_databag() {
	ERR_FAIL_COND_MSG(R::get_databag_id() != UINT32_MAX, "This databag is already registered.");

	StringName databag_name = R::get_class_static();
	R::databag_id = databags.size();
	R::_bind_properties();
	databags.push_back(databag_name);
	databags_info.push_back(DatabagInfo{
			R::create_databag_no_type });
}
