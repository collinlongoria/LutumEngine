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
    Lutum::Application app;

    if (!app.Initialize())
        return EXIT_FAILURE;

    app.Run();

    return EXIT_SUCCESS;
}