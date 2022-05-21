#include "stdafx.h"
#include "XNALaraBinary.h"

#include "File.h"
#include "Path.h"
#include "BinaryWriter.h"

namespace Assets::Exporters
{
	bool XNALaraBinary::ExportAnimation(const Animation& Animation, const string& Path)
	{
		return false;
	}

	bool XNALaraBinary::ExportModel(const Model& Model, const string& Path)
	{
		auto Writer = IO::BinaryWriter(IO::File::Create(Path));

		Writer.Write<uint32_t>(Model.Bones.Count());

		for (auto& Bone : Model.Bones)
		{
			Writer.WriteNetString(Bone.Name());
			Writer.Write<int16_t>((int16_t)Bone.Parent());
			Writer.Write<Vector3>(Bone.GlobalPosition());
		}

		Writer.Write<uint32_t>(Model.Meshes.Count());

		uint32_t SubmeshIndex = 0;

		for (auto& Submesh : Model.Meshes)
		{
			Writer.WriteNetString(string::Format("KoreMesh%02d", SubmeshIndex));
			Writer.Write<uint32_t>(Submesh.Vertices.UVLayerCount());
			Writer.Write<uint32_t>(Submesh.Vertices.UVLayerCount());

			for (uint8_t i = 0; i < Submesh.Vertices.UVLayerCount(); i++)
			{
				Writer.WriteNetString(Submesh.MaterialIndices[i] > -1 ? Model.Materials[Submesh.MaterialIndices[i]].Name : string("default_material"));
				Writer.Write<uint32_t>(i);
			}

			Writer.Write<uint32_t>(Submesh.Vertices.Count());

			for (auto& Vertex : Submesh.Vertices)
			{
				Writer.Write<Vector3>(Vertex.Position());
				Writer.Write<Vector3>(Vertex.Normal());
				Writer.Write(&Vertex.Color()[0], 0, sizeof(uint8_t) * 4);

				for (uint8_t i = 0; i < Vertex.UVLayerCount(); i++)
					Writer.Write<Vector2>(Vertex.UVLayers(i));

				for (uint8_t i = 0; i < Vertex.UVLayerCount(); i++)
					Writer.Write<Quaternion>({ 0, 0, 0, 0 });

				uint16_t Bones[] = { 0, 0, 0, 0 };
				float Values[] = { 0.f, 0.f, 0.f, 0.f };

				for (uint8_t i = 0; i < std::min<uint8_t>(Vertex.WeightCount(), 4); i++)
				{
					Bones[i] = Vertex.Weights(i).Bone;
					Values[i] = Vertex.Weights(i).Value;
				}

				Writer.Write((uint8_t*)&Bones[0], 0, sizeof(uint16_t) * 4);
				Writer.Write((uint8_t*)&Values[0], 0, sizeof(float) * 4);
			}

			Writer.Write<uint32_t>(Submesh.Faces.Count());

			for (auto& Face : Submesh.Faces)
				Writer.Write((uint8_t*)&Face[0], 0, sizeof(uint32_t) * 3);

			SubmeshIndex++;
		}

		return true;
	}

	imstring XNALaraBinary::ModelExtension()
	{
		return ".mesh";
	}

	imstring XNALaraBinary::AnimationExtension()
	{
		return nullptr;
	}

	ExporterScale XNALaraBinary::ExportScale()
	{
		// Evaluate whether or not we need a scale constant.
		return ExporterScale::Default;
	}

	bool XNALaraBinary::SupportsAnimations()
	{
		return false;
	}

	bool XNALaraBinary::SupportsModels()
	{
		return true;
	}
}
