#include "stdafx.h"
#include "AutodeskMaya.h"

#include "File.h"
#include "Path.h"
#include "CRC32.h"
#include "StreamWriter.h"
#include "DictionaryBase.h"

namespace Assets::Exporters
{
	bool AutodeskMaya::ExportAnimation(const Animation& Animation, const string& Path)
	{
		return false;
	}

	bool AutodeskMaya::ExportModel(const Model& Model, const string& Path)
	{
		auto Writer = IO::StreamWriter(IO::File::Create(Path));
		auto FileName = IO::Path::GetFileNameWithoutExtension(Path);
		auto Hash = Hashing::CRC32::HashString(FileName);

		Writer.WriteLine(
			"//Maya ASCII 8.5 scene\n\n"
			"requires maya \"8.5\";\ncurrentUnit -l centimeter -a degree -t film;\nfileInfo \"application\" \"maya\";\nfileInfo \"product\" \"Maya Unlimited 8.5\";\nfileInfo \"version\" \"8.5\";\nfileInfo \"cutIdentifier\" \"200612162224-692032\";"
			"createNode transform -s -n \"persp\";\n\tsetAttr \".v\" no;\n\tsetAttr \".t\" -type \"double3\" 48.186233840145825 37.816674066853686 41.0540421364379 ;\n\tsetAttr \".r\" -type \"double3\" -29.738352729603015 49.400000000000432 0 ;\ncreateNode camera -s -n \"perspShape\" -p \"persp\";\n\tsetAttr -k off \".v\" no;\n\tsetAttr \".fl\" 34.999999999999993;\n\tsetAttr \".fcp\" 10000;\n\tsetAttr \".coi\" 73.724849603665149;\n\tsetAttr \".imn\" -type \"string\" \"persp\";\n\tsetAttr \".den\" -type \"string\" \"persp_depth\";\n\tsetAttr \".man\" -type \"string\" \"persp_mask\";\n\tsetAttr \".hc\" -type \"string\" \"viewSet -p %camera\";\ncreateNode transform -s -n \"top\";\n\tsetAttr \".v\" no;\n\tsetAttr \".t\" -type \"double3\" 0 100.1 0 ;\n\tsetAttr \".r\" -type \"double3\" -89.999999999999986 0 0 ;\ncreateNode camera -s -n \"topShape\" -p \"top\";\n\tsetAttr -k off \".v\" no;\n\tsetAttr \".rnd\" no;\n\tsetAttr \".coi\" 100.1;\n\tsetAttr \".ow\" 30;\n\tsetAttr \".imn\" -type \"string\" \"top\";\n\tsetAttr \".den\" -type \"string\" \"top_depth\";\n\tsetAttr \".man\" -type \"string\" \"top_mask\";\n\tsetAttr \".hc\" -type \"string\" \"viewSet -t %camera\";\n\tsetAttr \".o\" yes;\ncreateNode transform -s -n \"front\";\n\tsetAttr \".v\" no;\n\tsetAttr \".t\" -type \"double3\" 0 0 100.1 ;\ncreateNode camera -s -n \"frontShape\" -p \"front\";\n\tsetAttr -k off \".v\" no;\n\tsetAttr \".rnd\" no;\n\tsetAttr \".coi\" 100.1;\n\tsetAttr \".ow\" 30;\n\tsetAttr \".imn\" -type \"string\" \"front\";\n\tsetAttr \".den\" -type \"string\" \"front_depth\";\n\tsetAttr \".man\" -type \"string\" \"front_mask\";\n\tsetAttr \".hc\" -type \"string\" \"viewSet -f %camera\";\n\tsetAttr \".o\" yes;\ncreateNode transform -s -n \"side\";\n\tsetAttr \".v\" no;\n\tsetAttr \".t\" -type \"double3\" 100.1 0 0 ;\n\tsetAttr \".r\" -type \"double3\" 0 89.999999999999986 0 ;\ncreateNode camera -s -n \"sideShape\" -p \"side\";\n\tsetAttr -k off \".v\" no;\n\tsetAttr \".rnd\" no;\n\tsetAttr \".coi\" 100.1;\n\tsetAttr \".ow\" 30;\n\tsetAttr \".imn\" -type \"string\" \"side\";\n\tsetAttr \".den\" -type \"string\" \"side_depth\";\n\tsetAttr \".man\" -type \"string\" \"side_mask\";\n\tsetAttr \".hc\" -type \"string\" \"viewSet -s %camera\";\n\tsetAttr \".o\" yes;\ncreateNode lightLinker -n \"lightLinker1\";\n\tsetAttr -s 9 \".lnk\";\n\tsetAttr -s 9 \".slnk\";\ncreateNode displayLayerManager -n \"layerManager\";\ncreateNode displayLayer -n \"defaultLayer\";\ncreateNode renderLayerManager -n \"renderLayerManager\";\ncreateNode renderLayer -n \"defaultRenderLayer\";\n\tsetAttr \".g\" yes;\ncreateNode script -n \"sceneConfigurationScriptNode\";\n\tsetAttr \".b\" -type \"string\" \"playbackOptions -min 1 -max 24 -ast 1 -aet 48 \";\n\tsetAttr \".st\" 6;\nselect -ne :time1;\n\tsetAttr \".o\" 1;\nselect -ne :renderPartition;\n\tsetAttr -s 2 \".st\";\nselect -ne :renderGlobalsList1;\nselect -ne :defaultShaderList1;\n\tsetAttr -s 2 \".s\";\nselect -ne :postProcessList1;\n\tsetAttr -s 2 \".p\";\nselect -ne :lightList1;\nselect -ne :initialShadingGroup;\n\tsetAttr \".ro\" yes;\nselect -ne :initialParticleSE;\n\tsetAttr \".ro\" yes;\nselect -ne :hardwareRenderGlobals;\n\tsetAttr \".ctrs\" 256;\n\tsetAttr \".btrs\" 512;\nselect -ne :defaultHardwareRenderGlobals;\n\tsetAttr \".fn\" -type \"string\" \"im\";\n\tsetAttr \".res\" -type \"string\" \"ntsc_4d 646 485 1.333\";\nselect -ne :ikSystem;\n\tsetAttr -s 4 \".sol\";\nconnectAttr \":defaultLightSet.msg\" \"lightLinker1.lnk[0].llnk\";\nconnectAttr \":initialShadingGroup.msg\" \"lightLinker1.lnk[0].olnk\";\nconnectAttr \":defaultLightSet.msg\" \"lightLinker1.lnk[1].llnk\";\nconnectAttr \":initialParticleSE.msg\" \"lightLinker1.lnk[1].olnk\";\nconnectAttr \":defaultLightSet.msg\" \"lightLinker1.slnk[0].sllk\";\nconnectAttr \":initialShadingGroup.msg\" \"lightLinker1.slnk[0].solk\";\nconnectAttr \":defaultLightSet.msg\" \"lightLinker1.slnk[1].sllk\";\nconnectAttr \":initialParticleSE.msg\" \"lightLinker1.slnk[1].solk\";\nconnectAttr \"layerManager.dli[0]\" \"defaultLayer.id\";\nconnectAttr \"renderLayerManager.rlmi[0]\" \"defaultRenderLayer.rlid\";\nconnectAttr \"lightLinker1.msg\" \":lightList1.ln\" -na;"
		);

		Writer.WriteLineFmt(
			"createNode transform -n \"%s\";\n"
			"setAttr \".ove\" yes;",
			(char*)FileName
		);

		uint32_t SubmeshIndex = 0;

		for (auto& Submesh : Model.Meshes)
		{
			Writer.WriteLineFmt(
				"createNode transform -n \"KoreMesh_%08x_%02d\" -p \"%s\";\n"
				"setAttr \".rp\" -type \"double3\" 0.000000 0.000000 0.000000 ;\nsetAttr \".sp\" -type \"double3\" 0.000000 0.000000 0.000000 ;\n"
				"createNode mesh -n \"MeshShape_%d\" -p \"KoreMesh_%08x_%02d\";\n"
				"setAttr -k off \".v\";\nsetAttr \".vir\" yes;\nsetAttr \".vif\" yes;\n"
				"setAttr -s %d \".uvst\";",
				Hash, SubmeshIndex, (char*)FileName,
				SubmeshIndex, Hash, SubmeshIndex,
				Submesh.Vertices.UVLayerCount()
			);

			for (uint8_t i = 1; i < Submesh.Vertices.UVLayerCount() + 1; i++)
			{
				if (Submesh.Vertices.Count() == 1)
				{
					Writer.WriteFmt(
						"setAttr \".uvst[%d].uvsn\" -type \"string\" \"map%d\";\n"
						"setAttr -s 1 \".uvst[0].uvsp[0]\" -type \"float2\"",
						(i - 1), i
					);
				}
				else
				{
					Writer.WriteFmt(
						"setAttr \".uvst[%d].uvsn\" -type \"string\" \"map%d\";\n"
						"setAttr -s %d \".uvst[0].uvsp[0:%d]\" -type \"float2\"",
						(i - 1), i,
						Submesh.Vertices.Count(), (Submesh.Vertices.Count() - 1)
					);
				}

				for (auto& Vertex : Submesh.Vertices)
				{
					auto& Layer = Vertex.UVLayers(i - 1);
					Writer.WriteFmt(" %f %f", Layer.U, (1 - Layer.V));
				}

				Writer.Write(";\n");
			}

			Writer.WriteFmt(
				"setAttr \".cuvs\" -type \"string\" \"map1\";\nsetAttr \".dcol\" yes;\nsetAttr \".dcc\" -type \"string\" \"Ambient+Diffuse\";\nsetAttr \".ccls\" -type \"string\" \"colorSet1\";\nsetAttr \".clst[0].clsn\" -type \"string\" \"colorSet1\";\n"
				"setAttr -s %d \".clst[0].clsp\";\n"
				"setAttr \".clst[0].clsp[0:%d]\"",
				(Submesh.Faces.Count() * 3),
				(Submesh.Faces.Count() * 3) - 1
			);

			for (auto& Face : Submesh.Faces)
			{
				auto& Vertex1 = Submesh.Vertices[Face[2]].Color();
				auto& Vertex2 = Submesh.Vertices[Face[1]].Color();
				auto& Vertex3 = Submesh.Vertices[Face[0]].Color();

				Writer.WriteFmt(
					" %f %f %f %f"
					" %f %f %f %f"
					" %f %f %f %f",
					Vertex1[0] / 255.f, Vertex1[1] / 255.f, Vertex1[2] / 255.f, Vertex1[3] / 255.f,
					Vertex2[0] / 255.f, Vertex2[1] / 255.f, Vertex2[2] / 255.f, Vertex2[3] / 255.f,
					Vertex3[0] / 255.f, Vertex3[1] / 255.f, Vertex3[2] / 255.f, Vertex3[3] / 255.f
				);
			}

			Writer.WriteLineFmt(
				";\n"
				"setAttr \".covm[0]\"  0 1 1;\nsetAttr \".cdvm[0]\"  0 1 1;\nsetAttr -s %d \".vt\";",
				Submesh.Vertices.Count()
			);

			if (Submesh.Vertices.Count() == 1)
				Writer.Write("setAttr \".vt[0]\"");
			else
				Writer.WriteFmt("setAttr \".vt[0:%d]\"", Submesh.Vertices.Count() - 1);

			for (auto& Vertex : Submesh.Vertices)
			{
				auto& Position = Vertex.Position();
				Writer.WriteFmt(" %f %f %f", Position.X, Position.Y, Position.Z);
			}

			Writer.WriteFmt(
				";\n"
				"setAttr -s %d \".ed\";\n"
				"setAttr \".ed[0:%d]\"",
				(Submesh.Faces.Count() * 3),
				(Submesh.Faces.Count() * 3) - 1
			);

			for (auto& Face : Submesh.Faces)
				Writer.WriteFmt(" %d %d 0 %d %d 0 %d %d 0", Face[2], Face[1], Face[1], Face[0], Face[0], Face[2]);

			Writer.WriteFmt(
				";\n"
				"setAttr -s %d \".n\";\n"
				"setAttr \".n[0:%d]\" -type \"float3\"",
				(Submesh.Faces.Count() * 3),
				(Submesh.Faces.Count() * 3) - 1
			);

			for (auto& Face : Submesh.Faces)
			{
				auto& Vertex1 = Submesh.Vertices[Face[2]].Normal();
				auto& Vertex2 = Submesh.Vertices[Face[1]].Normal();
				auto& Vertex3 = Submesh.Vertices[Face[0]].Normal();

				Writer.WriteFmt(
					" %f %f %f"
					" %f %f %f"
					" %f %f %f",
					Vertex1.X, Vertex1.Y, Vertex1.Z,
					Vertex2.X, Vertex2.Y, Vertex2.Z,
					Vertex3.X, Vertex3.Y, Vertex3.Z
				);
			}

			Writer.WriteLine(";");

			if (Submesh.Faces.Count() == 1)
				Writer.WriteFmt("setAttr -s %d \".fc[0]\" -type \"polyFaces\"", Submesh.Faces.Count());
			else
				Writer.WriteFmt("setAttr -s %d \".fc[0:%d]\" -type \"polyFaces\"", Submesh.Faces.Count(), Submesh.Faces.Count() - 1);

			uint32_t FaceIndex = 0;

			for (auto& Face : Submesh.Faces)
			{
				Writer.WriteFmt(" f 3 %d %d %d", FaceIndex, (FaceIndex + 1), (FaceIndex + 2));

				for (uint8_t i = 0; i < Submesh.Vertices.UVLayerCount(); i++)
					Writer.WriteFmt(" mu %d 3 %d %d %d", i, Face[2], Face[1], Face[0]);

				Writer.WriteFmt(" mc 0 3 %d %d %d", FaceIndex, (FaceIndex + 1), (FaceIndex + 2));

				FaceIndex += 3;
			}

			Writer.WriteLine(
				";\n"
				"setAttr \".cd\" -type \"dataPolyComponent\" Index_Data Edge 0 ;\nsetAttr \".cvd\" -type \"dataPolyComponent\" Index_Data Vertex 0 ;\nsetAttr \".hfd\" -type \"dataPolyComponent\" Index_Data Face 0 ;"
			);

			SubmeshIndex++;
		}

		Writer.Write("\n");

		for (auto& Material : Model.Materials)
		{
			auto MaterialName = (char*)Material.Name;

			Writer.WriteLineFmt(
				"createNode shadingEngine -n \"%sSG\";\n"
				"setAttr \".ihi\" 0;\n"
				"setAttr \".ro\" yes;\n"
				"createNode materialInfo -n \"%sMI\";\r\ncreateNode lambert -n \"%s\";\n"
				"createNode place2dTexture -n \"%sP2DT\";",
				MaterialName,
				MaterialName, MaterialName,
				MaterialName
			);

			auto DiffuseTexture = (Material.Slots.ContainsKey(MaterialSlotType::Albedo)) 
				? MaterialSlotType::Albedo : (Material.Slots.ContainsKey(MaterialSlotType::Diffuse) ? MaterialSlotType::Diffuse : MaterialSlotType::Invalid);

			if (DiffuseTexture != MaterialSlotType::Invalid)
			{
				Writer.WriteLineFmt(
					"createNode file -n \"%sFILE\";\n"
					"setAttr \".ftn\" -type \"string\" \"%s\";",
					MaterialName,
					(char*)string(Material.Slots[DiffuseTexture].first).Replace("\\", "\\\\")
				);
			}
		}

		uint32_t LightConnectionIndex = 2;

		for (auto& Material : Model.Materials)
		{
			auto MaterialName = (char*)Material.Name;

			Writer.WriteLineFmt(
				"connectAttr \":defaultLightSet.msg\" \"lightLinker1.lnk[%d].llnk\";\n"
				"connectAttr \"%sSG.msg\" \"lightLinker1.lnk[%d].olnk\";\n"
				"connectAttr \":defaultLightSet.msg\" \"lightLinker1.slnk[%d].sllk\";\n"
				"connectAttr \"%sSG.msg\" \"lightLinker1.slnk[%d].solk\";\n"
				"connectAttr \"%s.oc\" \"%sSG.ss\";\n"
				"connectAttr \"%sSG.msg\" \"%sMI.sg\";\n"
				"connectAttr \"%s.msg\" \"%sMI.m\";",
				LightConnectionIndex,
				MaterialName, LightConnectionIndex,
				LightConnectionIndex,
				MaterialName, LightConnectionIndex,
				MaterialName, MaterialName,
				MaterialName, MaterialName,
				MaterialName, MaterialName
			);

			auto DiffuseTexture = (Material.Slots.ContainsKey(MaterialSlotType::Albedo))
				? MaterialSlotType::Albedo : (Material.Slots.ContainsKey(MaterialSlotType::Diffuse) ? MaterialSlotType::Diffuse : MaterialSlotType::Invalid);

			if (DiffuseTexture != MaterialSlotType::Invalid)
			{
				Writer.WriteLineFmt(
					"connectAttr \"%sFILE.msg\" \"%sMI.t\" -na;\n"
					"connectAttr \"%sFILE.oc\" \"%s.c\";\n"
					"connectAttr \"%sP2DT.c\" \"%sFILE.c\";\n"
					"connectAttr \"%sP2DT.tf\" \"%sFILE.tf\";\n"
					"connectAttr \"%sP2DT.rf\" \"%sFILE.rf\";\n"
					"connectAttr \"%sP2DT.mu\" \"%sFILE.mu\";\n"
					"connectAttr \"%sP2DT.mv\" \"%sFILE.mv\";\n"
					"connectAttr \"%sP2DT.s\" \"%sFILE.s\";\n"
					"connectAttr \"%sP2DT.wu\" \"%sFILE.wu\";\n"
					"connectAttr \"%sP2DT.wv\" \"%sFILE.wv\";\n"
					"connectAttr \"%sP2DT.re\" \"%sFILE.re\";\n"
					"connectAttr \"%sP2DT.of\" \"%sFILE.of\";\n"
					"connectAttr \"%sP2DT.r\" \"%sFILE.ro\";\n"
					"connectAttr \"%sP2DT.n\" \"%sFILE.n\";\n"
					"connectAttr \"%sP2DT.vt1\" \"%sFILE.vt1\";\n"
					"connectAttr \"%sP2DT.vt2\" \"%sFILE.vt2\";\n"
					"connectAttr \"%sP2DT.vt3\" \"%sFILE.vt3\";\n"
					"connectAttr \"%sP2DT.vc1\" \"%sFILE.vc1\";\n"
					"connectAttr \"%sP2DT.o\" \"%sFILE.uv\";\n"
					"connectAttr \"%sP2DT.ofs\" \"%sFILE.fs\";\n",
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName,
					MaterialName, MaterialName
				);
			}

			Writer.WriteLineFmt(
				"connectAttr \"%sSG.pa\" \":renderPartition.st\" -na;\n"
				"connectAttr \"%s.msg\" \":defaultShaderList1.s\" -na;\n"
				"connectAttr \"%sP2DT.msg\" \":defaultRenderUtilityList1.u\" -na;\n"
				"connectAttr \"%sFILE.msg\" \":defaultTextureList1.tx\" -na;\n",
				MaterialName,
				MaterialName,
				MaterialName,
				MaterialName
			);

			LightConnectionIndex++;
		}

		SubmeshIndex = 0;

		for (auto& Submesh : Model.Meshes)
		{
			Writer.WriteLineFmt(
				"setAttr -s %d \"MeshShape_%d.iog\";",
				Submesh.Vertices.UVLayerCount(), SubmeshIndex
			);

			for (uint8_t i = 0; i < Submesh.Vertices.UVLayerCount(); i++)
			{
				if (Submesh.MaterialIndices[i] < 0)
					continue;

				Writer.WriteLineFmt(
					"connectAttr \"MeshShape_%d.iog[%d]\" \"%sSG.dsm\" -na;",
					SubmeshIndex, i, (char*)Model.Materials[Submesh.MaterialIndices[i]].Name
				);
			}

			SubmeshIndex++;
		}

		Writer.WriteLine(
			"createNode transform -n \"Joints\";\nsetAttr \".ove\" yes;\n"
		);

		for (auto& Bone : Model.Bones)
		{
			if (Bone.Parent() == -1)
				Writer.WriteLineFmt("createNode joint -n \"%s\" -p \"Joints\";", (char*)Bone.Name());
			else
				Writer.WriteLineFmt("createNode joint -n \"%s\" -p \"%s\";", (char*)Bone.Name(), (char*)Model.Bones[Bone.Parent()].Name());

			if (Bone.GetFlag(BoneFlags::HasLocalSpaceMatrices))
			{
				auto Rotation = Bone.LocalRotation().ToEulerAngles();
				
				auto& Position = Bone.LocalPosition();
				auto& Scale = Bone.Scale();

				Writer.WriteLineFmt(
					"addAttr -ci true -sn \"liw\" -ln \"lockInfluenceWeights\" -bt \"lock\" -min 0 -max 1 -at \"bool\";\n"
					"setAttr \".uoc\" yes;\n"
					"setAttr \".ove\" yes;\n"
					"setAttr \".t\" -type \"double3\"  %f %f %f ;\n"
					"setAttr \".mnrl\" -type \"double3\" -360 -360 -360 ;\n"
					"setAttr \".mxrl\" -type \"double3\" 360 360 360 ;\n"
					"setAttr \".radi\"   0.50;\n"
					"setAttr \".jo\" -type \"double3\" %f %f %f;\n"
					"setAttr \".scale\" -type \"double3\" %f %f %f;\n",
					Position.X, Position.Y, Position.Z,
					Rotation.X, Rotation.Y, Rotation.Z,
					Scale.X, Scale.Y, Scale.Z
				);
			}
			else
			{
				auto Rotation = Bone.GlobalRotation().ToEulerAngles();

				auto& Position = Bone.GlobalPosition();
				auto& Scale = Bone.Scale();

				Writer.WriteLineFmt(
					"addAttr -ci true -sn \"liw\" -ln \"lockInfluenceWeights\" -bt \"lock\" -min 0 -max 1 -at \"bool\";\n"
					"setAttr \".uoc\" yes;\n"
					"setAttr \".ove\" yes;\n"
					"setAttr \".it\" no;\n"
					"setAttr \".t\" -type \"double3\"  %f %f %f ;\n"
					"setAttr \".mnrl\" -type \"double3\" -360 -360 -360 ;\n"
					"setAttr \".mxrl\" -type \"double3\" 360 360 360 ;\n"
					"setAttr \".radi\"   0.50;\n"
					"setAttr \".jo\" -type \"double3\" %f %f %f;\n"
					"setAttr \".scale\" -type \"double3\" %f %f %f;\n",
					Position.X, Position.Y, Position.Z,
					Rotation.X, Rotation.Y, Rotation.Z,
					Scale.X, Scale.Y, Scale.Z
				);
			}
		}

		auto Binder = IO::StreamWriter(IO::File::Create(IO::Path::Combine(IO::Path::GetDirectoryName(Path), FileName + "_BIND.mel")));

		Binder.WriteLine(
			"/*\n* Autodesk Maya Bind Script\n*/\n"
		);

		SubmeshIndex = 0;

		for (auto& Submesh : Model.Meshes)
		{
			Binder.WriteLineFmt(
				"global proc KoreMesh_%08x_%02d_BindFunc()\n{\n"
				"   select -r KoreMesh_%08x_%02d;",
				Hash, SubmeshIndex,
				Hash, SubmeshIndex
			);

			auto MaximumInfluence = Submesh.Vertices.WeightCount();

			uint32_t BoneMapIndex = 0;
			Dictionary<uint32_t, uint8_t> BoneMap;
			Dictionary<uint32_t, uint32_t> ReverseBoneMap;
			List<string> BoneNames;

			for (auto& Vertex : Submesh.Vertices)
			{
				for (uint8_t i = 0; i < Vertex.WeightCount(); i++)
				{
					auto& Weight = Vertex.Weights(i);

					if (BoneMap.Add(Weight.Bone, 0))
					{
						BoneNames.Add(Model.Bones[Weight.Bone].Name());
						ReverseBoneMap.Add(BoneMapIndex, Weight.Bone);
						BoneMapIndex++;
					}
				}
			}

			for (auto& BoneName : BoneNames)
				Binder.WriteLineFmt("   select -add %s;", (char*)BoneName);

			Binder.WriteLineFmt(
				"   newSkinCluster \"-toSelectedBones -mi %d -omi true -dr 5.0 -rui false\";\n"
				"   string $clu = findRelatedSkinCluster(\"KoreMesh_%08x_%02d\");",
				MaximumInfluence,
				Hash, SubmeshIndex
			);

			// If we are a complex skin, we need a weight matrix
			if (BoneNames.Count() > 1)
			{
				Binder.WriteFmt("   int $NV = %d;\n   matrix $WM[%d][%d] = <<", Submesh.Vertices.Count(), Submesh.Vertices.Count(), BoneNames.Count());

				for (uint32_t i = 0; i < Submesh.Vertices.Count(); i++)
				{
					auto Vertex = Submesh.Vertices[i];

					if (i != 0)
						Binder.Write(";");

					for (uint32_t b = 0; b < BoneNames.Count(); b++)
					{
						if (b != 0)
							Binder.Write(",");

						float WeightValue = 0.0f;

						for (uint8_t w = 0; w < Vertex.WeightCount(); w++)
						{
							if (Vertex.Weights(w).Bone == ReverseBoneMap[b])
							{
								WeightValue = Vertex.Weights(w).Value;
								break;
							}
						}

						if (WeightValue == 0.0f || WeightValue == 1.0f)
							Binder.WriteFmt("%d", (uint32_t)WeightValue);
						else
							Binder.WriteFmt("%f", WeightValue);
					}
				}

				Binder.WriteLine(">>;");
				Binder.Write("   for ($i = 0; $i < $NV; $i++) {");

				Binder.WriteFmt(" setAttr($clu + \".weightList[\" + $i + \"].weights[0:%d]\")", BoneNames.Count() - 1);

				for (uint32_t i = 0; i < BoneNames.Count(); i++)
					Binder.WriteFmt(" $WM[$i][%d]", i);

				Binder.WriteLine("; }");
			}

			Binder.WriteLine("}\n");
			SubmeshIndex++;
		}

		Binder.WriteLine("global proc RunAdvancedScript()\n{");

		SubmeshIndex = 0;

		for (auto& Submesh : Model.Meshes)
			Binder.WriteLineFmt("   catch(KoreMesh_%08x_%02d_BindFunc());", Hash, SubmeshIndex++);

		Binder.WriteLine("}\n\nglobal proc NamespacePurge()\n{\n   string $allNodes[] = `ls`;\n   for($node in $allNodes) {\n      string $buffer[];\n      tokenize $node \":\" $buffer;\n      string $newName = $buffer[size($buffer)-1];\n       catchQuiet(`rename $node $newName`);\n   }\n}\n\nprint(\"Currently binding the current model, please wait...\\n\");\nNamespacePurge();\nRunAdvancedScript();\nprint(\"The model has been binded.\\n\");\n");

		return true;
	}

	imstring AutodeskMaya::ModelExtension()
	{
		return ".ma";
	}

	imstring AutodeskMaya::AnimationExtension()
	{
		return ".anim";
	}

	ExporterScale AutodeskMaya::ExportScale()
	{
		return ExporterScale::CM;
	}

	bool AutodeskMaya::SupportsAnimations()
	{
		return true;
	}

	bool AutodeskMaya::SupportsModels()
	{
		return true;
	}
}
