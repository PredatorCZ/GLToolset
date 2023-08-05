/*  GLMOD
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
#include "spike/except.hpp"
#include "spike/master_printer.hpp"
#include "spike/reflect/reflector.hpp"
#include "utils/converters.hpp"

std::string_view filters[]{
    ".md2$",
};

static AppInfo_s appInfo{
    .filteredLoad = true,
    .header = GLMOD_DESC " v" GLMOD_VERSION ", " GLMOD_COPYRIGHT "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

static constexpr uint32 MD2_ID = CompileFourCC("IDP2");

void AppProcessFile(AppContext *ctx) {
  uint32 id;
  ctx->GetType(id);

  if (id == MD2_ID) {
    try {
      ctx->RequestFile(std::string(ctx->workingFile.ChangeExtension(".iqm")));
      printwarning("IQM vaiant found, skipping MD2 processing");
    } catch (const es::FileNotFoundError &e) {
      prime::utils::ProcessMD2(ctx);
    }
  } else {
    throw es::InvalidHeaderError(id);
  }
}
