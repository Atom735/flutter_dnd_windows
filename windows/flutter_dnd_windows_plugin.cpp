#include "include/flutter_dnd_windows/flutter_dnd_windows_plugin.h"

#include <Windows.h>
#include <flutter/flutter_view.h>
#include <flutter/event_channel.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <flutter_windows.h>

#include <malloc.h>
#include <codecvt>
#include <memory>
#include <optional>
#include <sstream>

#define _MY_ENABLE_WNDPROC 0

namespace
{

  using flutter::EncodableList;
  using flutter::EncodableMap;
  using flutter::EncodableValue;

  const char kChannelName[] = "flutter_dnd_windows";

  // helper function to convert 16-bit wstring to utf-8 encoded string
  std::string utf8_encode(const std::wstring &wstr)
  {
    if (wstr.empty())
      return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
  }

  class FlutterDndWindowsPlugin : public flutter::Plugin, public IDropTarget
  {
  public:
    static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

    // Creates a plugin that communicates on the given channel.
    FlutterDndWindowsPlugin(flutter::PluginRegistrarWindows *registrar);

    virtual ~FlutterDndWindowsPlugin();

  protected:
    // IUnknown implementation
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) override;
    ULONG STDMETHODCALLTYPE AddRef(void) override;
    ULONG STDMETHODCALLTYPE Release(void) override;

    // IDropTarget implementation
    HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
    HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
    HRESULT STDMETHODCALLTYPE DragLeave(void) override;
    HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;

    void DnD(LPCSTR type, DWORD grfKeyState, POINTL pt);

  private:
#if (_MY_ENABLE_WNDPROC > 0)
    // Called for top-level WindowProc delegation.
    std::optional<LRESULT> HandleWindowProc(HWND hwnd, UINT message,
                                            WPARAM wparam, LPARAM lparam);
#endif

    // The registrar for this plugin, for accessing the window.
    flutter::PluginRegistrarWindows *registrar_;

    std::unique_ptr<flutter::StreamHandlerError<>> HandleOnListne(
        const flutter::EncodableValue *arguments,
        std::unique_ptr<flutter::EventSink<>> &&events);

    std::unique_ptr<flutter::StreamHandlerError<>> HandleOnCancel(
        const flutter::EncodableValue *arguments);

#if (_MY_ENABLE_WNDPROC > 0)
    // The ID of the WindowProc delegate registration.
    int window_proc_id_ = -1;
#endif
    IDataObject *pDataObject_;

    HWND hwnd_registered_;

    std::unique_ptr<flutter::EventSink<>> event_sink_;
  };

  // static
  void FlutterDndWindowsPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarWindows *registrar)
  {
    auto channel = std::make_unique<flutter::EventChannel<>>(
        registrar->messenger(), kChannelName,
        &flutter::StandardMethodCodec::GetInstance());

    auto plugin = std::make_unique<FlutterDndWindowsPlugin>(registrar);

    auto handler = std::make_unique<flutter::StreamHandlerFunctions<>>(
        [plugin_pointer = plugin.get()](const auto &arguments, auto &&events) {
          return plugin_pointer->HandleOnListne(arguments, std::move(events));
        },
        [plugin_pointer = plugin.get()](const auto &arguments) {
          return plugin_pointer->HandleOnCancel(arguments);
        });

    channel->SetStreamHandler(std::move(handler));

    registrar->AddPlugin(std::move(plugin));
  }

  FlutterDndWindowsPlugin::FlutterDndWindowsPlugin(flutter::PluginRegistrarWindows *registrar)
      : registrar_(registrar)
  {
#if (_MY_ENABLE_WNDPROC > 0)
    window_proc_id_ = registrar_->RegisterTopLevelWindowProcDelegate(
        [this](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
          return HandleWindowProc(hwnd, message, wparam, lparam);
        });
#endif
    auto hwnd = registrar_->GetView()->GetNativeWindow();

    //DnD: Initialize OLE
    OleInitialize(nullptr);

    // DnD: Register Drag & Drop
    if (SUCCEEDED(RegisterDragDrop(hwnd, this)))
    {
      hwnd_registered_ = hwnd;
    }

    // DragAcceptFiles(hwnd, TRUE);
  }

  FlutterDndWindowsPlugin::~FlutterDndWindowsPlugin()
  {
    //DnD: Unregister Drag & Drop
    if (hwnd_registered_)
    {
      RevokeDragDrop(hwnd_registered_);
      hwnd_registered_ = NULL;
    }

    //DnD: Uninitialize OLE
    OleUninitialize();

#if (_MY_ENABLE_WNDPROC > 0)
    registrar_->UnregisterTopLevelWindowProcDelegate(window_proc_id_);

#endif
  }

  //DnD: Implement IUnknown
  HRESULT STDMETHODCALLTYPE
  FlutterDndWindowsPlugin::QueryInterface(REFIID riid, _COM_Outptr_ void **ppvObject)
  {
    return S_OK;
  }

  //DnD: Implement IUnknown
  ULONG STDMETHODCALLTYPE FlutterDndWindowsPlugin::AddRef()
  {
    return 0;
  }

  //DnD: Implement IUnknown
  ULONG STDMETHODCALLTYPE FlutterDndWindowsPlugin::Release()
  {
    return 0;
  }

  //DnD: Implement IDropTarget
  HRESULT STDMETHODCALLTYPE FlutterDndWindowsPlugin::DragEnter(IDataObject *pDataObject, DWORD grfKeyState,
                                                               POINTL pt, DWORD *pdwEffect)
  {
    pDataObject_ = pDataObject;
    DnD("enter", grfKeyState, pt);
    // channel.get()->InvokeMethod("entered", nullptr);
    // TODO: Implement
    return S_OK;
  }

  //DnD: Implement IDropTarget
  HRESULT STDMETHODCALLTYPE FlutterDndWindowsPlugin::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
  {
    DnD("over", grfKeyState, pt);
    // channel.get()->InvokeMethod("updated", nullptr);
    // TODO: Implement
    return S_OK;
  }

  //DnD: Implement IDropTarget
  HRESULT STDMETHODCALLTYPE FlutterDndWindowsPlugin::DragLeave(void)
  {
    if (event_sink_)
    {
      auto map = EncodableMap();
      map[EncodableValue("type")] = EncodableValue("leave");
      event_sink_->Success(EncodableValue(map));
    }
    // TODO: Implement
    // channel.get()->InvokeMethod("exited", nullptr);
    pDataObject_ = nullptr;
    return S_OK;
  }

  //DnD: Implement IDropTarget
  HRESULT STDMETHODCALLTYPE FlutterDndWindowsPlugin::Drop(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
  {
    DnD("drop", grfKeyState, pt);
    pDataObject_ = nullptr;
    return S_OK;
  }

  void FlutterDndWindowsPlugin::DnD(LPCSTR type, DWORD grfKeyState, POINTL pt)
  {

    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT,
                      -1, TYMED_HGLOBAL};
    STGMEDIUM stgm;
    if (event_sink_ && pDataObject_ && SUCCEEDED(pDataObject_->GetData(&fmte, &stgm)))
    {
      HDROP hDropInfo = (HDROP)(stgm.hGlobal);

      // Get the number of files
      UINT nNumOfFiles = DragQueryFileW(hDropInfo, 0xFFFFFFFF, NULL, 0);

      auto list = EncodableList();

      for (UINT nIndex = 0; nIndex < nNumOfFiles; ++nIndex)
      {
        //fetch the length of the path
        UINT cch = DragQueryFileW(hDropInfo, nIndex, NULL, 0);
        LPWSTR sItem = (LPWSTR)(alloca((cch + 1) * sizeof(WCHAR)));
        //fetch the path and store it in 16bit wide char
        DragQueryFileW(hDropInfo, nIndex, sItem, cch + 1);

        std::wstring pathInfoUtf16(sItem, cch);
        std::string pathInfoUtf8 = ::utf8_encode(pathInfoUtf16);
        list.push_back(flutter::EncodableValue(pathInfoUtf8));
      }

      auto map = EncodableMap();
      map[EncodableValue("type")] = EncodableValue(type);
      map[EncodableValue("btns")] = EncodableValue((int32_t)grfKeyState);

      RECT rc;
      GetClientRect(hwnd_registered_, &rc);

      map[EncodableValue("x")] = EncodableValue((int64_t)(pt.x - rc.left));
      map[EncodableValue("y")] = EncodableValue((int64_t)(pt.y - rc.top));
      map[EncodableValue("items")] = EncodableValue(list);

      DragFinish(hDropInfo);

      ReleaseStgMedium(&stgm);

      event_sink_->Success(EncodableValue(map));
    }
  }

#if (_MY_ENABLE_WNDPROC > 0)
  std::optional<LRESULT> FlutterDndWindowsPlugin::HandleWindowProc(HWND hwnd,
                                                                   UINT message,
                                                                   WPARAM wparam,
                                                                   LPARAM lparam)
  {
    std::optional<LRESULT> result;

    if (event_sink_)
    {
      std::ostringstream str;
      str << hwnd << '\t' << message << '\t' << wparam << '\t' << lparam;
      event_sink_->Success(EncodableValue(str.str().c_str()));
      if (message == WM_DROPFILES)
      {
        HDROP hDrop = (HDROP)(wparam);
        POINT pt;
        BOOL pt_in = DragQueryPoint(hDrop, &pt);

        str.clear();
        str << "POINT(" << pt_in << "): " << pt.x << " x " << pt.y;
        event_sink_->Success(EncodableValue(str.str().c_str()));
      }
    }
    return result;
  }
#endif

  std::unique_ptr<flutter::StreamHandlerError<>> FlutterDndWindowsPlugin::HandleOnListne(
      const flutter::EncodableValue *arguments,
      std::unique_ptr<flutter::EventSink<>> &&events)
  {
    if (event_sink_)
    {
      auto error = std::make_unique<flutter::StreamHandlerError<>>(
          "error", "Stream can created only once", nullptr);
      return std::move(error);
    }
    event_sink_ = std::move(events);
    return std::unique_ptr<flutter::StreamHandlerError<>>(nullptr);
  }

  std::unique_ptr<flutter::StreamHandlerError<>> FlutterDndWindowsPlugin::HandleOnCancel(
      const flutter::EncodableValue *arguments)
  {
    return std::unique_ptr<flutter::StreamHandlerError<>>(nullptr);
  }

} // namespace

void FlutterDndWindowsPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar)
{
  FlutterDndWindowsPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
