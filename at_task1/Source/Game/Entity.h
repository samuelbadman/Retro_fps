#pragma once

class Entity
{
public:
	Entity() : ID(entt::entity{}), pEnttRegistry(nullptr) {}
	Entity(entt::entity id, entt::registry* pRegistry) : ID(id), pEnttRegistry(pRegistry) {}

	template<typename T>
	T& AddComponent()
	{
		return pEnttRegistry->emplace<T>(ID);
	}

	template<typename T>
	T& GetComponent()
	{
		return pEnttRegistry->get<T>(ID);
	}

	template<typename T>
	const T& GetComponent() const
	{
		return pEnttRegistry->get<T>(ID);
	}

	template<typename T>
	void DeleteComponent()
	{
		pEnttRegistry->erase<T>(ID);
	}

	template<typename ...Ts>
	bool HasAllComponents() const
	{
		return pEnttRegistry->all_of<Ts...>(ID);
	}

	template<typename ...Ts>
	bool HasAnyOfComponent() const
	{
		return pEnttRegistry->any_of<Ts...>(ID);
	}

	bool Valid() const
	{
		return (pEnttRegistry != nullptr) && (pEnttRegistry->valid(ID));
	}

	entt::entity GetID() const { return ID; }

	bool operator==(const Entity& other) const
	{
		return ID == other.ID;
	}

private:
	entt::entity ID;
	entt::registry* pEnttRegistry;
};