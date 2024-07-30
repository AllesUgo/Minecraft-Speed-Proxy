#include "BasicUI.h"
#ifdef WIN32
#include <unordered_map>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "Imm32.lib")

static std::unordered_map<HWND, RbsLib::Windows::BasicUI::Window*> RunningWindowsList;

auto CALLBACK RbsLib::Windows::BasicUI::Window::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
{

	auto x = RunningWindowsList.find(hwnd);
	if (x == RunningWindowsList.end())
	{
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	auto self_ref = x->second;
	RECT rc;
	switch (message)
	{
	case WM_MOUSEMOVE:
		self_ref->MouseMove(LOWORD(lParam), HIWORD(lParam), wParam);
		return 0;
	case WM_TIMER:
		self_ref->OnTimer(wParam);
		return 0;
	case WM_PAINT:
		self_ref->PaintWindow();
		return 0;
	case WM_SIZE:
		self_ref->OnWindowSizeChanged();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

void RbsLib::Windows::BasicUI::Window::CreateD2D1Factory(void)
{
	auto hr = D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED,
		&this->d2d1_factory
	);
	if (!SUCCEEDED(hr)) throw BasicUIException("Create factory failed");
}

void RbsLib::Windows::BasicUI::Window::CreateD2D1RenderTarget(void)
{
	RECT rc;
	GetClientRect(this->hwnd, &rc);
	auto hr = this->d2d1_factory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(
			this->hwnd,
			D2D1::SizeU(
				rc.right - rc.left,
				rc.bottom - rc.top)
		),
		&this->d2d1_render_target
	);
	if (!SUCCEEDED(hr)) throw BasicUIException("Create render target failed");
	this->BindedRect = rc;
}

void RbsLib::Windows::BasicUI::Window::ReleaseD2D1RenderTarget(void)
{
	if (this->d2d1_render_target != nullptr)
	{
		this->d2d1_render_target->Release();
		this->d2d1_render_target = nullptr;
	}
}

void RbsLib::Windows::BasicUI::Window::ReleaseD2D1Factory(void)
{
	if (this->d2d1_factory != nullptr)
	{
		this->d2d1_factory->Release();
		this->d2d1_factory = nullptr;
	}
}

bool RbsLib::Windows::BasicUI::Window::IsD2D1RenderTargetInitialized(void) const noexcept
{
	return this->d2d1_render_target;
}

void RbsLib::Windows::BasicUI::Window::_OnPaintHandler(Window& window)
{
	for (auto& x : window.ui_element_list)
	{
		x->Draw(window);
	}
	/*
	ID2D1SolidColorBrush* pBlackBrush = NULL;
	window.d2d1_render_target->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::Black),
		&pBlackBrush
	);
	window.d2d1_render_target->Clear(D2D1::ColorF(0, 255, 255));
	// Create the path geometry.
	ID2D1PathGeometry* m_pPathGeometry = NULL;
	window.d2d1_factory->CreatePathGeometry(&m_pPathGeometry);
	// Write to the path geometry using the geometry sink to create a star.
	ID2D1GeometrySink* pSink = NULL;
	m_pPathGeometry->Open(&pSink);
	pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
	pSink->BeginFigure(D2D1::Point2F(20, 50), D2D1_FIGURE_BEGIN_FILLED);
	pSink->AddLine(D2D1::Point2F(100, 50));
	pSink->AddLine(D2D1::Point2F(100, 100));
	pSink->AddLine(D2D1::Point2F(20, 100));
	pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
	pSink->Close();
	pSink->Release();

	ID2D1Layer* layer = NULL;

	window.d2d1_render_target->CreateLayer(NULL, &layer);
	window.d2d1_render_target->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), m_pPathGeometry), layer);
	window.d2d1_render_target->Clear(D2D1::ColorF(D2D1::ColorF::Yellow));
	window.d2d1_render_target->PopLayer();
	layer->Release();
	m_pPathGeometry->Release();
	pBlackBrush->Release();
	*/
}

void RbsLib::Windows::BasicUI::Window::_OnTimerHandler(Window& window, UINT_PTR id)
{
	for (auto& x : window.ui_element_list)
	{
		x->OnTimer(window,id);
	}
}

void RbsLib::Windows::BasicUI::Window::_OnWindowSizeChangedHandler(Window& window)
{
	for (auto& x : window.ui_element_list)
	{
		x->OnWindowSizeChanged(window);
	}
}

void RbsLib::Windows::BasicUI::Window::_MouseMoveHandler(Window& window, int x, int y, int key_status)
{
	for (auto& element : window.ui_element_list)
	{
		element->MouseMove(window, x, y, key_status);
	}
}


void RbsLib::Windows::BasicUI::Window::PaintWindow(void)
{
	PAINTSTRUCT ps;
	BeginPaint(hwnd, &ps);
	//检查呈现器是否已初始化
	if (!this->IsD2D1RenderTargetInitialized())
	{
		this->CreateD2D1RenderTarget();
	}
	this->d2d1_render_target->BeginDraw();
	//进行绘图操作

	this->OnPaintHandler(*this);

	//完成绘图
	if (SUCCEEDED(this->d2d1_render_target->EndDraw()))
	{
		//绘图成功
	}
	else
	{
		//绘图失败
		this->ReleaseD2D1RenderTarget();
	}
	EndPaint(hwnd, &ps);
}

void RbsLib::Windows::BasicUI::Window::OnWindowSizeChanged(void)
{
	this->WindowSizeChangedHandler(*this);
}

void RbsLib::Windows::BasicUI::Window::OnTimer(UINT_PTR id)
{
	this->OnTimerHandler(*this, id);
}

void RbsLib::Windows::BasicUI::Window::MouseMove(int x, int y, int Key_status)
{
	this->MouseMoveHandler(*this, x, y, Key_status);
}

void RbsLib::Windows::BasicUI::Window::ResetD2D1RenderTarget(void)
{
	this->ReleaseD2D1RenderTarget();
}

void RbsLib::Windows::BasicUI::Window::WindowSize(int x, int y, int width, int height)
{
	::MoveWindow(this->hwnd, x, y, width, height, TRUE);
}

RECT RbsLib::Windows::BasicUI::Window::WindowSize() const
{
	RECT rc;
	GetWindowRect(this->hwnd, &rc);
	return rc;
}

HWND RbsLib::Windows::BasicUI::Window::GetHWND(void)
{
	return this->hwnd;
}


void RbsLib::Windows::BasicUI::Window::AddUIElement(std::shared_ptr<UIElement> ui_element)
{
	this->ui_element_list.push_back(ui_element);
}

void RbsLib::Windows::BasicUI::Window::Draw(void)
{
	//::SendMessage(this->hwnd, WM_PAINT, 0, 0);
	//this->PaintWindow();
	InvalidateRect(
		this->hwnd,
		NULL,
		FALSE
	);
}

const RECT& RbsLib::Windows::BasicUI::Window::GetD2D1RenderTargetRect(void) const
{
	return this->BindedRect;
}

RECT RbsLib::Windows::BasicUI::Window::GetRect(void) const
{
	RECT rc;
	GetClientRect(this->hwnd, &rc);
	return rc;
}

RbsLib::Windows::BasicUI::Window::Window(const std::string& window_name, int width, int heigth, Color color)
	:window_name(window_name)
{
	std::string class_name = window_name + "RBSBasicUI";
	WNDCLASSA wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;				//窗口样式;旗标组合;--水平或垂直改变重绘REDRAW;
	wndclass.lpfnWndProc = this->WindowProc;
	wndclass.cbClsExtra = 0;									//为类对象预留的额外计数字节cb;[=couter byte;]
	wndclass.cbWndExtra = 0;									//为窗体预留的额外计数字节cb;
	wndclass.hInstance = 0;							//App实例句柄;--当前窗口实例的ID代号;由Windows提供;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);		//Icon图标句柄;--普通应用图标;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);			//Cursor光标句柄;--箭头鼠标指针;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	//HBRUSH背景画刷句柄;--选择预设的标准画刷--白色;
	wndclass.lpszMenuName = NULL;									//菜单名;--没有菜单为NULL;
	wndclass.lpszClassName = class_name.c_str();
	if (!RegisterClass(&wndclass))
	{
		throw BasicUIException("RegisterClass failed");
	}
	this->hwnd = CreateWindow(class_name.c_str(),
		window_name.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width, heigth,
		NULL, NULL, NULL, NULL);
	this->OnPaintHandler += this->_OnPaintHandler;
	this->OnTimerHandler += this->_OnTimerHandler;
	this->WindowSizeChangedHandler += this->_OnWindowSizeChangedHandler;
	this->MouseMoveHandler += this->_MouseMoveHandler;
	RunningWindowsList.insert({ this->hwnd,this });
	this->CreateD2D1Factory();
}

RbsLib::Windows::BasicUI::Window::~Window()
{
	RunningWindowsList.erase(this->hwnd);
	DestroyWindow(this->hwnd);
	UnregisterClass((window_name + "RBSBasicUI").c_str(), 0);
	this->ReleaseD2D1RenderTarget();
	this->ReleaseD2D1Factory();
}

void RbsLib::Windows::BasicUI::Window::Show()
{
	
	ShowWindow(this->hwnd, SW_SHOW);
	UpdateWindow(this->hwnd);
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

HWND RbsLib::Windows::BasicUI::Window::GetHWND(void) const noexcept
{
	return this->hwnd;
}

void RbsLib::Windows::BasicUI::Window::SetWindowSize(int width, int height)
{
	SetWindowPos(this->hwnd, 0, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

void RbsLib::Windows::BasicUI::Window::SetTimer(UINT_PTR id, UINT elapse)
{
	//检查是否已经有相同ID的定时器
	for (auto& x : this->timer_list)
	{
		if (x == id)
		{
			throw BasicUIException("Timer id conflict");
		}
	}
	::SetTimer(this->hwnd, id, elapse, nullptr);
	this->timer_list.push_back(id);
}

void RbsLib::Windows::BasicUI::Window::KillTimer(UINT_PTR id)
{
	//检查是否有这个定时器
	if (std::find(this->timer_list.begin(), this->timer_list.end(), id) == this->timer_list.end())
	{
		throw BasicUIException("Timer not found");
	}
	if (::KillTimer(this->hwnd, id))
	{
		this->timer_list.remove(id);
	}
	else
		throw BasicUIException("Kill timer failed");
}

void RbsLib::Windows::BasicUI::Window::ClearTimer(void)
{
	for (auto& x : this->timer_list)
	{
		::KillTimer(this->hwnd, x);
	}
	this->timer_list.clear();
}



const char* RbsLib::Windows::BasicUI::BasicUIException::what(void) const noexcept
{
	return str.c_str();
}

RbsLib::Windows::BasicUI::BasicUIException::BasicUIException(const std::string& str)
	:str(str)
{
}

void RbsLib::Windows::BasicUI::UIElement::OnTimer(Window&,UINT_PTR)
{
}

void RbsLib::Windows::BasicUI::UIElement::OnWindowSizeChanged(Window& window)
{
}

void RbsLib::Windows::BasicUI::UIElement::MouseMove(Window& window, int x, int y, int key_status)
{
}

#endif
