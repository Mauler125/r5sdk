#include "stdafx.h"
#include "KaydaraFBXContainer.h"

namespace Assets::Exporters
{
	constexpr std::tuple<const char*, int> HeaderExtensionProperties[] = 
	{ 
		{"FBXHeaderVersion", 1003},
		{"FBXVersion", 7400},
		{"EncryptionType", 0}
	};

	constexpr std::tuple<const char*, int> HeaderExtensionTimeProperties[] =
	{
		{"Version", 1000},
		{"Year", 2019},
		{"Month", 4},
		{"Day", 26},
		{"Hour", 10},
		{"Minute", 39},
		{"Second", 36},
		{"Millisecond", 240}
	};

	constexpr std::pair<const char*, const char*> HeaderExtensionCreatorProperty = 
	{ 
		"Creator", "KoreLib by DTZxPorter" 
	};

	constexpr std::pair<const char*, const char*> HeaderFileIdNode =
	{
		"FileId", "\x28\xb3\x2a\xeb\xb6\x24\xcc\xc2\xbf\xc8\xb0\x2a\xa9\x2b\xfc\xf1"
	};

	constexpr std::pair<const char*, const char*> HeaderCreationTimeNode =
	{
		"CreationTime", "1970-01-01 10:00:00:000"
	};

	constexpr std::tuple<const char*, const char*, const char*> HeaderDefinitionsNode[] =
	{
		{"ObjectType", "GlobalSettings", ""},
		{"ObjectType", "NodeAttribute", ""},
		{"ObjectType", "Geometry", "FbxMesh"},
		{"ObjectType", "Model", "FbxNode"},
		{"ObjectType", "Pose", ""},
		{"ObjectType", "Deformer", ""},
		{"ObjectType", "Material", "FbxSurfacePhong"},
		{"ObjectType", "Texture", "FbxFileTexture"}
	};

	constexpr uint8_t FooterData[] = 
	{
		0xFA, 0xBC, 0xAB, 0x09, 0xD0, 0xC8, 0xD4, 0x66, 0xB1, 0x76, 0xFB, 0x83,
		0x1C, 0xF7, 0x26, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0xE8, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x5A, 0x8C, 0x6A, 0xDE, 0xF5,
		0xD9, 0x7E, 0xEC, 0xE9, 0x0C, 0xE3, 0x75, 0x8F, 0x29, 0x0B
	};

#pragma pack(push, 1)
	struct KaydaraFBXHeader
	{
		char Magic[0x15];

		uint16_t VersionMinor;
		uint32_t VersionMajor;
	};
#pragma pack(pop)

	KaydaraFBXDocument::KaydaraFBXDocument()
	{
		this->InitializeFBXHeaderExtension();
		this->InitializeGenerics();
		this->InitializeGlobalSettings();
		this->InitializeDocuments();
		this->InitializeReferences();
		this->InitializeDefinitions();
		this->InitializeDynamics();
	}

	void KaydaraFBXDocument::Serialize(IO::BinaryWriter& Writer)
	{
		Writer.Write<KaydaraFBXHeader>({ "Kaydara FBX Binary  ", 26, 7400 });

		for (auto& Child : this->Nodes)
			Child.Prepare();
		for (auto& Child : this->Nodes)
			Child.Serialize(Writer);

		uint8_t EmptyNode[13]{};
		Writer.Write(EmptyNode, 0, sizeof(EmptyNode));

		Writer.Write((uint8_t*)&FooterData[0], 0, sizeof(FooterData));
	}

	KaydaraFBXNode& KaydaraFBXDocument::GetRootNode(const char* Name)
	{
		for (auto& Node : this->Nodes)
			if (Node.Name == Name)
				return Node;

		throw std::exception("No FBX node was found");
	}

	void KaydaraFBXDocument::InitializeFBXHeaderExtension()
	{
		auto& Node = this->Nodes.Emplace("FBXHeaderExtension");

		for (auto& Property : HeaderExtensionProperties)
			Node.Children.Emplace(std::get<0>(Property)).Properties.EmplaceBack('I', &std::get<1>(Property));

		auto& CreationTime = Node.Children.Emplace("CreationTimeStamp");

		for (auto& Property : HeaderExtensionTimeProperties)
			CreationTime.Children.Emplace(std::get<0>(Property)).Properties.EmplaceBack('I', &std::get<1>(Property));
	}

	void KaydaraFBXDocument::InitializeGenerics()
	{
		string s1(HeaderFileIdNode.second);
		string s2(HeaderCreationTimeNode.second);
		string s3(HeaderExtensionCreatorProperty.second);
		this->Nodes.Emplace(HeaderFileIdNode.first).Properties.EmplaceBack('R', &s1);
		this->Nodes.Emplace(HeaderCreationTimeNode.first).Properties.EmplaceBack('S', &s2);
		this->Nodes.Emplace(HeaderExtensionCreatorProperty.first).Properties.EmplaceBack('S', &s3);
	}

	void KaydaraFBXDocument::InitializeGlobalSettings()
	{
		auto& Node = this->Nodes.Emplace("GlobalSettings");

		Node.Children.Emplace("Version").AddPropertyInteger32(1000);

		auto& Properties = Node.Children.Emplace("Properties70");

		{
			auto& Prop = Properties.Children.Emplace("P");

			Prop.AddPropertyString("UpAxis");
			Prop.AddPropertyString("int");
			Prop.AddPropertyString("Integer");
			Prop.AddPropertyString("");
			Prop.AddPropertyInteger32(1);
		}

		{
			auto& Prop = Properties.Children.Emplace("P");

			Prop.AddPropertyString("FrontAxis");
			Prop.AddPropertyString("int");
			Prop.AddPropertyString("Integer");
			Prop.AddPropertyString("");
			Prop.AddPropertyInteger32(2);
		}
	}

	void KaydaraFBXDocument::InitializeDocuments()
	{
		auto& Node = this->Nodes.Emplace("Documents");

		Node.Children.Emplace("Count").AddPropertyInteger32(1);

		auto& Document = Node.Children.Emplace("Document");

		Document.AddPropertyInteger64(318825901);
		Document.AddPropertyString("Scene");
		Document.AddPropertyString("Scene");

		Document.Children.Emplace("RootNode").AddPropertyInteger64(0);
	}

	void KaydaraFBXDocument::InitializeReferences()
	{
		this->Nodes.EmplaceBack("References");
	}

	void KaydaraFBXDocument::InitializeDefinitions()
	{
		auto& Node = this->Nodes.Emplace("Definitions");

		Node.Children.Emplace("Version").AddPropertyInteger32(100);
		Node.Children.Emplace("Count").AddPropertyInteger32(ARRAYSIZE(HeaderDefinitionsNode));

		for (auto& Type : HeaderDefinitionsNode)
		{
			auto& Definition = Node.Children.Emplace(std::get<0>(Type));

			Definition.AddPropertyString(std::get<1>(Type));
			Definition.Children.Emplace("Count").AddPropertyInteger32(1);

			if (strlen(std::get<2>(Type)))
				Definition.Children.Emplace("PropertyTemplate").AddPropertyString(std::get<2>(Type));
		}
	}

	void KaydaraFBXDocument::InitializeDynamics()
	{
		// Create each dynamic node
		auto& NodeObjects = this->Nodes.Emplace("Objects");
		auto& NodeConnections = this->Nodes.Emplace("Connections");
		auto& NodeTakes = this->Nodes.Emplace("Takes");

		NodeTakes.Children.Emplace("Current").AddPropertyString("");
	}

	KaydaraFBXProperty::KaydaraFBXProperty()
		: _Type(0), _IntegralValue{}
	{
	}

	KaydaraFBXProperty::KaydaraFBXProperty(const char Type, const void* Data)
		: _Type(Type), _IntegralValue{}
	{
		switch (Type)
		{
		case 'Y': _IntegralValue.Integer16 = *(uint16_t*)Data; break;
		case 'C': _IntegralValue.Boolean = *(bool*)Data; break;
		case 'I': _IntegralValue.Integer32 = *(uint32_t*)Data; break;
		case 'F': _IntegralValue.Float32 = *(float*)Data; break;
		case 'D': _IntegralValue.Float64 = *(double*)Data; break;
		case 'L': _IntegralValue.Integer64 = *(uint64_t*)Data; break;

		case 'R':
		case 'S':
			_StringValue = *(string*)Data;
			break;
		}
	}

	KaydaraFBXProperty::KaydaraFBXProperty(const char Type, const void* Data, const uint32_t Count)
		: _Type(Type), _IntegralValue{}
	{
		if (Data == nullptr || Count == 0)
			return;

		for (uint32_t i = 0; i < Count; i++)
		{
			KaydaraFBXIntegralProperty Property;

			switch (Type)
			{
			case 'b': Property.Boolean = ((bool*)Data)[i]; break;
			case 'i': Property.Integer32 = ((uint32_t*)Data)[i]; break;
			case 'f': Property.Float32 = ((float*)Data)[i]; break;
			case 'd': Property.Float64 = ((double*)Data)[i]; break;
			case 'l': Property.Integer64 = ((uint64_t*)Data)[i]; break;
			}

			this->_IntegralArrayValues.EmplaceBack(std::move(Property));
		}
	}

	void KaydaraFBXProperty::AddValueBoolean(bool Value)
	{
		KaydaraFBXIntegralProperty Property;
		Property.Boolean = Value;

		this->_IntegralArrayValues.EmplaceBack(std::move(Property));
	}

	void KaydaraFBXProperty::AddValueInteger32(uint32_t Value)
	{
		KaydaraFBXIntegralProperty Property;
		Property.Integer32 = Value;

		this->_IntegralArrayValues.EmplaceBack(std::move(Property));
	}

	void KaydaraFBXProperty::AddValueInteger64(uint64_t Value)
	{
		KaydaraFBXIntegralProperty Property;
		Property.Integer64 = Value;

		this->_IntegralArrayValues.EmplaceBack(std::move(Property));
	}

	void KaydaraFBXProperty::AddValueFloat32(float Value)
	{
		KaydaraFBXIntegralProperty Property;
		Property.Float32 = Value;

		this->_IntegralArrayValues.EmplaceBack(std::move(Property));
	}

	void KaydaraFBXProperty::AddValueFloat64(double Value)
	{
		KaydaraFBXIntegralProperty Property;
		Property.Float64 = Value;

		this->_IntegralArrayValues.EmplaceBack(std::move(Property));
	}

	void KaydaraFBXProperty::Serialize(IO::BinaryWriter& Writer)
	{
		Writer.Write<char>(this->_Type);

		switch (this->_Type)
		{
		case 'Y':
			Writer.Write<uint16_t>(this->_IntegralValue.Integer16);
			break;
		case 'C':
			Writer.Write<bool>(this->_IntegralValue.Boolean);
			break;
		case 'I':
			Writer.Write<uint32_t>(this->_IntegralValue.Integer32);
			break;
		case 'F':
			Writer.Write<float>(this->_IntegralValue.Float32);
			break;
		case 'D':
			Writer.Write<double>(this->_IntegralValue.Float64);
			break;
		case 'L':
			Writer.Write<uint64_t>(this->_IntegralValue.Integer64);
			break;
		case 'R':
		case 'S':
			Writer.Write<uint32_t>(this->_StringValue.Length());
			Writer.Write((uint8_t*)&this->_StringValue[0], 0, this->_StringValue.Length());
			break;

		default:
			Writer.Write<uint32_t>(this->_IntegralArrayValues.Count());
			Writer.Write<uint32_t>(0);

			{
				uint32_t CompressedLength = this->_IntegralArrayValues.Count();

				switch (this->_Type)
				{
				case 'f': CompressedLength *= sizeof(float); break;
				case 'd': CompressedLength *= sizeof(double); break;
				case 'l': CompressedLength *= sizeof(uint64_t); break;
				case 'i': CompressedLength *= sizeof(uint32_t); break;
				case 'b': CompressedLength *= sizeof(bool); break;
				}

				Writer.Write<uint32_t>(CompressedLength);

				for (auto& Value : this->_IntegralArrayValues)
				{
					switch (this->_Type)
					{
					case 'f':
						Writer.Write<float>(Value.Float32);
						break;
					case 'd':
						Writer.Write<double>(Value.Float64);
						break;
					case 'l':
						Writer.Write<uint64_t>(Value.Integer64);
						break;
					case 'i':
						Writer.Write<uint32_t>(Value.Integer32);
						break;
					case 'b':
						Writer.Write<bool>(Value.Boolean);
						break;
					}
				}
			}
			break;
		}
	}

	const uint32_t KaydaraFBXProperty::GetLength() const
	{
		switch (this->_Type)
		{
		case 'Y': return sizeof(uint16_t) + sizeof(char);
		case 'C': return sizeof(bool) + sizeof(char);
		case 'I': return sizeof(uint32_t) + sizeof(char);
		case 'F': return sizeof(float) + sizeof(char);
		case 'D': return sizeof(double) + sizeof(char);
		case 'L': return sizeof(uint64_t) + sizeof(char);
		case 'S':
		case 'R': 
			return this->_StringValue.Length() + sizeof(uint32_t) + sizeof(char);
		case 'f': return this->_IntegralArrayValues.Count() * sizeof(float) + 13;
		case 'd': return this->_IntegralArrayValues.Count() * sizeof(double) + 13;
		case 'l': return this->_IntegralArrayValues.Count() * sizeof(uint64_t) + 13;
		case 'i': return this->_IntegralArrayValues.Count() * sizeof(uint32_t) + 13;
		case 'b': return this->_IntegralArrayValues.Count() * sizeof(bool) + 13;
		default:
			return 0;
		}
	}

	KaydaraFBXIntegralProperty KaydaraFBXProperty::GetIntegralValue() const
	{
		return this->_IntegralValue;
	}

	KaydaraFBXNode::KaydaraFBXNode(const string& Name)
		: Name(Name)
	{
	}

	KaydaraFBXNode::KaydaraFBXNode(const char* Name)
		: Name(Name)
	{
	}

	void KaydaraFBXNode::Prepare()
	{
		for (auto& Child : this->Children)
			Child.Prepare();

		if (this->Children.Count() > 0 || (this->Properties.Count() == 0 && this->Name.Length() > 0))
			this->Children.EmplaceBack();
	}

	void KaydaraFBXNode::Serialize(IO::BinaryWriter& Writer)
	{
		if (this->Name.Length() == 0 && this->Children.Count() == 0 && this->Properties.Count() == 0)
		{
			uint8_t EmptyNode[13]{};
			Writer.Write(EmptyNode, 0, sizeof(EmptyNode));

			return;
		}

		uint32_t PropertyListLength = 0;
		uint32_t NodeLength = 13 + Name.Length();

		for (auto& Property : this->Properties)
			PropertyListLength += Property.GetLength();

		NodeLength += PropertyListLength;

		for (auto& Child : this->Children)
			NodeLength += Child.GetLength();

		Writer.Write<uint32_t>((uint32_t)Writer.GetBaseStream()->GetPosition() + NodeLength);
		Writer.Write<uint32_t>(this->Properties.Count());
		Writer.Write<uint32_t>(PropertyListLength);
		Writer.Write<uint8_t>((uint8_t)Name.Length());
		Writer.Write((uint8_t*)&Name[0], 0, Name.Length());

		for (auto& Property : this->Properties)
			Property.Serialize(Writer);
		for (auto& Child : this->Children)
			Child.Serialize(Writer);
	}

	void KaydaraFBXNode::AddPropertyBoolean(bool Value)
	{
		this->Properties.EmplaceBack('C', &Value);
	}

	void KaydaraFBXNode::AddPropertyInteger16(uint16_t Value)
	{
		this->Properties.EmplaceBack('Y', &Value);
	}

	void KaydaraFBXNode::AddPropertyInteger32(uint32_t Value)
	{
		this->Properties.EmplaceBack('I', &Value);
	}

	void KaydaraFBXNode::AddPropertyInteger64(uint64_t Value)
	{
		this->Properties.EmplaceBack('L', &Value);
	}

	void KaydaraFBXNode::AddPropertyFloat32(float Value)
	{
		this->Properties.EmplaceBack('F', &Value);
	}

	void KaydaraFBXNode::AddPropertyFloat64(double Value)
	{
		this->Properties.EmplaceBack('D', &Value);
	}

	void KaydaraFBXNode::AddPropertyString(const string& Value)
	{
		this->Properties.EmplaceBack('S', &Value);
	}

	void KaydaraFBXNode::AddPropertyString(const char* Value)
	{
		string s(Value);
		this->Properties.EmplaceBack('S', &s);
	}

	void KaydaraFBXNode::AddPropertyString(const char* Value, const uint32_t Length)
	{
		string s(Value, Length);
		this->Properties.EmplaceBack('S', &s);
	}

	void KaydaraFBXNode::AddPropertyRaw(const char* Value)
	{
		string s(Value);
		this->Properties.EmplaceBack('R', &s);
	}

	void KaydaraFBXNode::AddPropertyRaw(const char* Value, const uint32_t Length)
	{
		string s(Value, Length);
		this->Properties.EmplaceBack('R', &s);
	}

	KaydaraFBXNode* KaydaraFBXNode::FindByUID(const uint64_t UID)
	{
		for (auto& Child : this->Children)
		{
			if (Child.Properties.Count() > 0 && Child.Properties[0].GetIntegralValue().Integer64 == UID)
				return &Child;
		}

		return nullptr;
	}

	const uint32_t KaydaraFBXNode::GetLength() const
	{
		uint32_t Result = 13 + Name.Length();

		for (auto& Child : this->Children)
			Result += Child.GetLength();
		for (auto& Property : this->Properties)
			Result += Property.GetLength();

		return Result;
	}
}
