#include "utils/converters/texture_compiler.hpp"
#include "utils/reflect_impl.hpp"

REFLECT(ENUM(prime::utils::TextureCompilerRGBAType),
  EMEMBER(RGBA),
  EMEMBER(BC3),
  EMEMBER(BC7)
  //EMEMBER(RGBA5551)
);

REFLECT(ENUM(prime::utils::TextureCompilerRGBType),
  EMEMBER(RGBX),
  EMEMBER(BC1),
  EMEMBER(BC7)
  //EMEMBER(RGB565)
);

REFLECT(ENUM(prime::utils::TextureCompilerRGType),
  EMEMBER(RG),
  EMEMBER(BC5),
  EMEMBER(BC7)
);

REFLECT(ENUM(prime::utils::TextureCompilerMonochromeType),
  EMEMBER(Monochrome),
  EMEMBER(BC4),
  EMEMBER(BC7)
);

REFLECT(ENUM(prime::utils::TextureCompilerNormalType),
  EMEMBER(BC3),
  EMEMBER(BC5),
  EMEMBER(BC5S),
  EMEMBER(BC7),
  EMEMBER(RG),
  EMEMBER(RGS)
);

REFLECT(ENUM(prime::utils::TextureCompilerEntryType),
  EMEMBER(Mipmap),
  EMEMBER(FrontFace),
  EMEMBER(BackFace),
  EMEMBER(LeftFace),
  EMEMBER(RightFace),
  EMEMBER(TopFace),
  EMEMBER(BottomFace)
);

REFLECT(CLASS(prime::utils::TextureCompilerEntry),
  MEMBER(level),
  MEMBER(type),
  MEMBER(paths)
);

REFLECT(CLASS(prime::utils::TextureCompiler),
  MEMBER(rgbaType),
  MEMBER(rgbType),
  MEMBER(rgType),
  MEMBER(monochromeType),
  MEMBER(normalType),
  MEMBER(isNormalMap),
  MEMBER(generateMipmaps),
  MEMBER(streamLimit),
  MEMBER(entries)
);
