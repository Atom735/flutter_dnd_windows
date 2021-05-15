#include <flutter/dart_project.h>
#include <flutter/flutter_view_controller.h>
#include <windows.h>

#include "flutter_window.h"
#include "run_loop.h"
#include "utils.h"

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev,
                      _In_ wchar_t *command_line, _In_ int show_command)
{
  // Attach to console when present (e.g., 'flutter run') or create a
  // new console when running with a debugger.
  if (!::AttachConsole(ATTACH_PARENT_PROCESS) && ::IsDebuggerPresent())
  {
    CreateAndAttachConsole();
  }

  // Initialize COM, so that it is available for use in the library and/or
  // plugins.
  ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  // Ничего не происходит при инициализации
  RunLoop run_loop;
  // При инициализации устанавливаются переменные путей и всё...
  // Там только строки строкам присваиваются
  flutter::DartProject project(L"data");
  // Это WinAPI функция
  std::vector<std::string> command_line_arguments =
      GetCommandLineArguments();
  // Там просто перемещается вектор аргументов к дарт проекту
  project.set_dart_entrypoint_arguments(std::move(command_line_arguments));
  // Связывает луп и проект с окном, просто указатели копирует, ничего более
  FlutterWindow window(&run_loop, project);
  Win32Window::Point origin(10, 10);
  Win32Window::Size size(1280, 720);
  // Создаёт и отображает окно Windows, там только WinAPI функции вызываются
  // Так же там создаётся класс окна, если он не был создан
  if (!window.CreateAndShow(L"flutter_dnd_windows_example", origin, size))
  {
    return EXIT_FAILURE;
  }
  // Устанавливает флаг на выход из лупа, если окно закроется
  window.SetQuitOnClose(true);
  // Выполняется луп
  // При создании рамки окна, FlutterWindow закрепляется за HWND этого окна
  // В остальных случаях вызвает HanldeMessage, в котором происходит
  // Реакция на смену размеров окна, активацию окна и смену dpi
  // Если нет системных сообщений то отрабатывает [ProcessFlutterMessages]
  // Который отрабатывает события всех зарегегстрированных движков Flutter в
  // данном лупе
  run_loop.Run();

  ::CoUninitialize();
  return EXIT_SUCCESS;
}
