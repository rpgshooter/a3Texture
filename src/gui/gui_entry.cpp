#include "gui_main.h"

// Entry point for the standalone a3texture-gui executable. The CLI binary
// reaches the same code path through RunGui() when started without arguments.
int main(int, char**) {
    return RunGui();
}
