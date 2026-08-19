#include "../include/paa.h"
#include "../include/image_loader.h"
#include "../include/channel_packer.h"
#include "../include/texture_role.h"
#include "../include/p3d_reader.h"
#include "../include/viewer_3d.h"
#include "../include/model_renderer.h"
#include "../include/rvmat_writer.h"
#include "../include/rvmat_shaders.h"
#include "../include/rvmat_parser.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <portable-file-dialogs.h>

#include <vector>
#include <string>
#include <filesystem>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <fstream>
#include <memory>
#include <map>
#include <functional>

namespace fs = std::filesystem;

namespace {

const char* kImageFilter = "*.png *.tga *.jpg *.jpeg *.tif *.tiff";

bool isSupportedImage(const std::string& path) {
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".png" || ext == ".tga" || ext == ".jpg" ||
           ext == ".jpeg" || ext == ".tif" || ext == ".tiff";
}

const char* formatLabel(arma3::PAAFormat format) {
    switch (format) {
        case arma3::PAAFormat::DXT1: return "DXT1";
        case arma3::PAAFormat::DXT5: return "DXT5";
        default: return "auto";
    }
}

GLuint uploadTexture(const uint8_t* rgba, int width, int height) {
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, rgba);
    return id;
}

arma3::ImageData downscaleTo(const arma3::ImageData& src, uint32_t maxSide) {
    if (src.width <= maxSide && src.height <= maxSide) {
        return src;
    }

    const double scale = double(maxSide) / std::max(src.width, src.height);
    arma3::ImageData out;
    out.width = std::max(1u, uint32_t(src.width * scale));
    out.height = std::max(1u, uint32_t(src.height * scale));
    out.data.resize(size_t(out.width) * out.height * 4);

    for (uint32_t y = 0; y < out.height; y++) {
        const uint32_t sy = std::min<uint32_t>(src.height - 1, uint32_t(y / scale));
        for (uint32_t x = 0; x < out.width; x++) {
            const uint32_t sx = std::min<uint32_t>(src.width - 1, uint32_t(x / scale));
            for (int c = 0; c < 4; c++) {
                out.data[(size_t(y) * out.width + x) * 4 + c] =
                    src.data[(size_t(sy) * src.width + sx) * 4 + c];
            }
        }
    }
    return out;
}

// One palette, with every widget colour derived from it, so changing the
// accent is one edit rather than twenty.
struct Theme {
    ImVec4 background{0.09f, 0.10f, 0.12f, 1.00f};
    ImVec4 surface   {0.15f, 0.17f, 0.20f, 1.00f};
    ImVec4 accent    {0.40f, 0.82f, 0.88f, 1.00f};
    ImVec4 text      {0.88f, 0.90f, 0.93f, 1.00f};
    ImVec4 muted     {0.55f, 0.60f, 0.66f, 1.00f};
    ImVec4 good      {0.44f, 0.84f, 0.53f, 1.00f};
    ImVec4 bad       {0.93f, 0.45f, 0.40f, 1.00f};
};

const Theme& theme() {
    static const Theme instance;
    return instance;
}

ImVec4 mix(const ImVec4& from, const ImVec4& to, float amount) {
    return ImVec4(from.x + (to.x - from.x) * amount,
                  from.y + (to.y - from.y) * amount,
                  from.z + (to.z - from.z) * amount,
                  from.w + (to.w - from.w) * amount);
}

// Positive lifts toward white, negative sinks toward the background.
ImVec4 shade(const ImVec4& colour, float amount) {
    const ImVec4 white(1.0f, 1.0f, 1.0f, colour.w);
    return amount >= 0.0f ? mix(colour, white, amount)
                          : mix(colour, theme().background, -amount);
}

void applyStyle() {
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

    const Theme& t = theme();

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

// Named for meaning rather than colour, all taken from the palette.
const ImVec4 kAccent = theme().accent;
const ImVec4 kOk = theme().good;
const ImVec4 kBad = theme().bad;
const ImVec4 kDim = theme().muted;

inline std::string resolveModelTexture(const std::string& reference,
                                       const std::string& modelDir,
                                       const std::string& driveRoot) {
    std::string relative = reference;
    std::replace(relative.begin(), relative.end(), '\\', '/');
    if (relative.empty() || relative.front() == '#') return {};

    const fs::path asPaa = fs::path(relative).replace_extension(".paa");
    const std::string leaf = asPaa.filename().string();

    std::vector<fs::path> candidates;
    if (!driveRoot.empty()) {
        candidates.push_back(fs::path(driveRoot) / asPaa);
        candidates.push_back(fs::path(driveRoot) / relative);
    }
    if (!modelDir.empty()) {
        candidates.push_back(fs::path(modelDir) / asPaa);
        candidates.push_back(fs::path(modelDir) / leaf);
        candidates.push_back(fs::path(modelDir) / "data" / leaf);
        candidates.push_back(fs::path(modelDir).parent_path() / "data" / leaf);
    }

    std::error_code ec;
    for (const auto& candidate : candidates) {
        if (fs::exists(candidate, ec) && fs::is_regular_file(candidate, ec)) {
            return candidate.string();
        }
    }
    return {};
}

struct TexturePreview {
    GLuint textures[5] = {0, 0, 0, 0, 0};
    uint32_t width = 0;
    uint32_t height = 0;

    void release() {
        for (GLuint& id : textures) {
            if (id) glDeleteTextures(1, &id);
            id = 0;
        }
        width = height = 0;
    }

    void build(const std::vector<uint8_t>& rgba, uint32_t w, uint32_t h) {
        release();
        if (rgba.size() < size_t(w) * h * 4) return;

        arma3::ImageData full;
        full.width = w;
        full.height = h;
        full.data = rgba;

        const arma3::ImageData scaled = downscaleTo(full, 1024);
        width = scaled.width;
        height = scaled.height;

        textures[0] = uploadTexture(scaled.data.data(), scaled.width, scaled.height);

        for (int c = 0; c < 4; c++) {
            std::vector<uint8_t> grey(scaled.data.size());
            for (size_t i = 0; i < grey.size() / 4; i++) {
                const uint8_t v = scaled.data[i * 4 + c];
                grey[i * 4 + 0] = v;
                grey[i * 4 + 1] = v;
                grey[i * 4 + 2] = v;
                grey[i * 4 + 3] = 255;
            }
            textures[c + 1] = uploadTexture(grey.data(), scaled.width, scaled.height);
        }
    }

    bool valid() const { return textures[0] != 0; }
};

// The shader lists run to 150 entries, so the combo is filtered by typing.
inline bool shaderCombo(const char* label, std::string& value,
                        const std::vector<arma3::ShaderInfo>& options, char* filter,
                        size_t filterSize) {
    bool changed = false;
    if (ImGui::BeginCombo(label, value.c_str())) {
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##filter", "type to narrow", filter, filterSize);

        std::string needle = filter;
        std::transform(needle.begin(), needle.end(), needle.begin(), ::tolower);

        for (const auto& option : options) {
            std::string name = option.name;
            std::string lowered = name;
            std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::tolower);
            if (!needle.empty() && lowered.find(needle) == std::string::npos) continue;

            if (ImGui::Selectable(option.name, value == option.name)) {
                value = option.name;
                changed = true;
            }
            if (ImGui::IsItemHovered() && option.description[0]) {
                ImGui::SetTooltip("%s", option.description);
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

// Draws the channel switcher and the image. Returns the chosen channel.
inline void drawChannelSwitcher(int& mode) {
    const char* labels[5] = {"All", "R", "G", "B", "A"};
    for (int i = 0; i < 5; i++) {
        if (i) ImGui::SameLine();
        const bool active = mode == i;
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.44f, 0.49f, 1.0f));
        if (ImGui::Button(labels[i], ImVec2(52, 0))) mode = i;
        if (active) ImGui::PopStyleColor();
    }
}

} // namespace

struct ConversionJob {
    std::string name;
    arma3::SwizzleType swizzle = arma3::SwizzleType::NONE;
    bool completed = false;
    bool success = false;
    std::string errorMessage;
    int64_t durationMs = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct PackSlot {
    std::string path;
    int channel = 0;
    bool invert = false;
};

class PAAConverterApp {
public:
    // The dialogs run in a helper process. Waiting on one stops the render
    // loop, and the compositor then offers to kill the window as hung, so they
    // are polled instead.
    void pumpDialogs() {
        if (pendingFiles && pendingFiles->ready(0)) {
            auto files = pendingFiles->result();
            auto handler = onFiles;
            pendingFiles.reset();
            onFiles = nullptr;
            if (handler) handler(files);
        }
        if (pendingFolder && pendingFolder->ready(0)) {
            auto folder = pendingFolder->result();
            auto handler = onFolder;
            pendingFolder.reset();
            onFolder = nullptr;
            if (handler && !folder.empty()) handler(folder);
        }
        if (pendingSave && pendingSave->ready(0)) {
            auto file = pendingSave->result();
            auto handler = onSave;
            pendingSave.reset();
            onSave = nullptr;
            if (handler && !file.empty()) handler(file);
        }
    }

    bool dialogBusy() const {
        return pendingFiles || pendingFolder || pendingSave;
    }

    void askForFiles(const std::string& title, const std::vector<std::string>& filter,
                     bool multi, std::function<void(std::vector<std::string>)> handler) {
        if (dialogBusy()) return;
        pendingFiles = std::make_unique<pfd::open_file>(
            title, "", filter, multi ? pfd::opt::multiselect : pfd::opt::none);
        onFiles = std::move(handler);
    }

    void askForFolder(const std::string& title, std::function<void(std::string)> handler) {
        if (dialogBusy()) return;
        pendingFolder = std::make_unique<pfd::select_folder>(title, "");
        onFolder = std::move(handler);
    }

    void askForSavePath(const std::string& title, const std::vector<std::string>& filter,
                        std::function<void(std::string)> handler) {
        if (dialogBusy()) return;
        pendingSave = std::make_unique<pfd::save_file>(title, "", filter);
        onSave = std::move(handler);
    }

    void render() {
        pumpDialogs();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        ImGui::Begin("a3Texture", nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        if (ImGui::BeginTabBar("tabs")) {
            if (ImGui::BeginTabItem("Convert")) {
                activeTab = 0;
                renderConvert();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Pack channels")) {
                activeTab = 1;
                renderPack();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("View PAA")) {
                activeTab = 2;
                renderViewer();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Model")) {
                activeTab = 3;
                renderModel();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Material")) {
                activeTab = 4;
                renderMaterial();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    void handleFileDrop(const std::vector<std::string>& files) {
        int accepted = 0;
        std::vector<std::string> rejected;

        for (const auto& file : files) {
            std::string ext = fs::path(file).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".paa") {
                accepted++;
                openPaa(file);
                continue;
            }

            if (ext == ".p3d") {
                accepted++;
                openModel(file);
                continue;
            }

            if (!isSupportedImage(file)) {
                rejected.push_back(fs::path(file).filename().string());
                continue;
            }
            accepted++;

            if (activeTab == 1) {
                const arma3::PackPreset* preset = currentPreset();
                for (int i = 0; preset && i < preset->sourceCount; i++) {
                    if (packSlots[i].path.empty()) {
                        packSlots[i].path = file;
                        break;
                    }
                }
            } else {
                inputFiles.push_back(arma3::describeSource(file));
            }
        }

        dropMessage.clear();
        dropRejected = false;

        if (!rejected.empty()) {
            dropRejected = true;
            dropMessage = "Ignored " + std::to_string(rejected.size()) +
                          " file(s) that are not images: " + rejected.front();
            if (rejected.size() > 1) dropMessage += ", ...";
        } else if (accepted) {
            dropMessage = "Added " + std::to_string(accepted) + " file(s)";
        }
    }

    void releaseTextures() {
        for (GLuint& id : previewTextures) {
            if (id) glDeleteTextures(1, &id);
            id = 0;
        }
        viewPreview.release();
    }

private:
    // ---------------------------------------------------------------- convert

    void renderConvert() {
        ImGui::Spacing();
        ImGui::TextColored(kDim,
            "Drop images anywhere on this window, or add them below. "
            "PNG, TGA, JPG and TIFF.");
        if (!dropMessage.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(dropRejected ? kBad : kOk, "  %s", dropMessage.c_str());
        }
        ImGui::Spacing();

        if (ImGui::Button("Add files...")) addFiles();
        ImGui::SameLine();
        if (ImGui::Button("Remove all")) inputFiles.clear();
        ImGui::SameLine();
        ImGui::TextColored(kDim, "%zu queued", inputFiles.size());

        ImGui::Spacing();
        renderQueue();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetNextItemWidth(200);
        ImGui::Combo("Format", &selectedFormat,
                     "Auto (from texture type)\0DXT1\0DXT5\0");
        ImGui::SameLine(0, 20);
        ImGui::SetNextItemWidth(140);
        ImGui::Combo("Quality", &selectedQuality, "Fast\0Normal\0High\0");

        ImGui::SetNextItemWidth(-160);
        ImGui::InputTextWithHint("##outdir", "Same folder as each input",
                                 outputDir, sizeof(outputDir));
        ImGui::SameLine();
        if (ImGui::Button("Output folder...")) {
            askForFolder("Select output folder", [this](std::string folder) {
                snprintf(outputDir, sizeof(outputDir), "%s", folder.c_str());
            });
        }

        ImGui::Spacing();

        const bool busy = converting.load();
        const bool canConvert = !busy && !arma3::planOutputs(inputFiles).empty();

        if (!canConvert) ImGui::BeginDisabled();
        if (ImGui::Button(busy ? "Converting..." : "Convert", ImVec2(160, 38))) {
            startConversion();
        }
        if (!canConvert) ImGui::EndDisabled();

        if (busy) {
            ImGui::SameLine(0, 16);
            const int done = completedJobs.load();
            const int total = totalJobs.load();
            ImGui::SetNextItemWidth(-1);
            ImGui::ProgressBar(total ? float(done) / total : 0.0f, ImVec2(-1, 24));
        }

        renderResults();
    }

    void renderQueue() {
        if (inputFiles.empty()) {
            ImGui::BeginChild("empty", ImVec2(0, 110), true);
            ImGui::Spacing();
            ImGui::TextColored(kDim, "   Nothing queued yet.");
            ImGui::EndChild();
            return;
        }

        const auto options = arma3::roleOptions();

        ImGui::BeginChild("queue", ImVec2(0, 190), true);
        if (ImGui::BeginTable("files", 5,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("What it is", ImGuiTableColumnFlags_WidthFixed, 190);
            ImGui::TableSetupColumn("Invert", ImGuiTableColumnFlags_WidthFixed, 55);
            ImGui::TableSetupColumn("Group", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 30);
            ImGui::TableHeadersRow();

            int removeIndex = -1;
            for (size_t i = 0; i < inputFiles.size(); i++) {
                ImGui::TableNextRow();
                ImGui::PushID(int(i));

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(
                    fs::path(inputFiles[i].path).filename().string().c_str());
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", inputFiles[i].path.c_str());
                }

                ImGui::TableNextColumn();
                int current = 0;
                for (size_t option = 0; option < options.size(); option++) {
                    if (options[option].role == inputFiles[i].role) current = int(option);
                }

                std::string items;
                for (const auto& option : options) {
                    items += option.label;
                    items.push_back('\0');
                }
                items.push_back('\0');

                ImGui::SetNextItemWidth(-1);
                if (ImGui::Combo("##role", &current, items.c_str())) {
                    inputFiles[i].role = options[current].role;
                }

                ImGui::TableNextColumn();
                const bool packed = inputFiles[i].role == arma3::TextureRole::ArmaMap ||
                                    inputFiles[i].role == arma3::TextureRole::Ignore;
                if (packed) {
                    ImGui::TextColored(kDim, " -");
                } else {
                    ImGui::Checkbox("##inv", &inputFiles[i].invert);
                }

                ImGui::TableNextColumn();
                if (inputFiles[i].role == arma3::TextureRole::Ignore) {
                    ImGui::TextColored(kDim, " -");
                } else {
                    ImGui::SetNextItemWidth(-1);
                    ImGui::InputInt("##grp", &inputFiles[i].group, 0);
                    if (inputFiles[i].group < 0) inputFiles[i].group = 0;
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip(
                            "0 groups by filename.\n"
                            "Give rows the same number to pack them together\n"
                            "when their names do not match.");
                    }
                }

                ImGui::TableNextColumn();
                if (ImGui::SmallButton("x")) removeIndex = int(i);

                ImGui::PopID();
            }

            if (removeIndex >= 0) inputFiles.erase(inputFiles.begin() + removeIndex);
            ImGui::EndTable();
        }
        ImGui::EndChild();

        renderPlan();
    }

    void renderPlan() {
        const auto outputs = arma3::planOutputs(inputFiles);

        ImGui::Spacing();
        if (outputs.empty()) {
            ImGui::TextColored(kDim, "Nothing to write yet.");
            return;
        }

        ImGui::TextColored(kAccent, "Will write %zu texture%s", outputs.size(),
                           outputs.size() == 1 ? "" : "s");

        ImGui::BeginChild("plan", ImVec2(0, 130), true);
        for (const auto& output : outputs) {
            ImGui::TextColored(kAccent, "%s", output.name.c_str());
            ImGui::SameLine();
            ImGui::TextColored(kDim, "  %s",
                formatLabel(arma3::PAA::swizzleFormat(output.swizzle)));
            if (!output.note.empty()) {
                ImGui::SameLine();
                ImGui::TextColored(kBad, "  %s", output.note.c_str());
            }
            for (const auto& source : output.sources) {
                ImGui::TextColored(kDim, "        %s",
                                   fs::path(source).filename().string().c_str());
            }
        }
        ImGui::EndChild();
    }

    void renderResults() {
        std::lock_guard<std::mutex> lock(jobsMutex);
        if (jobs.empty()) return;

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const int ok = successCount.load();
        const int bad = failCount.load();
        ImGui::TextColored(kOk, "%d succeeded", ok);
        if (bad) {
            ImGui::SameLine();
            ImGui::TextColored(kBad, " %d failed", bad);
        }
        ImGui::SameLine();
        if (!converting.load() && ImGui::SmallButton("Clear")) {
            jobs.clear();
            successCount = 0;
            failCount = 0;
            return;
        }

        ImGui::BeginChild("results", ImVec2(0, 150), true);
        if (ImGui::BeginTable("res", 4,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 70);
            ImGui::TableSetupColumn("Result", ImGuiTableColumnFlags_WidthFixed, 90);
            ImGui::TableHeadersRow();

            for (const auto& job : jobs) {
                if (!job.completed) continue;
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(job.name.c_str());

                ImGui::TableNextColumn();
                ImGui::TextColored(kDim, "%ux%u", job.width, job.height);

                ImGui::TableNextColumn();
                ImGui::TextColored(kDim, "%lldms", (long long)job.durationMs);

                ImGui::TableNextColumn();
                if (job.success) {
                    ImGui::TextColored(kOk, "ok");
                } else {
                    ImGui::TextColored(kBad, "failed");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", job.errorMessage.c_str());
                    }
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
    }

    // ------------------------------------------------------------------- pack

    const arma3::PackPreset* currentPreset() const {
        const auto names = arma3::ChannelPacker::presetNames();
        if (packPreset < 0 || packPreset >= int(names.size())) return nullptr;
        return arma3::ChannelPacker::findPreset(names[packPreset]);
    }

    void renderPack() {
        const auto names = arma3::ChannelPacker::presetNames();

        std::string items;
        for (const auto& name : names) {
            items += "_" + name;
            items.push_back('\0');
        }
        items.push_back('\0');

        ImGui::Spacing();
        ImGui::SetNextItemWidth(200);
        if (ImGui::Combo("Texture type", &packPreset, items.c_str())) {
            clearPreview();
            packStatus.clear();
        }

        const arma3::PackPreset* preset = currentPreset();
        if (!preset) return;

        ImGui::SameLine(0, 20);
        ImGui::TextColored(kDim, "writes %s, swizzle applied on save",
            formatLabel(arma3::PAA::swizzleFormat(preset->swizzle)));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool multiSlot[4] = {false, false, false, false};
        for (int i = 0; i < preset->sourceCount; i++) {
            int used = 0;
            for (const auto& slot : preset->slots) {
                if (slot.source == i) used++;
            }
            multiSlot[i] = used > 1;
        }

        for (int i = 0; i < preset->sourceCount; i++) {
            ImGui::PushID(i);

            ImGui::TextColored(kAccent, "%s", preset->sourceLabels[i]);

            ImGui::SetNextItemWidth(-310);
            char buffer[512];
            snprintf(buffer, sizeof(buffer), "%s", packSlots[i].path.c_str());
            if (ImGui::InputTextWithHint("##path", "Drop an image here, or browse",
                                         buffer, sizeof(buffer))) {
                packSlots[i].path = buffer;
            }

            ImGui::SameLine();
            if (ImGui::Button("Browse...")) {
                askForFiles("Select image", {"Images", kImageFilter, "All files", "*"},
                            false, [this, i](std::vector<std::string> files) {
                    if (!files.empty()) packSlots[i].path = files[0];
                });
            }

            ImGui::SameLine();
            if (multiSlot[i]) {
                // All of this image's channels are used, so there is nothing to pick.
                ImGui::TextColored(kDim, " RGB ");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("This map is used whole, not one channel of it");
                }
            } else {
                ImGui::SetNextItemWidth(70);
                ImGui::Combo("##ch", &packSlots[i].channel, "R\0G\0B\0A\0");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Which channel to read from this image");
                }
            }

            ImGui::SameLine();
            ImGui::Checkbox("Invert", &packSlots[i].invert);

            ImGui::Spacing();
            ImGui::PopID();
        }

        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetNextItemWidth(-130);
        ImGui::InputTextWithHint("##packout", "Output .paa", packOutput,
                                 sizeof(packOutput));
        ImGui::SameLine();
        if (ImGui::Button("Save as...")) {
            askForSavePath("Save packed texture", {"PAA", "*.paa"}, [this](std::string file) {
                snprintf(packOutput, sizeof(packOutput), "%s", file.c_str());
            });
        }

        ImGui::Spacing();
        ImGui::Checkbox("Override resolution", &overrideSize);
        if (overrideSize) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90);
            ImGui::InputInt("w", &overrideWidth, 0);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90);
            ImGui::InputInt("h", &overrideHeight, 0);
        } else {
            ImGui::SameLine();
            ImGui::TextColored(kDim, "using the largest source");
        }

        ImGui::Spacing();

        bool ready = true;
        for (int i = 0; i < preset->sourceCount; i++) {
            if (packSlots[i].path.empty()) ready = false;
        }

        if (!ready) ImGui::BeginDisabled();
        if (ImGui::Button("Preview", ImVec2(130, 34))) doPack(false);
        ImGui::SameLine();
        if (!ready) ImGui::EndDisabled();

        const bool canSave = ready && packOutput[0] != '\0';
        if (!canSave) ImGui::BeginDisabled();
        if (ImGui::Button("Pack and save", ImVec2(150, 34))) doPack(true);
        if (!canSave) ImGui::EndDisabled();

        if (!packStatus.empty()) {
            ImGui::SameLine(0, 16);
            ImGui::TextColored(packFailed ? kBad : kOk, "%s", packStatus.c_str());
        }

        renderPreview();
    }

    void renderPreview() {
        if (!previewTextures[0]) return;

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(kDim,
            "A packed map is unreadable as a picture. Check the channels one at a time.");
        ImGui::Spacing();

        const char* labels[5] = {"All", "R", "G", "B", "A"};
        for (int i = 0; i < 5; i++) {
            if (i) ImGui::SameLine();
            const bool active = previewMode == i;
            if (active) ImGui::PushStyleColor(ImGuiCol_Button, shade(theme().accent, -0.62f));
            if (ImGui::Button(labels[i], ImVec2(52, 0))) previewMode = i;
            if (active) ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        const float side = std::min(320.0f, ImGui::GetContentRegionAvail().x);
        const float aspect = previewHeight ? float(previewHeight) / previewWidth : 1.0f;
        ImGui::Image((ImTextureID)(intptr_t)previewTextures[previewMode],
                     ImVec2(side, side * aspect));

        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextColored(kDim, "%ux%u", packedWidth, packedHeight);
        ImGui::Spacing();
        ImGui::TextColored(kDim, "Stored layout after swizzle:");
        for (int i = 0; i < 4; i++) {
            ImGui::TextColored(kDim, "  %s", storedSummary[i].c_str());
        }
        ImGui::EndGroup();
    }

    void doPack(bool save) {
        const arma3::PackPreset* preset = currentPreset();
        if (!preset) return;

        packStatus.clear();
        packFailed = false;

        try {
            arma3::ChannelPacker packer(*preset);

            for (int i = 0; i < preset->sourceCount; i++) {
                packer.setSource(i, arma3::ImageLoader::load(packSlots[i].path));
                if (packer.sourceSlotCount(i) == 1) {
                    packer.setSourceChannel(
                        i, static_cast<arma3::PackChannel>(packSlots[i].channel));
                }
                packer.setSourceInvert(i, packSlots[i].invert);
            }

            if (overrideSize && overrideWidth > 0 && overrideHeight > 0) {
                packer.setTargetSize(uint32_t(overrideWidth), uint32_t(overrideHeight));
            }

            arma3::ImageData packed = packer.pack();
            packedWidth = packed.width;
            packedHeight = packed.height;

            arma3::PAA paa;
            paa.setQuality(static_cast<arma3::Quality>(selectedQuality));
            paa.setImage(packed);
            paa.setSwizzle(preset->swizzle);

            if (save) {
                paa.writePAA(packOutput);
                packStatus = "Saved " + fs::path(packOutput).filename().string();
            } else {
                packStatus = "Preview only, not written";
            }

            // Preview the stored result, so the channels shown are what ships.
            buildPreview(paa.getPackedPixelData(0), packed.width, packed.height);
            describeStored(preset->swizzle);
        }
        catch (const std::exception& e) {
            packFailed = true;
            packStatus = e.what();
            clearPreview();
        }
    }

    void describeStored(arma3::SwizzleType type) {
        static const char* kNames[4] = {"R", "G", "B", "A"};
        const char* sources[4] = {"?", "?", "?", "?"};

        switch (type) {
            case arma3::SwizzleType::NOHQ:
                sources[0] = "0"; sources[1] = "normal Y";
                sources[2] = "normal Z"; sources[3] = "normal X, inverted";
                break;
            case arma3::SwizzleType::SMDI:
                sources[0] = "255"; sources[1] = "metallic / specular";
                sources[2] = "roughness"; sources[3] = "255";
                break;
            case arma3::SwizzleType::AS:
                sources[0] = "255"; sources[1] = "ambient occlusion";
                sources[2] = "255"; sources[3] = "255";
                break;
            case arma3::SwizzleType::DT:
                sources[0] = "detail"; sources[1] = "detail";
                sources[2] = "detail"; sources[3] = "255";
                break;
            default:
                break;
        }

        for (int i = 0; i < 4; i++) {
            storedSummary[i] = std::string(kNames[i]) + " = " + sources[i];
        }
    }

    void buildPreview(const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height) {
        clearPreview();
        if (rgba.size() < size_t(width) * height * 4) return;

        arma3::ImageData full;
        full.width = width;
        full.height = height;
        full.data = rgba;

        const arma3::ImageData scaled = downscaleTo(full, 512);
        previewWidth = scaled.width;
        previewHeight = scaled.height;

        previewTextures[0] = uploadTexture(scaled.data.data(), scaled.width, scaled.height);

        for (int c = 0; c < 4; c++) {
            std::vector<uint8_t> grey(scaled.data.size());
            for (size_t i = 0; i < grey.size() / 4; i++) {
                const uint8_t v = scaled.data[i * 4 + c];
                grey[i * 4 + 0] = v;
                grey[i * 4 + 1] = v;
                grey[i * 4 + 2] = v;
                grey[i * 4 + 3] = 255;
            }
            previewTextures[c + 1] = uploadTexture(grey.data(), scaled.width, scaled.height);
        }
    }

    void clearPreview() {
        releaseTextures();
        previewMode = 0;
    }


    // ----------------------------------------------------------------- viewer

    void openPaa(const std::string& path) {
        viewError.clear();
        viewMips.clear();
        viewTaggs.clear();
        viewPreview.release();
        viewLevel = 0;

        try {
            arma3::PAA paa(path);
            paa.readPAA();

            viewPath = path;
            viewFormat = paa.getFormat();
            viewSwizzle = paa.getSwizzle();

            for (const auto& mip : paa.getMipMaps()) {
                viewMips.push_back({mip.width, mip.height, mip.data});
            }
            for (const auto& tagg : paa.getTaggs()) {
                viewTaggs.push_back({tagg.signature, tagg.data});
            }

            if (viewMips.empty()) {
                viewError = "No mipmaps in file";
                return;
            }
            rebuildViewPreview();
        }
        catch (const std::exception& e) {
            viewError = e.what();
            viewPath = path;
        }
    }

    void rebuildViewPreview() {
        if (viewLevel < 0 || viewLevel >= int(viewMips.size())) return;
        const auto& mip = viewMips[viewLevel];
        viewPreview.build(mip.data, mip.width, mip.height);
    }

    void renderViewer() {
        ImGui::Spacing();
        ImGui::TextColored(kDim, "Open a .paa to inspect it, or drop one on the window.");
        ImGui::Spacing();

        if (ImGui::Button("Open .paa...")) {
            askForFiles("Open PAA", {"Arma texture", "*.paa", "All files", "*"},
                        false, [this](std::vector<std::string> files) {
                if (!files.empty()) openPaa(files[0]);
            });
        }

        if (!viewPath.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(kDim, "%s", fs::path(viewPath).filename().string().c_str());
        }

        if (!viewError.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(kBad, "%s", viewError.c_str());
            return;
        }

        if (viewMips.empty()) return;

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetNextItemWidth(230);
        if (ImGui::SliderInt("Mip level", &viewLevel, 0, int(viewMips.size()) - 1)) {
            rebuildViewPreview();
        }
        ImGui::SameLine(0, 20);
        ImGui::TextColored(kDim, "%ux%u of %zu levels",
                           viewMips[viewLevel].width, viewMips[viewLevel].height,
                           viewMips.size());

        ImGui::SameLine(0, 20);
        if (ImGui::Button("Export PNG...")) exportViewPng();

        ImGui::Spacing();
        drawChannelSwitcher(viewMode);
        ImGui::Spacing();

        if (viewPreview.valid()) {
            const float avail = ImGui::GetContentRegionAvail().x;
            const float side = std::min(430.0f, avail - 300.0f);
            const float aspect = viewPreview.width
                ? float(viewPreview.height) / viewPreview.width : 1.0f;
            ImGui::Image((ImTextureID)(intptr_t)viewPreview.textures[viewMode],
                         ImVec2(side, side * aspect));
            ImGui::SameLine();
        }

        ImGui::BeginGroup();
        ImGui::TextColored(kAccent, "%s", formatLabel(viewFormat));
        ImGui::SameLine();
        if (viewSwizzle == arma3::SwizzleType::NONE) {
            ImGui::TextColored(kDim, "  no swizzle tagg");
        } else {
            ImGui::TextColored(kDim, "  _%s", arma3::PAA::swizzleName(viewSwizzle));
        }

        ImGui::Spacing();
        ImGui::TextColored(kDim, "Taggs");
        for (const auto& tagg : viewTaggs) {
            std::string label = tagg.signature;
            std::string detail;

            if (tagg.signature == "GGATCGVA" && tagg.data.size() == 4) {
                label = "AVGCOLOR";
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "b=%u g=%u r=%u a=%u",
                         tagg.data[0], tagg.data[1], tagg.data[2], tagg.data[3]);
                detail = buffer;
            } else if (tagg.signature == "GGATCXAM") {
                label = "MAXCOLOR";
            } else if (tagg.signature == "GGATGALF") {
                label = "FLAGTRANSP";
            } else if (tagg.signature == "GGATSFFO") {
                label = "OFFSETS";
                detail = std::to_string(tagg.data.size() / 4) + " entries";
            } else if (tagg.signature == "GGATZIWS") {
                label = "SWIZZLE";
                char buffer[64] = {0};
                for (size_t i = 0; i < tagg.data.size() && i < 4; i++) {
                    snprintf(buffer + i * 3, sizeof(buffer) - i * 3, "%02X ", tagg.data[i]);
                }
                detail = buffer;
            }

            ImGui::TextColored(kDim, "  %-11s %s", label.c_str(), detail.c_str());
        }
        ImGui::EndGroup();
    }

    void exportViewPng() {
        if (viewLevel < 0 || viewLevel >= int(viewMips.size())) return;

        askForSavePath("Export PNG", {"PNG", "*.png"}, [this](std::string file) {
            if (fs::path(file).extension().empty()) file += ".png";
            writeViewPng(file);
        });
    }

    void writeViewPng(const std::string& file) {
        try {
            const auto& mip = viewMips[viewLevel];
            arma3::ImageData image;
            image.width = mip.width;
            image.height = mip.height;
            image.data = mip.data;
            arma3::ImageLoader::savePNG(file, image);
            viewError.clear();
        }
        catch (const std::exception& e) {
            viewError = e.what();
        }
    }


    // ------------------------------------------------------------------ model

    void openModel(const std::string& path) {
        modelError.clear();
        modelInfo = arma3::ReadP3DInfo(path.c_str());
        modelPath = path;
        modelLod = 0;

        if (!modelInfo.valid) {
            modelError = "Could not read the model";
            for (const auto& warning : modelInfo.warnings) modelError += "\n" + warning;
            return;
        }

        // Prefer the first LOD that actually has geometry.
        for (size_t i = 0; i < modelInfo.lods.size(); i++) {
            if (!modelInfo.lods[i].faces.empty()) {
                modelLod = int(i);
                break;
            }
        }
        uploadModelLod();
    }

    void uploadModelLod() {
        if (!renderer.IsInitialized()) {
            if (!renderer.Initialize()) {
                modelError = "Could not start the 3D renderer";
                return;
            }
        }

        if (modelLod < 0 || modelLod >= int(modelInfo.lods.size())) return;

        const arma3::P3DLOD& lod = modelInfo.lods[modelLod];
        if (lod.faces.empty()) {
            renderer.ClearMesh();
            modelError = modelInfo.type == "ODOL"
                ? "Binarized model: it carries no editable geometry, so there is "
                  "nothing to draw. Metadata and textures are still listed."
                : "This LOD has no faces";
            return;
        }

        modelError.clear();

        // Proxies are real geometry and dominate the bounds, so framing a
        // character puts it in the distance unless they are left out.
        std::vector<std::string> hidden;
        if (hideProxies) hidden = modelInfo.proxySelections;

        std::map<std::string, int> textureMap;

        arma3::Mesh mesh;
        if (!arma3::ConvertP3DToMesh(lod, mesh, hidden, "", &textureMap)) {
            modelError = "Could not build a mesh from this LOD";
            return;
        }

        loadModelTextures(textureMap, mesh);

        arma3::RendererMesh renderMesh;
        renderMesh.vertices.reserve(mesh.vertices.size());
        for (const auto& v : mesh.vertices) {
            renderMesh.vertices.push_back({v.x, v.y, v.z, v.nx, v.ny, v.nz,
                                           v.u, v.v, v.highlight, v.texIndex});
        }
        for (int i = 0; i < 3; i++) {
            renderMesh.boundsMin[i] = mesh.boundsMin[i];
            renderMesh.boundsMax[i] = mesh.boundsMax[i];
            renderMesh.center[i] = mesh.center[i];
        }
        renderMesh.size = std::max({mesh.boundsMax[0] - mesh.boundsMin[0],
                                    mesh.boundsMax[1] - mesh.boundsMin[1],
                                    mesh.boundsMax[2] - mesh.boundsMin[2]});
        renderMesh.valid = !renderMesh.vertices.empty();

        renderer.LoadMesh(renderMesh);
        renderer.FrameObject();
    }

    // The renderer only takes PAA, so an override in any other format is put
    // through this project's own conversion first. That also shows what the
    // conversion actually produces, on the model.
    std::string paaForPreview(const std::string& source) {
        std::string ext = fs::path(source).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".paa") return source;

        try {
            const std::string stem = fs::path(source).stem().string();
            const fs::path temp = fs::temp_directory_path() /
                ("a3paa_preview_" + std::to_string(std::hash<std::string>{}(source)) +
                 "_" + stem + ".paa");

            arma3::PAA paa;
            paa.setQuality(arma3::Quality::Fast);
            paa.setImage(arma3::ImageLoader::load(source));
            paa.setSwizzle(arma3::PAA::swizzleFromFilename(source));
            paa.writePAA(temp.string());
            return temp.string();
        }
        catch (const std::exception&) {
            return {};
        }
    }

    // Adds a slot per referenced texture and repoints the mesh at the slots
    // that actually loaded, since a failed one is not added at all.
    void loadModelTextures(const std::map<std::string, int>& textureMap, arma3::Mesh& mesh) {
        renderer.ClearTextureSlots();
        missingTextures.clear();
        textureRefs.clear();
        loadedTextures = 0;

        const std::string modelDir = fs::path(modelPath).parent_path().string();

        std::vector<std::string> byIndex(textureMap.size());
        for (const auto& [reference, index] : textureMap) {
            if (index >= 0 && index < int(byIndex.size())) byIndex[index] = reference;
        }
        textureRefs = byIndex;

        std::vector<int> remap(byIndex.size(), -1);
        for (size_t i = 0; i < byIndex.size(); i++) {
            if (byIndex[i].empty()) continue;

            std::string source;
            const auto override = textureOverrides.find(byIndex[i]);
            if (override != textureOverrides.end()) {
                source = paaForPreview(override->second);
            } else {
                source = resolveModelTexture(byIndex[i], modelDir, textureRoot);
            }

            if (source.empty()) {
                missingTextures.push_back(byIndex[i]);
                continue;
            }

            const int slot = renderer.AddTextureSlotWithMaterial(source);
            if (slot < 0) {
                missingTextures.push_back(byIndex[i]);
                continue;
            }
            remap[i] = slot;
            loadedTextures++;
        }

        for (auto& vertex : mesh.vertices) {
            const int original = int(vertex.texIndex);
            vertex.texIndex = (original >= 0 && original < int(remap.size()))
                ? float(remap[original])
                : -1.0f;
        }
    }

    void renderModel() {
        ImGui::Spacing();
        ImGui::TextColored(kDim, "Open a .p3d, or drop one on the window.");
        ImGui::Spacing();

        if (ImGui::Button("Open .p3d...")) {
            askForFiles("Open model", {"Arma model", "*.p3d", "All files", "*"},
                        false, [this](std::vector<std::string> files) {
                if (!files.empty()) openModel(files[0]);
            });
        }

        if (!modelPath.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(kDim, "%s  %s v%u",
                fs::path(modelPath).filename().string().c_str(),
                modelInfo.type.c_str(), modelInfo.version);
        }

        if (modelInfo.valid && !modelInfo.lods.empty()) {
            ImGui::Spacing();
            std::string items;
            for (const auto& lod : modelInfo.lods) {
                items += arma3::GetLODTypeName(lod.resolution) +
                         " (" + std::to_string(lod.faces.size()) + " faces)";
                items.push_back('\0');
            }
            items.push_back('\0');

            ImGui::SetNextItemWidth(280);
            if (ImGui::Combo("LOD", &modelLod, items.c_str())) uploadModelLod();

            ImGui::SameLine(0, 20);
            if (ImGui::Checkbox("Hide proxies", &hideProxies)) uploadModelLod();
            if (ImGui::IsItemHovered() && !modelInfo.proxySelections.empty()) {
                ImGui::SetTooltip("%zu proxy selection(s) in this model",
                                  modelInfo.proxySelections.size());
            }

            ImGui::SameLine(0, 20);
            ImGui::TextColored(kDim, "%d vertices, %d faces total",
                               modelInfo.totalVertices, modelInfo.totalFaces);
        }

        ImGui::Spacing();
        ImGui::SetNextItemWidth(-260);
        if (ImGui::InputTextWithHint("##root", "P drive root, for a3\\... texture paths",
                                     textureRoot, sizeof(textureRoot))) {
        }
        ImGui::SameLine();
        if (ImGui::Button("Browse root...")) {
            askForFolder("Select P drive root", [this](std::string folder) {
                snprintf(textureRoot, sizeof(textureRoot), "%s", folder.c_str());
                uploadModelLod();
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload")) uploadModelLod();

        if (!modelError.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(kBad, "%s", modelError.c_str());
        }

        ImGui::Spacing();

        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const ImVec2 size(std::max(320.0f, avail.x - 260.0f),
                          std::max(240.0f, avail.y - 20.0f));

        if (renderer.IsInitialized() && renderer.HasMesh()) {
            renderer.SetViewportSize(int(size.x), int(size.y));
            renderer.Render();

            ImGui::Image((ImTextureID)(intptr_t)renderer.GetOutputTexture(), size,
                         ImVec2(0, 1), ImVec2(1, 0));

            if (ImGui::IsItemHovered()) {
                const ImGuiIO& io = ImGui::GetIO();
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    renderer.RotateArcball(io.MouseDelta.x, io.MouseDelta.y,
                                           size.x, size.y);
                }
                if (io.MouseWheel != 0.0f) {
                    renderer.AdjustCameraDistance(-io.MouseWheel * 0.4f);
                }
            }

            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::TextColored(kDim, "Drag to rotate, scroll to zoom");
            if (ImGui::Button("Frame")) renderer.FrameObject();
            ImGui::Spacing();
            ImGui::TextColored(kDim, "Textures (%d of %zu loaded)",
                               loadedTextures, textureRefs.size());
            ImGui::Spacing();

            for (size_t i = 0; i < textureRefs.size(); i++) {
                if (textureRefs[i].empty()) continue;
                ImGui::PushID(int(i));

                const bool overridden = textureOverrides.count(textureRefs[i]) > 0;
                const bool missing = std::find(missingTextures.begin(), missingTextures.end(),
                                               textureRefs[i]) != missingTextures.end();

                ImGui::TextColored(missing ? kBad : (overridden ? kAccent : kDim), "%s",
                    fs::path(textureRefs[i]).filename().string().c_str());
                if (ImGui::IsItemHovered()) {
                    std::string tip = textureRefs[i];
                    if (overridden) tip += "\nshowing: " + textureOverrides[textureRefs[i]];
                    if (missing) tip += "\nnot found";
                    ImGui::SetTooltip("%s", tip.c_str());
                }

                if (ImGui::SmallButton("Replace")) {
                    const std::string reference = textureRefs[i];
                    askForFiles("Use this texture instead",
                                {"Textures", "*.paa *.png *.tga *.tif *.tiff *.jpg",
                                 "All files", "*"},
                                false, [this, reference](std::vector<std::string> files) {
                        if (files.empty()) return;
                        textureOverrides[reference] = files[0];
                        uploadModelLod();
                    });
                }
                if (overridden) {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Revert")) {
                        textureOverrides.erase(textureRefs[i]);
                        uploadModelLod();
                    }
                }

                ImGui::Spacing();
                ImGui::PopID();
            }

            if (!textureOverrides.empty()) {
                if (ImGui::SmallButton("Revert all")) {
                    textureOverrides.clear();
                    uploadModelLod();
                }
            }
            ImGui::EndGroup();
        } else if (modelInfo.valid && modelError.empty()) {
            ImGui::TextColored(kDim, "Pick a LOD with geometry.");
        }
    }


    // --------------------------------------------------------------- material

    void rebuildMaterial() {
        if (materialSource.empty()) return;
        material = arma3::materialForTextureSet(materialSource, textureRoot);
        applyMaterialTemplate();
    }

    void applyMaterialTemplate() {
        const auto names = arma3::rvmatTemplates();
        if (materialTemplate < 0 || materialTemplate >= int(names.size())) return;

        const arma3::RvmatMaterial preset = arma3::rvmatTemplate(names[materialTemplate]);
        material.ambient = preset.ambient;
        material.diffuse = preset.diffuse;
        material.forcedDiffuse = preset.forcedDiffuse;
        material.emissive = preset.emissive;
        material.specular = preset.specular;
        material.specularPower = preset.specularPower;
        material.pixelShaderID = preset.pixelShaderID;
        material.vertexShaderID = preset.vertexShaderID;
    }

    void renderMaterial() {
        ImGui::Spacing();
        ImGui::TextColored(kDim,
            "Start empty, build one from a texture set, or open an existing "
            "material.");
        ImGui::Spacing();

        if (ImGui::Button("New material")) {
            material = arma3::blankMaterial();
            materialSource.clear();
            materialPath.clear();
            materialActive = true;
            materialStatus = "Every stage on a placeholder; point them at textures as you go";
        }

        ImGui::SameLine();
        if (ImGui::Button("From a texture set...")) {
            askForFiles("Pick any texture from the set",
                        {"Textures", "*.paa *.png *.tga *.tif *.tiff", "All files", "*"},
                        false, [this](std::vector<std::string> files) {
                if (files.empty()) return;
                materialSource = files[0];
                materialPath.clear();
                materialActive = true;
                rebuildMaterial();
            });
        }

        ImGui::SameLine();
        if (ImGui::Button("Open .rvmat...")) {
            askForFiles("Open material", {"RVMAT", "*.rvmat", "All files", "*"},
                        false, [this](std::vector<std::string> files) {
                if (!files.empty()) openRvmat(files[0]);
            });
        }

        if (!materialPath.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(kAccent, "%s",
                fs::path(materialPath).filename().string().c_str());
        } else if (!materialSource.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(kDim, "from %s",
                fs::path(materialSource).filename().string().c_str());
        }

        if (!materialActive) return;

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Negative item widths measure against the content region, so the two
        // columns have to be real children or the editor eats the preview.
        const float total = ImGui::GetContentRegionAvail().x;
        const float previewWidth = std::max(340.0f, total * 0.42f);
        const float editorWidth = std::max(360.0f, total - previewWidth - 12.0f);

        ImGui::BeginChild("materialEditor", ImVec2(editorWidth, 0), false);

        std::string items;
        for (const auto& type : arma3::materialTypes()) {
            items += type.name;
            items.push_back('\0');
        }
        items.push_back('\0');

        ImGui::SetNextItemWidth(300);
        if (ImGui::Combo("Material type", &materialTemplate, items.c_str())) {
            const auto& types = arma3::materialTypes();
            if (materialTemplate >= 0 && materialTemplate < int(types.size())) {
                material.vertexShaderID = types[materialTemplate].vertexShader;
                material.pixelShaderID = types[materialTemplate].pixelShader;
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Sets the shader pair this material type uses");
        }

        ImGui::Spacing();
        ImGui::SetNextItemWidth(300);
        ImGui::ColorEdit4("Ambient", material.ambient.data(), ImGuiColorEditFlags_Float);
        ImGui::SetNextItemWidth(300);
        ImGui::ColorEdit4("Diffuse", material.diffuse.data(), ImGuiColorEditFlags_Float);
        ImGui::SetNextItemWidth(300);
        ImGui::ColorEdit4("Specular", material.specular.data(), ImGuiColorEditFlags_Float);

        // Emissive runs well past 1 on glowing surfaces, so it is typed rather
        // than picked.
        ImGui::SetNextItemWidth(260);
        ImGui::InputFloat4("Emissive", material.emissive.data());
        ImGui::SetNextItemWidth(260);
        ImGui::InputFloat4("Forced diffuse", material.forcedDiffuse.data());

        ImGui::SetNextItemWidth(260);
        ImGui::DragFloat("Specular power", &material.specularPower, 1.0f, 1.0f, 1000.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Rough 10-50, normal 50-150, shiny 150-500, mirror 500+");
        }

        ImGui::SetNextItemWidth(300);
        shaderCombo("Pixel shader", material.pixelShaderID, arma3::pixelShaders(),
                    pixelFilter, sizeof(pixelFilter));
        ImGui::SetNextItemWidth(300);
        shaderCombo("Vertex shader", material.vertexShaderID, arma3::vertexShaders(),
                    vertexFilter, sizeof(vertexFilter));

        ImGui::Spacing();
        ImGui::TextColored(kDim, "Stages");
        for (auto& [index, stage] : material.stages) {
            ImGui::PushID(index);
            char buffer[512];
            snprintf(buffer, sizeof(buffer), "%s", stage.texture.c_str());

            const bool procedural = !stage.texture.empty() && stage.texture.front() == '#';
            ImGui::TextColored(procedural ? kDim : kAccent, "%s", stageName(index));
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stage%d", index);

            ImGui::SameLine(120);
            ImGui::SetNextItemWidth(-110);
            if (ImGui::InputText("##tex", buffer, sizeof(buffer))) {
                stage.texture = buffer;
            }

            ImGui::SameLine();
            if (ImGui::SmallButton("Browse")) {
                const int slot = index;
                askForFiles("Pick a texture for this stage",
                            {"Textures", "*.paa", "All files", "*"}, false,
                            [this, slot](std::vector<std::string> files) {
                    if (files.empty()) return;
                    material.stages[slot].texture =
                        arma3::enginePath(files[0], textureRoot);
                });
            }
            ImGui::PopID();
        }

        ImGui::Spacing();
        if (ImGui::Button("Save .rvmat...", ImVec2(150, 32))) {
            askForSavePath("Save material", {"RVMAT", "*.rvmat"},
                           [this](std::string file) {
                if (fs::path(file).extension().empty()) file += ".rvmat";
                std::ofstream out(file, std::ios::binary);
                if (out) {
                    out << arma3::writeRvmat(material);
                    materialStatus = "Saved " + fs::path(file).filename().string();
                } else {
                    materialStatus = "Could not write " + file;
                }
            });
        }

        if (renderer.HasMesh()) {
            ImGui::SameLine();
            if (ImGui::Button("Apply to model", ImVec2(150, 32))) applyMaterialToModel(false);
            ImGui::SameLine();
            ImGui::Checkbox("Live", &materialLive);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Push edits to the model as you make them");
            }
        }

        // Edits above have already been made this frame, so this shows them
        // on the next draw of the model rather than a frame later.
        if (materialLive && renderer.HasMesh()) {
            applyMaterialToModel(true);
        }

        if (!materialStatus.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(kOk, "%s", materialStatus.c_str());
        }

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("materialPreview", ImVec2(0, 0), false);

        // Editing here while the model sits on another tab means never seeing
        // the change, so the model is drawn alongside when one is loaded.
        if (renderer.IsInitialized() && renderer.HasMesh()) {
            const float width = ImGui::GetContentRegionAvail().x;
            const float height = std::min(320.0f, ImGui::GetContentRegionAvail().y * 0.45f);

            renderer.SetViewportSize(int(width), int(height));
            renderer.Render();

            ImGui::Image((ImTextureID)(intptr_t)renderer.GetOutputTexture(),
                         ImVec2(width, height), ImVec2(0, 1), ImVec2(1, 0));

            if (ImGui::IsItemHovered()) {
                const ImGuiIO& io = ImGui::GetIO();
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    renderer.RotateArcball(io.MouseDelta.x, io.MouseDelta.y, width, height);
                }
                if (io.MouseWheel != 0.0f) {
                    renderer.AdjustCameraDistance(-io.MouseWheel * 0.4f);
                }
            }
            ImGui::Spacing();
        }

        ImGui::TextColored(kDim, "This is what gets written");
        const std::string text = arma3::writeRvmat(material);
        ImGui::InputTextMultiline("##preview", const_cast<char*>(text.c_str()), text.size() + 1,
                                  ImVec2(-1, ImGui::GetContentRegionAvail().y - 4),
                                  ImGuiInputTextFlags_ReadOnly);
        ImGui::EndChild();
    }

    // Stages have fixed meanings, so name them rather than number them.
    static const char* stageName(int index) {
        switch (index) {
            case 1: return "Normal";
            case 2: return "Detail";
            case 3: return "Macro";
            case 4: return "Ambient shadow";
            case 5: return "Specular";
            case 6: return "Fresnel";
            case 7: return "Environment";
            default: return "Stage";
        }
    }

    void openRvmat(const std::string& path) {
        auto parsed = rvmat::Parser::parseFile(path);
        if (!parsed) {
            materialStatus = "Could not read that material";
            return;
        }

        material = arma3::RvmatMaterial{};
        material.ambient = parsed->ambient.toArray();
        material.diffuse = parsed->diffuse.toArray();
        material.forcedDiffuse = parsed->forcedDiffuse.toArray();
        material.emissive = parsed->emissive.toArray();
        material.specular = parsed->specular.toArray();
        material.specularPower = parsed->specularPower;
        if (!parsed->pixelShaderID.empty()) material.pixelShaderID = parsed->pixelShaderID;
        if (!parsed->vertexShaderID.empty()) material.vertexShaderID = parsed->vertexShaderID;

        for (const auto& [index, stage] : parsed->stages) {
            arma3::RvmatStage copy;
            copy.texture = stage.texture;
            copy.uvSource = stage.uvSource;
            material.stages[index] = copy;
        }

        materialPath = path;
        materialSource.clear();
        materialActive = true;
        materialStatus = "Opened " + fs::path(path).filename().string();
    }

    // Colours are cheap enough to push every frame; textures are only touched
    // when the stage actually points somewhere new.
    void applyMaterialToModel(bool quiet) {
        auto& slots = renderer.GetTextureSlotsMutable();
        for (auto& slot : slots) {
            for (int i = 0; i < 4; i++) {
                slot.material.ambient[i] = material.ambient[i];
                slot.material.diffuse[i] = material.diffuse[i];
                slot.material.specular[i] = material.specular[i];
                slot.material.emissive[i] = material.emissive[i];
            }
            slot.material.specularPower = material.specularPower;
            slot.material.hasRvmat = true;
        }

        applyStageTexture(1, appliedNormal, true);
        applyStageTexture(5, appliedSpecular, false);

        if (!quiet) {
            materialStatus = slots.empty() ? "The model has no textures to apply it to"
                                           : "Applied to the model";
        }
    }

    void applyStageTexture(int stageIndex, std::string& applied, bool isNormal) {
        const auto stage = material.stages.find(stageIndex);
        if (stage == material.stages.end()) return;

        const std::string& reference = stage->second.texture;
        if (reference.empty() || reference.front() == '#') return;   // procedural
        if (reference == applied) return;

        const std::string modelDir = fs::path(modelPath).parent_path().string();
        const std::string resolved = resolveModelTexture(reference, modelDir, textureRoot);
        if (resolved.empty()) return;

        if (isNormal) {
            renderer.LoadNormalTexture(resolved);
        } else {
            renderer.LoadSpecularTexture(resolved);
        }
        applied = reference;
    }

    // ---------------------------------------------------------------- actions

    void addFiles() {
        askForFiles("Select images", {"Images", kImageFilter, "All files", "*"},
                    true, [this](std::vector<std::string> files) {
            for (const auto& file : files) {
                inputFiles.push_back(arma3::describeSource(file));
            }
        });
    }

    void startConversion() {
        const auto outputs = arma3::planOutputs(inputFiles);
        if (outputs.empty()) return;

        {
            std::lock_guard<std::mutex> lock(jobsMutex);
            jobs.clear();
            for (const auto& output : outputs) {
                ConversionJob job;
                job.name = output.name;
                job.swizzle = output.swizzle;
                jobs.push_back(job);
            }
        }

        successCount = 0;
        failCount = 0;
        completedJobs = 0;
        totalJobs = int(outputs.size());
        converting = true;

        const int formatChoice = selectedFormat;
        const int qualityChoice = selectedQuality;
        const std::string dir = outputDir;

        std::thread([this, outputs, formatChoice, qualityChoice, dir] {
            std::atomic<size_t> next{0};
            unsigned workers = std::max(1u, std::thread::hardware_concurrency());
            workers = std::min<unsigned>(workers, unsigned(outputs.size()));

            auto worker = [&] {
                for (size_t index = next++; index < outputs.size(); index = next++) {
                    const arma3::PlannedOutput& plan = outputs[index];

                    bool ok = true;
                    std::string error;
                    uint32_t width = 0;
                    uint32_t height = 0;
                    int64_t ms = 0;

                    try {
                        const auto start = std::chrono::high_resolution_clock::now();

                        arma3::ChannelPacker packer;
                        for (const auto& file : plan.sources) {
                            packer.addSource(arma3::ImageLoader::load(file));
                        }
                        for (int c = 0; c < 4; c++) {
                            packer.setSlot(static_cast<arma3::PackChannel>(c), plan.slots[c]);
                        }

                        arma3::ImageData packed = packer.pack();
                        width = packed.width;
                        height = packed.height;

                        arma3::PAA paa;
                        paa.setQuality(static_cast<arma3::Quality>(qualityChoice));
                        paa.setThreadCount(1);
                        paa.setImage(packed);
                        paa.setSwizzle(plan.swizzle);

                        arma3::PAAFormat format = arma3::PAAFormat::UNKNOWN;
                        if (formatChoice == 1) format = arma3::PAAFormat::DXT1;
                        else if (formatChoice == 2) format = arma3::PAAFormat::DXT5;

                        const std::string base = dir.empty()
                            ? fs::path(plan.sources.front()).parent_path().string()
                            : dir;
                        paa.writePAA((fs::path(base) / plan.name).string(), format);

                        const auto end = std::chrono::high_resolution_clock::now();
                        ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 end - start).count();
                    }
                    catch (const std::exception& e) {
                        ok = false;
                        error = e.what();
                    }

                    {
                        std::lock_guard<std::mutex> lock(jobsMutex);
                        auto& job = jobs[index];
                        job.completed = true;
                        job.success = ok;
                        job.errorMessage = error;
                        job.width = width;
                        job.height = height;
                        job.durationMs = ms;
                    }

                    if (ok) successCount++; else failCount++;
                    completedJobs++;
                }
            };

            std::vector<std::thread> pool;
            pool.reserve(workers);
            for (unsigned i = 0; i < workers; i++) pool.emplace_back(worker);
            for (auto& thread : pool) thread.join();

            converting = false;
        }).detach();
    }

    // ------------------------------------------------------------------ state

    std::unique_ptr<pfd::open_file> pendingFiles;
    std::unique_ptr<pfd::select_folder> pendingFolder;
    std::unique_ptr<pfd::save_file> pendingSave;
    std::function<void(std::vector<std::string>)> onFiles;
    std::function<void(std::string)> onFolder;
    std::function<void(std::string)> onSave;

    int activeTab = 0;
    std::string dropMessage;
    bool dropRejected = false;
    std::vector<arma3::SourceFile> inputFiles;
    char outputDir[512] = {0};
    int selectedFormat = 0;
    int selectedQuality = 1;

    std::mutex jobsMutex;
    std::vector<ConversionJob> jobs;
    std::atomic<bool> converting{false};
    std::atomic<int> completedJobs{0};
    std::atomic<int> totalJobs{0};
    std::atomic<int> successCount{0};
    std::atomic<int> failCount{0};

    int packPreset = 0;
    PackSlot packSlots[4];
    char packOutput[512] = {0};
    bool overrideSize = false;
    int overrideWidth = 2048;
    int overrideHeight = 2048;
    std::string packStatus;
    bool packFailed = false;

    struct ViewMip { uint32_t width; uint32_t height; std::vector<uint8_t> data; };
    struct ViewTagg { std::string signature; std::vector<uint8_t> data; };

    arma3::RvmatMaterial material;
    std::string materialSource;
    std::string materialStatus;
    int materialTemplate = 0;
    std::string materialPath;
    bool materialActive = false;
    bool materialLive = true;
    std::string appliedNormal;
    std::string appliedSpecular;
    char pixelFilter[64] = {0};
    char vertexFilter[64] = {0};

    arma3::P3DInfo modelInfo;
    arma3::ModelRenderer renderer;
    std::string modelPath;
    std::string modelError;
    int modelLod = 0;
    bool hideProxies = true;
    char textureRoot[512] = {0};
    int loadedTextures = 0;
    std::vector<std::string> missingTextures;
    std::vector<std::string> textureRefs;
    std::map<std::string, std::string> textureOverrides;

    std::string viewPath;
    std::string viewError;
    std::vector<ViewMip> viewMips;
    std::vector<ViewTagg> viewTaggs;
    arma3::PAAFormat viewFormat = arma3::PAAFormat::UNKNOWN;
    arma3::SwizzleType viewSwizzle = arma3::SwizzleType::NONE;
    TexturePreview viewPreview;
    int viewLevel = 0;
    int viewMode = 0;

    GLuint previewTextures[5] = {0, 0, 0, 0, 0};
    int previewMode = 0;
    uint32_t previewWidth = 0;
    uint32_t previewHeight = 0;
    uint32_t packedWidth = 0;
    uint32_t packedHeight = 0;
    std::string storedSummary[4];
};

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void glfw_drop_callback(GLFWwindow* window, int count, const char** paths) {
    auto* app = static_cast<PAAConverterApp*>(glfwGetWindowUserPointer(window));
    std::vector<std::string> files;
    for (int i = 0; i < count; i++) files.push_back(paths[i]);
    app->handleFileDrop(files);
}

int main(int, char**) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1180, 820, "a3Texture",
                                          nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return 1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    applyStyle();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    PAAConverterApp app;
    glfwSetWindowUserPointer(window, &app);
    glfwSetDropCallback(window, glfw_drop_callback);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        app.render();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.07f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    app.releaseTextures();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
