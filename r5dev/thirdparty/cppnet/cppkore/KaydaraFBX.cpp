#include "stdafx.h"
#include "KaydaraFBX.h"
#include "KaydaraFBXContainer.h"

#include "File.h"
#include "Matrix.h"
#include "Path.h"
#include "BinaryWriter.h"

namespace Assets::Exporters
{
	void AddObjectConnection(KaydaraFBXNode& ConnectionsNode, uint64_t From, uint64_t To)
	{
		auto& Connection = ConnectionsNode.Children.Emplace("C");

		Connection.AddPropertyString("OO");
		Connection.AddPropertyInteger64(From);
		Connection.AddPropertyInteger64(To);
	}

	void AddObjectPropertyConnection(KaydaraFBXNode& ConnectionsNode, uint64_t From, uint64_t To, const char* PropertyName)
	{
		auto& Connection = ConnectionsNode.Children.Emplace("C");

		Connection.AddPropertyString("OP");
		Connection.AddPropertyInteger64(From);
		Connection.AddPropertyInteger64(To);
		Connection.AddPropertyString(PropertyName);
	}

	void InitializeRootNode(KaydaraFBXNode& RootNode)
	{
		RootNode.Children.Emplace("Version").AddPropertyInteger32(232);
		auto& Properties = RootNode.Children.Emplace("Properties70");

		{
			auto& Prop = Properties.Children.Emplace("P");

			Prop.AddPropertyString("Lcl Rotation");
			Prop.AddPropertyString("Lcl Rotation");
			Prop.AddPropertyString("");
			Prop.AddPropertyString("A");
			Prop.AddPropertyFloat64(-90);	// To match default Maya axis
			Prop.AddPropertyFloat64(0);
			Prop.AddPropertyFloat64(0);
		}
	}

	void InitializeMaterialTexture(KaydaraFBXNode& ObjectsNode, Material& Material, uint64_t TextureId)
	{
		auto& Texture = ObjectsNode.Children.Emplace("Texture");

		auto TextureName = Material.Name + "_c" + string("\u0000\u0001Texture", 9);

		Texture.AddPropertyInteger64(TextureId);
		Texture.AddPropertyString(TextureName);
		Texture.AddPropertyString("");

		Texture.Children.Emplace("Type").AddPropertyString("TextureVideoClip");
		Texture.Children.Emplace("Version").AddPropertyInteger32(202);
		Texture.Children.Emplace("TextureName").AddPropertyString(TextureName);

		auto& Properties = Texture.Children.Emplace("Properties70");

		{
			auto& Prop = Properties.Children.Emplace("P");

			Prop.AddPropertyString("CurrentTextureBlendMode");
			Prop.AddPropertyString("enum");
			Prop.AddPropertyString("");
			Prop.AddPropertyString("");
			Prop.AddPropertyInteger32(0);
		}

		{
			auto& Prop = Properties.Children.Emplace("P");

			Prop.AddPropertyString("UVSet");
			Prop.AddPropertyString("KString");
			Prop.AddPropertyString("");
			Prop.AddPropertyString("");
			Prop.AddPropertyString("map1");
		}

		{
			auto& Prop = Properties.Children.Emplace("P");

			Prop.AddPropertyString("UseMaterial");
			Prop.AddPropertyString("bool");
			Prop.AddPropertyString("");
			Prop.AddPropertyString("");
			Prop.AddPropertyInteger32(1);
		}

		Texture.Children.Emplace("Media").AddPropertyString(Material.Name + "_c" + string("\u0000\u0001Video", 7));

		string DiffuseMap = "";
		if (Material.Slots.ContainsKey(MaterialSlotType::Albedo))
			DiffuseMap = Material.Slots[MaterialSlotType::Albedo].first;
		else if (Material.Slots.ContainsKey(MaterialSlotType::Diffuse))
			DiffuseMap = Material.Slots[MaterialSlotType::Diffuse].first;

		Texture.Children.Emplace("FileName").AddPropertyString(DiffuseMap.Replace("\\", "/"));
		Texture.Children.Emplace("RelativeFilename").AddPropertyString(DiffuseMap);
	}

	void InitializeMeshSkinCluster(KaydaraFBXNode& ObjectsNode, KaydaraFBXNode& ConnectionsNode, uint64_t SubmeshIndex, uint64_t GeometryId, uint64_t DeformerId)
	{
		auto& SkinDeformer = ObjectsNode.Children.Emplace("Deformer");

		SkinDeformer.AddPropertyInteger64(DeformerId);
		SkinDeformer.AddPropertyString(string::Format("KoreLibMesh%02d", SubmeshIndex) + string("\u0000\u0001Deformer", 10));
		SkinDeformer.AddPropertyString("Skin");

		SkinDeformer.Children.Emplace("Version").AddPropertyInteger32(101);
		SkinDeformer.Children.Emplace("Link_DeformAcuracy").AddPropertyFloat64(50);

		AddObjectConnection(ConnectionsNode, DeformerId, GeometryId);
	}

	void InitializeMeshSubDeformer(KaydaraFBXNode& ObjectsNode, Bone& Bone, uint64_t SubmeshIndex, uint64_t BoneIndex, uint64_t DeformerId)
	{
		auto& SubDeformer = ObjectsNode.Children.Emplace("Deformer");

		SubDeformer.AddPropertyInteger64(DeformerId);
		SubDeformer.AddPropertyString(string::Format("KoreLibMesh%02d_Bone%02d", SubmeshIndex, BoneIndex) + string("\u0000\u0001SubDeformer", 13));
		SubDeformer.AddPropertyString("Cluster");

		SubDeformer.Children.Emplace("Version").AddPropertyInteger32(100);

		auto GlobalMatrix = Math::Matrix::CreateFromQuaternion(Bone.GlobalRotation());
		GlobalMatrix.SetPosition(Bone.GlobalPosition());

		auto InverseGlobalMatrix = GlobalMatrix.Inverse();

		SubDeformer.Children.Emplace("Indexes").Properties.EmplaceBack('i', nullptr, 0);
		SubDeformer.Children.Emplace("Weights").Properties.EmplaceBack('d', nullptr, 0);

		auto& Transform = SubDeformer.Children.Emplace("Transform").Properties.Emplace('d', nullptr, 0);
		auto& TransformLink = SubDeformer.Children.Emplace("TransformLink").Properties.Emplace('d', nullptr, 0);

		for (uint32_t i = 0; i < 16; i++)
		{
			Transform.AddValueFloat64(InverseGlobalMatrix.GetMatrix()[i]);
			TransformLink.AddValueFloat64(GlobalMatrix.GetMatrix()[i]);
		}
	}

	bool KaydaraFBX::ExportAnimation(const Animation& Animation, const string& Path)
	{
		return false;
	}

	bool KaydaraFBX::ExportModel(const Model& Model, const string& Path)
	{
		auto Writer = IO::BinaryWriter(IO::File::Create(Path));
		auto FileName = IO::Path::GetFileNameWithoutExtension(Path);

		KaydaraFBXDocument Document;

		auto& ObjectsNode = Document.GetRootNode("Objects");
		auto& ConnectionsNode = Document.GetRootNode("Connections");

		uint32_t SubmeshIndex = 0;
		uint32_t BoneIndex = 0;
		uint32_t MatIndex = 0;

		uint64_t UniqueId = 0xDEADC0DE;
		uint64_t ModelId = 0x10000001;
		uint64_t DeformerId = 0x80000008;
		uint64_t RootModelId = 0xC0DEDEAD;
		uint64_t RootJointsId = 0xC0DEBEAD;

		auto& RootModelNode = ObjectsNode.Children.Emplace("Model");
		auto& RootJointsNode = ObjectsNode.Children.Emplace("Model");

		RootModelNode.AddPropertyInteger64(RootModelId);
		RootModelNode.AddPropertyString(FileName + string("\u0000\u0001Model", 7));
		RootModelNode.AddPropertyString("Null");

		RootJointsNode.AddPropertyInteger64(RootJointsId);
		RootJointsNode.AddPropertyString("Joints\u0000\u0001Model", 13);
		RootJointsNode.AddPropertyString("Null");

		InitializeRootNode(RootModelNode);
		InitializeRootNode(RootJointsNode);

		Dictionary<int32_t, uint64_t> BoneIdMap;

		for (auto& Bone : Model.Bones)
		{
			auto& SkeletonNode = ObjectsNode.Children.Emplace("NodeAttribute");

			SkeletonNode.AddPropertyInteger64(ModelId);
			SkeletonNode.AddPropertyString("\u0000\u0001NodeAttribute", 15);
			SkeletonNode.AddPropertyString("LimbNode");

			auto& SkeletonNodeProps = SkeletonNode.Children.Emplace("Properties70");

			{
				auto& Prop = SkeletonNodeProps.Children.Emplace("P");

				Prop.AddPropertyString("Size");
				Prop.AddPropertyString("double");
				Prop.AddPropertyString("Number");
				Prop.AddPropertyString("");
				Prop.AddPropertyFloat64(16.5);
			}

			SkeletonNode.Children.Emplace("TypeFlags").AddPropertyString("Skeleton");

			auto& JointModelNode = ObjectsNode.Children.Emplace("Model");

			auto& Position = Bone.LocalPosition();
			auto Rotation = Bone.LocalRotation().ToEulerAngles();

			JointModelNode.AddPropertyInteger64(UniqueId);
			JointModelNode.AddPropertyString(Bone.Name() + string("\u0000\u0001Model", 7));
			JointModelNode.AddPropertyString("LimbNode");

			JointModelNode.Children.Emplace("Version").AddPropertyInteger32(232);
			auto & JointNodeProps = JointModelNode.Children.Emplace("Properties70");

			{
				auto& Prop = JointNodeProps.Children.Emplace("P");

				Prop.AddPropertyString("PreRotation");
				Prop.AddPropertyString("Vector3D");
				Prop.AddPropertyString("Vector");
				Prop.AddPropertyString("");
				Prop.AddPropertyFloat64(Rotation.X);
				Prop.AddPropertyFloat64(Rotation.Y);
				Prop.AddPropertyFloat64(Rotation.Z);
			}

			{
				auto& Prop = JointNodeProps.Children.Emplace("P");

				Prop.AddPropertyString("RotationActive");
				Prop.AddPropertyString("bool");
				Prop.AddPropertyString("");
				Prop.AddPropertyString("");
				Prop.AddPropertyInteger32(1);
			}

			{
				auto& Prop = JointNodeProps.Children.Emplace("P");

				Prop.AddPropertyString("InheritType");
				Prop.AddPropertyString("enum");
				Prop.AddPropertyString("");
				Prop.AddPropertyString("");
				Prop.AddPropertyInteger32(2);
			}

			{
				auto& Prop = JointNodeProps.Children.Emplace("P");

				Prop.AddPropertyString("ScalingMax");
				Prop.AddPropertyString("Vector3D");
				Prop.AddPropertyString("Vector");
				Prop.AddPropertyString("");
				Prop.AddPropertyFloat64(0);
				Prop.AddPropertyFloat64(0);
				Prop.AddPropertyFloat64(0);
			}

			{
				auto& Prop = JointNodeProps.Children.Emplace("P");

				Prop.AddPropertyString("DefaultAttributeIndex");
				Prop.AddPropertyString("int");
				Prop.AddPropertyString("Integer");
				Prop.AddPropertyString("");
				Prop.AddPropertyInteger32(0);
			}

			{
				auto& Prop = JointNodeProps.Children.Emplace("P");

				Prop.AddPropertyString("Lcl Translation");
				Prop.AddPropertyString("Lcl Translation");
				Prop.AddPropertyString("");
				Prop.AddPropertyString("A");
				Prop.AddPropertyFloat64(Position.X);
				Prop.AddPropertyFloat64(Position.Y);
				Prop.AddPropertyFloat64(Position.Z);
			}

			AddObjectConnection(ConnectionsNode, ModelId, UniqueId);

			auto ParentIndex = Bone.Parent();

			if (ParentIndex >= 0)
			{
				AddObjectConnection(ConnectionsNode, UniqueId, BoneIdMap[ParentIndex]);
			}
			else
			{
				AddObjectConnection(ConnectionsNode, UniqueId, RootJointsId);
			}

			BoneIdMap.Add(BoneIndex, UniqueId);

			UniqueId++;
			ModelId++;
			BoneIndex++;
		}

		Dictionary<int32_t, uint64_t> MatIdMap;

		for (auto& Mat : Model.Materials)
		{
			auto& MaterialNode = ObjectsNode.Children.Emplace("Material");

			MaterialNode.AddPropertyInteger64(UniqueId);
			MaterialNode.AddPropertyString(Mat.Name + string("\u0000\u0001Material", 10));
			MaterialNode.AddPropertyString("");

			MaterialNode.Children.Emplace("Version").AddPropertyInteger32(102);
			MaterialNode.Children.Emplace("ShadingModel").AddPropertyString("Lambert");
			MaterialNode.Children.Emplace("MultiLayer").AddPropertyInteger32(0);

			MatIdMap.Add(MatIndex, UniqueId);

			UniqueId++;
			MatIndex++;

			string DiffuseMap = "";
			if (Mat.Slots.ContainsKey(MaterialSlotType::Albedo))
				DiffuseMap = Mat.Slots[MaterialSlotType::Albedo].first;
			else if (Mat.Slots.ContainsKey(MaterialSlotType::Diffuse))
				DiffuseMap = Mat.Slots[MaterialSlotType::Diffuse].first;

			if (!string::IsNullOrWhiteSpace(DiffuseMap))
			{
				InitializeMaterialTexture(ObjectsNode, Mat, UniqueId);
				AddObjectPropertyConnection(ConnectionsNode, UniqueId, UniqueId - 1, "DiffuseColor");

				UniqueId++;
			}
		}

		for (auto& Mesh : Model.Meshes)
		{
			auto& MeshModelNode = ObjectsNode.Children.Emplace("Model");

			MeshModelNode.AddPropertyInteger64(ModelId);
			MeshModelNode.AddPropertyString(string::Format("KoreLibMesh%02d", SubmeshIndex) + string("\u0000\u0001Model", 7));
			MeshModelNode.AddPropertyString("Mesh");

			MeshModelNode.Children.Emplace("Version").AddPropertyInteger32(232);
			auto& MeshNodeProps = MeshModelNode.Children.Emplace("Properties70");

			{
				auto& MeshProp = MeshNodeProps.Children.Emplace("P");
				MeshProp.AddPropertyString("Lcl Rotation");
				MeshProp.AddPropertyString("Lcl Rotation");
				MeshProp.AddPropertyString("");
				MeshProp.AddPropertyString("A");
				MeshProp.AddPropertyFloat64(0);
				MeshProp.AddPropertyFloat64(0);
				MeshProp.AddPropertyFloat64(0);
			}

			{
				auto& MeshProp = MeshNodeProps.Children.Emplace("P");
				MeshProp.AddPropertyString("DefaultAttributeIndex");
				MeshProp.AddPropertyString("int");
				MeshProp.AddPropertyString("Integer");
				MeshProp.AddPropertyString("");
				MeshProp.AddPropertyInteger32(0);
			}

			{
				auto& MeshProp = MeshNodeProps.Children.Emplace("P");
				MeshProp.AddPropertyString("InheritType");
				MeshProp.AddPropertyString("enum");
				MeshProp.AddPropertyString("");
				MeshProp.AddPropertyString("");
				MeshProp.AddPropertyInteger32(0);
			}

			MeshModelNode.Children.Emplace("MultiLayer").AddPropertyInteger32(0);
			MeshModelNode.Children.Emplace("MultiTake").AddPropertyInteger32(0);
			MeshModelNode.Children.Emplace("Shading").AddPropertyBoolean(true);
			MeshModelNode.Children.Emplace("Culling").AddPropertyString("CullingOff");

			auto& MeshNode = ObjectsNode.Children.Emplace("Geometry");

			MeshNode.AddPropertyInteger64(UniqueId);
			MeshNode.AddPropertyString(string::Format("KoreLibMesh%02d", SubmeshIndex) + string("\u0000\u0001Geometry", 10));
			MeshNode.AddPropertyString("Mesh");

			MeshNode.Children.EmplaceBack("Properties70");
			MeshNode.Children.Emplace("GeometryVersion").AddPropertyInteger32(124);

			auto& VertexBuffer = MeshNode.Children.Emplace("Vertices").Properties.Emplace('d', nullptr, 0);

			for (auto& Vertex : Mesh.Vertices)
			{
				auto& Position = Vertex.Position();

				VertexBuffer.AddValueFloat64(Position.X);
				VertexBuffer.AddValueFloat64(Position.Y);
				VertexBuffer.AddValueFloat64(Position.Z);
			}

			auto& FaceBuffer = MeshNode.Children.Emplace("PolygonVertexIndex").Properties.Emplace('i', nullptr, 0);

			for (auto& Face : Mesh.Faces)
			{
				FaceBuffer.AddValueInteger32(Face[2]);
				FaceBuffer.AddValueInteger32(Face[1]);
				FaceBuffer.AddValueInteger32(0xffffffff ^ Face[0]);
			}

			auto& LayerNormals = MeshNode.Children.Emplace("LayerElementNormal");

			LayerNormals.AddPropertyInteger32(0);

			LayerNormals.Children.Emplace("Version").AddPropertyInteger32(101);
			LayerNormals.Children.Emplace("Name").AddPropertyString("");
			LayerNormals.Children.Emplace("MappingInformationType").AddPropertyString("ByVertice");
			LayerNormals.Children.Emplace("ReferenceInformationType").AddPropertyString("Direct");

			auto& NormalsBuffer = LayerNormals.Children.Emplace("Normals").Properties.Emplace('d', nullptr, 0);

			for (auto& Vertex : Mesh.Vertices)
			{
				auto& Normal = Vertex.Normal();

				NormalsBuffer.AddValueFloat64(Normal.X);
				NormalsBuffer.AddValueFloat64(Normal.Y);
				NormalsBuffer.AddValueFloat64(Normal.Z);
			}

			for (uint8_t i = 0; i < Mesh.Vertices.UVLayerCount(); i++)
			{
				auto& LayerUVs = MeshNode.Children.Emplace("LayerElementUV");

				LayerUVs.AddPropertyInteger32(i);

				LayerUVs.Children.Emplace("Version").AddPropertyInteger32(101);
				LayerUVs.Children.Emplace("Name").AddPropertyString(string::Format("map%d", i + 1));
				LayerUVs.Children.Emplace("MappingInformationType").AddPropertyString("ByVertice");
				LayerUVs.Children.Emplace("ReferenceInformationType").AddPropertyString("Direct");

				auto& UVsBuffer = LayerUVs.Children.Emplace("UV").Properties.Emplace('d', nullptr, 0);

				for (auto& Vertex : Mesh.Vertices)
				{
					auto& UVLayer = Vertex.UVLayers(i);

					UVsBuffer.AddValueFloat64(UVLayer.U);
					UVsBuffer.AddValueFloat64((1.0l - UVLayer.V));
				}
			}

			{
				auto& LayerMats = MeshNode.Children.Emplace("LayerElementMaterial");

				LayerMats.AddPropertyInteger32(0);

				LayerMats.Children.Emplace("Version").AddPropertyInteger32(101);
				LayerMats.Children.Emplace("Name").AddPropertyString("");
				LayerMats.Children.Emplace("MappingInformationType").AddPropertyString("AllSame");
				LayerMats.Children.Emplace("ReferenceInformationType").AddPropertyString("IndexToDirect");

				auto& IndicesBuffer = LayerMats.Children.Emplace("Materials").Properties.Emplace('i', nullptr, 0);
				IndicesBuffer.AddValueInteger32(0);
			}

			auto& LayerInfo = MeshNode.Children.Emplace("Layer");

			LayerInfo.AddPropertyInteger32(0);

			LayerInfo.Children.Emplace("Version").AddPropertyInteger32(100);
			
			{
				auto& LayerElement = LayerInfo.Children.Emplace("LayerElement");
				LayerElement.Children.Emplace("Type").AddPropertyString("LayerElementNormal");
				LayerElement.Children.Emplace("TypedIndex").AddPropertyInteger32(0);
			}

			{
				auto& LayerElement = LayerInfo.Children.Emplace("LayerElement");
				LayerElement.Children.Emplace("Type").AddPropertyString("LayerElementMaterial");
				LayerElement.Children.Emplace("TypedIndex").AddPropertyInteger32(0);
			}

			{
				auto& LayerElement = LayerInfo.Children.Emplace("LayerElement");
				LayerElement.Children.Emplace("Type").AddPropertyString("LayerElementUV");
				LayerElement.Children.Emplace("TypedIndex").AddPropertyInteger32(0);
			}

			for (uint8_t i = 1; i < Mesh.Vertices.UVLayerCount(); i++)
			{
				auto& LayerInfo = MeshNode.Children.Emplace("Layer");

				LayerInfo.AddPropertyInteger32(i);

				auto& LayerElement = LayerInfo.Children.Emplace("LayerElement");
				LayerElement.Children.Emplace("Type").AddPropertyString("LayerElementUV");
				LayerElement.Children.Emplace("TypedIndex").AddPropertyInteger32(i);
			}

			AddObjectConnection(ConnectionsNode, ModelId, RootModelId);
			AddObjectConnection(ConnectionsNode, UniqueId, ModelId);

			if (Mesh.MaterialIndices[0] >= 0)
				AddObjectConnection(ConnectionsNode, MatIdMap[Mesh.MaterialIndices[0]], ModelId);			

			InitializeMeshSkinCluster(ObjectsNode, ConnectionsNode, SubmeshIndex, UniqueId, DeformerId);

			uint64_t RootDeformer = DeformerId++;

			Dictionary<int32_t, uint64_t> SubDeformers;
			Dictionary<int32_t, KaydaraFBXNode*> SubDeformerNodes;

			for (auto& Vertex : Mesh.Vertices)
			{
				for (uint8_t i = 0; i < Vertex.WeightCount(); i++)
				{
					auto& Weight = Vertex.Weights(i);
					int32_t BoneId = Weight.Bone;

					if (!SubDeformers.ContainsKey(BoneId))
					{
						// Make new subdeformer, connect to root, and add it
						InitializeMeshSubDeformer(ObjectsNode, Model.Bones[BoneId], SubmeshIndex, BoneId, DeformerId);

						// Add a connection
						AddObjectConnection(ConnectionsNode, DeformerId, RootDeformer);
						AddObjectConnection(ConnectionsNode, BoneIdMap[BoneId], DeformerId);

						// Add it
						SubDeformers.Add(BoneId, DeformerId++);
					}				
				}
			}

			for (auto& SubDef : SubDeformers)
				SubDeformerNodes.Add(SubDef.Key(), ObjectsNode.FindByUID(SubDef.Value()));

			uint32_t VertexIndex = 0;

			for (auto& Vertex : Mesh.Vertices)
			{
				for (uint8_t i = 0; i < Vertex.WeightCount(); i++)
				{
					auto& Weight = Vertex.Weights(i);
					int32_t BoneId = Weight.Bone;

					auto& DeformerNode = SubDeformerNodes[BoneId];

					DeformerNode->Children[1].Properties[0].AddValueInteger32(VertexIndex);
					DeformerNode->Children[2].Properties[0].AddValueFloat64(Weight.Value);
				}

				VertexIndex++;
			}
			
			UniqueId++;
			SubmeshIndex++;
			ModelId++;
		}

		AddObjectConnection(ConnectionsNode, RootModelId, 0);
		AddObjectConnection(ConnectionsNode, RootJointsId, 0);

		Document.Serialize(Writer);

		return true;
	}

	imstring KaydaraFBX::ModelExtension()
	{
		return ".fbx";
	}

	imstring KaydaraFBX::AnimationExtension()
	{
		return ".fbx";
	}

	ExporterScale KaydaraFBX::ExportScale()
	{
		return ExporterScale::Default;
	}

	bool KaydaraFBX::SupportsAnimations()
	{
		return false;
	}

	bool KaydaraFBX::SupportsModels()
	{
		return true;
	}
}
