/*
* File: FileDialogs.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/10/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/FileDialogs.hpp"

#include <mutex>

#include <SDL3/SDL_dialog.h>

#include "Lutum/Core/Log.hpp"

namespace Lutum::FileDialogs {
namespace {

    struct DialogState {
        std::mutex mutex;
        std::vector<Result> results;
    };

    DialogState& State() {
        static DialogState state;
        return state;
    }

    // Heap context so filter strings outline the native dialog
    struct DialogContext {
        Purpose purpose{};
        std::string filterName;
        std::string filterPattern;
        SDL_DialogFileFilter filter{};
    };

    void DialogCallback(void* userdata, const char* const* filelist, int /*filterIndex*/) {
        auto* ctx = static_cast<DialogContext*>(userdata);

        Result result;
        result.purpose = ctx->purpose;
        if (!filelist)
            LUTUM_ERROR("FileDialogs: dialog error: {}", SDL_GetError());
        else
            for (const char* const* f = filelist; *f; ++f)
                result.paths.emplace_back(*f);

        {
            DialogState& state = State();
            std::lock_guard guard(state.mutex);
            state.results.push_back(std::move(result));
        }
        delete ctx;
    }

    DialogContext* MakeContext(Purpose purpose, const char* name, const char* pattern) {
        auto* ctx = new DialogContext;
        ctx->purpose = purpose;
        ctx->filterName = name ? name : "";
        ctx->filterPattern = pattern ? pattern : "";
        ctx->filter = {ctx->filterName.c_str(), ctx->filterPattern.c_str()};
        return ctx;
    }

} // anonymous namespace

void ShowOpenFile(Purpose purpose, const char* filterName, const char* filterPattern, bool allowMany) {
    DialogContext* ctx = MakeContext(purpose, filterName, filterPattern);
    SDL_ShowOpenFileDialog(DialogCallback, ctx, nullptr, &ctx->filter, 1, nullptr, allowMany);
}

void ShowSaveFile(Purpose purpose, const char* filterName, const char* filterPattern) {
    DialogContext* ctx = MakeContext(purpose, filterName, filterPattern);
    SDL_ShowSaveFileDialog(DialogCallback, ctx, nullptr, &ctx->filter, 1, nullptr);
}

void ShowOpenFolder(Purpose purpose) {
    DialogContext* ctx = MakeContext(purpose, nullptr, nullptr);
    SDL_ShowOpenFolderDialog(DialogCallback, ctx, nullptr, nullptr, false);
}

std::vector<Result> Drain() {
    DialogState& state = State();
    std::lock_guard lock(state.mutex);
    return std::exchange(state.results, {});
}

}