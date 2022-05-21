#include "stdafx.h"
#include "CastAsset.h"
#include "CastNode.h"
#include "File.h"
#include "XXHash.h"
#include "BinaryWriter.h"

namespace Assets::Exporters
{
	struct CastHeader
	{
		uint32_t Magic;			// char[4] cast	(0x74736163)
		uint32_t Version;		// 0x1
		uint32_t RootNodes;		// Number of root nodes, which contain various sub nodes if necessary
		uint32_t Flags;			// Reserved for flags, or padding, whichever is needed
	};

	static_assert(sizeof(CastHeader) == 0x10, "Cast header size mismatch");
	

	bool CastAsset::ExportAnimation(const Animation& Animation, const string& Path)
	{
		auto Writer = IO::BinaryWriter(IO::File::Create(Path));

		// Magic, version 1, one root node, no flags.
		Writer.Write<CastHeader>({ 0x74736163, 0x1, 0x1, 0x0 });

		// This is the base of the virtual scene
		auto Root = CastNode(CastId::Root);
		auto& AnimNode = Root.Children.Emplace(CastId::Animation, Hashing::XXHash::HashString("animation"));
		auto& SkeletonNode = AnimNode.Children.Emplace(CastId::Skeleton, Hashing::XXHash::HashString("skeleton"));

		AnimNode.Properties.Emplace(CastPropertyId::Float, "fr").AddFloat(Animation.FrameRate);
		AnimNode.Properties.Emplace(CastPropertyId::Byte, "lo").AddByte((uint8_t)Animation.Looping);

		for (auto& Bone : Animation.Bones)
		{
			auto& BoneNode = SkeletonNode.Children.Emplace(CastId::Bone);

			BoneNode.Properties.Emplace(CastPropertyId::String, "n").SetString(Bone.Name());
			BoneNode.Properties.Emplace(CastPropertyId::Integer32, "p").AddInteger32(Bone.Parent());

			if (Bone.GetFlag(Assets::BoneFlags::HasLocalSpaceMatrices))
			{
				BoneNode.Properties.Emplace(CastPropertyId::Vector3, "lp").AddVector3(Bone.LocalPosition());
				BoneNode.Properties.Emplace(CastPropertyId::Vector4, "lr").AddVector4(Bone.LocalRotation());
			}
			if (Bone.GetFlag(Assets::BoneFlags::HasGlobalSpaceMatrices))
			{
				BoneNode.Properties.Emplace(CastPropertyId::Vector3, "wp").AddVector3(Bone.GlobalPosition());
				BoneNode.Properties.Emplace(CastPropertyId::Vector4, "wr").AddVector4(Bone.GlobalRotation());
			}
			if (Bone.GetFlag(Assets::BoneFlags::HasScale))
			{
				BoneNode.Properties.Emplace(CastPropertyId::Vector3, "s").AddVector3(Bone.Scale());
			}
		}

		for (auto& Kvp : Animation.Curves)
		{
			for (auto& Curve : Kvp.Value())
			{
				auto& CurveNode = AnimNode.Children.Emplace(CastId::Curve, 0);

				CurveNode.Properties.Emplace(CastPropertyId::String, "nn").SetString(Curve.Name);

				constexpr const char* PropertyNameMap[] = {
					"ex",
					"rq",
					"rx",
					"ry",
					"rz",
					"tx",
					"ty",
					"tz",
					"sx",
					"sy",
					"sz",
					"vb"
				};

				constexpr const char* ModeNameMap[] = {
					"absolute",
					"additive",
					"relative"
				};

				CurveNode.Properties.Emplace(CastPropertyId::String, "kp").SetString(PropertyNameMap[(uint32_t)Curve.Property]);
				CurveNode.Properties.Emplace(CastPropertyId::String, "m").SetString(ModeNameMap[(uint32_t)Curve.Mode]);

				auto KeyframeValueProperty = CastPropertyId::Float;

				switch (Curve.Property)
				{
				case CurveProperty::RotateQuaternion:
					KeyframeValueProperty = CastPropertyId::Vector4;
					break;

				case CurveProperty::RotateX:
				case CurveProperty::RotateY:
				case CurveProperty::RotateZ:
				case CurveProperty::TranslateX:
				case CurveProperty::TranslateY:
				case CurveProperty::TranslateZ:
				case CurveProperty::ScaleX:
				case CurveProperty::ScaleY:
				case CurveProperty::ScaleZ:
					KeyframeValueProperty = CastPropertyId::Float;
					break;

				case CurveProperty::Visibility:
					KeyframeValueProperty = CastPropertyId::Byte;
					break;
				}

				auto KeyframeFrameProperty = CastPropertyId::Float;

				if (Curve.IsFrameIntegral())
				{
					uint32_t LargestFrameIndex = 0;
					for (auto& KeyFrame : Curve.Keyframes)
						LargestFrameIndex = max(LargestFrameIndex, KeyFrame.Frame.Integer32);

					if (LargestFrameIndex <= 0xFF)
						KeyframeFrameProperty = CastPropertyId::Byte;
					else if (LargestFrameIndex <= 0xFFFF)
						KeyframeFrameProperty = CastPropertyId::Short;
					else
						KeyframeFrameProperty = CastPropertyId::Integer32;
				}

				auto& KeyFrameBuffer = CurveNode.Properties.Emplace(KeyframeFrameProperty, "kb");
				auto& KeyValueBuffer = CurveNode.Properties.Emplace(KeyframeValueProperty, "kv");

				for (auto& KeyFrame : Curve.Keyframes)
				{
					switch (KeyframeFrameProperty)
					{
					case CastPropertyId::Float:
						KeyFrameBuffer.AddFloat(KeyFrame.Frame.Float);
						break;
					case CastPropertyId::Byte:
						KeyFrameBuffer.AddByte((uint8_t)KeyFrame.Frame.Integer32);
						break;
					case CastPropertyId::Short:
						KeyFrameBuffer.AddShort((uint16_t)KeyFrame.Frame.Integer32);
						break;
					case CastPropertyId::Integer32:
						KeyFrameBuffer.AddInteger32(KeyFrame.Frame.Integer32);
						break;
					}

					switch (Curve.Property)
					{
					case CurveProperty::RotateQuaternion:
						KeyValueBuffer.AddVector4(KeyFrame.Value.Vector4);
						break;

					case CurveProperty::RotateX:
					case CurveProperty::RotateY:
					case CurveProperty::RotateZ:
					case CurveProperty::TranslateX:
					case CurveProperty::TranslateY:
					case CurveProperty::TranslateZ:
					case CurveProperty::ScaleX:
					case CurveProperty::ScaleY:
					case CurveProperty::ScaleZ:
						KeyValueBuffer.AddFloat(KeyFrame.Value.Float);
						break;

					case CurveProperty::Visibility:
						KeyValueBuffer.AddByte(KeyFrame.Value.Byte);
						break;
					}
				}
			}
		}

		for (auto& Notetrack : Animation.Notificiations)
		{
			auto& TrackNode = AnimNode.Children.Emplace(CastId::NotificationTrack, Hashing::XXHash::HashString(Notetrack.Key()));

			TrackNode.Properties.Emplace(CastPropertyId::String, "n").SetString(Notetrack.Key());

			auto& KeyBuffer = TrackNode.Properties.Emplace(CastPropertyId::Integer32, "kb");
			
			for (auto& Key : Notetrack.Value())
				KeyBuffer.AddInteger32(Key);
		}

		// Finally, serialize the node to the disk
		Root.Write(Writer);

		return true;
	}

	bool CastAsset::ExportModel(const Model& Model, const string& Path)
	{
		auto Writer = IO::BinaryWriter(IO::File::Create(Path));

		// Magic, version 1, one root node, no flags.
		Writer.Write<CastHeader>({ 0x74736163, 0x1, 0x1, 0x0 });

		// This is the base of the virtual scene
		auto Root = CastNode(CastId::Root);
		auto& ModelNode = Root.Children.Emplace(CastId::Model, Hashing::XXHash::HashString("model"));
		auto& SkeletonNode = ModelNode.Children.Emplace(CastId::Skeleton, Hashing::XXHash::HashString("skeleton"));

		auto BoneCount = Model.Bones.Count();

		for (auto& Bone : Model.Bones)
		{
			auto& BoneNode = SkeletonNode.Children.Emplace(CastId::Bone);

			BoneNode.Properties.Emplace(CastPropertyId::String, "n").SetString(Bone.Name());
			BoneNode.Properties.Emplace(CastPropertyId::Integer32, "p").AddInteger32(Bone.Parent());

			if (Bone.GetFlag(Assets::BoneFlags::HasLocalSpaceMatrices))
			{
				BoneNode.Properties.Emplace(CastPropertyId::Vector3, "lp").AddVector3(Bone.LocalPosition());
				BoneNode.Properties.Emplace(CastPropertyId::Vector4, "lr").AddVector4(Bone.LocalRotation());
			}
			if (Bone.GetFlag(Assets::BoneFlags::HasGlobalSpaceMatrices))
			{
				BoneNode.Properties.Emplace(CastPropertyId::Vector3, "wp").AddVector3(Bone.GlobalPosition());
				BoneNode.Properties.Emplace(CastPropertyId::Vector4, "wr").AddVector4(Bone.GlobalRotation());
			}
			if (Bone.GetFlag(Assets::BoneFlags::HasScale))
			{
				BoneNode.Properties.Emplace(CastPropertyId::Vector3, "s").AddVector3(Bone.Scale());
			}
		}

		Dictionary<uint32_t, uint64_t> MaterialHashMap;
		uint32_t MaterialIndex = 0;

		for (auto& Mat : Model.Materials)
		{
			auto& MatNode = ModelNode.Children.Emplace(CastId::Material, Hashing::XXHash::HashString(Mat.Name));

			MatNode.Properties.Emplace(CastPropertyId::String, "n").SetString(Mat.Name);
			MatNode.Properties.Emplace(CastPropertyId::String, "t").SetString("pbr");

			for (auto& Kvp : Mat.Slots)
			{
				auto FileHash = Hashing::XXHash::HashString(Kvp.second.first);
				MatNode.Children.Emplace(CastId::File, FileHash).Properties.Emplace(CastPropertyId::String, "p").SetString(Kvp.second.first);

				// Cast material property mapping
				constexpr const char* MaterialSlotNames[] =
				{
					"extra",		// Invalid
					"albedo",
					"diffuse",
					"normal",
					"specular",
					"emissive",
					"gloss",
					"roughness",
					"ao",
					"cavity"
				};

				MatNode.Properties.Emplace(CastPropertyId::Integer64, MaterialSlotNames[(uint32_t)Kvp.first]).AddInteger64(FileHash);
			}

			MaterialHashMap.Add(MaterialIndex++, MatNode.Hash);
		}

		uint32_t MeshIndex = 0;

		for (auto& Mesh : Model.Meshes)
		{
			auto& MeshNode = ModelNode.Children.Emplace(CastId::Mesh, Hashing::XXHash::HashString(string::Format("mesh%02d", MeshIndex++)));
			
			MeshNode.Properties.EmplaceBack(CastPropertyId::Vector3, "vp");
			MeshNode.Properties.EmplaceBack(CastPropertyId::Vector3, "vn");
			MeshNode.Properties.EmplaceBack(CastPropertyId::Integer32, "vc");

			auto VertexCount = Mesh.Vertices.Count();

			if (VertexCount <= 0xFF)
				MeshNode.Properties.EmplaceBack(CastPropertyId::Byte, "f");
			else if (VertexCount <= 0xFFFF)
				MeshNode.Properties.EmplaceBack(CastPropertyId::Short, "f");
			else
				MeshNode.Properties.EmplaceBack(CastPropertyId::Integer32, "f");

			// Configure the uv layer count, and maximum influence
			MeshNode.Properties.Emplace(CastPropertyId::Byte, "ul").AddByte((uint8_t)Mesh.Vertices.UVLayerCount());
			MeshNode.Properties.Emplace(CastPropertyId::Byte, "mi").AddByte((uint8_t)Mesh.Vertices.WeightCount());

			if (BoneCount <= 0xFF)
				MeshNode.Properties.Emplace(CastPropertyId::Byte, "wb");
			else if (BoneCount <= 0xFFFF)
				MeshNode.Properties.Emplace(CastPropertyId::Short, "wb");
			else
				MeshNode.Properties.Emplace(CastPropertyId::Integer32, "wb");

			MeshNode.Properties.Emplace(CastPropertyId::Float, "wv");

			List<CastProperty*> UVLayers;

			for (uint8_t i = 0; i < Mesh.Vertices.UVLayerCount(); i++)
				MeshNode.Properties.EmplaceBack(CastPropertyId::Vector2, string::Format("u%d", i));

			auto& VertexPositions = MeshNode.Properties[0];
			auto& VertexNormals = MeshNode.Properties[1];
			auto& VertexColors = MeshNode.Properties[2];
			auto& FaceIndices = MeshNode.Properties[3];
			auto& VertexWeightBones = MeshNode.Properties[6];
			auto& VertexWeightValues = MeshNode.Properties[7];

			for (auto& Layer : MeshNode.Properties)
			{
				if (Layer.Name != "ul" && Layer.Name.StartsWith("u"))
					UVLayers.Add(&Layer);
			}

			for (auto& Vertex : Mesh.Vertices)
			{
				VertexPositions.AddVector3(Vertex.Position());
				VertexNormals.AddVector3(Vertex.Normal());
				VertexColors.AddInteger32(*(uint32_t*)&Vertex.Color());

				for (uint8_t i = 0; i < Mesh.Vertices.WeightCount(); i++)
				{
					if (BoneCount <= 0xFF)
						VertexWeightBones.AddByte((uint8_t)Vertex.Weights(i).Bone);
					else if (BoneCount <= 0xFFFF)
						VertexWeightBones.AddShort((uint16_t)Vertex.Weights(i).Bone);
					else
						VertexWeightBones.AddInteger32(Vertex.Weights(i).Bone);

					VertexWeightValues.AddFloat(Vertex.Weights(i).Value);
				}

				for (uint8_t i = 0; i < Mesh.Vertices.UVLayerCount(); i++)
				{
					UVLayers[i]->AddVector2(Vertex.UVLayers(i));
				}
			}

			for (auto& Face : Mesh.Faces)
			{
				if (VertexCount <= 0xFF)
				{
					FaceIndices.AddByte((uint8_t)Face[2]);
					FaceIndices.AddByte((uint8_t)Face[1]);
					FaceIndices.AddByte((uint8_t)Face[0]);
				}
				else if (VertexCount <= 0xFFFF)
				{
					FaceIndices.AddShort((uint16_t)Face[2]);
					FaceIndices.AddShort((uint16_t)Face[1]);
					FaceIndices.AddShort((uint16_t)Face[0]);
				}
				else
				{
					FaceIndices.AddInteger32(Face[2]);
					FaceIndices.AddInteger32(Face[1]);
					FaceIndices.AddInteger32(Face[0]);
				}
			}

			if (Mesh.MaterialIndices.Count() > 0 && Mesh.MaterialIndices[0] > -1)
			{
				MeshNode.Properties.Emplace(CastPropertyId::Integer64, "m").AddInteger64(MaterialHashMap[Mesh.MaterialIndices[0]]);
			}
		}
		
		// Finally, serialize the node to the disk
		Root.Write(Writer);

		return true;
	}

	imstring CastAsset::ModelExtension()
	{
		return ".cast";
	}

	imstring CastAsset::AnimationExtension()
	{
		return ".cast";
	}

	ExporterScale CastAsset::ExportScale()
	{
		return ExporterScale::Default;
	}

	bool CastAsset::SupportsAnimations()
	{
		return true;
	}

	bool CastAsset::SupportsModels()
	{
		return true;
	}
}
