#include "stdafx.h"
#include "WavefrontOBJ.h"

#include "File.h"
#include "Path.h"
#include "StreamWriter.h"

namespace Assets::Exporters
{
	bool WavefrontOBJ::ExportAnimation(const Animation& Animation, const string& Path)
	{
		return false;
	}

	bool WavefrontOBJ::ExportModel(const Model& Model, const string& Path)
	{
		auto Writer = IO::StreamWriter(IO::File::Create(Path));
		auto MaterialPath = IO::Path::ChangeExtension(Path, ".mtl");

		Writer.WriteLineFmt("\nmtllib %s\n", (char*)IO::Path::GetFileName(MaterialPath));

		for (auto& Submesh : Model.Meshes)
		{
			for (auto& Face : Submesh.Faces)
			{
				auto& FaceVert1 = Submesh.Vertices[Face[0]].Position();
				auto& FaceVert2 = Submesh.Vertices[Face[1]].Position();
				auto& FaceVert3 = Submesh.Vertices[Face[2]].Position();

				Writer.WriteLineFmt(
					"v %f %f %f\n"
					"v %f %f %f\n"
					"v %f %f %f",
					FaceVert1.X, FaceVert1.Y, FaceVert1.Z,
					FaceVert2.X, FaceVert2.Y, FaceVert2.Z,
					FaceVert3.X, FaceVert3.Y, FaceVert3.Z
				);
			}
		}

		for (auto& Submesh : Model.Meshes)
		{
			for (auto& Face : Submesh.Faces)
			{
				auto& FaceVert1 = Submesh.Vertices[Face[0]].UVLayers(0);
				auto& FaceVert2 = Submesh.Vertices[Face[1]].UVLayers(0);
				auto& FaceVert3 = Submesh.Vertices[Face[2]].UVLayers(0);

				Writer.WriteLineFmt(
					"vt %f %f\n"
					"vt %f %f\n"
					"vt %f %f",
					FaceVert1.U, (1 - FaceVert1.V),
					FaceVert2.U, (1 - FaceVert2.V),
					FaceVert3.U, (1 - FaceVert3.V)
				);
			}
		}

		for (auto& Submesh : Model.Meshes)
		{
			for (auto& Face : Submesh.Faces)
			{
				auto& FaceVert1 = Submesh.Vertices[Face[0]].Normal();
				auto& FaceVert2 = Submesh.Vertices[Face[1]].Normal();
				auto& FaceVert3 = Submesh.Vertices[Face[2]].Normal();

				Writer.WriteLineFmt(
					"vn %f %f %f\n"
					"vn %f %f %f\n"
					"vn %f %f %f",
					FaceVert1.X, FaceVert1.Y, FaceVert1.Z,
					FaceVert2.X, FaceVert2.Y, FaceVert2.Z,
					FaceVert3.X, FaceVert3.Y, FaceVert3.Z
				);
			}
		}

		uint64_t VertexIndex = 1;

		for (auto& Submesh : Model.Meshes)
		{
			if (Submesh.MaterialIndices[0] > -1)
			{
				auto& Material = Model.Materials[Submesh.MaterialIndices[0]];

				Writer.WriteLineFmt(
					"g %s\n"
					"usemtl %s",
					(char*)Material.Name,
					(char*)Material.Name
				);
			}
			else
			{
				Writer.WriteLine(
					"g default_material\n"
					"usemtl default_material"
				);
			}

			for (auto& Face : Submesh.Faces)
			{
				Writer.WriteLineFmt(
					"f %d/%d/%d %d/%d/%d %d/%d/%d",
					(VertexIndex + 2), (VertexIndex + 2), (VertexIndex + 2),
					(VertexIndex + 1), (VertexIndex + 1), (VertexIndex + 1),
					VertexIndex, VertexIndex, VertexIndex
				);

				VertexIndex += 3;
			}
		}

		
		auto MatWriter = IO::StreamWriter(IO::File::Create(MaterialPath));

		for (auto& Material : Model.Materials)
		{
			MatWriter.WriteLineFmt("newmtl %s", (char*)Material.Name);

			MatWriter.WriteLine(
				"illum 4\n"
				"Kd 0.00 0.00 0.00\n"
				"Ka 0.00 0.00 0.00\n"
				"Ks 0.50 0.50 0.50"
			);

			if (Material.Slots.ContainsKey(MaterialSlotType::Albedo))
				MatWriter.WriteLineFmt("map_Kd %s", (char*)Material.Slots[MaterialSlotType::Albedo].first);
			else if (Material.Slots.ContainsKey(MaterialSlotType::Diffuse))
				MatWriter.WriteLineFmt("map_Kd %s", (char*)Material.Slots[MaterialSlotType::Diffuse].first);

			if (Material.Slots.ContainsKey(MaterialSlotType::Normal))
				MatWriter.WriteLineFmt("map_bump %s", (char*)Material.Slots[MaterialSlotType::Normal].first);
			if (Material.Slots.ContainsKey(MaterialSlotType::Specular))
				MatWriter.WriteLineFmt("map_Ks %s", (char*)Material.Slots[MaterialSlotType::Specular].first);
		}

		return true;
	}

	imstring WavefrontOBJ::ModelExtension()
	{
		return ".obj";
	}

	imstring WavefrontOBJ::AnimationExtension()
	{
		return nullptr;
	}

	ExporterScale WavefrontOBJ::ExportScale()
	{
		// OBJ is considered generic, so we won't enforce a scale constant
		return ExporterScale::Default;
	}

	bool WavefrontOBJ::SupportsAnimations()
	{
		return false;
	}

	bool WavefrontOBJ::SupportsModels()
	{
		return true;
	}
}
