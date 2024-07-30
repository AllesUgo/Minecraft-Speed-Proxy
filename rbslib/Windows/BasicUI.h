#pragma once
#include "../BaseType.h"
#ifdef WIN32
#include <string>
#include <Windows.h>
#include <exception>
#include <d2d1.h>
#include <list>
#include "../Function.h"
#include <memory>
namespace RbsLib::Windows::BasicUI
{
	class Window;
	class UIElement;
	struct Point
	{
		int x;
		int y;
	};
	struct BoxSize
	{
		int width;
		int height;
	};
	class BasicUIException :public std::exception
	{
		std::string str;
	public:
		const char* what(void)const noexcept override;
		BasicUIException(const std::string& str);
	};
	enum class Color
	{
		NONE = 0,
		WHITE = 1,
		BLACK = 2
	};
	class Window/*若要为窗口过程添加Handle，请继承Window并在构造函数中添加Handle*/
	{
	private:
		HWND hwnd;
		std::list<UINT_PTR> timer_list;
		std::list<std::shared_ptr<UIElement>> ui_element_list;
		RECT BindedRect;
		
		Window(HWND hwnd)noexcept;

		static auto CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT;

		void CreateD2D1Factory(void);
		void CreateD2D1RenderTarget(void);
		void ReleaseD2D1RenderTarget(void);
		void ReleaseD2D1Factory(void);
		bool IsD2D1RenderTargetInitialized(void) const noexcept;

		//默认的消息处理函数
		static void _OnPaintHandler(Window& window);
		static void _OnTimerHandler(Window& window, UINT_PTR id);
		static void _OnWindowSizeChangedHandler(Window& window);
		static void _MouseMoveHandler(Window& window, int x, int y, int key_status);
		//消息循环处理函数
		void PaintWindow(void);
		void OnWindowSizeChanged(void);
		void OnTimer(UINT_PTR id);
		void MouseMove(int x, int y,int Key_status);
		
		
	public:
		ID2D1Factory* d2d1_factory = nullptr;
		ID2D1HwndRenderTarget* d2d1_render_target = nullptr;
		std::string window_name;

		//回调列表
		RbsLib::Function::Function<void(Window&)> WindowSizeChangedHandler;
		RbsLib::Function::Function<void(Window&)> OnPaintHandler;
		RbsLib::Function::Function<void(Window&, UINT_PTR)> OnTimerHandler;
		RbsLib::Function::Function<void(Window&, int x, int y, int key_status)> MouseMoveHandler;

		Window(const std::string& window_name, int width, int heigth, Color color);
		Window(const Window&) = delete;
		~Window();
		void Show();
		HWND GetHWND(void) const noexcept;
		void SetWindowSize(int width, int height);
		void SetTimer(UINT_PTR id, UINT elapse);
		void KillTimer(UINT_PTR id);
		void ClearTimer(void);
		void AddUIElement(std::shared_ptr<UIElement> ui_element);
		void Draw(void);
		const RECT& GetD2D1RenderTargetRect(void) const;
		RECT GetRect(void) const;
		void ResetD2D1RenderTarget(void);
		void WindowSize(int x,int y,int width, int height);
		RECT WindowSize()const;
		HWND GetHWND(void);
	};
	class UIElement
	{
	protected:
		int id = 0;
	public:
		virtual void Draw(Window& window) = 0;
		virtual void OnTimer(Window& window,UINT_PTR timer_id);
		virtual void OnWindowSizeChanged(Window& window);
		virtual void MouseMove(Window& window, int x, int y, int key_status);
	};
	class Button :public UIElement
	{
	private:
		Point point;
		BoxSize box_size;
		std::string text;
	public:
		Button(const Point& point, const BoxSize& box_size, const std::string& text);
		void Draw(Window&window) override;
		void Text(const std::string& text);
		const std::string& Text(void) const;
		void Move(const Point& point);
		void Size(const BoxSize& box_size);
		const BoxSize& Size(void) const;
		struct Events
		{
			RbsLib::Function::Function<void(Button&)> OnClick;
		};
	};
}
#endif