#pragma once
#include <imgui.h>

namespace a3tex {

    class Theme {
        public:

            struct Window_Theme {
                ImVec4 background{0.09f, 0.10f, 0.12f, 1.00f};
                ImVec4 surface   {0.15f, 0.17f, 0.20f, 1.00f};
                ImVec4 accent    {0.40f, 0.82f, 0.88f, 1.00f};
                ImVec4 text      {0.88f, 0.90f, 0.93f, 1.00f};
                ImVec4 muted     {0.55f, 0.60f, 0.66f, 1.00f};
                ImVec4 good      {0.44f, 0.84f, 0.53f, 1.00f};
                ImVec4 bad       {0.93f, 0.45f, 0.40f, 1.00f};
            };


            static const Window_Theme& theme() {
            static constexpr Window_Theme instance;
            return instance;
            };

    };
}

inline ImVec4 mix(const ImVec4& from, const ImVec4& to, const float amount) {
    return {from.x + (to.x - from.x) * amount,
                  from.y + (to.y - from.y) * amount,
                  from.z + (to.z - from.z) * amount,
                  from.w + (to.w - from.w) * amount};
}

inline ImVec4 shade(const ImVec4& colour, const float amount) {
    const ImVec4 white(1.0f, 1.0f, 1.0f, colour.w);
    return amount >= 0.0f ? mix(colour, white, amount)
                          : mix(colour, a3tex::Theme::theme().background, -amount);
}

inline void applyStyle() {
    a3tex::Theme::Window_Theme t = a3tex::Theme::theme();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(16, 14);
    style.FramePadding = ImVec2(10, 6);
    style.ItemSpacing = ImVec2(10, 8);
    style.CellPadding = ImVec2(8, 6);
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 1.0f;
    // The accent reads as text at full strength and as a surface when sunk
    // most of the way into the background.
    const ImVec4 accentFill = shade(t.accent, -0.62f);
    const ImVec4 accentPressed = shade(t.accent, -0.50f);

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text]                 = t.text;
    c[ImGuiCol_TextDisabled]         = t.muted;

    c[ImGuiCol_WindowBg]             = t.background;
    c[ImGuiCol_ChildBg]              = shade(t.background, 0.03f);
    c[ImGuiCol_PopupBg]              = shade(t.background, 0.02f);
    c[ImGuiCol_Border]               = shade(t.surface, 0.08f);

    c[ImGuiCol_FrameBg]              = t.surface;
    c[ImGuiCol_FrameBgHovered]       = shade(t.surface, 0.05f);
    c[ImGuiCol_FrameBgActive]        = shade(t.surface, 0.09f);

    c[ImGuiCol_Button]               = shade(t.surface, 0.03f);
    c[ImGuiCol_ButtonHovered]        = shade(t.surface, 0.10f);
    c[ImGuiCol_ButtonActive]         = accentPressed;

    c[ImGuiCol_Header]               = accentFill;
    c[ImGuiCol_HeaderHovered]        = shade(accentFill, 0.06f);
    c[ImGuiCol_HeaderActive]         = shade(accentFill, 0.12f);

    c[ImGuiCol_Tab]                  = shade(t.background, 0.05f);
    c[ImGuiCol_TabHovered]           = shade(accentFill, 0.10f);
    c[ImGuiCol_TabActive]            = accentFill;
    c[ImGuiCol_TabUnfocused]         = shade(t.background, 0.03f);
    c[ImGuiCol_TabUnfocusedActive]   = shade(accentFill, -0.30f);

    c[ImGuiCol_TitleBg]              = t.background;
    c[ImGuiCol_TitleBgActive]        = shade(t.background, 0.04f);

    c[ImGuiCol_CheckMark]            = t.accent;
    c[ImGuiCol_SliderGrab]           = shade(t.accent, -0.25f);
    c[ImGuiCol_SliderGrabActive]     = t.accent;

    c[ImGuiCol_ScrollbarBg]          = t.background;
    c[ImGuiCol_ScrollbarGrab]        = shade(t.surface, 0.08f);
    c[ImGuiCol_ScrollbarGrabHovered] = shade(t.surface, 0.16f);
    c[ImGuiCol_ScrollbarGrabActive]  = accentPressed;

    c[ImGuiCol_Separator]            = shade(t.surface, 0.06f);
    c[ImGuiCol_PlotHistogram]        = shade(t.accent, -0.20f);
    c[ImGuiCol_PlotLines]            = shade(t.accent, -0.20f);

    c[ImGuiCol_TableHeaderBg]        = shade(t.background, 0.05f);
    c[ImGuiCol_TableBorderLight]     = shade(t.surface, 0.05f);
    c[ImGuiCol_TableBorderStrong]    = shade(t.surface, 0.12f);
    c[ImGuiCol_TableRowBgAlt]        = shade(t.background, 0.02f);
}
