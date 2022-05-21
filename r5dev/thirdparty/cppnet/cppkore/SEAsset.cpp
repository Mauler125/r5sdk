#include "stdafx.h"
#include "SEAsset.h"

#include "File.h"
#include "BinaryWriter.h"

namespace Assets::Exporters
{
	constexpr static const char* SEModelMagic = "SEModel";
	constexpr static const char* SEAnimMagic = "SEAnim";

	// Specifies the data present for this file container
	enum class SEModelDataPresenceFlags : uint8_t
	{
		// Whether or not this model contains a bone block
		SEMODEL_PRESENCE_BONE = 1 << 0,
		// Whether or not this model contains submesh blocks
		SEMODEL_PRESENCE_MESH = 1 << 1,
		// Whether or not this model contains inline material blocks
		SEMODEL_PRESENCE_MATERIALS = 1 << 2,
		// The file contains a custom data block
		SEMODEL_PRESENCE_CUSTOM = 1 << 7,
	};

	// Specifies the data present for bones
	enum class SEModelBonePresenceFlags : uint8_t
	{
		// Whether or not bones contain global-space matricies
		SEMODEL_PRESENCE_GLOBAL_MATRIX = 1 << 0,
		// Whether or not bones contain local-space matricies
		SEMODEL_PRESENCE_LOCAL_MATRIX = 1 << 1,
		// Whether or not bones contain scales
		SEMODEL_PRESENCE_SCALES = 1 << 2,
	};

	// Specifies the data present for meshes
	enum class SEModelMeshPresenceFlags : uint8_t
	{
		// Whether or not meshes contain at least 1 uv map
		SEMODEL_PRESENCE_UVSET = 1 << 0,
		// Whether or not meshes contain vertex normals
		SEMODEL_PRESENCE_NORMALS = 1 << 1,
		// Whether or not meshes contain vertex colors (RGBA)
		SEMODEL_PRESENCE_COLOR = 1 << 2,
		// Whether or not meshes contain at least 1 weighted skin
		SEMODEL_PRESENCE_WEIGHTS = 1 << 3,
	};

	// Specifies how the data is interpreted by the importer
	enum class SEAnimAnimationType : uint8_t
	{
		// Animation translations are set to this exact value each frame
		SEANIM_ABSOLUTE = 0,
		// This animation is applied to existing animation data in the scene
		SEANIM_ADDITIVE = 1,
		// Animation translations are based on rest position in scene
		SEANIM_RELATIVE = 2
	};

	// Specifies the data present for each frame of every bone (Internal use only, matches specification v1.0.1)
	enum class SEAnimDataPresenceFlags : uint8_t
	{
		// Animation has bone location keyframes
		SEANIM_BONE_LOC = 1 << 0,
		// Animation has bone rotation keyframes
		SEANIM_BONE_ROT = 1 << 1,
		// Animation has bone scale keyframes
		SEANIM_BONE_SCALE = 1 << 2,
		// The file contains notetrack data
		SEANIM_PRESENCE_NOTE = 1 << 6,
		// The file contains a custom data block
		SEANIM_PRESENCE_CUSTOM = 1 << 7,
	};

	bool SEAsset::ExportAnimation(const Animation& Animation, const string& Path)
	{
		auto Writer = IO::BinaryWriter(IO::File::Create(Path));

		Writer.Write((void*)SEAnimMagic, 0, strlen(SEAnimMagic));	// Magic
		Writer.Write<uint16_t>(0x1);								// Version
		Writer.Write<uint16_t>(0x1C);								// Header size

		// We need to iterate all curves and find the must used type
		// When we do, we'll default to that, and inject modifiers for the rest...
		uint32_t Slots[3] = { 0, 0, 0 };

		// This is a traditional map of bones for the format
		List<string> Bones;

		{
			Dictionary<string, uint32_t> SEBones;

			for (auto& Kvp : Animation.Curves)
			{
				for (auto& Curve : Kvp.Value())
				{
					Slots[(uint32_t)Curve.Mode]++;
				}
				
				if (Kvp.Value().Count() > 0)
					SEBones.Add(Kvp.Key(), 0);
			}

			for (auto& Kvp : SEBones)
				Bones.EmplaceBack(Kvp.Key());
		}

		auto AnimationType = AnimationCurveMode::Absolute;

		uint32_t LargestSize = 0;
		for (uint32_t i = 0; i < 3; i++)
		{
			if (Slots[i] > LargestSize)
			{
				LargestSize = Slots[i];
				AnimationType = (AnimationCurveMode)i;
			}
		}

		switch (AnimationType)
		{
		case AnimationCurveMode::Absolute:
			Writer.Write<uint8_t>((uint8_t)SEAnimAnimationType::SEANIM_ABSOLUTE);
			break;
		case AnimationCurveMode::Additive:
			Writer.Write<uint8_t>((uint8_t)SEAnimAnimationType::SEANIM_ADDITIVE);
			break;
		case AnimationCurveMode::Relative:
			Writer.Write<uint8_t>((uint8_t)SEAnimAnimationType::SEANIM_RELATIVE);
			break;
		}

		Dictionary<uint32_t, AnimationCurveMode> BoneTypeModifiers;

		for (uint32_t i = 0; i < Bones.Count(); i++)
		{
			auto& Bone = Bones[i];

			if (Animation.Curves.ContainsKey(Bone))
			{
				auto& Curves = Animation.Curves[Bone];

				for (auto& Curve : Curves)
				{
					if (Curve.Mode != AnimationType)
						BoneTypeModifiers.Add(i, Curve.Mode);
				}
			}
		}

		Writer.Write<uint8_t>(Animation.Looping ? (1 << 0) : 0);

		auto DataPresentFlags = 0;
		DataPresentFlags |= (uint8_t)SEAnimDataPresenceFlags::SEANIM_BONE_LOC;
		DataPresentFlags |= (uint8_t)SEAnimDataPresenceFlags::SEANIM_BONE_ROT;
		DataPresentFlags |= (uint8_t)SEAnimDataPresenceFlags::SEANIM_BONE_SCALE;

		if (Animation.Notificiations.Count() > 0)
			DataPresentFlags |= (uint8_t)SEAnimDataPresenceFlags::SEANIM_PRESENCE_NOTE;

		Writer.Write<uint8_t>((uint8_t)DataPresentFlags);
		
		Writer.Write<uint8_t>(0);	// Data property flags
		Writer.Write<uint16_t>(0);	// Reserved bytes

		Writer.Write<float>(Animation.FrameRate);

		auto FrameCount = Animation.FrameCount(true);			// We must use legacy mode for SEAnims
		auto BoneCount = Bones.Count();
		auto NotificationCount = Animation.NotificationCount();	// Same as SEAnims

		Writer.Write<uint32_t>(FrameCount);
		Writer.Write<uint32_t>(BoneCount);
		Writer.Write<uint8_t>((uint8_t)BoneTypeModifiers.Count());

		uint8_t Reserved[] = { 0, 0, 0 };
		Writer.Write(&Reserved[0], 0, 3);

		Writer.Write<uint32_t>(NotificationCount);

		for (auto& Bone : Bones)
			Writer.WriteCString(Bone);

		for (auto& BoneTypeKvp : BoneTypeModifiers)
		{
			if (BoneCount <= 0xFF)
				Writer.Write<uint8_t>((uint8_t)BoneTypeKvp.Key());
			else if (BoneCount <= 0xFFFF)
				Writer.Write<uint16_t>((uint16_t)BoneTypeKvp.Key());
			else
				Writer.Write<uint32_t>((uint32_t)BoneTypeKvp.Key());

			switch (BoneTypeKvp.Value())
			{
			case AnimationCurveMode::Absolute:
				Writer.Write<uint8_t>((uint8_t)SEAnimAnimationType::SEANIM_ABSOLUTE);
				break;
			case AnimationCurveMode::Relative:
				Writer.Write<uint8_t>((uint8_t)SEAnimAnimationType::SEANIM_RELATIVE);
				break;
			case AnimationCurveMode::Additive:
				Writer.Write<uint8_t>((uint8_t)SEAnimAnimationType::SEANIM_ADDITIVE);
				break;
			}
		}

		for (uint32_t i = 0; i < BoneCount; i++)
		{
			Writer.Write<uint8_t>(0);	// Flags

			auto& BoneCurves = Animation.Curves[Bones[i]];

			int32_t TrackSlots[7] = { -1, -1, -1, -1, -1, -1, -1 };

			uint32_t PositionTrackCount = 0;
			uint32_t RotationTrackCount = 0;
			uint32_t ScaleTrackCount = 0;

			for (uint32_t i = 0; i < BoneCurves.Count(); i++)
			{
				auto& Curve = BoneCurves[i];
				if (Curve.Property == CurveProperty::RotateQuaternion)
				{
					TrackSlots[(uint32_t)Curve.Property - 1] = i;
					RotationTrackCount = max(RotationTrackCount, Curve.Keyframes.Count());
				}
				else if (Curve.Property >= CurveProperty::TranslateX && Curve.Property <= CurveProperty::TranslateZ)
				{
					TrackSlots[(uint32_t)Curve.Property - 4] = i;
					PositionTrackCount = max(PositionTrackCount, Curve.Keyframes.Count());
				}
				else if (Curve.Property >= CurveProperty::ScaleX && Curve.Property <= CurveProperty::ScaleZ)
				{
					TrackSlots[(uint32_t)Curve.Property - 4] = i;
					ScaleTrackCount = max(ScaleTrackCount, Curve.Keyframes.Count());
				}
			}

			//
			// Positions
			//

			if (FrameCount <= 0xFF)
				Writer.Write<uint8_t>((uint8_t)PositionTrackCount);
			else if (FrameCount <= 0xFFFF)
				Writer.Write<uint16_t>((uint16_t)PositionTrackCount);
			else
				Writer.Write<uint32_t>(PositionTrackCount);

			for (uint32_t i = 0; i < PositionTrackCount; i++)
			{
				uint32_t Frame = 0;
				Vector3 Key{};

				if (TrackSlots[1] > -1)
				{
					Frame = BoneCurves[TrackSlots[1]].Keyframes[i].Frame.Integer32;
					Key.X = BoneCurves[TrackSlots[1]].Keyframes[i].Value.Float;
				}
				if (TrackSlots[2] > -1)
				{
					Frame = BoneCurves[TrackSlots[2]].Keyframes[i].Frame.Integer32;
					Key.Y = BoneCurves[TrackSlots[2]].Keyframes[i].Value.Float;
				}
				if (TrackSlots[3] > -1)
				{
					Frame = BoneCurves[TrackSlots[3]].Keyframes[i].Frame.Integer32;
					Key.Z = BoneCurves[TrackSlots[3]].Keyframes[i].Value.Float;
				}

				if (FrameCount <= 0xFF)
					Writer.Write<uint8_t>((uint8_t)Frame);
				else if (FrameCount <= 0xFFFF)
					Writer.Write<uint16_t>((uint16_t)Frame);
				else
					Writer.Write<uint32_t>(Frame);

				Writer.Write<Vector3>(Key);
			}

			//
			// Rotations
			//

			if (FrameCount <= 0xFF)
				Writer.Write<uint8_t>((uint8_t)RotationTrackCount);
			else if (FrameCount <= 0xFFFF)
				Writer.Write<uint16_t>((uint16_t)RotationTrackCount);
			else
				Writer.Write<uint32_t>(RotationTrackCount);

			for (uint32_t i = 0; i < RotationTrackCount; i++)
			{
				uint32_t Frame = 0;
				Quaternion Key{0, 0, 0, 1};

				if (TrackSlots[0] > -1)
				{
					Frame = BoneCurves[TrackSlots[0]].Keyframes[i].Frame.Integer32;
					Key = BoneCurves[TrackSlots[0]].Keyframes[i].Value.Vector4;
				}

				if (FrameCount <= 0xFF)
					Writer.Write<uint8_t>((uint8_t)Frame);
				else if (FrameCount <= 0xFFFF)
					Writer.Write<uint16_t>((uint16_t)Frame);
				else
					Writer.Write<uint32_t>(Frame);

				Writer.Write<Quaternion>(Key);
			}

			//
			// Scales
			//

			if (FrameCount <= 0xFF)
				Writer.Write<uint8_t>((uint8_t)ScaleTrackCount);
			else if (FrameCount <= 0xFFFF)
				Writer.Write<uint16_t>((uint16_t)ScaleTrackCount);
			else
				Writer.Write<uint32_t>(ScaleTrackCount);

			for (uint32_t i = 0; i < ScaleTrackCount; i++)
			{
				uint32_t Frame = 0;
				Vector3 Key{};

				if (TrackSlots[4] > -1)
				{
					Frame = BoneCurves[TrackSlots[4]].Keyframes[i].Frame.Integer32;
					Key.X = BoneCurves[TrackSlots[4]].Keyframes[i].Value.Float;
				}
				if (TrackSlots[5] > -1)
				{
					Frame = BoneCurves[TrackSlots[5]].Keyframes[i].Frame.Integer32;
					Key.Y = BoneCurves[TrackSlots[5]].Keyframes[i].Value.Float;
				}
				if (TrackSlots[6] > -1)
				{
					Frame = BoneCurves[TrackSlots[6]].Keyframes[i].Frame.Integer32;
					Key.Z = BoneCurves[TrackSlots[6]].Keyframes[i].Value.Float;
				}

				if (FrameCount <= 0xFF)
					Writer.Write<uint8_t>((uint8_t)Frame);
				else if (FrameCount <= 0xFFFF)
					Writer.Write<uint16_t>((uint16_t)Frame);
				else
					Writer.Write<uint32_t>(Frame);

				Writer.Write<Vector3>(Key);
			}
		}

		for (auto& NoteTrack : Animation.Notificiations)
		{
			for (auto& Key : NoteTrack.Value())
			{
				if (FrameCount <= 0xFF)
					Writer.Write<uint8_t>((uint8_t)Key);
				else if (FrameCount <= 0xFFFF)
					Writer.Write<uint16_t>((uint16_t)Key);
				else
					Writer.Write<uint32_t>(Key);

				Writer.WriteCString(NoteTrack.Key());
			}
		}

		return true;
	}

	bool SEAsset::ExportModel(const Model& Model, const string& Path)
	{
		auto Writer = IO::BinaryWriter(IO::File::Create(Path));

		Writer.Write((void*)SEModelMagic, 0, strlen(SEModelMagic));	// Magic
		Writer.Write<uint16_t>(0x1);								// Version
		Writer.Write<uint16_t>(0x14);								// Header size

		auto FileDpFlags = 0;

		if (!Model.Bones.Empty())
			FileDpFlags |= (uint8_t)SEModelDataPresenceFlags::SEMODEL_PRESENCE_BONE;
		if (!Model.Meshes.Empty())
			FileDpFlags |= (uint8_t)SEModelDataPresenceFlags::SEMODEL_PRESENCE_MESH;
		if (!Model.Materials.Empty())
			FileDpFlags |= (uint8_t)SEModelDataPresenceFlags::SEMODEL_PRESENCE_MATERIALS;

		bool HasGlobalMatrix = false;
		bool HasLocalMatrix = false;
		bool HasScales = false;

		for (auto& Bone : Model.Bones)
		{
			if (Bone.GetFlag(BoneFlags::HasGlobalSpaceMatrices))
				HasGlobalMatrix = true;
			if (Bone.GetFlag(BoneFlags::HasLocalSpaceMatrices))
				HasLocalMatrix = true;
			if (Bone.GetFlag(BoneFlags::HasScale))
				HasScales = true;

			if (HasGlobalMatrix && HasLocalMatrix && HasScales)
				break;
		}

		auto BoneDpFlags = 0;

		if (HasGlobalMatrix)
			BoneDpFlags |= (uint8_t)SEModelBonePresenceFlags::SEMODEL_PRESENCE_GLOBAL_MATRIX;
		if (HasLocalMatrix)
			BoneDpFlags |= (uint8_t)SEModelBonePresenceFlags::SEMODEL_PRESENCE_LOCAL_MATRIX;
		if (HasScales)
			BoneDpFlags |= (uint8_t)SEModelBonePresenceFlags::SEMODEL_PRESENCE_SCALES;

		auto MeshDpFlags = 0;

		MeshDpFlags |= (uint8_t)SEModelMeshPresenceFlags::SEMODEL_PRESENCE_NORMALS;
		MeshDpFlags |= (uint8_t)SEModelMeshPresenceFlags::SEMODEL_PRESENCE_COLOR;
		MeshDpFlags |= (uint8_t)SEModelMeshPresenceFlags::SEMODEL_PRESENCE_UVSET;
		
		bool HasBones = false;
		if (!Model.Bones.Empty())
		{
			MeshDpFlags |= (uint8_t)SEModelMeshPresenceFlags::SEMODEL_PRESENCE_WEIGHTS;
			HasBones = true;
		}

		Writer.Write<uint8_t>(FileDpFlags);
		Writer.Write<uint8_t>(BoneDpFlags);
		Writer.Write<uint8_t>(MeshDpFlags);

		auto BoneCount = Model.Bones.Count();
		auto MeshCount = Model.Meshes.Count();
		auto MaterialCount = Model.Materials.Count();

		Writer.Write<uint32_t>(BoneCount);
		Writer.Write<uint32_t>(MeshCount);
		Writer.Write<uint32_t>(MaterialCount);

		uint8_t Reserved[3]{};
		Writer.Write(Reserved, 0, 3);

		for (auto& Bone : Model.Bones)
			Writer.WriteCString(Bone.Name());

		for (auto& Bone : Model.Bones)
		{
			Writer.Write<uint8_t>(0);				// Bone Flags
			Writer.Write<int32_t>(Bone.Parent());	// Parent

			if (HasGlobalMatrix)
			{
				Writer.Write<Vector3>(Bone.GlobalPosition());
				Writer.Write<Quaternion>(Bone.GlobalRotation());
			}

			if (HasLocalMatrix)
			{
				Writer.Write<Vector3>(Bone.LocalPosition());
				Writer.Write<Quaternion>(Bone.LocalRotation());
			}

			if (HasScales)
				Writer.Write<Vector3>(Bone.Scale());
		}

		for (auto& Mesh : Model.Meshes)
		{
			Writer.Write<uint8_t>(0);			// Mesh Flags

			auto VertexCount = Mesh.Vertices.Count();
			auto FaceCount = Mesh.Faces.Count();
			auto UVLayerCount = Mesh.Vertices.UVLayerCount();
			auto MaxSkinCount = Mesh.Vertices.WeightCount();

			Writer.Write<uint8_t>(UVLayerCount);
			Writer.Write<uint8_t>(MaxSkinCount);
			Writer.Write<uint32_t>(VertexCount);
			Writer.Write<uint32_t>(FaceCount);

			for (auto& Vertex : Mesh.Vertices)
				Writer.Write<Vector3>(Vertex.Position());

			for (auto& Vertex : Mesh.Vertices)
			{
				for (uint32_t i = 0; i < UVLayerCount; i++)
					Writer.Write<Vector2>(Vertex.UVLayers(i));
			}

			for (auto& Vertex : Mesh.Vertices)
				Writer.Write<Vector3>(Vertex.Normal());

			for (auto& Vertex : Mesh.Vertices)
				Writer.Write<VertexColor>(Vertex.Color());

			if (MaxSkinCount > 0 && HasBones)
			{
				for (auto& Vertex : Mesh.Vertices)
				{
					for (uint32_t i = 0; i < MaxSkinCount; i++)
					{
						auto& Weight = Vertex.Weights(i);

						if (BoneCount <= 0xFF)
							Writer.Write<uint8_t>((uint8_t)Weight.Bone);
						else if (BoneCount <= 0xFFFF)
							Writer.Write<uint16_t>((uint16_t)Weight.Bone);
						else
							Writer.Write<uint32_t>(Weight.Bone);

						Writer.Write<float>(Weight.Value);
					}
				}
			}

			for (auto& Face : Mesh.Faces)
			{
				if (VertexCount <= 0xFF)
				{
					Writer.Write<uint8_t>((uint8_t)Face[0]);
					Writer.Write<uint8_t>((uint8_t)Face[1]);
					Writer.Write<uint8_t>((uint8_t)Face[2]);
				}
				else if (VertexCount <= 0xFFFF)
				{
					Writer.Write<uint16_t>((uint16_t)Face[0]);
					Writer.Write<uint16_t>((uint16_t)Face[1]);
					Writer.Write<uint16_t>((uint16_t)Face[2]);
				}
				else
				{
					Writer.Write<uint32_t>(Face[0]);
					Writer.Write<uint32_t>(Face[1]);
					Writer.Write<uint32_t>(Face[2]);
				}
			}

			for (auto& MaterialIndex : Mesh.MaterialIndices)
				Writer.Write<int32_t>(MaterialIndex);
		}

		for (auto& Material : Model.Materials)
		{
			Writer.WriteCString(Material.Name);
			Writer.Write<bool>(true);

			if (Material.Slots.ContainsKey(MaterialSlotType::Albedo))
				Writer.WriteCString(Material.Slots[MaterialSlotType::Albedo].first);
			else if (Material.Slots.ContainsKey(MaterialSlotType::Diffuse))
				Writer.WriteCString(Material.Slots[MaterialSlotType::Diffuse].first);
			else
				Writer.Write<uint8_t>(0);

			if (Material.Slots.ContainsKey(MaterialSlotType::Normal))
				Writer.WriteCString(Material.Slots[MaterialSlotType::Normal].first);
			else
				Writer.Write<uint8_t>(0);

			if (Material.Slots.ContainsKey(MaterialSlotType::Specular))
				Writer.WriteCString(Material.Slots[MaterialSlotType::Specular].first);
			else
				Writer.Write<uint8_t>(0);
		}

		return true;
	}

	imstring SEAsset::ModelExtension()
	{
		return ".semodel";
	}

	imstring SEAsset::AnimationExtension()
	{
		return ".seanim";
	}

	ExporterScale SEAsset::ExportScale()
	{
		return ExporterScale::Default;
	}

	bool SEAsset::SupportsAnimations()
	{
		return true;
	}

	bool SEAsset::SupportsModels()
	{
		return true;
	}
}
