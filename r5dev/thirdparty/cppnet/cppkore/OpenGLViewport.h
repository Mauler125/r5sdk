#pragma once

#include <cstdint>
#include "Control.h"

namespace Forms
{
	// Represents an OpenGL viewport control.
	class OpenGLViewport : public Control
	{
	public:
		OpenGLViewport();
		virtual ~OpenGLViewport();

		// Gets the aspect ratio of this viewport.
		float AspectRatio();

		// Redraws the frame.
		void Redraw();

		// We must define base events here
		virtual void OnHandleCreated();
		virtual void OnRender();
		virtual void OnBackColorChanged();
		virtual void OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs);

		// We must define event handlers here
		EventBase<void(*)(Control*)> Render;

	protected:
		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();

		// Internal cached flags
		HDC _DCHandle;
		HGLRC _GLHandle;
	};
}