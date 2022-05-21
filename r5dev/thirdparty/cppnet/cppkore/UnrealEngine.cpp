#include "stdafx.h"
#include "UnrealEngine.h"

#include "File.h"
#include "Path.h"
#include "MathHelper.h"
#include "BinaryWriter.h"

namespace Assets::Exporters
{
#pragma pack(push, 1)
	struct ChunkHeader
	{
		char ChunkID[20];
		uint32_t TypeFlags;
		uint32_t DataSize;
		uint32_t DataCount;

		ChunkHeader(const char* ID, uint32_t DataSize, uint32_t DataCount)
			: TypeFlags(0x1E83B9), DataSize(DataSize), DataCount(DataCount)
		{
			std::memcpy(ChunkID, ID, strlen(ID));
		}
	};

	struct MaterialHeader
	{
		char MaterialName[64];
		uint32_t MaterialIndex;
		uint32_t PolyFlags;
		uint32_t Padding[0x4];
	};

	struct AnimInfoHeader
	{
		char AnimName[64];
		char Group[64];
		uint32_t BoneCount;
		uint32_t RootInclude;
		uint32_t CompressionFlags;
		uint32_t KeyQuotum;
		float KeyReduction;
		float TrackTime;
		float AnimationRate;
		uint32_t StartBone;
		uint32_t FirstRawFrame;
		uint32_t RawFrameCount;
	};

	struct BoneHeader
	{
		char BoneName[64];
		uint32_t Flags;
		uint32_t NumChildren;
		uint32_t ParentIndex;

		Math::Quaternion Rotation;
		Math::Vector3 Position;

		float Unused[4];
	};

	struct BoneKey
	{
		Math::Vector3 Position;
		Math::Quaternion Rotation;
		float Time;
	};
#pragma pack(pop)

	bool UnrealEngine::ExportAnimation(const Animation& Animation, const string& Path)
	{
		auto Writer = IO::BinaryWriter(IO::File::Create(Path));

		Writer.Write<ChunkHeader>({ "ANIMHEAD", 0, 0 });

		auto BoneCount = Animation.Bones.Count();

		Writer.Write<ChunkHeader>({ "BONENAMES", sizeof(BoneHeader), BoneCount });

		uint32_t BoneIndex = 0;
		for (auto& Bone : Animation.Bones)
		{
			BoneHeader Header{};
			std::memcpy(Header.BoneName, Bone.Name().ToCString(), Bone.Name().Length() > 64 ? 64 : Bone.Name().Length());

			Header.Rotation = Bone.LocalRotation();
			Header.Position = Bone.LocalPosition();

			Header.ParentIndex = (Bone.Parent() == -1) ? 0 : Bone.Parent();

			uint32_t ChildCount = 0;
			for (auto& BChild : Animation.Bones)
				if (BChild.Parent() == BoneIndex)
					ChildCount++;

			Header.NumChildren = ChildCount;

			Writer.Write<BoneHeader>(Header);

			BoneIndex++;
		}

		Writer.Write<ChunkHeader>({ "ANIMINFO", sizeof(AnimInfoHeader), 1 });

		{
			AnimInfoHeader Header{};
			std::memcpy(Header.AnimName, "CppKoreAnimation", 17);
			std::memcpy(Header.Group, "None", 4);

			auto FrameCount = Animation.FrameCount();

			Header.BoneCount = Animation.Bones.Count();
			Header.RootInclude = 1;
			Header.TrackTime = (float)FrameCount;
			Header.RawFrameCount = FrameCount;
			Header.KeyQuotum = (Header.BoneCount * Header.RawFrameCount);

			Writer.Write<AnimInfoHeader>(Header);
		}

		List<BoneKey> AnimKeys;

		// TODO: Build animation keys (UE3 PSA)
		// foreach bone:
		// foreach frame:
		// output value at time... POS/ROT

		Writer.Write<ChunkHeader>({ "ANIMKEYS", sizeof(BoneKey), AnimKeys.Count() });

		{
			Writer.Write((uint8_t*)&AnimKeys[0], 0, AnimKeys.Count() * sizeof(BoneKey));
		}

		return true;
	}

	bool UnrealEngine::ExportModel(const Model& Model, const string& Path)
	{
		auto Writer = IO::BinaryWriter(IO::File::Create(Path));

		Writer.Write<ChunkHeader>({ "ACTRHEAD", 0, 0 });

		auto VertexCount = Model.VertexCount();
		auto FaceCount = Model.FaceCount();
		auto MatCount = Model.Materials.Count();
		auto BoneCount = Model.Bones.Count();

		Writer.Write<ChunkHeader>({ "PNTS0000", 0xC, VertexCount });

		for (auto& Submesh : Model.Meshes)
			for (auto& Vertex : Submesh.Vertices)
				Writer.Write<Vector3>(Vertex.Position());

		Writer.Write<ChunkHeader>({ "VTXW0000", 0x10, VertexCount });

		for (auto& Submesh : Model.Meshes)
		{
			for (uint32_t i = 0; i < Submesh.Vertices.Count(); i++)
			{
				auto Vertex = Submesh.Vertices[i];

				Writer.Write<uint32_t>(i);
				Writer.Write<Vector2>(Vertex.UVLayers(0));
				Writer.Write<uint32_t>(0x0);
			}
		}

		Writer.Write<ChunkHeader>({ "FACE0032", 0x12, FaceCount });

		uint32_t FaceShift = 0x0;

		for (auto& Submesh : Model.Meshes)
		{
			for (auto& Face : Submesh.Faces)
			{
				Writer.Write<uint32_t>(Face[0] + FaceShift);
				Writer.Write<uint32_t>(Face[1] + FaceShift);
				Writer.Write<uint32_t>(Face[2] + FaceShift);
				
				Writer.Write<uint16_t>(Submesh.MaterialIndices.Count() > 0 ? Submesh.MaterialIndices[0] : 0);
				Writer.Write<uint32_t>(0x0);
			}

			FaceShift += Submesh.Vertices.Count();
		}

		Writer.Write<ChunkHeader>({ "MATT0000", 0x58, MatCount });

		for (auto& Material : Model.Materials)
		{
			MaterialHeader Header{};
			std::memcpy(Header.MaterialName, Material.Name.ToCString(), Material.Name.Length() > 64 ? 64 : Material.Name.Length());

			Writer.Write<MaterialHeader>(Header);
		}

		Writer.Write<ChunkHeader>({ "REFSKELT", 0x78, BoneCount });

		uint32_t BoneIndex = 0;
		for (auto& Bone : Model.Bones)
		{
			BoneHeader Header{};
			std::memcpy(Header.BoneName, Bone.Name().ToCString(), Bone.Name().Length() > 64 ? 64 : Bone.Name().Length());

			Header.Rotation = Bone.LocalRotation();
			Header.Position = Bone.LocalPosition();

			Header.ParentIndex = (Bone.Parent() == -1) ? 0 : Bone.Parent();

			uint32_t ChildCount = 0;
			for (auto& BChild : Model.Bones)
				if (BChild.Parent() == BoneIndex)
					ChildCount++;

			Header.NumChildren = ChildCount;

			Writer.Write<BoneHeader>(Header);

			BoneIndex++;
		}

		uint32_t WeightCount = 0;

		for (auto& Submesh : Model.Meshes)
			for (auto& Vertex : Submesh.Vertices)
				WeightCount += Vertex.WeightCount();

		Writer.Write<ChunkHeader>({ "RAWWEIGHTS", 0xC, WeightCount });

		FaceShift = 0;

		for (auto& Submesh : Model.Meshes)
		{
			for (auto& Vertex : Submesh.Vertices)
			{
				for (uint8_t i = 0; i < Vertex.WeightCount(); i++)
				{
					auto& Weight = Vertex.Weights(i);

					Writer.Write<float>(Weight.Value);
					Writer.Write<uint32_t>(FaceShift);
					Writer.Write<uint32_t>(Weight.Bone);
				}

				FaceShift++;
			}
		}

		return true;
	}

	imstring UnrealEngine::ModelExtension()
	{
		return ".psk";
	}

	imstring UnrealEngine::AnimationExtension()
	{
		return ".psa";
	}

	ExporterScale UnrealEngine::ExportScale()
	{
		return ExporterScale::Default;
	}

	bool UnrealEngine::SupportsAnimations()
	{
		return true;
	}

	bool UnrealEngine::SupportsModels()
	{
		return true;
	}
}
