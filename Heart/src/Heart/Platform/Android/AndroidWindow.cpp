#include "hepch.h"
#include "AndroidWindow.h"

#ifdef HE_PLATFORM_ANDROID

#include "Heart/Input/Input.h"
#include "Heart/Events/KeyEvents.h"
#include "Heart/Events/ButtonEvents.h"
#include "Heart/Events/WindowEvents.h"
#include "Heart/Core/App.h"
#include "Heart/Platform/Android/AndroidApp.h"
#include "Flourish/Api/RenderContext.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_android.h"

namespace Heart
{
    static KeyCode ConvertKeyCode(int code)
    {
        switch (code)
        {
        case AKEYCODE_TAB:
            return KeyCode::Tab;
        case AKEYCODE_DPAD_LEFT:
            return KeyCode::ArrowLeft;
        case AKEYCODE_DPAD_RIGHT:
            return KeyCode::ArrowRight;
        case AKEYCODE_DPAD_UP:
            return KeyCode::ArrowUp;
        case AKEYCODE_DPAD_DOWN:
            return KeyCode::ArrowDown;
        case AKEYCODE_PAGE_UP:
            return KeyCode::PageUp;
        case AKEYCODE_PAGE_DOWN:
            return KeyCode::PageDown;
        case AKEYCODE_MOVE_HOME:
            return KeyCode::Home;
        case AKEYCODE_MOVE_END:
            return KeyCode::End;
        case AKEYCODE_INSERT:
            return KeyCode::Insert;
        case AKEYCODE_FORWARD_DEL:
            return KeyCode::Delete;
        case AKEYCODE_DEL:
            return KeyCode::Backspace;
        case AKEYCODE_SPACE:
            return KeyCode::Space;
        case AKEYCODE_ENTER:
            return KeyCode::Enter;
        case AKEYCODE_BACK:
            return KeyCode::Back;
        case AKEYCODE_ESCAPE:
            return KeyCode::Escape;
        case AKEYCODE_APOSTROPHE:
            return KeyCode::Apostrophe;
        case AKEYCODE_COMMA:
            return KeyCode::Comma;
        case AKEYCODE_MINUS:
            return KeyCode::Minus;
        case AKEYCODE_PERIOD:
            return KeyCode::Period;
        case AKEYCODE_SLASH:
            return KeyCode::Slash;
        case AKEYCODE_SEMICOLON:
            return KeyCode::Semicolon;
        case AKEYCODE_EQUALS:
            return KeyCode::Equals;
        case AKEYCODE_LEFT_BRACKET:
            return KeyCode::LeftBracket;
        case AKEYCODE_BACKSLASH:
            return KeyCode::Backslash;
        case AKEYCODE_RIGHT_BRACKET:
            return KeyCode::RightBracket;
        case AKEYCODE_CAPS_LOCK:
            return KeyCode::CapsLock;
        case AKEYCODE_SCROLL_LOCK:
            return KeyCode::ScrollLock;
        case AKEYCODE_NUM_LOCK:
            return KeyCode::NumLock;
        case AKEYCODE_SYSRQ:
            return KeyCode::PrintScreen;
        case AKEYCODE_BREAK:
            return KeyCode::Pause;
        case AKEYCODE_CTRL_LEFT:
            return KeyCode::LeftCtrl;
        case AKEYCODE_SHIFT_LEFT:
            return KeyCode::LeftShift;
        case AKEYCODE_ALT_LEFT:
            return KeyCode::LeftAlt;
        case AKEYCODE_META_LEFT:
            return KeyCode::LeftSuper;
        case AKEYCODE_CTRL_RIGHT:
            return KeyCode::RightCtrl;
        case AKEYCODE_SHIFT_RIGHT:
            return KeyCode::RightShift;
        case AKEYCODE_ALT_RIGHT:
            return KeyCode::RightAlt;
        case AKEYCODE_META_RIGHT:
            return KeyCode::RightSuper;
        case AKEYCODE_MENU:
            return KeyCode::Menu;
        case AKEYCODE_0:
            return KeyCode::Zero;
        case AKEYCODE_1:
            return KeyCode::One;
        case AKEYCODE_2:
            return KeyCode::Two;
        case AKEYCODE_3:
            return KeyCode::Three;
        case AKEYCODE_4:
            return KeyCode::Four;
        case AKEYCODE_5:
            return KeyCode::Five;
        case AKEYCODE_6:
            return KeyCode::Six;
        case AKEYCODE_7:
            return KeyCode::Seven;
        case AKEYCODE_8:
            return KeyCode::Eight;
        case AKEYCODE_9:
            return KeyCode::Nine;
        case AKEYCODE_A:
            return KeyCode::A;
        case AKEYCODE_B:
            return KeyCode::B;
        case AKEYCODE_C:
            return KeyCode::C;
        case AKEYCODE_D:
            return KeyCode::D;
        case AKEYCODE_E:
            return KeyCode::E;
        case AKEYCODE_F:
            return KeyCode::F;
        case AKEYCODE_G:
            return KeyCode::G;
        case AKEYCODE_H:
            return KeyCode::H;
        case AKEYCODE_I:
            return KeyCode::I;
        case AKEYCODE_J:
            return KeyCode::J;
        case AKEYCODE_K:
            return KeyCode::K;
        case AKEYCODE_L:
            return KeyCode::L;
        case AKEYCODE_M:
            return KeyCode::M;
        case AKEYCODE_N:
            return KeyCode::N;
        case AKEYCODE_O:
            return KeyCode::O;
        case AKEYCODE_P:
            return KeyCode::P;
        case AKEYCODE_Q:
            return KeyCode::Q;
        case AKEYCODE_R:
            return KeyCode::R;
        case AKEYCODE_S:
            return KeyCode::S;
        case AKEYCODE_T:
            return KeyCode::T;
        case AKEYCODE_U:
            return KeyCode::U;
        case AKEYCODE_V:
            return KeyCode::V;
        case AKEYCODE_W:
            return KeyCode::W;
        case AKEYCODE_X:
            return KeyCode::X;
        case AKEYCODE_Y:
            return KeyCode::Y;
        case AKEYCODE_Z:
            return KeyCode::Z;
        case AKEYCODE_F1:
            return KeyCode::F1;
        case AKEYCODE_F2:
            return KeyCode::F2;
        case AKEYCODE_F3:
            return KeyCode::F3;
        case AKEYCODE_F4:
            return KeyCode::F4;
        case AKEYCODE_F5:
            return KeyCode::F5;
        case AKEYCODE_F6:
            return KeyCode::F6;
        case AKEYCODE_F7:
            return KeyCode::F7;
        case AKEYCODE_F8:
            return KeyCode::F8;
        case AKEYCODE_F9:
            return KeyCode::F9;
        case AKEYCODE_F10:
            return KeyCode::F10;
        case AKEYCODE_F11:
            return KeyCode::F11;
        case AKEYCODE_F12:
            return KeyCode::F12;
        default:
            return KeyCode::None;
        }
    }

    int OnInputEvent(struct android_app *app, AInputEvent *inputEvent)
    {
        int eventType = AInputEvent_getType(inputEvent);
        int eventSource = AInputEvent_getSource(inputEvent);

        AndroidWindow& window = (AndroidWindow&)App::Get().GetWindow();

        switch (eventType)
        {
            case AINPUT_EVENT_TYPE_KEY:
            {
                int keyCode = AKeyEvent_getKeyCode(inputEvent);
                int scanCode = AKeyEvent_getScanCode(inputEvent);
                int action = AKeyEvent_getAction(inputEvent);
                int metaState = AKeyEvent_getMetaState(inputEvent);

                switch (action)
                {
                    case AKEY_EVENT_ACTION_DOWN:
                    case AKEY_EVENT_ACTION_UP:
                    {
                        bool pressed = action == AKEY_EVENT_ACTION_DOWN;

                        KeyCode key = ConvertKeyCode(keyCode);
                        Input::AddKeyEvent(key, pressed);

                        if (pressed)
                        {
                            KeyPressedEvent event(key, false);
                            window.Emit(event);
                        }
                        else
                        {
                            KeyReleasedEvent event(key);
                            window.Emit(event);
                        }

                        break;
                    }
                    default:
                        break;
                }

                break;
            }
            case AINPUT_EVENT_TYPE_MOTION:
            {
                int action = AMotionEvent_getAction(inputEvent);
                int pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
                action &= AMOTION_EVENT_ACTION_MASK;

                int toolType = AMotionEvent_getToolType(inputEvent, pointerIndex);

                if (((eventSource & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK) ||
                    ((eventSource & AINPUT_SOURCE_GAMEPAD) == AINPUT_SOURCE_GAMEPAD))
                {
                    Input::AddAxisEvent(AxisCode::GamepadLeftX, AMotionEvent_getAxisValue(inputEvent, AMOTION_EVENT_AXIS_X, 0), true);
                    Input::AddAxisEvent(AxisCode::GamepadLeftY, AMotionEvent_getAxisValue(inputEvent, AMOTION_EVENT_AXIS_Y, 0), true);
                    Input::AddAxisEvent(AxisCode::GamepadRightX, AMotionEvent_getAxisValue(inputEvent, AMOTION_EVENT_AXIS_Z, 0), true);
                    Input::AddAxisEvent(AxisCode::GamepadRightY, AMotionEvent_getAxisValue(inputEvent, AMOTION_EVENT_AXIS_RZ, 0), true);
                    Input::AddAxisEvent(AxisCode::GamepadTriggerLeft, AMotionEvent_getAxisValue(inputEvent, AMOTION_EVENT_AXIS_BRAKE, 0), true);
                    Input::AddAxisEvent(AxisCode::GamepadTriggerRight, AMotionEvent_getAxisValue(inputEvent, AMOTION_EVENT_AXIS_GAS, 0), true);
                }

                switch (action)
                {
                    case AMOTION_EVENT_ACTION_DOWN:
                    case AMOTION_EVENT_ACTION_UP:
                    {
                        if (toolType == AMOTION_EVENT_TOOL_TYPE_FINGER || toolType == AMOTION_EVENT_TOOL_TYPE_UNKNOWN)
                        {
                            bool pressed = action == AMOTION_EVENT_ACTION_DOWN;
                            float xPos = AMotionEvent_getX(inputEvent, pointerIndex);
                            float yPos = AMotionEvent_getY(inputEvent, pointerIndex);
                            Input::AddAxisEvent(AxisCode::MouseX, (double)xPos, false);
                            Input::AddAxisEvent(AxisCode::MouseY, (double)yPos, false);
                            Input::AddButtonEvent(ButtonCode::Button1, pressed);

                            if (pressed)
                            {
					            ButtonPressedEvent event(ButtonCode::Button1);
                                window.Emit(event);
                            }
                            else
                            {
					            ButtonReleasedEvent event(ButtonCode::Button1);
                                window.Emit(event);
                            }
                        }

                        break;
                    }
                    case AMOTION_EVENT_ACTION_BUTTON_PRESS:
                    case AMOTION_EVENT_ACTION_BUTTON_RELEASE:
                    {
                        int buttonState = AMotionEvent_getButtonState(inputEvent);
                        Input::AddButtonEvent(ButtonCode::Button1, buttonState & AMOTION_EVENT_BUTTON_PRIMARY);
                        Input::AddButtonEvent(ButtonCode::Button2, buttonState & AMOTION_EVENT_BUTTON_SECONDARY);
                        Input::AddButtonEvent(ButtonCode::Button3, buttonState & AMOTION_EVENT_BUTTON_TERTIARY);

                        break;
                    }
                    case AMOTION_EVENT_ACTION_HOVER_MOVE:
                    case AMOTION_EVENT_ACTION_MOVE:
                    {
                        float xPos = AMotionEvent_getX(inputEvent, pointerIndex);
                        float yPos = AMotionEvent_getY(inputEvent, pointerIndex);
                        Input::AddAxisEvent(AxisCode::MouseX, (double)xPos, false);
                        Input::AddAxisEvent(AxisCode::MouseY, (double)yPos, false);

                        break;
                    }
                    case AMOTION_EVENT_ACTION_SCROLL:
                    {
                        float horiz = AMotionEvent_getAxisValue(inputEvent, AMOTION_EVENT_AXIS_HSCROLL, pointerIndex);
                        float vert = AMotionEvent_getAxisValue(inputEvent, AMOTION_EVENT_AXIS_VSCROLL, pointerIndex);
                        Input::AddAxisEvent(AxisCode::ScrollX, (double)horiz, true);
                        Input::AddAxisEvent(AxisCode::ScrollY, (double)vert, true);

                        break;
                    }
                    default:
                        break;
                }
            }
            default:
                break;
        }

        return ImGui_ImplAndroid_HandleInputEvent(inputEvent);
    }

    AndroidWindow::AndroidWindow(const WindowCreateInfo &createInfo)
        : Window(createInfo)
    {
        // Wait for native window to be available
        while (!AndroidApp::NativeWindow)
            PollEvents();

        HE_ENGINE_LOG_TRACE("Android native window = {0}", static_cast<void *>(AndroidApp::NativeWindow));

        AndroidApp::App->onInputEvent = OnInputEvent;

        m_Window = AndroidApp::NativeWindow;
        s_WindowCount++;

        RecreateRenderContext();
    }

    AndroidWindow::~AndroidWindow()
    {
    }

    bool AndroidWindow::PollEvents()
    {
        bool recreated = false;

        if (AndroidApp::App->destroyRequested != 0)
        {
            App::Get().Close();
            return recreated;
        }

        if (AndroidApp::NativeWindow && AndroidApp::NativeWindow != m_Window)
        {
            // Window has been recreated, so we need to remake the render context

            HE_ENGINE_LOG_WARN("Detected android window recreation");

            m_Window = AndroidApp::NativeWindow;
            recreated = true;

            RecreateRenderContext();
        }

        while (true)
        {
            // Poll and process the Android OS system events.
            struct android_poll_source *source = nullptr;
            int events = 0;
            int timeoutMilliseconds = AndroidApp::Paused ? -1 : 0;
            if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, (void **)&source) >= 0)
            {
                if (source != nullptr)
                    source->process(AndroidApp::App, source);

                continue;
            }

            break;
        }

        return recreated;
    }

    void AndroidWindow::EndFrame()
    {
        Flourish::Context::PushFrameRenderContext(m_RenderContext.get());
    }

    void AndroidWindow::DisableCursor()
    {
    }

    void AndroidWindow::EnableCursor()
    {
    }

    void AndroidWindow::SetWindowTitle(const char *newTitle)
    {
        m_WindowData.Title = newTitle;
    }

    void AndroidWindow::SetFullscreen(bool fullscreen)
    {
    }

    void AndroidWindow::ToggleFullscreen()
    {
    }

    bool AndroidWindow::IsFullscreen() const
    {
        return false;
    }

    void AndroidWindow::RecreateRenderContext()
    {
        m_RenderContext = nullptr;

        Flourish::RenderContextCreateInfo ctxCreateInfo;
        ctxCreateInfo.Window = (ANativeWindow *)m_Window;
        ctxCreateInfo.Width = m_WindowData.Width;
        ctxCreateInfo.Height = m_WindowData.Height;
        m_RenderContext = Flourish::RenderContext::Create(ctxCreateInfo);
    }
}

#endif
