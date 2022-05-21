#pragma once

#include <cstdint>
#include "StringBase.h"
#include "DictionaryBase.h"

namespace Assets
{
	// Represents the supported 3D material slots
	enum class MaterialSlotType : uint32_t
	{
		// No valid mapping, but it's here
		Invalid,

		// Albedo color, different from diffuse
		Albedo,
		// Full color with lighing
		Diffuse,

		// Normal map, most likely XY compressed
		Normal,

		// Reflectivity
		Specular,

		// Glow / coloring
		Emissive,
		
		// Gloss is inverted roughness
		Gloss,
		// Roughness is inverted gloss
		Roughness,

		// Generalized lighting AO
		AmbientOcclusion,
		// More focused lighting AO
		Cavity,
	};

	// Represents a 3D material and provided diffuse, normal, and specular images.
	class Material
	{
	public:
		// Initialize a new default material.
		Material();
		// Initialize a new material.
		Material(const string& Name, const uint64_t Hash);
		// Destroy all material info.
		~Material() = default;

		// The unique name for this material.
		string Name;

		// Name of the source identifier for this material.
		string SourceString;
		// Hash of the source identifier for this material.
		uint64_t SourceHash;
		
		// The material texture slots
		Dictionary<MaterialSlotType, std::pair<string, uint64_t>> Slots;

		// The unique hash identifier for this material.
		uint64_t Hash;
	};
}