#include "stdafx.h"
#include <thread>
#include "AssetRenderer.h"
#include "Path.h"
#include "Thread.h"

// Include the internal font, and shaders
#include "FontArial.h"
#include "ModelFragmentShader.h"
#include "ModelVertexShader.h"

// We need to include the OpenGL classes
#include "Mangler.h"

namespace Assets
{
	AssetRenderer::AssetRenderer()
		: OpenGLViewport(), _Camera(0.5f* (float)MathHelper::PI, 0.45f* (float)MathHelper::PI, 100.f), _UseWireframe(false), _ShowBones(true), _ShowMaterials(true), _DrawingMode(DrawMode::Model), _DrawInformation{}, _BonePointBuffer(0), _BonePointCount(0), _DrawTexture(0)
	{
	}

	AssetRenderer::~AssetRenderer()
	{
		ClearViewModel();
		ClearViewTexture();
	}

	void AssetRenderer::SetViewModel(const Model& Model)
	{
		this->_DrawingMode = DrawMode::Model;
		ClearViewModel();
		ClearViewTexture();

		for (auto& Submesh : Model.Meshes)
		{
			auto Draw = DrawObject();

			glGenVertexArrays(1, &Draw.VertexArrayObject);
			glBindVertexArray(Draw.VertexArrayObject);

			// We'll take the POSITION, NORMAL, COLOR, and first UV pair...
			const uint32_t Stride = sizeof(Vector3) + sizeof(Vector3) + sizeof(VertexColor) + sizeof(Vector2);
			auto VertexBuffer = std::make_unique<uint8_t[]>((uint64_t)Submesh.Vertices.Count() * Stride);

			uint64_t Offset = 0;
			for (auto& Vertex : Submesh.Vertices)
			{
				std::memcpy(&VertexBuffer.get()[Offset], &Vertex.Position(), Stride);
				Offset += Stride;
			}

			glGenBuffers(1, &Draw.VertexBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, Draw.VertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, (Stride * (uint64_t)Submesh.Vertices.Count()), VertexBuffer.get(), GL_STATIC_DRAW);

			glGenBuffers(1, &Draw.FaceBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Draw.FaceBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, (uint64_t)Submesh.Faces.Count() * 3 * sizeof(uint32_t), &Submesh.Faces[0], GL_STATIC_DRAW);

			wglSwapIntervalEXT(1);

			Draw.FaceCount = Submesh.Faces.Count();
			Draw.VertexCount = Submesh.Vertices.Count();

			if (Submesh.MaterialIndices.Count() > 0 && this->_MaterialStreamer != nullptr && Model.Materials.Count() != 0)
			{
				Assets::Material& Material = Model.Materials[Submesh.MaterialIndices[0]];

				if (Material.Slots.ContainsKey(MaterialSlotType::Albedo))
				{
					auto MaterialDiffuseMap = this->_MaterialStreamer("", Material.Slots[MaterialSlotType::Albedo].second);
					if (MaterialDiffuseMap != nullptr)
					{
						glGenTextures(1, &Draw.Material.AlbedoMap);
						this->LoadDXTextureOGL(*MaterialDiffuseMap.get(), Draw.Material.AlbedoMap);
					}
				}
				
				if (Material.Slots.ContainsKey(MaterialSlotType::Normal))
				{
					auto MaterialNormalMap = this->_MaterialStreamer("", Material.Slots[MaterialSlotType::Normal].second);
					if (MaterialNormalMap != nullptr)
					{
						glGenTextures(1, &Draw.Material.NormalMap);
						this->LoadDXTextureOGL(*MaterialNormalMap.get(), Draw.Material.NormalMap);

					}
				}

				if (Material.Slots.ContainsKey(MaterialSlotType::Gloss))
				{
					auto MaterialGlossMap = this->_MaterialStreamer("", Material.Slots[MaterialSlotType::Gloss].second);
					if (MaterialGlossMap != nullptr)
					{
						glGenTextures(1, &Draw.Material.RoughnessMap);
						this->LoadDXTextureOGL(*MaterialGlossMap.get(), Draw.Material.RoughnessMap);

					}
				}

				if (Material.Slots.ContainsKey(MaterialSlotType::Specular))
				{
					auto MaterialSpecMap = this->_MaterialStreamer("", Material.Slots[MaterialSlotType::Specular].second);
					if (MaterialSpecMap != nullptr)
					{
						glGenTextures(1, &Draw.Material.MetallicMap);
						this->LoadDXTextureOGL(*MaterialSpecMap.get(), Draw.Material.MetallicMap);

					}
				}

				Draw.LoadedMaterial = true;
			}

			this->_DrawObjects.Emplace(Draw);
			this->_DrawInformation.VertexCount += Submesh.Vertices.Count();
			this->_DrawInformation.TriangleCount += Submesh.Faces.Count();
		}

		glGenBuffers(1, &this->_BonePointBuffer);
		List<Math::Vector3> Positions;

		for (int32_t i = (int32_t)Model.Bones.Count() - 1; i >= 0; i--)
		{
			auto& CurrentBone = Model.Bones[i];

			Positions.EmplaceBack(Math::Matrix::TransformVector(CurrentBone.GlobalPosition(), this->_Camera.GetModelMatrix()));

			if (CurrentBone.Parent() > -1)
				Positions.EmplaceBack(Math::Matrix::TransformVector(Model.Bones[CurrentBone.Parent()].GlobalPosition(), this->_Camera.GetModelMatrix()));
			else
				Positions.EmplaceBack(0.f, 0.f, 0.f);
		}

		this->_BonePointCount = Positions.Count();

		glBindBuffer(GL_ARRAY_BUFFER, this->_BonePointBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Math::Vector3) * Positions.Count(), &Positions[0], GL_STATIC_DRAW);

		this->_DrawInformation.BoneCount = Model.Bones.Count();
		this->_DrawInformation.MeshCount = Model.Meshes.Count();

		// Trigger a resize, then redraw, first redraw needs invalidation...
		this->OnResize();
		this->Invalidate();
	}

	void AssetRenderer::SetAssetName(const string& Name)
	{
		this->_DrawInformation.AssetName = Name;
		this->Redraw();
	}

	void AssetRenderer::ClearViewModel()
	{
		for (auto& Draw : _DrawObjects)
		{
			glDeleteVertexArrays(1, &Draw.VertexArrayObject);
			glDeleteBuffers(1, &Draw.VertexBuffer);
			glDeleteBuffers(1, &Draw.FaceBuffer);
			
			if (Draw.LoadedMaterial)
			{
				glDeleteTextures(1, &Draw.Material.AlbedoMap);
				glDeleteTextures(1, &Draw.Material.AmbientOccluionMap);
				glDeleteTextures(1, &Draw.Material.MetallicMap);
				glDeleteTextures(1, &Draw.Material.NormalMap);
				glDeleteTextures(1, &Draw.Material.RoughnessMap);
			}
		}

		glDeleteBuffers(1, &this->_BonePointBuffer);
		this->_BonePointBuffer = 0;
		this->_BonePointCount = 0;

		_DrawObjects.Clear();
		_DrawInformation = {};
	}

	void AssetRenderer::SetMaterialStreamer(MaterialStreamCallback Callback)
	{
		this->_MaterialStreamer = std::move(Callback);
	}

	void AssetRenderer::SetViewTexture(const Texture& Texture)
	{
		this->_DrawingMode = DrawMode::Texture;
		ClearViewTexture();
		ClearViewModel();

		glGenTextures(1, &this->_DrawTexture);

		// Load into the draw texture slot
		this->LoadDXTextureOGL((Assets::Texture&)Texture, this->_DrawTexture);

		this->_DrawInformation.Width = Texture.Width();
		this->_DrawInformation.Height = Texture.Height();

		// Determine best fit scale to fit the image
		float Scale = min((float)this->_ClientWidth / (float)Texture.Width(), (float)this->_ClientHeight / (float)Texture.Height());
		this->_DrawInformation.Scale = min(100, (int)(Scale * 100));
	}

	void AssetRenderer::ClearViewTexture()
	{
		glDeleteTextures(1, &this->_DrawTexture);
		_DrawInformation = {};
	}

	void AssetRenderer::SetUseWireframe(bool Value)
	{
		if (Value != this->_UseWireframe)
		{
			this->_UseWireframe = Value;
			this->Redraw();
		}
	}

	void AssetRenderer::SetShowBones(bool Value)
	{
		if (Value != this->_ShowBones)
		{
			this->_ShowBones = Value;
			this->Redraw();
		}
	}

	void AssetRenderer::SetShowMaterials(bool Value)
	{
		if (Value != this->_ShowMaterials)
		{
			this->_ShowMaterials = Value;
			this->Redraw();
		}
	}

	void AssetRenderer::SetZUpAxis(bool ZUp)
	{
		if (ZUp)
			this->_Camera.SetUpAxis(RenderViewCameraUpAxis::Z);
		else
			this->_Camera.SetUpAxis(RenderViewCameraUpAxis::Y);

		if (this->GetState(Forms::ControlStates::StateCreated))
			this->Redraw();
	}

	void AssetRenderer::ResetView()
	{
		this->_Camera.Reset(0.5f * (float)MathHelper::PI, 0.45f * (float)MathHelper::PI, 100.f);
		this->Redraw();
	}

	void AssetRenderer::ZoomModelToView()
	{
		// Using the bounding box, we need to 
		// TODO: Implement this zoom code
	}

	void AssetRenderer::OnRender()
	{
		// Clear the frame
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		this->RenderBackground();
		
		switch (this->_DrawingMode)
		{
		case DrawMode::Model:
			this->RenderGrid();
			this->RenderModel();
			break;
		case DrawMode::Texture:
			this->RenderTexture();
			break;
		}

		this->RenderHUD();

		// Render the frame
		SwapBuffers(this->_DCHandle);
	}

	void AssetRenderer::OnResize()
	{
		glViewport(0, 0, this->_ClientWidth, this->_ClientHeight);

		// Update the projection matrix
		this->_Camera.UpdateProjectionMatrix(45.f, (float)this->_ClientWidth, (float)this->_ClientHeight, .1f, 10000.f);

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(this->_Camera.GetProjectionMatrix().GetMatrix());

		// Switch back to the modelview matrix
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// Ask the control to do the resize event
		OpenGLViewport::OnResize();
	}

	void AssetRenderer::OnHandleCreated()
	{
		OpenGLViewport::OnHandleCreated();

		// Now, the opengl context is completely valid, we can initialize our font / shaders
		this->_RenderFont.LoadFont(FontArial_PTRF, sizeof(FontArial_PTRF));

#if _DEBUG
		// Grab the root path of the libraries...
		auto CppNetPath = IO::Path::GetDirectoryName(IO::Path::GetDirectoryName(__FILE__));
		auto Vert = IO::Path::Combine(CppNetPath, "cppkore_shaders\\preview.vert");
		auto Frag = IO::Path::Combine(CppNetPath, "cppkore_shaders\\preview.frag");
		printf("-- NOTICE: Using debug shaders from disk --\n");
		printf("Vertex: %s\nFrag: %s\n", Vert.ToCString(), Frag.ToCString());
		this->_ModelShader.LoadShader(Vert, Frag);
#else
		this->_ModelShader.LoadShader(ModelVertexShader_Src, ModelFragmentShader_Src);
#endif
	}

	void AssetRenderer::OnKeyUp(const std::unique_ptr<Forms::KeyEventArgs>& EventArgs)
	{
		if (EventArgs->KeyCode() == Forms::Keys::W)
		{
			this->SetUseWireframe(!this->_UseWireframe);
		}
		else if (EventArgs->KeyCode() == Forms::Keys::B)
		{
			this->SetShowBones(!this->_ShowBones);
		}
		else if (EventArgs->KeyCode() == Forms::Keys::T)
		{
			this->SetShowMaterials(!this->_ShowMaterials);
		}

		OpenGLViewport::OnKeyUp(EventArgs);
	}

	void AssetRenderer::OnMouseDown(const std::unique_ptr<Forms::MouseEventArgs>& EventArgs)
	{
		this->_TargetMousePosition = Vector2((float)EventArgs->X, (float)EventArgs->Y);
		OpenGLViewport::OnMouseDown(EventArgs);
	}

	void AssetRenderer::OnMouseMove(const std::unique_ptr<Forms::MouseEventArgs>& EventArgs)
	{
		auto IsAltKey = (GetKeyState(VK_MENU) & 0x8000);

		if (EventArgs->Button == Forms::MouseButtons::Left && IsAltKey)
		{
			float dPhi = ((float)(this->_TargetMousePosition.Y - EventArgs->Y) / 200.f);
			float dTheta = ((float)(this->_TargetMousePosition.X - EventArgs->X) / 200.f);

			this->_Camera.Rotate(dTheta, dPhi);
			this->Redraw();
		}
		else if (EventArgs->Button == Forms::MouseButtons::Middle && IsAltKey)
		{
			float dx = ((float)(this->_TargetMousePosition.X - EventArgs->X));
			float dy = ((float)(this->_TargetMousePosition.Y - EventArgs->Y));

			this->_Camera.Pan(dx * .1f, dy * .1f);
			this->Redraw();
		}
		else if (EventArgs->Button == Forms::MouseButtons::Right && IsAltKey)
		{
			float dx = ((float)(this->_TargetMousePosition.X - EventArgs->X) / 2.f);

			this->_Camera.Zoom(-dx);
			this->Redraw();
		}

		this->_TargetMousePosition = Vector2((float)EventArgs->X, (float)EventArgs->Y);

		OpenGLViewport::OnMouseMove(EventArgs);
	}

	void AssetRenderer::OnMouseWheel(const std::unique_ptr<Forms::HandledMouseEventArgs>& EventArgs)
	{
		if (this->_DrawingMode == DrawMode::Texture)
		{
			if (EventArgs->Delta > 0)
				this->_DrawInformation.Scale += 3;
			else
				this->_DrawInformation.Scale -= 3;
			this->_DrawInformation.Scale = max(min(200, this->_DrawInformation.Scale), 0);
		}
		else if (this->_DrawingMode == DrawMode::Model)
		{
			this->_Camera.Zoom((float)EventArgs->Delta * .04f);
		}

		this->Redraw();

		OpenGLViewport::OnMouseWheel(EventArgs);
	}

	void AssetRenderer::RenderBackground()
	{
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glBegin(GL_QUADS);

		glColor3f(this->_BackColor.GetR() / 255.f, this->_BackColor.GetG() / 255.f, this->_BackColor.GetB() / 255.f);

		glVertex2f(-1.0, -1.0);
		glVertex2f(1.0, -1.0);
		glVertex2f(1.0, 1.0);
		glVertex2f(-1.0, 1.0);

		glEnd();
	}

	void AssetRenderer::RenderGrid()
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(this->_Camera.GetProjectionMatrix().GetMatrix());

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(this->_Camera.GetViewMatrix().GetMatrix());

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		glBegin(GL_LINES);
		glLineWidth(1.f);

		auto Size = 5.0f;
		auto MinSize = -Size;
		auto Step = 0.5f;

		for (GLfloat i = MinSize; i <= Size; i += Step)
		{
			if (i == 0)
			{
				glColor3f(0, 0, 0);
			}
			else
			{
				glColor3f(.70f, .70f, .70f);
			}

			glVertex3f(i, 0, Size); glVertex3f(i, 0, MinSize);
			glVertex3f(Size, 0, i); glVertex3f(MinSize, 0, i);
		}

		glEnd();
	}

	void AssetRenderer::RenderModel()
	{
		this->_ModelShader.Use();

		if (_UseWireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// Calculate and load the matrix
		auto Model = this->_Camera.GetModelMatrix();
		auto View = this->_Camera.GetViewMatrix();
		auto Proj = this->_Camera.GetProjectionMatrix();

		const uint32_t Stride = sizeof(Vector3) + sizeof(Vector3) + sizeof(VertexColor) + sizeof(Vector2);

		glUniformMatrix4fv(this->_ModelShader.GetUniformLocation("model"), 1, GL_FALSE, Model.GetMatrix());
		glUniformMatrix4fv(this->_ModelShader.GetUniformLocation("view"), 1, GL_FALSE, View.GetMatrix());
		glUniformMatrix4fv(this->_ModelShader.GetUniformLocation("projection"), 1, GL_FALSE, Proj.GetMatrix());

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);

		for (auto& Draw : this->_DrawObjects)
		{

			if (Draw.LoadedMaterial && this->_ShowMaterials)
			{
				const GLint DiffuseLoaded = 1;
				glUniform1iv(this->_ModelShader.GetUniformLocation("diffuseLoaded"), 1, &DiffuseLoaded);


			}
			else
			{
				const GLint DiffuseLoaded = 0;
				glUniform1iv(this->_ModelShader.GetUniformLocation("diffuseLoaded"), 1, &DiffuseLoaded);


			}



			glEnable(GL_TEXTURE_2D);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Draw.Material.AlbedoMap);

			const GLint DiffuseSlot = 0;
			glUniform1iv(this->_ModelShader.GetUniformLocation("diffuseTexture"), 1, &DiffuseSlot);


			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, Draw.Material.NormalMap);


			const GLint NormalSlot = 1;
			glUniform1iv(this->_ModelShader.GetUniformLocation("normalTexture"), 1, &NormalSlot);



			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, Draw.Material.RoughnessMap);


			const GLint GlossSlot = 2;
			glUniform1iv(this->_ModelShader.GetUniformLocation("glossTexture"), 1, &GlossSlot);



			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, Draw.Material.MetallicMap);


			const GLint SpecularSlot = 3;
			glUniform1iv(this->_ModelShader.GetUniformLocation("specularTexture"), 1, &SpecularSlot);

			glActiveTexture(GL_TEXTURE0);

			glBindBuffer(GL_ARRAY_BUFFER, Draw.VertexBuffer);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Stride, (void*)0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, Stride, (void*)12);
			glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, Stride, (void*)24);
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, Stride, (void*)28);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Draw.FaceBuffer);
			glDrawElements(GL_TRIANGLES, Draw.FaceCount * 3, GL_UNSIGNED_INT, (void*)0);
		}

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);

		this->_ModelShader.Detatch();

		if (this->_ShowBones)
		{
			glDisable(GL_DEPTH_TEST);

			glColor3f(18 / 255.f, 0, 54 / 255.f);
			glBindBuffer(GL_ARRAY_BUFFER, this->_BonePointBuffer);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

			glDrawArrays(GL_LINES, 0, this->_BonePointCount);
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	void AssetRenderer::RenderTexture()
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(Matrix::CreateOrthographic(0, (float)this->_ClientWidth, (float)this->_ClientHeight, 0, -1, 1).GetMatrix());
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glColor3f(1.0f, 1.0f, 1.0f);
		glDisable(GL_BLEND);

		float Scale = (float)this->_DrawInformation.Scale / 100.f;

		float ImageWidth = this->_DrawInformation.Width * Scale;
		float ImageHeight = this->_DrawInformation.Height * Scale;

		float Width = (ImageWidth);
		float Height = (ImageHeight);
		float X = (this->_ClientWidth - (float)Width) / 2.0f;
		float Y = (this->_ClientHeight - (float)Height) / 2.0f;

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, this->_DrawTexture);
		glBegin(GL_QUADS);
		glTexCoord2i(0, 0); glVertex2f(X, Y);
		glTexCoord2i(0, 1); glVertex2f(X, Y + Height);
		glTexCoord2i(1, 1); glVertex2f(X + Width, Y + Height);
		glTexCoord2i(1, 0); glVertex2f(X + Width, Y);
		glEnd();
	}

	void AssetRenderer::RenderHUD()
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(Matrix::CreateOrthographic(0, (float)this->_ClientWidth, (float)this->_ClientHeight, 0, -1, 1).GetMatrix());

		// Scale the font size for dpi aware controls
		auto Screen = GetDC(nullptr);
		auto Ratio = GetDeviceCaps(Screen, LOGPIXELSX);
		ReleaseDC(nullptr, Screen);

		// We based layout on stock 96 dpi, which is 100% scaling on windows
		// so scale linearly based on that
		const auto Scale = (1.0f - (96.f / (float)Ratio)) + 1.0f;
		const auto FontScale = 0.7f * Scale;

		switch (this->_DrawingMode)
		{
		case DrawMode::Model:
			glColor4f(3 / 255.f, 169 / 255.f, 244 / 255.f, 1);

			_RenderFont.RenderString("Model", 22, 22, FontScale); _RenderFont.RenderString(":", 80, 22, FontScale);
			_RenderFont.RenderString("Meshes", 22, 38, FontScale); _RenderFont.RenderString(":", 80, 38, FontScale);
			_RenderFont.RenderString("Verts", 22, 54, FontScale); _RenderFont.RenderString(":", 80, 54, FontScale);
			_RenderFont.RenderString("Tris", 22, 70, FontScale); _RenderFont.RenderString(":", 80, 70, FontScale);
			_RenderFont.RenderString("Bones", 22, 86, FontScale); _RenderFont.RenderString(":", 80, 86, FontScale);

			glColor4f(35 / 255.f, 206 / 255.f, 107 / 255.f, 1);

			_RenderFont.RenderString(string((this->_ShowBones) ? "Hide Bones (b), " : "Draw Bones (b), ") + string((this->_ShowMaterials) ? "Shaded View (t), " : "Material View (t), ") + string((this->_UseWireframe) ? "Hide Wireframe (w)" : "Draw Wireframe (w)"), 22, this->_Height - 44.f, FontScale);

			glColor4f(0.9f, 0.9f, 0.9f, 1);

			_RenderFont.RenderString((this->_DrawInformation.AssetName == "") ? string("N/A") : this->_DrawInformation.AssetName, 96, 22, FontScale);
			_RenderFont.RenderString(string::Format("%d", this->_DrawInformation.MeshCount), 96, 38, FontScale);
			_RenderFont.RenderString(string::Format("%d", this->_DrawInformation.VertexCount), 96, 54, FontScale);
			_RenderFont.RenderString(string::Format("%d", this->_DrawInformation.TriangleCount), 96, 70, FontScale);
			_RenderFont.RenderString(string::Format("%d", this->_DrawInformation.BoneCount), 96, 86, FontScale);
			break;
		case DrawMode::Texture:
			glColor4f(3 / 255.f, 169 / 255.f, 244 / 255.f, 1);

			_RenderFont.RenderString("Image", 22, 22, FontScale); _RenderFont.RenderString(":", 80, 22, FontScale);
			_RenderFont.RenderString("Width", 22, 38, FontScale); _RenderFont.RenderString(":", 80, 38, FontScale);
			_RenderFont.RenderString("Height", 22, 54, FontScale); _RenderFont.RenderString(":", 80, 54, FontScale);
			_RenderFont.RenderString("Scale", 22, 70, FontScale); _RenderFont.RenderString(":", 80, 70, FontScale);

			glColor4f(0.9f, 0.9f, 0.9f, 1);

			_RenderFont.RenderString((this->_DrawInformation.AssetName == "") ? string("N/A") : this->_DrawInformation.AssetName, 96, 22, FontScale);
			_RenderFont.RenderString(string::Format("%d", this->_DrawInformation.Width), 96, 38, FontScale);
			_RenderFont.RenderString(string::Format("%d", this->_DrawInformation.Height), 96, 54, FontScale);
			_RenderFont.RenderString(string::Format("%d%%", this->_DrawInformation.Scale), 96, 70, FontScale);
			break;
		}
	}

	void AssetRenderer::LoadDXTextureOGL(Texture& Texture, const uint32_t TextureSlot)
	{
		glBindTexture(GL_TEXTURE_2D, TextureSlot);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		switch (Texture.Format())
		{
		case DXGI_FORMAT::DXGI_FORMAT_BC6H_UF16:
			Texture.ConvertToFormat(DXGI_FORMAT_BC1_UNORM);
		case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB:
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, Texture.Width(), Texture.Height(), 0, Texture.BlockSize(), Texture.GetPixels());
			break;
		case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB:
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, Texture.Width(), Texture.Height(), 0, Texture.BlockSize(), Texture.GetPixels());
			break;
		case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB:
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, Texture.Width(), Texture.Height(), 0, Texture.BlockSize(), Texture.GetPixels());
			break;
		case DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT::DXGI_FORMAT_BC4_SNORM:
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RED_RGTC1, Texture.Width(), Texture.Height(), 0, Texture.BlockSize(), Texture.GetPixels());
			break;
		case DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM:
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RG_RGTC2, Texture.Width(), Texture.Height(), 0, Texture.BlockSize(), Texture.GetPixels());
			break;
		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB:
		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM:
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, Texture.Width(), Texture.Height(), 0, Texture.BlockSize(), Texture.GetPixels());
			break;
		case DXGI_FORMAT::DXGI_FORMAT_R8_UNORM:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, Texture.Width(), Texture.Height(), 0, GL_RED, GL_UNSIGNED_BYTE, Texture.GetPixels());
			break;
		case DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, Texture.Width(), Texture.Height(), 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, Texture.GetPixels());
			break;
		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Texture.Width(), Texture.Height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, Texture.GetPixels());
			break;
		default:
#if _DEBUG
			printf("Unsupported DXGI->OGL mapping\n");
			__debugbreak();
#endif
			break;
		}
	}

	DrawObject::DrawObject()
		: VertexArrayObject(0), VertexBuffer(0), FaceBuffer(0), LoadedMaterial(false), FaceCount(0), VertexCount(0), Material{(uint32_t)-1, (uint32_t)-1, (uint32_t)-1, (uint32_t)-1, (uint32_t)-1}
	{
	}
}