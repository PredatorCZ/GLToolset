/*  GLTEX
    Copyright(C) 2022 Lukas Cone

    This program is free software : you can redistribute it and / or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.If not, see <https://www.gnu.org/licenses/>.
*/

#include "project.h"
#include "spike/app_context.hpp"
#include "spike/reflect/reflector.hpp"
#include "utils/converters.hpp"

static AppInfo_s appInfo{
    .filteredLoad = true,
    .header = GLTEX_DESC " v" GLTEX_VERSION ", " GLTEX_COPYRIGHT "Lukas Cone",
    .settings = reinterpret_cast<ReflectorFriend *>(
        prime::utils::ProcessImageSettings()),
    .filters = prime::utils::ProcessImageFilters(),
};

AppInfo_s *AppInitModule() { return &appInfo; }

void AppProcessFile(AppContext *ctx) { prime::utils::ProcessImage(ctx); }
