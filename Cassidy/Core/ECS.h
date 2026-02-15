#pragma once
#include <vector>
#include <unordered_map>
#include <unordered_set>

/* 
	Archetype-based ECS.
	
	Archetypes are tables of entities, where the rows of the table are
	entities and the columns are the components of the entity.

	Based on Sander Martens's writing on ECS:
	https://ajmmertens.medium.com/building-an-ecs-1-where-are-my-entities-and-components-63d07c7da742
*/ 

namespace cassidy::ECS
{
	typedef uint32_t Entity;
	typedef uint32_t ComponentID;
	typedef uint32_t ArchetypeID;

	class World
	{
	public:

	private:

	};

	// Use std::type_index for RTTI to store component IDs?
	// Could store std::type_index(typeid(component)) in Type part of archetype
	// https://en.cppreference.com/w/cpp/types/type_index.html
	// https://stackoverflow.com/questions/20022546/why-use-stdtype-index-instead-of-stdtype-info

	using Type = std::vector<ComponentID>;
	struct Column {
		void* elements;			// Pointer to beginning of component data
		size_t elementSize;	// sizeof(components)
		size_t count;				// Number of components
	};
	struct Archetype
	{
		ArchetypeID id;
		Type type;
		std::vector<Column> components;
	};

	// Used to find an archetype given a type (list of components):
	std::unordered_map<Type, Archetype> archetypeIndex;

	// Used to find archetype a given entity belongs to and acquire
	// component from it:
	struct EntityRecord
	{
		Archetype& archetype;
		size_t rowIndex;
	};
	std::unordered_map<Entity, EntityRecord> entityIndex;

	// Used to find list of archetypes that contain a given component:
	struct ArchetypeRecord
	{
		size_t columnIndex;
	};
	using ArchetypeMap = std::unordered_map<ArchetypeID, ArchetypeRecord>;
	std::unordered_map<ComponentID, ArchetypeMap> componentIndex;

	static bool hasComponent(Entity entity, ComponentID component)
	{
		Archetype& archetype = entityIndex[entity].archetype;
		ArchetypeMap& map = componentIndex[component];
		return map.count(archetype.id) != 0;
	}

	template <typename T>
	static T* getComponent(Entity entity, ComponentID component)
	{
		EntityRecord& entityRecord = entityIndex[entity];
		Archetype& archetype = entityRecord.archetype;

		// Early-out if archetype does not have component:
		ArchetypeMap archetypes = componentIndex[component];
		if (archetypes.count(archetype.id) == 0)
			return nullptr;

		ArchetypeRecord& archetypeRecord = archetypes[archetype.id];
		return archetype.components[archetypeRecord.columnIndex][entityRecord.rowIndex];
	}

	static void addComponent(Entity entity, ComponentID component)
	{
		
	}
}