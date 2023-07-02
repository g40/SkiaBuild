/*
 * Copyright 2020 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <iostream>
#include <filesystem>
#include <regex>

#include "include/core/SkMatrix.h"
#include "include/core/SkStream.h"
#include "include/core/SkSurface.h"
#include "include/encode/SkPngEncoder.h"
#include "modules/skresources/include/SkResources.h"
#include "modules/svg/include/SkSVGDOM.h"
#include "src/utils/SkOSPath.h"
#include "tools/flags/CommandLineFlags.h"

#pragma comment(lib, "skia")
#pragma comment(lib, "skresources")
#pragma comment(lib, "sksg")
#pragma comment(lib, "skshaper")
#pragma comment(lib, "skunicode")
#pragma comment(lib, "svg")
#pragma comment(lib, "skottie")
#pragma comment(lib, "opengl32")

int width = 1400;
int height = 980;

// std::regex("\\.(?:txt)")

static 
std::vector<std::string> 
find_files(const std::string& search_path, const std::regex& regex = std::regex("\\.(?:txt)"))
{
    std::vector<std::string> ret;
    const std::filesystem::directory_iterator end;
    for (std::filesystem::directory_iterator iter{search_path}; iter != end; iter++)
    {
        if (std::filesystem::is_regular_file(*iter))
        {
            const std::string file_ext = iter->path().extension().string();
            if (std::regex_match(file_ext, regex))
            {
                ret.push_back(iter->path().string());
            }
        }
    }
    return ret;
}

static 
std::vector<std::string> 
find_files(const std::string& search_path, const std::string& ext = ".svg")
{
    std::vector<std::string> ret;
    const std::filesystem::directory_iterator end;
    for (std::filesystem::directory_iterator iter{search_path}; iter != end; iter++)
    {
        if (std::filesystem::is_regular_file(*iter))
        {
            const std::string file_ext = iter->path().extension().string();
            if (file_ext == ext)
            {
                ret.push_back(iter->path().string());
            }
        }
    }
    return ret;
}

int main(int argc, char** argv) 
{
    std::string cwd = std::filesystem::current_path().string();
    if (argv[1] != nullptr)
        cwd = argv[1];
    std::vector<std::string> files = find_files(cwd,".svg");
    //std::vector<std::string> files = find_files(cwd, std::regex("\\*\\.svg"));
    for (auto file: files)
    {
        std::string ipName = file;
        std::string opName = ipName;
        opName += ".png";

        SkFILEStream in(ipName.c_str());
        if (!in.isValid()) {
            std::cerr << "Could not open " << ipName << "\n";
            return 1;
        }

        auto rp = skresources::DataURIResourceProviderProxy::Make(
            skresources::FileResourceProvider::Make(SkOSPath::Dirname(ipName.c_str()),
                /*predecode=*/true),
            /*predecode=*/true);

        auto svg_dom = SkSVGDOM::Builder()
            .setFontManager(SkFontMgr::RefDefault())
            .setResourceProvider(std::move(rp))
            .make(in);
        if (!svg_dom) {
            std::cerr << "Could not parse " << ipName << "\n";
            continue;
        }

        auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(width, height));

        svg_dom->setContainerSize(SkSize::Make(width, height));
        svg_dom->render(surface->getCanvas());

        SkPixmap pixmap;
        surface->peekPixels(&pixmap);

        SkFILEWStream out(opName.c_str());
        if (!out.isValid()) {
            std::cerr << "Could not open " << opName << " for writing.\n";
            continue;
        }

        // Use default encoding options.
        SkPngEncoder::Options png_options;

        if (!SkPngEncoder::Encode(&out, pixmap, png_options)) {
            std::cerr << "PNG encoding failed.\n";
            continue;
        }

        std::cout << "Parsed:" << ipName << " => " << opName << std::endl;
    }
    return 0;
}
