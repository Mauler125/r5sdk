#pragma once

#include <memory>
#include <cstdint>
#include "ListBase.h"
#include "StringBase.h"

// The separate model parts
#include "Bone.h"
#include "Mesh.h"
#include "Material.h"

// Hash algorithm for materials
#include "XXHash.h"

namespace Assets
{
	// A container class that holds 3D model data.
	class Model
	{
	public:
		// Initialize a blank 3D model without any known count information.
		Model();
		// Initialize a 3D model with the given model count for meshes.
		Model(uint32_t SubmeshCount);
		// Initialize a 3D model with the given model counts for meshes, and bones.
		Model(uint32_t SubmeshCount, uint32_t BoneCount);
		// Initialize a 3D model with the given model counts for meshes, bones, and materials.
		Model(uint32_t SubmeshCount, uint32_t BoneCount, uint32_t MaterialCount);
		// Destroy all 3D model resources.
		~Model() = default;

		// The name of the model
		string Name;

		// Ensure that our model is not copied or assigned to for performance reasons.
		Model(const Model&) = delete;
		Model& operator=(Model const&) = delete;

		// A collection of 3D bones for this model.
		List<Bone> Bones;
		// A collection of 3D meshes for this model.
		List<Mesh> Meshes;
		// A collection of 3D materials for this model.
		List<Material> Materials;

		// Adds a material to the collection if it doesn't already exist, returning it's index.
		template<typename T>
		uint32_t AddMaterial(const string& MaterialName, const T& SourceMap = T())
		{
			auto MaterialHashCode = Hashing::XXHash::HashString(MaterialName);

			for (uint32_t i = 0; i < Materials.Count(); i++)
				if (Materials[i].Hash == MaterialHashCode)
					return i;

			if constexpr (std::is_integral<T>::value) {
				auto& Mat = Materials.Emplace(MaterialName, MaterialHashCode);
				Mat.SourceHash = Mat.Hash = SourceMap;
			}
			else {
				Materials.Emplace(MaterialName, MaterialHashCode).SourceString = SourceMap;
			}

			return Materials.Count() - 1;
		}

		// Calculates the total number of vertices that make up this model.
		uint32_t VertexCount() const;
		// Calculates the total number of faces that make up this model.
		uint32_t FaceCount() const;

		// Calculates local transformations from the globals.
		void GenerateLocalTransforms(bool Position, bool Rotation);
		// Calculates global transformations from the locals.
		void GenerateGlobalTransforms(bool Position, bool Rotation);

		// Calculates vertex normals from existing vertices.
		void CalculateVertexNormals();

		// Scales the model with the given factor.
		void Scale(float Factor);
	};
}