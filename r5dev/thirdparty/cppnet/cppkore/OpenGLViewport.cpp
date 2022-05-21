#include "stdafx.h"
#include "OpenGLViewport.h"

// We need to include the OpenGL classes
#include "Mangler.h"

namespace Forms
{
	OpenGLViewport::OpenGLViewport()
		: Control(), _DCHandle(nullptr), _GLHandle(nullptr)
	{
		SetStyle(ControlStyles::UserPaint | ControlStyles::AllPaintingInWmPaint | ControlStyles::Opaque | ControlStyles::UserMouse, true);
		this->SetDoubleBuffered(false);
	}

	OpenGLViewport::~OpenGLViewport()
	{
		if (_DCHandle)
			ReleaseDC(this->_Handle, this->_DCHandle);
		if (_GLHandle)
			wglDeleteContext(this->_GLHandle);
	}

	float OpenGLViewport::AspectRatio()
	{
		return (float)this->_ClientWidth / (float)this->_ClientHeight;
	}

	void OpenGLViewport::Redraw()
	{
		this->OnRender();
	}

	void OpenGLViewport::OnHandleCreated()
	{
		// We initialize OpenGL here, this can only be setup once...
		this->_DCHandle = GetDC(this->_Handle);

		// The recommended pixel format
		PIXELFORMATDESCRIPTOR PixelFormat =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
			PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
			24,                   // Colordepth of the framebuffer.
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,                   // Number of bits for the depthbuffer
			0,                    // Number of bits for the stencilbuffer
			0,                    // Number of Aux buffers in the framebuffer.
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};

		auto PixelFormatIndex = ChoosePixelFormat(this->_DCHandle, &PixelFormat);
		SetPixelFormat(this->_DCHandle, PixelFormatIndex, &PixelFormat);

		this->_GLHandle = wglCreateContext(this->_DCHandle);
		wglMakeCurrent(this->_DCHandle, this->_GLHandle);

		if (!MGL::Mangler::Initalize())
			throw std::exception("Error initializing OpenGL!");
		
		auto Background = this->BackColor();
		glClearColor(Background.GetR() / 255.f, Background.GetG() / 255.f, Background.GetB() / 255.f, Background.GetA() / 255.f);

		// We must call base event last
		Control::OnHandleCreated();
	}

	void OpenGLViewport::OnRender()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Render.RaiseEvent(this);

		SwapBuffers(this->_DCHandle);
	}

	void OpenGLViewport::OnBackColorChanged()
	{
		auto Background = this->BackColor();
		glClearColor(Background.GetR() / 255.f, Background.GetG() / 255.f, Background.GetB() / 255.f, Background.GetA() / 255.f);

		// We must call base event last
		Control::OnBackColorChanged();
	}

	void OpenGLViewport::OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs)
	{
		// Proxy this off, they don't need paint event args...
		OnRender();
	}

	CreateParams OpenGLViewport::GetCreateParams()
	{
		auto Cp = Control::GetCreateParams();
		Cp.ClassName = "OpenGLViewport";
		
		Cp.Style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		Cp.ClassStyle |= CS_VREDRAW | CS_HREDRAW | CS_OWNDC;

		return Cp;
	}
}
