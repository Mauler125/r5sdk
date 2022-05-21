#include "stdafx.h"
#include "Control.h"
#include "DropTarget.h"
#include "DragEventArgs.h"

namespace Forms
{
	DropTarget::DropTarget(Control* Target)
		: IDropTarget(), _Target(Target), _LastObject(nullptr), _LastEffect(DragDropEffects::None)
	{
	}

	HRESULT STDMETHODCALLTYPE DropTarget::DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
	{
		auto EventArgs = std::make_unique<DragEventArgs>(pDataObj, grfKeyState, pt.x, pt.y, (DragDropEffects)*pdwEffect, this->_LastEffect);

		if (this->_Target != nullptr)
		{
			this->_Target->OnDragEnter(EventArgs);
			*pdwEffect = (DWORD)EventArgs->Effect;
		}

		this->_LastObject = pDataObj;
		this->_LastEffect = (DragDropEffects)*pdwEffect;

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
	{
		auto EventArgs = std::make_unique<DragEventArgs>(this->_LastObject, grfKeyState, pt.x, pt.y, (DragDropEffects)*pdwEffect, this->_LastEffect);

		if (this->_Target != nullptr)
		{
			this->_Target->OnDragOver(EventArgs);
			*pdwEffect = (DWORD)EventArgs->Effect;
		}

		this->_LastEffect = (DragDropEffects)*pdwEffect;

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DropTarget::DragLeave()
	{
		if (this->_Target != nullptr)
			this->_Target->OnDragLeave();

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DropTarget::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
	{
		auto EventArgs = std::make_unique<DragEventArgs>(pDataObj, grfKeyState, pt.x, pt.y, (DragDropEffects)*pdwEffect, this->_LastEffect);

		if (this->_Target != nullptr)
		{
			this->_Target->OnDragDrop(EventArgs);
			*pdwEffect = (DWORD)EventArgs->Effect;
		}

		this->_LastObject = nullptr;
		this->_LastEffect = DragDropEffects::None;

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DropTarget::QueryInterface(REFIID riid, void** ppvObject)
	{
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE DropTarget::AddRef(void)
	{
		return NULL;
	}

	ULONG STDMETHODCALLTYPE DropTarget::Release(void)
	{
		return NULL;
	}
}
