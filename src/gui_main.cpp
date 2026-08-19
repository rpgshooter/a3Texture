#include "../include/paa.h"
#include "../include/image_loader.h"
#include "../include/channel_packer.h"

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

    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg]        = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_ChildBg]         = ImVec4(0.11f, 0.13f, 0.15f, 1.00f);
    c[ImGuiCol_FrameBg]         = ImVec4(0.15f, 0.17f, 0.20f, 1.00f);
    c[ImGuiCol_FrameBgHovered]  = ImVec4(0.19f, 0.22f, 0.26f, 1.00f);
    c[ImGuiCol_FrameBgActive]   = ImVec4(0.22f, 0.26f, 0.30f, 1.00f);
    c[ImGuiCol_Border]          = ImVec4(0.22f, 0.25f, 0.29f, 1.00f);
    c[ImGuiCol_Button]          = ImVec4(0.18f, 0.21f, 0.25f, 1.00f);
    c[ImGuiCol_ButtonHovered]   = ImVec4(0.24f, 0.29f, 0.34f, 1.00f);
    c[ImGuiCol_ButtonActive]    = ImVec4(0.11f, 0.45f, 0.50f, 1.00f);
    c[ImGuiCol_Header]          = ImVec4(0.13f, 0.36f, 0.40f, 1.00f);
    c[ImGuiCol_HeaderHovered]   = ImVec4(0.16f, 0.44f, 0.49f, 1.00f);
    c[ImGuiCol_HeaderActive]    = ImVec4(0.18f, 0.50f, 0.56f, 1.00f);
    c[ImGuiCol_Tab]             = ImVec4(0.13f, 0.15f, 0.18f, 1.00f);
    c[ImGuiCol_TabHovered]      = ImVec4(0.20f, 0.45f, 0.50f, 1.00f);
    c[ImGuiCol_TabActive]       = ImVec4(0.16f, 0.38f, 0.43f, 1.00f);
    c[ImGuiCol_TitleBgActive]   = ImVec4(0.12f, 0.14f, 0.17f, 1.00f);
    c[ImGuiCol_CheckMark]       = ImVec4(0.40f, 0.82f, 0.88f, 1.00f);
    c[ImGuiCol_SliderGrab]      = ImVec4(0.30f, 0.68f, 0.75f, 1.00f);
    c[ImGuiCol_PlotHistogram]   = ImVec4(0.25f, 0.70f, 0.76f, 1.00f);
    c[ImGuiCol_TableHeaderBg]   = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
    c[ImGuiCol_TableBorderLight]= ImVec4(0.20f, 0.23f, 0.27f, 1.00f);
    c[ImGuiCol_TableBorderStrong]=ImVec4(0.25f, 0.28f, 0.33f, 1.00f);
}

const ImVec4 kAccent(0.40f, 0.82f, 0.88f, 1.00f);
const ImVec4 kOk(0.44f, 0.84f, 0.53f, 1.00f);
const ImVec4 kBad(0.93f, 0.45f, 0.40f, 1.00f);
const ImVec4 kDim(0.55f, 0.60f, 0.66f, 1.00f);

} // namespace

struct ConversionJob {
    std::string inputPath;
    std::string outputPath;
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
    void render() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        ImGui::Begin("Arma 3 PAA Converter", nullptr,
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
            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    void handleFileDrop(const std::vector<std::string>& files) {
        for (const auto& file : files) {
            if (!isSupportedImage(file)) continue;

            if (activeTab == 1) {
                const arma3::PackPreset* preset = currentPreset();
                for (int i = 0; preset && i < preset->sourceCount; i++) {
                    if (packSlots[i].path.empty()) {
                        packSlots[i].path = file;
                        break;
                    }
                }
            } else {
                inputFiles.push_back(file);
            }
        }
    }

    void releaseTextures() {
        for (GLuint& id : previewTextures) {
            if (id) glDeleteTextures(1, &id);
            id = 0;
        }
    }

private:
    // ---------------------------------------------------------------- convert

    void renderConvert() {
        ImGui::Spacing();
        ImGui::TextColored(kDim,
            "Drop images here, or add them below. PNG, TGA, JPG and TIFF.");
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
            auto folder = pfd::select_folder("Select output folder", "").result();
            if (!folder.empty()) {
                snprintf(outputDir, sizeof(outputDir), "%s", folder.c_str());
            }
        }

        ImGui::Spacing();

        const bool busy = converting.load();
        const bool canConvert = !busy && !inputFiles.empty();

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
            ImGui::BeginChild("empty", ImVec2(0, 120), true);
            ImGui::Spacing();
            ImGui::TextColored(kDim, "   Nothing queued yet.");
            ImGui::EndChild();
            return;
        }

        ImGui::BeginChild("queue", ImVec2(0, 160), true);
        if (ImGui::BeginTable("files", 4,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 90);
            ImGui::TableSetupColumn("Format", ImGuiTableColumnFlags_WidthFixed, 70);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 30);
            ImGui::TableHeadersRow();

            int removeIndex = -1;
            for (size_t i = 0; i < inputFiles.size(); i++) {
                ImGui::TableNextRow();
                ImGui::PushID(int(i));

                const std::string name = fs::path(inputFiles[i]).filename().string();
                const auto swizzle = arma3::PAA::swizzleFromFilename(inputFiles[i]);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(name.c_str());
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", inputFiles[i].c_str());
                }

                ImGui::TableNextColumn();
                if (swizzle == arma3::SwizzleType::NONE) {
                    ImGui::TextColored(kDim, "plain");
                } else {
                    ImGui::TextColored(kAccent, "_%s", arma3::PAA::swizzleName(swizzle));
                }

                ImGui::TableNextColumn();
                ImGui::TextColored(kDim, "%s",
                    formatLabel(selectedFormat == 1 ? arma3::PAAFormat::DXT1 :
                                selectedFormat == 2 ? arma3::PAAFormat::DXT5 :
                                arma3::PAA::swizzleFormat(swizzle)));

                ImGui::TableNextColumn();
                if (ImGui::SmallButton("x")) removeIndex = int(i);

                ImGui::PopID();
            }

            if (removeIndex >= 0) {
                inputFiles.erase(inputFiles.begin() + removeIndex);
            }
            ImGui::EndTable();
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
                ImGui::TextUnformatted(fs::path(job.inputPath).filename().string().c_str());

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
                auto files = pfd::open_file("Select image", "",
                    {"Images", kImageFilter, "All files", "*"}).result();
                if (!files.empty()) packSlots[i].path = files[0];
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(70);
            ImGui::Combo("##ch", &packSlots[i].channel, "R\0G\0B\0A\0");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Which channel to read from this image");
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
            auto file = pfd::save_file("Save packed texture", "", {"PAA", "*.paa"}).result();
            if (!file.empty()) snprintf(packOutput, sizeof(packOutput), "%s", file.c_str());
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
            if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.44f, 0.49f, 1.0f));
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
                packer.setSourceChannel(i, static_cast<arma3::PackChannel>(packSlots[i].channel));
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
                sources[0] = "255"; sources[1] = "specular level";
                sources[2] = "specular power"; sources[3] = "255";
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

        const arma3::ImageData small = downscaleTo(full, 512);
        previewWidth = small.width;
        previewHeight = small.height;

        previewTextures[0] = uploadTexture(small.data.data(), small.width, small.height);

        for (int c = 0; c < 4; c++) {
            std::vector<uint8_t> grey(small.data.size());
            for (size_t i = 0; i < grey.size() / 4; i++) {
                const uint8_t v = small.data[i * 4 + c];
                grey[i * 4 + 0] = v;
                grey[i * 4 + 1] = v;
                grey[i * 4 + 2] = v;
                grey[i * 4 + 3] = 255;
            }
            previewTextures[c + 1] = uploadTexture(grey.data(), small.width, small.height);
        }
    }

    void clearPreview() {
        releaseTextures();
        previewMode = 0;
    }

    // ---------------------------------------------------------------- actions

    void addFiles() {
        auto files = pfd::open_file("Select images", "",
            {"Images", kImageFilter, "All files", "*"},
            pfd::opt::multiselect).result();
        for (const auto& file : files) inputFiles.push_back(file);
    }

    void startConversion() {
        {
            std::lock_guard<std::mutex> lock(jobsMutex);
            jobs.clear();
            for (const auto& input : inputFiles) {
                ConversionJob job;
                job.inputPath = input;
                job.swizzle = arma3::PAA::swizzleFromFilename(input);

                const std::string dir = outputDir[0] ? outputDir
                                                     : fs::path(input).parent_path().string();
                job.outputPath =
                    (fs::path(dir) / (fs::path(input).stem().string() + ".paa")).string();
                jobs.push_back(job);
            }
        }

        successCount = 0;
        failCount = 0;
        completedJobs = 0;
        totalJobs = int(inputFiles.size());
        converting = true;

        const int formatChoice = selectedFormat;
        const int qualityChoice = selectedQuality;

        std::thread([this, formatChoice, qualityChoice] {
            std::atomic<size_t> next{0};
            unsigned workers = std::max(1u, std::thread::hardware_concurrency());
            {
                std::lock_guard<std::mutex> lock(jobsMutex);
                workers = std::min<unsigned>(workers, unsigned(jobs.size()));
            }

            auto worker = [&] {
                for (;;) {
                    size_t index = next++;

                    ConversionJob local;
                    {
                        std::lock_guard<std::mutex> lock(jobsMutex);
                        if (index >= jobs.size()) return;
                        local = jobs[index];
                    }

                    bool ok = true;
                    std::string error;
                    uint32_t width = 0;
                    uint32_t height = 0;
                    int64_t ms = 0;

                    try {
                        const auto start = std::chrono::high_resolution_clock::now();

                        arma3::PAA paa;
                        paa.setQuality(static_cast<arma3::Quality>(qualityChoice));
                        paa.setThreadCount(1);
                        paa.loadImage(local.inputPath);
                        paa.setSwizzle(local.swizzle);

                        width = paa.getMipMaps()[0].width;
                        height = paa.getMipMaps()[0].height;

                        arma3::PAAFormat format = arma3::PAAFormat::UNKNOWN;
                        if (formatChoice == 1) format = arma3::PAAFormat::DXT1;
                        else if (formatChoice == 2) format = arma3::PAAFormat::DXT5;

                        paa.writePAA(local.outputPath, format);

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

    int activeTab = 0;
    std::vector<std::string> inputFiles;
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

    GLFWwindow* window = glfwCreateWindow(940, 760, "Arma 3 PAA Converter",
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
