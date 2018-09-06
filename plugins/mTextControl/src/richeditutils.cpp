#include "stdafx.h"

class CREOleCallback : public IRichEditOleCallback
{
private:
	unsigned refCount;
	IStorage *pictStg;
	int nextStgId;
public:
	CREOleCallback()
	{
		refCount = 1;
		pictStg = nullptr;
		nextStgId = 0;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID * ppvObj)
	{
		if (IsEqualIID(riid, IID_IRichEditOleCallback)) {
			*ppvObj = this;
			this->AddRef();
			return S_OK;
		}
		*ppvObj = nullptr;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef()
	{
		if (this->refCount == 0) {
			if (S_OK != StgCreateDocfile(nullptr, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_DELETEONRELEASE, 0, &this->pictStg))
				this->pictStg = nullptr;
			this->nextStgId = 0;
		}
		return ++this->refCount;
	}

	ULONG STDMETHODCALLTYPE Release()
	{
		if (--this->refCount == 0) {
			if (this->pictStg)
				this->pictStg->Release();
		}
		return this->refCount;
	}

	HRESULT STDMETHODCALLTYPE  ContextSensitiveHelp(BOOL)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE  DeleteObject(LPOLEOBJECT)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE  GetClipboardData(CHARRANGE *, DWORD, LPDATAOBJECT *)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE  GetContextMenu(WORD, LPOLEOBJECT, CHARRANGE *, HMENU *)
	{
		return E_INVALIDARG;
	}

	HRESULT STDMETHODCALLTYPE  GetDragDropEffect(BOOL, DWORD, LPDWORD)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE  GetInPlaceContext(LPOLEINPLACEFRAME *, LPOLEINPLACEUIWINDOW *, LPOLEINPLACEFRAMEINFO)
	{
		return E_INVALIDARG;
	}

	HRESULT STDMETHODCALLTYPE  GetNewStorage(LPSTORAGE * lplpstg)
	{
		wchar_t sztName[64];
		mir_snwprintf(sztName, L"s%u", this->nextStgId);
		if (this->pictStg == nullptr)
			return STG_E_MEDIUMFULL;

		return this->pictStg->CreateStorage(sztName, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, 0, lplpstg);

	}

	HRESULT STDMETHODCALLTYPE  QueryAcceptData(LPDATAOBJECT, CLIPFORMAT *, DWORD, BOOL, HGLOBAL)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE  QueryInsertObject(LPCLSID, LPSTORAGE, LONG)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE  ShowContainerUI(BOOL)
	{
		return S_OK;
	}
};

IRichEditOleCallback *reOleCallback = nullptr;

void InitRichEdit(ITextServices *ts)
{
	LRESULT lResult;
	ts->TxSendMessage(EM_SETOLECALLBACK, 0, (LPARAM)reOleCallback, &lResult);
}

LRESULT CALLBACK RichEditProxyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ITextServices *ts = (ITextServices *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (ts && (msg != WM_DESTROY)) {
		LRESULT lResult;
		ts->TxSendMessage(msg, wParam, lParam, &lResult);
		return lResult;
	}
	return 1;
}

void LoadRichEdit()
{
	reOleCallback = new CREOleCallback;

	WNDCLASSEX wcl;
	wcl.cbSize = sizeof(wcl);
	wcl.lpfnWndProc = RichEditProxyWndProc;
	wcl.style = CS_GLOBALCLASS;
	wcl.cbClsExtra = 0;
	wcl.cbWndExtra = 0;
	wcl.hInstance = g_plugin.getInst();
	wcl.hIcon = nullptr;
	wcl.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcl.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wcl.lpszMenuName = nullptr;
	wcl.lpszClassName = L"NBRichEditProxyWndClass";
	wcl.hIconSm = nullptr;
	RegisterClassEx(&wcl);
}

void UnloadRichEdit()
{
	delete reOleCallback;
}

HWND CreateProxyWindow(ITextServices *ts)
{
	HWND hwnd = CreateWindow(L"NBRichEditProxyWndClass", L"", 0, 0, 0, 0, 0, nullptr, nullptr, g_plugin.getInst(), nullptr);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ts);
	return hwnd;
}
