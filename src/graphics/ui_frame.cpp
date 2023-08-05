#include "graphics/ui_frame.hpp"
#include "imgui.h"
#include "spike/type/pointer.hpp"
#include "utils/flatbuffers.hpp"

namespace prime::graphics {
using ImPointer = int32;
using UIFrameCtx = uint64;
struct Function;
using ImSignature = void (*)(prime::graphics::Function &);
} // namespace prime::graphics

template <class C>
es::PointerX86<C> &AsPointer(prime::graphics::ImPointer *ptr) {
  return reinterpret_cast<es::PointerX86<C> &>(*ptr);
}

template <class C>
const es::PointerX86<const C> &
AsPointer(const prime::graphics::ImPointer *ptr) {
  return reinterpret_cast<const es::PointerX86<const C> &>(*ptr);
}

#include "ui_frame.fbs.hpp"

namespace {
void ImCallWindow(prime::graphics::Function &payload) {
  auto &item =
      *const_cast<prime::graphics::ImWindow *>(payload.payload_as_window());
  item.mutate_result(ImGui::BeginChild(item.id(), item.size(), item.border(),
                                       uint32(item.flags())));
}

void ImCallWindowEnd(prime::graphics::Function &) { ImGui::EndChild(); }

template <class C> C ImFormatArgNumber(const void *argData) {
  static const C dummy = 0;
  const C *asNumber[]{&dummy, static_cast<const C *>(argData)};
  return *asNumber[bool(argData)];
}

template <class fn>
void ImFormatGetArg(const prime::graphics::ImFormatArg *arg, fn &&cb) {
  const void *argData = AsPointer<char>(&arg->variable()).Get();

  // Optim: could use remap array instead
  switch (arg->type()) {
  case ImGuiDataType_S64:
    cb(ImFormatArgNumber<int64>(argData));
    break;

  case ImGuiDataType_U64:
    cb(ImFormatArgNumber<uint64>(argData));
    break;

  case ImGuiDataType_S32:
    cb(ImFormatArgNumber<int32>(argData));
    break;

  case ImGuiDataType_U32:
    cb(ImFormatArgNumber<uint32>(argData));
    break;

  case ImGuiDataType_Float:
    cb(ImFormatArgNumber<float>(argData));
    break;

  default:
    break;
  }
}

template <class fn>
void ImFormatUnpack1D(const prime::graphics::ImFormat &format, fn &&cb) {
  auto fmt = format.fmt()->c_str();
  auto arg = (*format.args())[0];

  ImFormatGetArg(arg, [&](auto value) { cb(fmt, value); });
}

template <class fn>
void ImFormatUnpack2D(const prime::graphics::ImFormat &format, fn &&cb) {
  auto fmt = format.fmt()->c_str();
  auto arg0 = (*format.args())[0];
  auto arg1 = (*format.args())[1];
  ImFormatGetArg(arg0, [&](auto value0) {
    ImFormatGetArg(arg1, [&](auto value1) { cb(fmt, value0, value1); });
  });
}

template <class fn>
void ImFormatUnpack(const prime::graphics::ImFormat &fmt, fn &&cb) {
  const std::function<void()> funcRemap[3]{
      [&] { cb("%s", fmt.fmt()->c_str()); },
      [&] {
        ImFormatUnpack1D(fmt,
                         [&cb](const char *fmt, auto tp0) { cb(fmt, tp0); });
      },
      [&] {
        ImFormatUnpack2D(fmt, [&cb](const char *fmt, auto tp0, auto tp1) {
          cb(fmt, tp0, tp1);
        });
      },
  };

  funcRemap[fmt.args() ? fmt.args()->size() : 0]();
}

void ImCallText(prime::graphics::Function &payload) {
  auto &item =
      *const_cast<prime::graphics::ImFormat *>(payload.payload_as_text());
  ImFormatUnpack(
      item, [](const char *fmt, auto... args) { ImGui::Text(fmt, args...); });
}

void ImCallTextDisabled(prime::graphics::Function &payload) {
  auto &item = *const_cast<prime::graphics::ImFormat *>(
      payload.payload_as_textDisabled());
  ImFormatUnpack(item, [](const char *fmt, auto... args) {
    ImGui::TextDisabled(fmt, args...);
  });
}

void ImCallTextWrapped(prime::graphics::Function &payload) {
  auto &item = *const_cast<prime::graphics::ImFormat *>(
      payload.payload_as_textWrapped());
  ImFormatUnpack(item, [](const char *fmt, auto... args) {
    ImGui::TextWrapped(fmt, args...);
  });
}

void ImCallBulletText(prime::graphics::Function &payload) {
  auto &item =
      *const_cast<prime::graphics::ImFormat *>(payload.payload_as_bulletText());
  ImFormatUnpack(item, [](const char *fmt, auto... args) {
    ImGui::BulletText(fmt, args...);
  });
}

void ImCallTextColored(prime::graphics::Function &payload) {
  auto &item = *const_cast<prime::graphics::ImTextColored *>(
      payload.payload_as_textColored());

  ImFormatUnpack(*item.format(), [&item](const char *fmt, auto... args) {
    ImGui::TextColored(*item.color(), fmt, args...);
  });
}

void ImCallLabelText(prime::graphics::Function &payload) {
  auto &item = *const_cast<prime::graphics::ImLabelText *>(
      payload.payload_as_labelText());

  ImFormatUnpack(*item.format(), [&item](const char *fmt, auto... args) {
    ImGui::LabelText(item.label()->c_str(), fmt, args...);
  });
}

void ImCallButton(prime::graphics::Function &payload) {
  auto &item =
      *const_cast<prime::graphics::ImButton *>(payload.payload_as_button());

  item.mutate_result(ImGui::Button(item.label()->c_str(), *item.size()));
}

void ImCallSmallButton(prime::graphics::Function &payload) {
  auto &item = *const_cast<prime::graphics::ImSmallButton *>(
      payload.payload_as_smallButton());

  item.mutate_result(ImGui::SmallButton(item.label()->c_str()));
}

void ImCallInvisibleButton(prime::graphics::Function &payload) {
  auto &item = *const_cast<prime::graphics::ImInvisibleButton *>(
      payload.payload_as_invisibleButton());

  item.mutate_result(ImGui::InvisibleButton(item.strId()->c_str(), *item.size(),
                                            uint32(item.flags())));
}

void ImCallArrowButton(prime::graphics::Function &payload) {
  auto &item = *const_cast<prime::graphics::ImArrowButton *>(
      payload.payload_as_arrowButton());

  item.mutate_result(
      ImGui::ArrowButton(item.strId()->c_str(), int32(item.dir())));
}

void ImCallImage(prime::graphics::Function &payload) {
  auto &item =
      *const_cast<prime::graphics::ImImage *>(payload.payload_as_image());

  ImGui::Image(ImTextureID(uint64(item.texture())), item.size(), item.uv0(),
               item.uv1(), item.tint(), item.boderColor());
}

void ImCallImageButton(prime::graphics::Function &payload) {
  auto &item = *const_cast<prime::graphics::ImImageButton *>(
      payload.payload_as_imageButton());
  auto &image = item.image();

  item.mutate_result(ImGui::ImageButton(
      ImTextureID(uint64(image.texture())), image.size(), image.uv0(),
      image.uv1(), item.framePadding(), image.boderColor(), image.tint()));
}

void ImCallCheckbox(prime::graphics::Function &payload) {
  auto &item =
      *const_cast<prime::graphics::ImCheckbox *>(payload.payload_as_checkbox());
  bool v = item.result();
  ImGui::Checkbox(item.label()->c_str(), &v);
  item.mutate_result(v);
}

void ImCallCheckboxFlags(prime::graphics::Function &payload) {
  auto &item = *const_cast<prime::graphics::ImCheckboxFlags *>(
      payload.payload_as_checkboxFlags());
  auto &ptr = AsPointer<uint32>(item.mutable_variable());
  static uint32 dummy = 0;
  uint32 *addrs[]{
      &dummy,
      ptr.Get(),
  };

  item.mutate_result(ImGui::CheckboxFlags(item.label()->c_str(),
                                          addrs[bool(ptr)], item.mask()));
}

void ImCallRadioButton(prime::graphics::Function &payload) {
  auto &item = *const_cast<prime::graphics::ImRadioButton *>(
      payload.payload_as_radioButton());
  auto &ptr = AsPointer<int32>(item.mutable_variable());
  static int32 dummy = 0;
  int32 *addrs[]{
      &dummy,
      ptr.Get(),
  };

  int32 &addr = *addrs[bool(ptr)];

  if (ImGui::RadioButton(item.label()->c_str(), item.index() == addr)) {
    addr = item.index();
  }
}

void ImCallProgressBar(prime::graphics::Function &payload) {
  auto &item = *const_cast<prime::graphics::ImProgressBar *>(
      payload.payload_as_progressBar());
  const char *overlay[]{nullptr, item.overlay()->c_str()};

  ImGui::ProgressBar(item.fraction(), *item.size(),
                     overlay[item.overlay() && item.overlay()->size() > 0]);
}

static const std::vector<prime::graphics::ImSignature> functions{
    /**/ //
    ImCallWindow,
    ImCallWindowEnd,
    ImCallText,
    ImCallTextColored,
    ImCallTextDisabled,
    ImCallTextWrapped,
    ImCallBulletText,
    ImCallLabelText,
    ImCallButton,
    ImCallSmallButton,
    ImCallInvisibleButton,
    ImCallArrowButton,
    ImCallImage,
    ImCallImageButton,
    ImCallCheckbox,
    ImCallCheckboxFlags,
    ImCallRadioButton,
    ImCallProgressBar,
};

void AddUIFrame(prime::graphics::UIFrame &hdr) {
  if (hdr.stack()) {
    for (auto s : *hdr.stack()) {
      auto func = const_cast<prime::graphics::Function *>(s);
      *func->mutable_signature() = functions.at(uint32(s->payload_type()));
    }
  }
}

void FixupPointer(prime::graphics::VariableDescription &g,
                  const prime::graphics::VariablePool *vars,
                  prime::graphics::ImPointer &ptr) {
  const void *addr = [&]() -> const void * {
    switch (g.type()) {
    case ImGuiDataType_U8:
      return vars->bools()->data() + g.ptr();

    case ImGuiDataType_S32:
    case ImGuiDataType_U32:
      return vars->var32()->data() + g.ptr();

    case ImGuiDataType_S64:
    case ImGuiDataType_U64:
      return vars->var64()->data() + g.ptr();

    case ImGuiDataType_Float:
      return vars->floats()->data() + g.ptr();

    default:
      __builtin_unreachable();
    }
    __builtin_unreachable();
  }();

  AsPointer<const char>(&ptr) = static_cast<const char *>(addr);
}

void FixupVariable(prime::graphics::UIFrame &root,
                   prime::graphics::UIFrameT &main,
                   prime::graphics::ImPointer &ptr) {
  if (main.varPool) {
    for (auto &g : main.globalVars) {
      if (g.hash() == uint32(ptr)) {
        FixupPointer(g, root.varPool(), ptr);
        return;
      }
    }
  }

  if (main.debug) {
    for (auto &l : main.debug->localVarDescs) {
      if (l.hash() == uint32(ptr)) {
        FixupPointer(l, root.varPool(), ptr);
        return;
      }
    }
  }
}

void FixupCheckboxFlags(prime::graphics::Function &payload,
                        prime::graphics::UIFrame &frame,
                        prime::graphics::UIFrameT &native) {
  auto &item = *const_cast<prime::graphics::ImCheckboxFlags *>(
      payload.payload_as_checkboxFlags());

  int32 *hash = item.mutable_variable();
  FixupVariable(frame, native, *hash);
}

void FixupRadioButton(prime::graphics::Function &payload,
                      prime::graphics::UIFrame &frame,
                      prime::graphics::UIFrameT &native) {
  auto &item = *const_cast<prime::graphics::ImRadioButton *>(
      payload.payload_as_radioButton());

  int32 *hash = item.mutable_variable();
  FixupVariable(frame, native, *hash);
}

void FixupFormat(prime::graphics::Function &payload,
                 prime::graphics::UIFrame &frame,
                 prime::graphics::UIFrameT &native) {
  auto &item =
      *static_cast<const prime::graphics::ImFormat *>(payload.payload());

  if (auto args = item.args()) {
    for (auto it = args->begin(); it < args->end(); it++) {
      int32 *hash = const_cast<int32 *>(&it->variable());
      FixupVariable(frame, native, *hash);
    }
  }
}

void FixupTextColored(prime::graphics::Function &payload,
                      prime::graphics::UIFrame &frame,
                      prime::graphics::UIFrameT &native) {
  auto &item = *payload.payload_as_textColored();

  if (auto args = item.format()->args()) {
    for (auto it = args->begin(); it < args->end(); it++) {
      int32 *hash = const_cast<int32 *>(&it->variable());
      FixupVariable(frame, native, *hash);
    }
  }
}

void FixupLabelText(prime::graphics::Function &payload,
                    prime::graphics::UIFrame &frame,
                    prime::graphics::UIFrameT &native) {
  auto &item = *payload.payload_as_labelText();

  if (auto args = item.format()->args()) {
    for (auto it = args->begin(); it < args->end(); it++) {
      int32 *hash = const_cast<int32 *>(&it->variable());
      FixupVariable(frame, native, *hash);
    }
  }
}

std::vector<void (*)(prime::graphics::Function &, prime::graphics::UIFrame &,
                     prime::graphics::UIFrameT &)>
    fixupers{
        /**/     //
        nullptr, // ImCallWindow,
        nullptr, // ImCallWindowEnd,
        FixupFormat,
        FixupTextColored,
        FixupFormat,
        FixupFormat,
        FixupFormat,
        FixupLabelText,
        nullptr, // EditButton,
        nullptr, // EditSmallButton,
        nullptr, // EditInvisibleButton,
        nullptr, // EditArrowButton,
        nullptr, // ImCallImage,
        nullptr, // ImCallImageButton,
        nullptr, // EditCheckbox,
        FixupCheckboxFlags,
        FixupRadioButton,
        nullptr, // EditProgressBar,
    };
} // namespace

namespace prime::graphics {
void Fixup(prime::graphics::UIFrame &root, prime::graphics::UIFrameT &native) {
  if (root.stack()) {
    for (auto s : *root.stack()) {
      auto func = const_cast<prime::graphics::Function *>(s);
      *func->mutable_signature() = functions.at(uint32(s->payload_type()));

      if (auto fixup = fixupers.at(uint32(s->payload_type()))) {
        fixup(*func, root, native);
      }
    }
  }

  if (auto gVars = root.globalVars()) {
    for (auto d : *gVars) {
      auto desc = const_cast<prime::graphics::VariableDescription *>(d);
      FixupPointer(*desc, root.varPool(), desc->mutable_ptr());
    }
  }
}

void Draw(UIFrame &root) {
  if (root.stack()) {
    for (auto s : *root.stack()) {
      (*s->signature())(*const_cast<Function *>(s));
    }
  }
}

} // namespace prime::graphics

template <> class prime::common::InvokeGuard<prime::graphics::UIFrame> {
  static inline const bool data =
      prime::common::AddResourceHandle<prime::graphics::UIFrame>({
          .Process =
              [](ResourceData &data) {
                auto hdr = utils::GetFlatbuffer<prime::graphics::UIFrame>(data);
                AddUIFrame(*hdr);
              },
          .Delete = nullptr,
          .Handle = [](ResourceData &data) -> void * {
            return utils::GetFlatbuffer<prime::graphics::UIFrame>(data);
          },
      });
};
