#pragma once
#include "externalLibs.h"
#include "cocoa/core/Core.h"
#include "cocoa/core/Entity.h"

namespace Cocoa
{
	struct Tag
	{
		const char* Name;
		bool Selected;
		bool HasChildren;
	};

	namespace NTag
	{
		COCOA void Serialize(json& j, Entity entity, const Tag& transform);
		COCOA void Deserialize(json& j, Entity entity);
	}
}