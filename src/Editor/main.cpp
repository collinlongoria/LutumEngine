/*
* File: App.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 6/30/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include <cstdlib>

#include "Editor/Application.hpp"

int main(int argc, char ** argv) {
    const char* projectPath = (argc > 1) ? argv[1] : "LutumProject";

    Lutum::Application app;
    if (!app.Initialize(projectPath))
        return EXIT_FAILURE;

    app.Run();

    return EXIT_SUCCESS;
}