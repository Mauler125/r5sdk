#include "stdafx.h"
#include "CoDXAssetExport.h"

#include "File.h"
#include "Path.h"
#include "Matrix.h"
#include "StreamWriter.h"

namespace Assets::Exporters
{
	bool CoDXAssetExport::ExportAnimation(const Animation& Animation, const string& Path)
	{
		return false;
	}

	bool CoDXAssetExport::ExportModel(const Model& Model, const string& Path)
	{
		auto Writer = IO::StreamWriter(IO::File::Create(Path));

		Writer.WriteLine(
			"MODEL\nVERSION 6\n"
		);

		Writer.WriteLineFmt("NUMBONES %d", Model.Bones.Count());

		uint32_t BoneIndex = 0;

		for (auto& Bone : Model.Bones)
		{
			Writer.WriteLineFmt("BONE %d %d \"%s\"", BoneIndex, Bone.Parent(), (char*)Bone.Name());
			BoneIndex++;
		}

		Writer.Write("\n");
		BoneIndex = 0;

		for (auto& Bone : Model.Bones)
		{
			auto Rotation = ::Math::Matrix::CreateFromQuaternion(Bone.GlobalRotation());

			auto& Position = Bone.GlobalPosition();
			auto& Scale = Bone.Scale();

			Writer.WriteLineFmt(
				"BONE %d\n"
				"OFFSET %f, %f, %f\n"
				"SCALE %f, %f, %f\n"
				"X %f, %f, %f\n"
				"Y %f, %f, %f\n"
				"Z %f, %f, %f\n",
				BoneIndex,
				Position.X, Position.Y, Position.Z,
				Scale.X, Scale.Y, Scale.Z,
				MathHelper::Clamp(Rotation.Mat(0, 0), -1.f, 1.f), MathHelper::Clamp(Rotation.Mat(0, 1), -1.f, 1.f), MathHelper::Clamp(Rotation.Mat(0, 2), -1.f, 1.f),
				MathHelper::Clamp(Rotation.Mat(1, 0), -1.f, 1.f), MathHelper::Clamp(Rotation.Mat(1, 1), -1.f, 1.f), MathHelper::Clamp(Rotation.Mat(1, 2), -1.f, 1.f),
				MathHelper::Clamp(Rotation.Mat(2, 0), -1.f, 1.f), MathHelper::Clamp(Rotation.Mat(2, 1), -1.f, 1.f), MathHelper::Clamp(Rotation.Mat(2, 2), -1.f, 1.f)
			);

			BoneIndex++;
		}

		auto TotalVertexCount = Model.VertexCount();
		auto TotalFaceCount = Model.FaceCount();

		if (TotalVertexCount > UINT16_MAX)
			Writer.WriteLineFmt("NUMVERTS32 %d", TotalVertexCount);
		else
			Writer.WriteLineFmt("NUMVERTS %d", TotalVertexCount);

		uint32_t VertexIndex = 0;

		for (auto& Submesh : Model.Meshes)
		{
			for (auto& Vertex : Submesh.Vertices)
			{
				auto& Position = Vertex.Position();

				if (TotalVertexCount > UINT16_MAX)
					Writer.WriteLineFmt(
						"VERT32 %d\n"
						"OFFSET %f, %f, %f",
						VertexIndex,
						Position.X, Position.Y, Position.Z
					);
				else
					Writer.WriteLineFmt(
						"VERT %d\n"
						"OFFSET %f, %f, %f",
						VertexIndex,
						Position.X, Position.Y, Position.Z
					);

				if (Vertex.WeightCount() == 1)
					Writer.WriteLineFmt(
						"BONES 1\n"
						"BONE %d 1.0\n",
						Vertex.Weights(0).Bone
					);
				else
				{
					Writer.WriteLineFmt("BONES %d", Vertex.WeightCount());

					for (uint8_t i = 0; i < Vertex.WeightCount(); i++)
						Writer.WriteLineFmt("BONE %d %f", Vertex.Weights(i).Bone, Vertex.Weights(i).Value);

					Writer.Write("\n");
				}

				VertexIndex++;
			}
		}

		Writer.WriteLineFmt("NUMFACES %d", TotalFaceCount);

		uint32_t SubmeshIndex = 0;
		VertexIndex = 0;

		for (auto& Submesh : Model.Meshes)
		{
			for (auto& Face : Submesh.Faces)
			{
				if (SubmeshIndex > UINT8_MAX || Submesh.MaterialIndices[0] > UINT8_MAX)
					Writer.WriteLineFmt("TRI16 %d %d 0 0", SubmeshIndex, Submesh.MaterialIndices[0]);
				else
					Writer.WriteLineFmt("TRI %d %d 0 0", SubmeshIndex, Submesh.MaterialIndices[0]);

				//
				// Face vertex [2]
				//

				Writer.WriteLineFmt((TotalVertexCount > UINT16_MAX) ? "VERT32 %d" : "VERT %d", (Face[2] + VertexIndex));

				auto& Face1Normal = Submesh.Vertices[Face[2]].Normal();
				auto& Face1Color = Submesh.Vertices[Face[2]].Color();

				Writer.WriteFmt(
					"NORMAL %f %f %f\n"
					"COLOR %f %f %f %f\n"
					"UV %d",
					Face1Normal.X, Face1Normal.Y, Face1Normal.Z,
					Face1Color[0], Face1Color[1], Face1Color[2], Face1Color[3],
					Submesh.Vertices.UVLayerCount()
				);

				for (uint8_t i = 0; i < Submesh.Vertices.UVLayerCount(); i++)
					Writer.WriteFmt(" %f %f", Submesh.Vertices[Face[2]].UVLayers(i).U, Submesh.Vertices[Face[2]].UVLayers(i).V);
				Writer.Write("\n");

				//
				// Face vertex [0]
				//

				Writer.WriteLineFmt((TotalVertexCount > UINT16_MAX) ? "VERT32 %d" : "VERT %d", (Face[0] + VertexIndex));

				auto& Face2Normal = Submesh.Vertices[Face[0]].Normal();
				auto& Face2Color = Submesh.Vertices[Face[0]].Color();

				Writer.WriteFmt(
					"NORMAL %f %f %f\n"
					"COLOR %f %f %f %f\n"
					"UV %d",
					Face2Normal.X, Face2Normal.Y, Face2Normal.Z,
					Face2Color[0], Face2Color[1], Face2Color[2], Face2Color[3],
					Submesh.Vertices.UVLayerCount()
				);

				for (uint8_t i = 0; i < Submesh.Vertices.UVLayerCount(); i++)
					Writer.WriteFmt(" %f %f", Submesh.Vertices[Face[0]].UVLayers(i).U, Submesh.Vertices[Face[0]].UVLayers(i).V);
				Writer.Write("\n");

				//
				// Face vertex [1]
				//

				Writer.WriteLineFmt((TotalVertexCount > UINT16_MAX) ? "VERT32 %d" : "VERT %d", (Face[1] + VertexIndex));

				auto& Face3Normal = Submesh.Vertices[Face[1]].Normal();
				auto& Face3Color = Submesh.Vertices[Face[1]].Color();

				Writer.WriteFmt(
					"NORMAL %f %f %f\n"
					"COLOR %f %f %f %f\n"
					"UV %d",
					Face3Normal.X, Face3Normal.Y, Face3Normal.Z,
					Face3Color[0], Face3Color[1], Face3Color[2], Face3Color[3],
					Submesh.Vertices.UVLayerCount()
				);

				for (uint8_t i = 0; i < Submesh.Vertices.UVLayerCount(); i++)
					Writer.WriteFmt(" %f %f", Submesh.Vertices[Face[1]].UVLayers(i).U, Submesh.Vertices[Face[1]].UVLayers(i).V);
				Writer.Write("\n");
			}

			VertexIndex += Submesh.Vertices.Count();
			SubmeshIndex++;
		}

		Writer.WriteLineFmt("\nNUMOBJECTS %d", Model.Meshes.Count());
		SubmeshIndex = 0;

		for (auto& Submesh : Model.Meshes)
			Writer.WriteLineFmt("OBJECT %d \"KoreMesh_%d\"", SubmeshIndex, SubmeshIndex++);
		Writer.Write("\n");

		Writer.WriteLineFmt("NUMMATERIALS %d", Model.Materials.Count());

		uint32_t MaterialIndex = 0;

		for (auto& Material : Model.Materials)
		{
			Writer.WriteFmt("MATERIAL %d \"%s\" \"Phong\" \"", MaterialIndex, (char*)Material.Name);

			if (Material.Slots.ContainsKey(MaterialSlotType::Albedo))
				Writer.WriteFmt("color:%s", (char*)Material.Slots[MaterialSlotType::Albedo].first);
			else if (Material.Slots.ContainsKey(MaterialSlotType::Diffuse))
				Writer.WriteFmt("color:%s", (char*)Material.Slots[MaterialSlotType::Diffuse].first);

			Writer.WriteLine("\"\nCOLOR 0.000000 0.000000 0.000000 1.000000\nTRANSPARENCY 0.000000 0.000000 0.000000 1.000000\nAMBIENTCOLOR 1.000000 1.000000 1.000000 1.000000\nINCANDESCENCE 0.000000 0.000000 0.000000 1.000000\nCOEFFS 0.800000 0.000000\nGLOW 0.000000 0\nREFRACTIVE 6 1.000000\nSPECULARCOLOR 0.500000 0.500000 0.500000 1.000000\nREFLECTIVECOLOR 0.000000 0.000000 0.000000 1.000000\nREFLECTIVE 1 0.500000\nBLINN -1.000000 -1.000000\nPHONG 20.000000");

			MaterialIndex++;
		}

		return true;
	}

	imstring CoDXAssetExport::ModelExtension()
	{
		return ".xmodel_export";
	}

	imstring CoDXAssetExport::AnimationExtension()
	{
		return ".xanim_export";
	}

	ExporterScale CoDXAssetExport::ExportScale()
	{
		// Call of Duty uses inches as the primary scale constant.
		return ExporterScale::Inch;
	}

	bool CoDXAssetExport::SupportsAnimations()
	{
		return true;
	}

	bool CoDXAssetExport::SupportsModels()
	{
		return true;
	}
}
