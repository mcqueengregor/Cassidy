#pragma once
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Based on Sander Martens's writing on ECS.
// https://ajmmertens.medium.com/building-an-ecs-1-where-are-my-entities-and-components-63d07c7da742

namespace cassidy::ECS
{
	typedef uint64_t Entity;
	typedef uint32_t ComponentID;
	typedef uint32_t ArchetypeID;

	class Registry
	{
	public:

	private:

	};

	struct Column {
		void* elements;			// Pointer to beginning of component data
		size_t elementSize;	// sizeof(component)
		size_t count;				// Number of components
	};

	using Type = std::vector<ComponentID>;
	struct Archetype
	{
		ArchetypeID id;
		Type type;
		std::vector<Column> components;
	};

	// Used to find an archetype given a type (list of components):
	std::unordered_map<Type, Archetype> archetypeIndex;

	// Used to find archetype a given entity belongs to:
	std::unordered_map<Entity, Archetype&> entityIndex;

	// Used to find list of archetypes that contain a given component:
	using ArchetypeSet = std::unordered_set<ArchetypeID>;
	std::unordered_map<ComponentID, ArchetypeSet> componentIndex;

	static bool hasComponent(Entity entity, ComponentID component)
	{
		Archetype& archetype = entityIndex[entity];
		ArchetypeSet& set = componentIndex[component];
		return set.count(archetype.id) != 0;
	}
}