#pragma once

#include <cstdint>
#include <oleidl.h>
#include "DragDropEffects.h"

namespace Forms
{
	// Defined elsewhere
	class Control;

	// Handles drag and drop events for a target control.
	class DropTarget : public IDropTarget
	{
	public:
		DropTarget(Control* Target);
		~DropTarget() = default;

		virtual HRESULT STDMETHODCALLTYPE DragEnter(
			/* [unique][in] */ __RPC__in_opt IDataObject *pDataObj,
			/* [in] */ DWORD grfKeyState,
			/* [in] */ POINTL pt,
			/* [out][in] */ __RPC__inout DWORD *pdwEffect);

		virtual HRESULT STDMETHODCALLTYPE DragOver(
			/* [in] */ DWORD grfKeyState,
			/* [in] */ POINTL pt,
			/* [out][in] */ __RPC__inout DWORD *pdwEffect);

		virtual HRESULT STDMETHODCALLTYPE DragLeave();

		virtual HRESULT STDMETHODCALLTYPE Drop(
			/* [unique][in] */ __RPC__in_opt IDataObject *pDataObj,
			/* [in] */ DWORD grfKeyState,
			/* [in] */ POINTL pt,
			/* [out][in] */ __RPC__inout DWORD *pdwEffect);

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);

		virtual ULONG STDMETHODCALLTYPE AddRef(void);

		virtual ULONG STDMETHODCALLTYPE Release(void);

	private:
		// Internal cached control
		Control* _Target;
		DragDropEffects _LastEffect;
		IDataObject* _LastObject;
	};
}