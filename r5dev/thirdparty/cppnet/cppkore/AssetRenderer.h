#pragma once

#include <cstdint>
#include <memory>
#include <atomic>
#include <functional>
#include "Model.h"
#include "Animation.h"
#include "Texture.h"
#include "ListBase.h"
#include "OpenGLViewport.h"
#include "RenderViewCamera.h"
#include "RenderShader.h"
#include "RenderFont.h"

namespace Assets
{
	// A 3D uniform buffer object
	struct DrawObjectMaterial
	{
		uint32_t AlbedoMap;
		uint32_t NormalMap;
		uint32_t MetallicMap;
		uint32_t RoughnessMap;
		uint32_t AmbientOccluionMap;
	};

	// A 3D buffer object that is being drawn.
	struct DrawObject
	{
		uint32_t VertexArrayObject;
		uint32_t VertexBuffer;
		uint32_t FaceBuffer;

		bool LoadedMaterial;
		DrawObjectMaterial Material;

		uint32_t FaceCount;
		uint32_t VertexCount;

		DrawObject();
	};

	enum class DrawMode
	{
		Model,
		Animation,
		Texture
	};

	// A 3D asset renderer control.
	class AssetRenderer : public Forms::OpenGLViewport
	{
	public:
		AssetRenderer();
		virtual ~AssetRenderer();

		// Special function to stream in a material image
		using MaterialStreamCallback = std::function<std::unique_ptr<Texture>(const string, const uint64_t)>;

		// Clears the current model, if any, and assigns the new one
		void SetViewModel(const Model& Model);
		// Clears the current model
		void ClearViewModel();

		// Applies a custom material streamer routine
		void SetMaterialStreamer(MaterialStreamCallback Callback);

		// Clears the current texture, if any, and assigns the new one
		void SetViewTexture(const Texture& Texture);
		// Clears the current texture
		void ClearViewTexture();

		// Sets the name of the model
		void SetAssetName(const string& Name);

		// Enable or disable wireframe rendering
		void SetUseWireframe(bool Value);
		// Enable or disable bone rendering
		void SetShowBones(bool Value);
		// Enable or disable material rendering
		void SetShowMaterials(bool Value);

		// Changes the up axis
		void SetZUpAxis(bool ZUp);

		// Resets the current view to the default view
		void ResetView();
		// Brings the current model into view
		void ZoomModelToView();

		// We must define base events here
		virtual void OnRender();
		virtual void OnResize();
		virtual void OnHandleCreated();
		virtual void OnKeyUp(const std::unique_ptr<Forms::KeyEventArgs>& EventArgs);
		virtual void OnMouseDown(const std::unique_ptr<Forms::MouseEventArgs>& EventArgs);
		virtual void OnMouseMove(const std::unique_ptr<Forms::MouseEventArgs>& EventArgs);
		virtual void OnMouseWheel(const std::unique_ptr<Forms::HandledMouseEventArgs>& EventArgs);

	private:
		// Internal buffers
		List<DrawObject> _DrawObjects;

		// Internal buffer for texture mode
		uint32_t _DrawTexture;

		// Internal bone point buffer
		uint32_t _BonePointBuffer;
		// Internal bone point count
		uint32_t _BonePointCount;

		struct
		{
			uint32_t VertexCount;
			uint32_t TriangleCount;
			uint32_t MeshCount;

			uint32_t Width;
			uint32_t Height;

			int32_t Scale;

			string AssetName;

			uint32_t BoneCount;
		} _DrawInformation;

		DrawMode _DrawingMode;

		// The target mouse position
		Vector2 _TargetMousePosition;

		// Function to handle streaming in material images
		MaterialStreamCallback _MaterialStreamer;

		// The view camera instance
		RenderViewCamera _Camera;
		// The shader for the model
		RenderShader _ModelShader;
		// The render font instance
		RenderFont _RenderFont;

		// Whether or not to use wireframe mode
		bool _UseWireframe;
		// Whether or not to show bones
		bool _ShowBones;
		// Whether or not to render with materials
		bool _ShowMaterials;

		// Internal routine to render the gradient background
		void RenderBackground();
		// Internal routine to render the grid
		void RenderGrid();
		// Internal routine to render the model
		void RenderModel();
		// Internal routine to render the texture
		void RenderTexture();
		// Internal routine to render the hud
		void RenderHUD();

		// Internal routine to load a texture to an index
		void LoadDXTextureOGL(Texture& Texture, const uint32_t TextureSlot);
	};
}