#pragma once

// Runs the ImGui/GLFW desktop application: creates the window and GL context,
// pumps the event loop until the window closes, then tears everything down.
// Returns the process exit code. Defined in gui_main.cpp.
int RunGui();
