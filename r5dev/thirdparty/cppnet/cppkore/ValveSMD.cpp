#include "stdafx.h"
#include "ValveSMD.h"

#include "File.h"
#include "Path.h"
#include "MathHelper.h"
#include "StreamWriter.h"

namespace Assets::Exporters
{
	void ProcessVertex(IO::StreamWriter& Writer, const Vertex& Vertex)
	{
		auto Normal = Vertex.Normal().GetNormalized();

		auto& Position = Vertex.Position();
		auto& UVLayer = Vertex.UVLayers(0);

		Writer.WriteFmt("0 %f %f %f %f %f %f %f %f %d ", Position.X, Position.Y, Position.Z, Normal.X, Normal.Y, Normal.Z, UVLayer.U, (1 - UVLayer.V), Vertex.WeightCount());

		for (uint8_t i = 0; i < Vertex.WeightCount(); i++)
			Writer.WriteFmt("%d %f ", Vertex.Weights(i).Bone, Vertex.Weights(i).Value);

		Writer.Write("\n");
	}

	bool ValveSMD::ExportAnimation(const Animation& Animation, const string& Path)
	{
		return false;
	}

	bool ValveSMD::ExportModel(const Model& Model, const string& Path)
	{
		auto Writer = IO::StreamWriter(IO::File::Create(Path));

		Writer.WriteLine(
			"version 1\n"
			"nodes"
		);

		uint32_t BoneIndex = 0;

		for (auto& Bone : Model.Bones)
		{
			Writer.WriteLineFmt("%d \"%s\" %d", BoneIndex, (char*)Bone.Name(), Bone.Parent());
			BoneIndex++;
		}

		Writer.WriteLine(
			"end\n"
			"skeleton\n"
			"time 0"
		);

		BoneIndex = 0;

		for (auto& Bone : Model.Bones)
		{
			auto Euler = Bone.LocalRotation().ToEulerAngles();

			Writer.WriteLineFmt("%d %f %f %f %f %f %f", BoneIndex, Bone.LocalPosition().X, Bone.LocalPosition().Y, Bone.LocalPosition().Z, MathHelper::DegreesToRadians(Euler.X), MathHelper::DegreesToRadians(Euler.Y), MathHelper::DegreesToRadians(Euler.Z));
			BoneIndex++;
		}

		Writer.WriteLine("end");

		for (auto& Submesh : Model.Meshes)
		{
			Writer.WriteLine("triangles");

			for (auto& Face : Submesh.Faces)
			{
				if (Submesh.MaterialIndices[0] > -1)
					Writer.WriteLine(Model.Materials[Submesh.MaterialIndices[0]].Name);
				else
					Writer.WriteLine("default_material");

				ProcessVertex(Writer, Submesh.Vertices[Face[2]]);
				ProcessVertex(Writer, Submesh.Vertices[Face[1]]);
				ProcessVertex(Writer, Submesh.Vertices[Face[0]]);
			}

			Writer.WriteLine("end");
		}

		return true;
	}

	imstring ValveSMD::ModelExtension()
	{
		return ".smd";
	}

	imstring ValveSMD::AnimationExtension()
	{
		return ".smd";
	}

	ExporterScale ValveSMD::ExportScale()
	{
		// Evaluate whether or not SMDs need a scale constant.
		return ExporterScale::Default;
	}

	bool ValveSMD::SupportsAnimations()
	{
		return true;
	}

	bool ValveSMD::SupportsModels()
	{
		return true;
	}
}
