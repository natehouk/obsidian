// Obsidian — plugin entry point (the class is in ObsidianPlugin.hpp, which the UI also includes). License: MIT.
#include "ObsidianPlugin.hpp"

START_NAMESPACE_DISTRHO

Plugin* createPlugin() { return new ObsidianPlugin(); }

END_NAMESPACE_DISTRHO
