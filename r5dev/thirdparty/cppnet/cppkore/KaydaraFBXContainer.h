#pragma once

#include <cstdint>
#include <tuple>
#include "StringBase.h"
#include "ListBase.h"
#include "BinaryWriter.h"

namespace Assets::Exporters
{
	// Represents any integral property of a node.
	union KaydaraFBXIntegralProperty
	{
		bool Boolean;
		uint16_t Integer16;
		uint32_t Integer32;
		uint64_t Integer64;
		float Float32;
		double Float64;
	};

	// Represents a property value of a node.
	class KaydaraFBXProperty
	{
	public:
		KaydaraFBXProperty();
		KaydaraFBXProperty(const char Type, const void* Data);
		KaydaraFBXProperty(const char Type, const void* Data, const uint32_t Count);

		// Add value proxies for array types.
		void AddValueBoolean(bool Value);
		void AddValueInteger32(uint32_t Value);
		void AddValueInteger64(uint64_t Value);
		void AddValueFloat32(float Value);
		void AddValueFloat64(double Value);

		// Serialize the property to the writer.
		void Serialize(IO::BinaryWriter& Writer);

		// Returns the length, in bytes, of this property.
		const uint32_t GetLength() const;

		// Returns the integral value, if any, of the property.
		KaydaraFBXIntegralProperty GetIntegralValue() const;

	private:
		// Internal property type
		char _Type;

		// Internal value if it's integral
		KaydaraFBXIntegralProperty _IntegralValue;
		// Internal value for integral arrays
		List<KaydaraFBXIntegralProperty> _IntegralArrayValues;

		// Internal value for string and raw types
		string _StringValue;
	};

	// Represents a node of an FBXDocument.
	class KaydaraFBXNode
	{
	public:
		KaydaraFBXNode() = default;
		KaydaraFBXNode(const string& Name);
		explicit KaydaraFBXNode(const char* Name);

		// Prepares a new node.
		void Prepare();
		// Serialize the node to the writer.
		void Serialize(IO::BinaryWriter& Writer);

		// Add property proxies
		void AddPropertyBoolean(bool Value);
		void AddPropertyInteger16(uint16_t Value);
		void AddPropertyInteger32(uint32_t Value);
		void AddPropertyInteger64(uint64_t Value);
		void AddPropertyFloat32(float Value);
		void AddPropertyFloat64(double Value);
		void AddPropertyString(const string& Value);
		void AddPropertyString(const char* Value);
		void AddPropertyString(const char* Value, const uint32_t Length);
		void AddPropertyRaw(const char* Value);
		void AddPropertyRaw(const char* Value, const uint32_t Length);

		// Finds a child by it's UID.
		KaydaraFBXNode* FindByUID(const uint64_t UID);

		// Returns the length, in bytes, of this node.
		const uint32_t GetLength() const;

		// A list of properties attached to this node.
		List<KaydaraFBXProperty> Properties;
		// A list of child nodes.
		List<KaydaraFBXNode> Children;
		// The name of this node.
		string Name;
	};

	// Represents the root of an FBX container.
	class KaydaraFBXDocument
	{
	public:
		KaydaraFBXDocument();
		~KaydaraFBXDocument() = default;

		// Serialize the document to the writer.
		void Serialize(IO::BinaryWriter& Writer);

		// A list of document nodes.
		List<KaydaraFBXNode> Nodes;

		// Returns a root document node by name
		KaydaraFBXNode& GetRootNode(const char* Name);

	private:

		// Internal routines to setup a document
		void InitializeFBXHeaderExtension();
		void InitializeGenerics();
		void InitializeGlobalSettings();
		void InitializeDocuments();
		void InitializeReferences();
		void InitializeDefinitions();
		void InitializeDynamics();
	};
}