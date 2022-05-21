#include "stdafx.h"
#include "Material.h"
#include "XXHash.h"

namespace Assets
{
	Material::Material()
		: Material("default_material", Hashing::XXHash::ComputeHash((uint8_t*)"default_material", 0, 16))
	{
	}

	Material::Material(const string& Name, const uint64_t Hash)
		: Name(Name), Hash(Hash), SourceHash((uint64_t)-1), SourceString("")
	{
	}
}
