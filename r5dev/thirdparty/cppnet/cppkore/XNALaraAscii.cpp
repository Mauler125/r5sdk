#include "stdafx.h"
#include "XNALaraAscii.h"

#include "File.h"
#include "Path.h"
#include "StreamWriter.h"

namespace Assets::Exporters
{
	bool XNALaraAscii::ExportAnimation(const Animation& Animation, const string& Path)
	{
		return false;
	}

	bool XNALaraAscii::ExportModel(const Model& Model, const string& Path)
	{
		auto Writer = IO::StreamWriter(IO::File::Create(Path));

		Writer.WriteLineFmt("%d", Model.Bones.Count());

		for (auto& Bone : Model.Bones)
		{
			Writer.WriteLineFmt(
				"%s\n"
				"%d\n"
				"%f %f %f",
				(char*)Bone.Name(),
				Bone.Parent(),
				Bone.GlobalPosition().X, Bone.GlobalPosition().Y, Bone.GlobalPosition().Z
			);
		}

		Writer.WriteLineFmt("%d", Model.Meshes.Count());

		uint32_t SubmeshIndex = 0;

		for (auto& Submesh : Model.Meshes)
		{
			Writer.WriteLineFmt(
				"KoreMesh%02d\n"
				"%d\n"
				"1",
				SubmeshIndex,
				Submesh.Vertices.UVLayerCount()
			);

			Writer.WriteLineFmt(
				"%s\n"
				"0\n"
				"%d",
				Submesh.MaterialIndices[0] > -1 ? (char*)Model.Materials[Submesh.MaterialIndices[0]].Name : "default_material",
				Submesh.Vertices.Count()
			);

			for (auto& Vertex : Submesh.Vertices)
			{
				auto& Position = Vertex.Position();
				auto& Normal = Vertex.Normal();
				auto& Color = Vertex.Color();

				Writer.WriteLineFmt(
					"%f %f %f\n"
					"%f %f %f\n"
					"%d %d %d %d",
					Position.X, Position.Y, Position.Z,
					Normal.X, Normal.Y, Normal.Z,
					Color[0], Color[1], Color[2], Color[3]
				);

				for (uint8_t i = 0; i < Vertex.UVLayerCount(); i++)
					Writer.WriteLineFmt("%f %f", Vertex.UVLayers(i).U, Vertex.UVLayers(i).V);

				uint32_t Bones[] = { 0, 0, 0, 0 };
				float Values[] = { 0.f, 0.f, 0.f, 0.f };

				for (uint8_t i = 0; i < std::min<uint8_t>(Vertex.WeightCount(), 4); i++)
				{
					auto& Weight = Vertex.Weights(i);

					Bones[i] = Weight.Bone;
					Values[i] = Weight.Value;
				}

				Writer.WriteLineFmt(
					"%d %d %d %d\n"
					"%f %f %f %f",
					Bones[0], Bones[1], Bones[2], Bones[3],
					Values[0], Values[1], Values[2], Values[3]
				);
			}

			Writer.WriteLineFmt("%d", Submesh.Faces.Count());

			for (auto& Face : Submesh.Faces)
				Writer.WriteLineFmt("%d %d %d", Face[0], Face[1], Face[2]);

			SubmeshIndex++;
		}

		return true;
	}

	imstring XNALaraAscii::ModelExtension()
	{
		return ".mesh.ascii";
	}

	imstring XNALaraAscii::AnimationExtension()
	{
		return nullptr;
	}

	ExporterScale XNALaraAscii::ExportScale()
	{
		// Evaluate whether or not XNALara needs a scale constant.
		return ExporterScale::Default;
	}

	bool XNALaraAscii::SupportsAnimations()
	{
		return false;
	}

	bool XNALaraAscii::SupportsModels()
	{
		return true;
	}
}
