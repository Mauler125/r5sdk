#include "stdafx.h"
#include "Model.h"
#include "Matrix.h"

namespace Assets
{
	Model::Model()
		: Model(0, 0, 0)
	{
	}

	Model::Model(uint32_t SubmeshCount)
		: Model(SubmeshCount, 0, 0)
	{
	}

	Model::Model(uint32_t SubmeshCount, uint32_t BoneCount)
		: Model(SubmeshCount, BoneCount, 0)
	{
	}

	Model::Model(uint32_t SubmeshCount, uint32_t BoneCount, uint32_t MaterialCount)
		: Meshes(SubmeshCount), Bones(BoneCount), Materials(MaterialCount), Name("error")
	{
	}

	uint32_t Model::VertexCount() const
	{
		uint32_t Result = 0;

		for (auto& Mesh : Meshes)
			Result += Mesh.Vertices.Count();

		return Result;
	}

	uint32_t Model::FaceCount() const
	{
		uint32_t Result = 0;

		for (auto& Mesh : Meshes)
			Result += Mesh.Faces.Count();

		return Result;
	}

	void Model::GenerateLocalTransforms(bool Position, bool Rotation)
	{
		for (auto& Bone : Bones)
		{
			if (Bone.Parent() != -1)
			{
				if (Position)
				{
					auto ParentMatrix = Matrix::CreateFromQuaternion(~Bones[Bone.Parent()].GlobalRotation());

					// Inverse parent matrix * parent position difference
					Bone.SetLocalPosition(Matrix::TransformVector(Bone.GlobalPosition() - Bones[Bone.Parent()].GlobalPosition(), ParentMatrix));
				}

				// Inverse parent rotation * rotation
				if (Rotation)
					Bone.SetLocalRotation(~Bones[Bone.Parent()].GlobalRotation() * Bone.GlobalRotation());
			}
			else
			{
				// The global transform is the same as the local
				if (Position)
					Bone.SetLocalPosition(Bone.GlobalPosition());
				if (Rotation)
					Bone.SetLocalRotation(Bone.GlobalRotation());
			}
		}
	}

	void Model::GenerateGlobalTransforms(bool Position, bool Rotation)
	{
		for (auto& Bone : Bones)
		{
			if (Bone.Parent() != -1)
			{
				if (Position)
				{
					auto& ParentRotation = Bones[Bone.Parent()].GlobalRotation();
					auto& ParentPosition = Bones[Bone.Parent()].GlobalPosition();
					auto& LocalPosition = Bone.LocalPosition();

					auto PositionQuaternion = Quaternion(LocalPosition.X, LocalPosition.Y, LocalPosition.Z, 0);

					// Parent rotation * local position transform, then * inverse parent rotation
					auto Result = (ParentRotation * PositionQuaternion) * ~ParentRotation;
					// Add to the parent's global position
					Bone.SetGlobalPosition({ ParentPosition.X + Result.X, ParentPosition.Y + Result.Y, ParentPosition.Z + Result.Z });
				}

				// Parent rotation * local rotation transform
				if (Rotation)
					Bone.SetGlobalRotation(Bones[Bone.Parent()].GlobalRotation() * Bone.LocalRotation());
			}
			else
			{
				// The local transform is the same as the global
				if (Position)
					Bone.SetGlobalPosition(Bone.LocalPosition());
				if (Rotation)
					Bone.SetGlobalRotation(Bone.LocalRotation());
			}
		}
	}

	void Model::CalculateVertexNormals()
	{
		for (auto& Mesh : Meshes)
		{
			for (auto& Vertex : Mesh.Vertices)
				Vertex.SetNormal({ 0, 0, 0 });

			for (auto& Face : Mesh.Faces)
			{
				auto CrossProduct = (Mesh.Vertices[Face[1]].Position() - Mesh.Vertices[Face[0]].Position()).Cross(Mesh.Vertices[Face[2]].Position() - Mesh.Vertices[Face[0]].Position());

				Mesh.Vertices[Face[0]].SetNormal(Mesh.Vertices[Face[0]].Normal() + CrossProduct);
				Mesh.Vertices[Face[1]].SetNormal(Mesh.Vertices[Face[1]].Normal() + CrossProduct);
				Mesh.Vertices[Face[2]].SetNormal(Mesh.Vertices[Face[2]].Normal() + CrossProduct);
			}

			for (auto& Vertex : Mesh.Vertices)
				Vertex.SetNormal(Vertex.Normal().GetNormalized());
		}
	}

	void Model::Scale(float Factor)
	{
		for (auto& Bone : Bones)
		{
			Bone.SetLocalPosition(Bone.LocalPosition() * Factor);
			Bone.SetGlobalPosition(Bone.GlobalPosition() * Factor);
		}

		for (auto& Mesh : Meshes)
		{
			for (auto& Vertex : Mesh.Vertices)
				Vertex.SetPosition(Vertex.Position() * Factor);
		}
	}
}
