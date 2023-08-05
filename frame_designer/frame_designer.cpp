#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "ImGuiFileDialog.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "spike/io/binreader.hpp"
#include "spike/io/binwritter.hpp"
#include "spike/io/fileinfo.hpp"
#include "spike/master_printer.hpp"
#include "spike/reflect/reflector.hpp"
#include "spike/type/flags.hpp"
#include <optional>
#include <variant>

#include "graphics/ui_frame.hpp"
#include "spike/type/pointer.hpp"
#include "utils/reflect.hpp"

namespace prime::graphics {
using ImPointer = int32;
struct Function;
using ImSignature = void (*)(prime::graphics::Function &);

struct UIFrameCtx {
  std::string varName;
  int dataType = 0;
  int numElements = 0;
  uint64 value[4]{};
  std::map<uint32, std::string> strings;

  // fbs hack
  UIFrameCtx &operator=(const UIFrameCtx &) { return *this; }
};
} // namespace prime::graphics

#include "ui_frame.fbs.hpp"

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei length,
                                const GLchar *message, const void *userParam) {
  const char *sevName = " unknown";
  switch (severity) {
  case GL_DEBUG_SEVERITY_HIGH:
    sevName = " high";
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    sevName = " medium";
    break;
  case GL_DEBUG_SEVERITY_LOW:
    sevName = " low";
    break;
  }

  if (type == GL_DEBUG_TYPE_ERROR) {
    printerror(sevName << " severity: " << message);
  } else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
    printinfo(message);
  } else {
    printwarning(sevName << " severity: " << message);
  }
}

#define ENUM_MEMBERD(type, desc)                                               \
  EnumProxy { #type, static_cast<uint64>(enum_type::type), desc }

REFLECT(
    ENUMERATION(prime::graphics::WindowFlags),
    ENUM_MEMBERD(NoTitleBar, "Disable title-bar"),
    ENUM_MEMBERD(NoResize, "Disable user resizing with the lower-right grip"),
    ENUM_MEMBERD(NoMove, "Disable user moving the window"),
    ENUM_MEMBERD(NoScrollbar,
                 "Disable scrollbars (window can still scroll with "
                 "mouse or programmatically)"),
    ENUM_MEMBERD(NoScrollWithMouse,
                 "Disable user vertically scrolling with mouse wheel. On child "
                 "window, mouse wheel will be forwarded to the parent unless "
                 "NoScrollbar is also set."),
    ENUM_MEMBERD(
        NoCollapse,
        "Disable user collapsing window by double-clicking on it. Also "
        "referred to as Window Menu Button (e.g. within a docking node)."),
    ENUM_MEMBERD(AlwaysAutoResize,
                 "Resize every window to its content every frame"),
    ENUM_MEMBERD(NoBackground,
                 "Disable drawing background color and outside border."),
    ENUM_MEMBERD(NoSavedSettings, "Never load/save settings."),
    ENUM_MEMBERD(NoMouseInputs,
                 "Disable catching mouse, hovering test with pass through."),
    ENUM_MEMBERD(MenuBar, "Has a menu-bar"),
    ENUM_MEMBERD(HorizontalScrollbar, "Allow horizontal scrollbar to appear."),
    ENUM_MEMBERD(
        NoFocusOnAppearing,
        "Disable taking focus when transitioning from hidden to visible state"),
    ENUM_MEMBERD(
        NoBringToFrontOnFocus,
        "Disable bringing window to front when taking focus (e.g. clicking "
        "on it or programmatically giving it focus)"),
    ENUM_MEMBERD(AlwaysVerticalScrollbar, "Always show vertical scrollbar."),
    ENUM_MEMBERD(AlwaysHorizontalScrollbar,
                 "Always show horizontal scrollbar."),
    ENUM_MEMBERD(
        AlwaysUseWindowPadding,
        "Ensure child windows without border uses style.WindowPadding (ignored "
        "by default for non-bordered child windows, because more convenient)"),
    ENUM_MEMBERD(NoNavInputs,
                 "No gamepad/keyboard navigation within the window"),
    ENUM_MEMBERD(NoNavFocus,
                 "No focusing toward this window with gamepad/keyboard "
                 "navigation (e.g. skipped by CTRL+TAB)"),
    ENUM_MEMBERD(
        UnsavedDocument,
        "Display a dot next to the title. When used in a tab/docking "
        "context, tab is selected when clicking the X + closure is not "
        "assumed (will wait for user to stop submitting the tab). "
        "Otherwise closure is assumed when pressing the X, so if you keep "
        "submitting the tab may reappear at end of tab bar."),
    ENUM_MEMBERD(NoDocking, "Disable docking of this window"));

REFLECT(ENUMERATION(prime::graphics::Dir), ENUM_MEMBER(Left),
        ENUM_MEMBER(Right), ENUM_MEMBER(Up), ENUM_MEMBER(Down));

#define ENUM_MEMBERA(item, name_)                                              \
  EnumProxy { #name_, static_cast<uint64>(item) }

REFLECT(ENUMERATION(ImGuiDataType), ENUM_MEMBERA(ImGuiDataType_S8, S8),
        ENUM_MEMBERA(ImGuiDataType_U8, U8),
        ENUM_MEMBERA(ImGuiDataType_S16, S16),
        ENUM_MEMBERA(ImGuiDataType_U16, U16),
        ENUM_MEMBERA(ImGuiDataType_S32, S32),
        ENUM_MEMBERA(ImGuiDataType_U32, U32),
        ENUM_MEMBERA(ImGuiDataType_S64, S64),
        ENUM_MEMBERA(ImGuiDataType_U64, U64),
        ENUM_MEMBERA(ImGuiDataType_Float, Float),
        ENUM_MEMBERA(ImGuiDataType_Double, Double));

MAKE_ENUM(ENUMSCOPE(class FunctionName, FunctionName), EMEMBER(Window),
          EMEMBER(WindowEnd), EMEMBER(Text), EMEMBER(TextColored),
          EMEMBER(TextDisabled), EMEMBER(TextWrapped), EMEMBER(BulletText),
          EMEMBER(LabelText), EMEMBER(Button), EMEMBER(SmallButton),
          EMEMBER(InvisibleButton), EMEMBER(ArrowButton), EMEMBER(Image),
          EMEMBER(ImageButton), EMEMBER(Checkbox), EMEMBER(CheckboxFlags),
          EMEMBER(RadioButton), EMEMBER(ProgressBar));

REFLECT(ENUMERATION(prime::graphics::ButtonFlags), ENUM_MEMBER(MouseButtonLeft),
        ENUM_MEMBER(MouseButtonRight), ENUM_MEMBER(MouseButtonMiddle));

template <FunctionName signature, class C>
C *AddItem(prime::graphics::UIFrameT &frame) {
  C *retval = new C();
  prime::graphics::FunctionT func;
  func.payload.type = prime::graphics::FunctionPayload(signature);
  func.payload.value = retval;
  frame.stack.emplace_back(
      std::make_unique<prime::graphics::FunctionT>(std::move(func)));

  static const auto reflected = GetReflectedEnum<FunctionName>();

  char buffer[32]{};
  snprintf(buffer, sizeof(buffer), "%s:%" PRIXMAX,
           reflected->names[uint32(signature)], uintptr_t(retval));

  frame.debug.get()->stackNames.emplace_back(buffer);

  return retval;
}

void AddLocalVar(prime::graphics::UIFrameT &frame, const char *name,
                 ImGuiDataType dataType, uint8 numElements, int32 ptr) {
  if (!frame.debugInfo) {
    frame.debugInfo = std::make_unique<prime::common::DebugInfoT>();
  }

  auto lastName = frame.debug->stackNames.back() + name;
  uint32 hash = JenkinsHash_(lastName);

  frame.debug->localVarDescs.emplace_back(dataType, numElements,
                                          frame.stack.size(), hash, ptr);

  auto &strings = frame.debug->context.strings;

  if (!strings.contains(hash)) {
    strings.emplace(hash, lastName);
    prime::common::StringHashT strhash;
    strhash.hash = hash;
    strhash.data = lastName;
    frame.debugInfo->strings.emplace_back(
        std::make_unique<prime::common::StringHashT>(std::move(strhash)));
  }
}

void AddButton(prime::graphics::UIFrameT &frame) {
  auto button =
      AddItem<FunctionName::Button, prime::graphics::ImButtonT>(frame);
  button->label = "button";
  AddLocalVar(frame, "_result", ImGuiDataType_U8, 1,
              prime::graphics::ImButton::VT_RESULT);
}

void AddSmallButton(prime::graphics::UIFrameT &frame) {
  auto button =
      AddItem<FunctionName::SmallButton, prime::graphics::ImSmallButtonT>(
          frame);
  button->label = "small button";
  AddLocalVar(frame, "_result", ImGuiDataType_U8, 1,
              prime::graphics::ImSmallButton::VT_RESULT);
}

void AddInvisibleButton(prime::graphics::UIFrameT &frame) {
  auto button = AddItem<FunctionName::InvisibleButton,
                        prime::graphics::ImInvisibleButtonT>(frame);
  button->size = {100, 10};
  button->strId = "invisible button";
  AddLocalVar(frame, "_result", ImGuiDataType_U8, 1,
              prime::graphics::ImInvisibleButton::VT_RESULT);
}

void AddArrowButton(prime::graphics::UIFrameT &frame) {
  auto button =
      AddItem<FunctionName::ArrowButton, prime::graphics::ImArrowButtonT>(
          frame);
  button->strId = "arrow button";
  AddLocalVar(frame, "_result", ImGuiDataType_U8, 1,
              prime::graphics::ImArrowButton::VT_RESULT);
}

void AddCheckbox(prime::graphics::UIFrameT &frame) {
  auto item =
      AddItem<FunctionName::Checkbox, prime::graphics::ImCheckboxT>(frame);
  item->label = "checkbox";
  AddLocalVar(frame, "_result", ImGuiDataType_U8, 1,
              prime::graphics::ImArrowButton::VT_RESULT);
}

void AddCheckboxFlags(prime::graphics::UIFrameT &frame) {
  auto item =
      AddItem<FunctionName::CheckboxFlags, prime::graphics::ImCheckboxFlagsT>(
          frame);
  item->label = "checkbox flags";
  item->variable = 0;
}

void AddRadioButton(prime::graphics::UIFrameT &frame) {
  auto item =
      AddItem<FunctionName::RadioButton, prime::graphics::ImRadioButtonT>(
          frame);
  item->label = "radio button";
}

void AddProgressBar(prime::graphics::UIFrameT &frame) {
  auto item =
      AddItem<FunctionName::ProgressBar, prime::graphics::ImProgressBarT>(
          frame);
  item->size = {-1, 0};
  AddLocalVar(frame, "_fraction", ImGuiDataType_Float, 1,
              prime::graphics::ImProgressBar::VT_FRACTION);
}

template <FunctionName func> void AddFormat(prime::graphics::UIFrameT &frame) {
  auto item = AddItem<func, prime::graphics::ImFormatT>(frame);
  item->fmt = "Sample Text";
}

void AddTextColored(prime::graphics::UIFrameT &frame) {
  auto item =
      AddItem<FunctionName::TextColored, prime::graphics::ImTextColoredT>(
          frame);
  item->format = std::make_unique<prime::graphics::ImFormatT>();
  item->format->fmt = "Sample Text";
  item->color =
      ImGui::ColorConvertU32ToFloat4(ImGui::GetColorU32(ImGuiCol_Text));
}

void AddLabelText(prime::graphics::UIFrameT &frame) {
  auto item =
      AddItem<FunctionName::LabelText, prime::graphics::ImLabelTextT>(frame);
  item->format = std::make_unique<prime::graphics::ImFormatT>();
  item->format->fmt = "Sample Text";
  item->label = "Label Text";
}

std::vector<void (*)(prime::graphics::UIFrameT &)> creators{
    /**/     //
    nullptr, // ImCallWindow,
    nullptr, // ImCallWindowEnd,
    AddFormat<FunctionName::Text>,
    AddTextColored,
    AddFormat<FunctionName::TextDisabled>,
    AddFormat<FunctionName::TextWrapped>,
    AddFormat<FunctionName::BulletText>,
    AddLabelText,
    AddButton,
    AddSmallButton,
    AddInvisibleButton,
    AddArrowButton,
    nullptr, // ImCallImage,
    nullptr, // ImCallImageButton,
    AddCheckbox,
    AddCheckboxFlags,
    AddRadioButton,
    AddProgressBar,
};

bool EditString(std::string *str, const char *id) {
  return ImGui::InputText(
      id, str->data(), str->capacity(), ImGuiInputTextFlags_CallbackResize,
      [](ImGuiInputTextCallbackData *data) {
        auto str = static_cast<std::string *>(data->UserData);
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
          if (data->BufSize > str->capacity()) {
            str->reserve(data->BufSize);
            data->Buf = str->data();
          }

          str->resize(data->BufTextLen);
        }
        return 0;
      },
      str);
}

bool EditButton(void *data, prime::graphics::UIFrameT &) {
  auto item = static_cast<prime::graphics::ImButtonT *>(data);
  bool edited = EditString(&item->label, "label");
  edited |= ImGui::InputFloat2("size", &item->size.x);
  return edited;
}

bool EditSmallButton(void *data, prime::graphics::UIFrameT &) {
  auto item = static_cast<prime::graphics::ImSmallButtonT *>(data);
  return EditString(&item->label, "label");
}

bool EditInvisibleButton(void *data, prime::graphics::UIFrameT &) {
  auto item = static_cast<prime::graphics::ImInvisibleButtonT *>(data);
  bool edited = EditString(&item->strId, "id");
  edited |= ImGui::InputFloat2("size", &item->size.x);
  static const auto reflected =
      GetReflectedEnum<prime::graphics::ButtonFlags>();

  for (size_t i = 0; i < reflected->numMembers; i++) {
    edited |= ImGui::CheckboxFlags(reflected->names[i],
                                   reinterpret_cast<uint32 *>(&item->flags),
                                   reflected->values[i]);
  }

  return edited;
}

bool EditArrowButton(void *data, prime::graphics::UIFrameT &) {
  auto item = static_cast<prime::graphics::ImArrowButtonT *>(data);
  bool edited = EditString(&item->strId, "id");
  static const auto *refEnum = GetReflectedEnum<prime::graphics::Dir>();
  int curitem = int(item->dir);
  if (ImGui::Combo("direction", &curitem, refEnum->names,
                   refEnum->numMembers)) {
    item->dir = decltype(item->dir)(curitem);
    edited = true;
  }

  return edited;
}

int EditVariable(ImGuiDataType need, prime::graphics::UIFrameT &main,
                 int varHash) {
  std::vector<const char *> names;
  int selectedIndex = -1;

  for (auto &g : main.globalVars) {
    if (g.type() == need) {
      auto found = std::find_if(
          main.debugInfo->strings.begin(), main.debugInfo->strings.end(),
          [&g](auto &str) { return str->hash == g.hash(); });

      if (found->get()->hash == uint32(varHash)) {
        selectedIndex = names.size();
      }

      names.emplace_back(found->get()->data.c_str());
    }
  }

  if (main.debug) {
    for (auto &l : main.debug->localVarDescs) {
      if (l.type() == need) {
        auto found = std::find_if(
            main.debugInfo->strings.begin(), main.debugInfo->strings.end(),
            [&l](auto &str) { return str->hash == l.hash(); });

        if (found->get()->hash == uint32(varHash)) {
          selectedIndex = names.size();
        }

        names.emplace_back(found->get()->data.c_str());
      }
    }
  }

  if (names.empty()) {
    ImGui::TextUnformatted("Create UInt variable!");
    return varHash;
  }

  if (ImGui::Combo("Variable", &selectedIndex, names.data(), names.size()))
      [[unlikely]] {
    return JenkinsHash_(names[selectedIndex]);
  }

  return varHash;
}

bool EditCheckbox(void *data, prime::graphics::UIFrameT &) {
  auto item = static_cast<prime::graphics::ImCheckboxT *>(data);
  return EditString(&item->label, "label");
}

bool EditCheckboxFlags(void *data, prime::graphics::UIFrameT &main) {
  auto item = static_cast<prime::graphics::ImCheckboxFlagsT *>(data);
  bool edited = EditString(&item->label, "label");
  int newVar = EditVariable(ImGuiDataType_U32, main, item->variable);
  edited |= newVar != item->variable;
  item->variable = newVar;

  if (ImGui::BeginTable("MaskBits", 4, ImGuiTableFlags_Borders)) {
    for (size_t b = 0; b < 8; b++) {
      ImGui::TableNextRow();
      ImGui::PushID(b);
      for (size_t c = 0; c < 4; c++) {
        ImGui::TableNextColumn();
        ImGui::PushID(c);
        edited |= ImGui::CheckboxFlags("##bit", &item->mask, 1 << (b * 4 + c));
        ImGui::SameLine();
        ImGui::Text("Bit %" PRIuMAX, b * 4 + c);
        ImGui::PopID();
      }
      ImGui::PopID();
    }

    ImGui::EndTable();
  }

  return edited;
}

bool EditRadioButton(void *data, prime::graphics::UIFrameT &main) {
  auto item = static_cast<prime::graphics::ImRadioButtonT *>(data);
  bool edited = EditString(&item->label, "label");
  edited |= ImGui::InputScalar("index", ImGuiDataType_S32, &item->index);
  int newVar = EditVariable(ImGuiDataType_S32, main, item->variable);
  edited |= newVar != item->variable;
  item->variable = newVar;
  return edited;
}

bool EditProgressBar(void *data, prime::graphics::UIFrameT &) {
  auto item = static_cast<prime::graphics::ImProgressBarT *>(data);
  bool edited = EditString(&item->overlay, "overlay");
  edited |= ImGui::InputFloat2("size", &item->size.x);
  return edited;
}

bool EditFormat(prime::graphics::ImFormatT *item,
                prime::graphics::UIFrameT &main) {
  bool edited = EditString(&item->fmt, "format");
  size_t curArg = 0;
  if (edited) {
    auto curPtr = item->fmt.begin();
    auto end = item->fmt.end();
    bool marked = false;
    ImGuiDataType types[2]{};

    while (curPtr < end) {
      if (*curPtr == '%') {
        curPtr++;
        if (curPtr < end && *curPtr != '%') {
          marked = true;
        }
      }

      if (marked) {
        switch (*curPtr) {
        case 'd':
        case 'i':
        case 'c':
          types[curArg++] = ImGuiDataType_S32;
          marked = false;
          break;

        case 'u':
        case 'o':
        case 'x':
        case 'X':
        case 'p':
          types[curArg++] = ImGuiDataType_U32;
          marked = false;
          break;

        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
        case 'a':
        case 'A':
          types[curArg++] = ImGuiDataType_Float;
          marked = false;
          break;

        case 'n':
          types[curArg++] = ImGuiDataType_COUNT;
          marked = false;
          break;

        case 's':
          types[curArg++] = ImGuiDataType_S8;
          marked = false;
          break;

        default:
          break;
        }
      }

      if (curArg > 1) {
        break;
      }
      curPtr++;
    }

    item->args.resize(curArg);
    curArg = 0;

    for (auto &a : item->args) {
      a.mutate_type(types[curArg++]);
    }

    curArg = 0;
  }

  for (auto &a : item->args) {
    ImGui::PushID(&a);
    int newVar = EditVariable(a.type(), main, a.variable());
    edited |= newVar != a.variable();
    a.mutable_variable() = newVar;
    ImGui::PopID();
  }

  return edited;
}

bool EditFormat(void *data, prime::graphics::UIFrameT &main) {
  auto item = static_cast<prime::graphics::ImFormatT *>(data);
  return EditFormat(item, main);
}

bool EditTextColored(void *data, prime::graphics::UIFrameT &main) {
  auto item = static_cast<prime::graphics::ImTextColoredT *>(data);
  bool edited = EditFormat(item->format.get(), main);
  edited |= ImGui::ColorPicker4("Text Color", &item->color.x);
  return edited;
}

bool EditLabelText(void *data, prime::graphics::UIFrameT &main) {
  auto item = static_cast<prime::graphics::ImLabelTextT *>(data);
  bool edited = EditString(&item->label, "Label");
  edited |= EditFormat(item->format.get(), main);
  return edited;
}

std::vector<bool (*)(void *, prime::graphics::UIFrameT &)> editors{
    /**/     //
    nullptr, // ImCallWindow,
    nullptr, // ImCallWindowEnd,
    EditFormat,
    EditTextColored,
    EditFormat,
    EditFormat,
    EditFormat,
    EditLabelText,
    EditButton,
    EditSmallButton,
    EditInvisibleButton,
    EditArrowButton,
    nullptr, // ImCallImage,
    nullptr, // ImCallImageButton,
    EditCheckbox,
    EditCheckboxFlags,
    EditRadioButton,
    EditProgressBar,
};

bool EditItem(prime::graphics::UIFrameT &frame, int at) {
  auto &func = frame.stack.at(at);
  auto editFunc = editors.at(uint32(func->payload.type));

  if (editFunc) {
    return editFunc(func->payload.value, frame);
  }

  return false;
}

void AddItem(prime::graphics::UIFrameT &frame, size_t at) {
  creators.at(at)(frame);
}

void PopulatVarsTable(const char *strId, prime::graphics::UIFrame &frame,
                      const prime::graphics::VariablePool *vars,
                      prime::graphics::VariablePoolT *nativeVars,
                      std::vector<prime::graphics::VariableDescription> &descs,
                      std::map<uint32, std::string> &strings) {
  if (ImGui::BeginTable(strId, 4, ImGuiTableFlags_Borders)) {
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 50);
    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 15);
    ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthStretch, 10);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 25);
    ImGui::TableHeadersRow();

    for (auto &g : descs) {
      ImGui::PushID(&g);
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      auto &str = strings.at(g.hash());
      ImGui::TextUnformatted(str.data(), str.data() + str.size());
      ImGui::TableNextColumn();
      static const auto *typeRef = GetReflectedEnum<ImGuiDataType>();
      ImGui::TextUnformatted(typeRef->names[g.type()]);
      ImGui::TableNextColumn();
      ImGui::Text("%u", g.numElements());
      ImGui::TableNextColumn();

      if (g.stackId()) {
        auto item = const_cast<prime::graphics::Function *>(
            (*frame.stack())[g.stackId() - 1]);
        int32 ptr = g.ptr();
        void *data =
            ptr > -1
                ? static_cast<flatbuffers::Table *>(item->mutable_payload())
                      ->GetStruct<void *>(ptr)
                : static_cast<char *>(item->mutable_payload()) - ptr;

        if (data) {
          switch (g.type()) {
          case ImGuiDataType_S64:
          case ImGuiDataType_U64:
          case ImGuiDataType_S32:
          case ImGuiDataType_U32:
          case ImGuiDataType_Float:
            ImGui::InputScalarN("##value", g.type(), data, g.numElements());
            break;

          case ImGuiDataType_U8:
            ImGui::Checkbox("##valueBool", static_cast<bool *>(data));
            break;

          default:
            break;
          }
        }
      } else if (vars) {
        switch (g.type()) {
        case ImGuiDataType_S64:
        case ImGuiDataType_U64: {
          auto data = const_cast<uint64_t *>(vars->var64()->data() + g.ptr());
          ImGui::InputScalarN("##value", g.type(), data, g.numElements());
          nativeVars->var64.at(g.ptr()) = *data;
          break;
        }
        case ImGuiDataType_U8: {
          auto data = reinterpret_cast<bool *>(
              const_cast<uint8_t *>(vars->bools()->data() + g.ptr()));
          ImGui::Checkbox("##valueBool", data);
          nativeVars->bools.at(g.ptr()) = *data;
          break;
        }

        case ImGuiDataType_S32:
        case ImGuiDataType_U32: {
          auto data = const_cast<uint32_t *>(vars->var32()->data() + g.ptr());
          ImGui::InputScalarN("##value", g.type(), data, g.numElements());
          nativeVars->var32.at(g.ptr()) = *data;
          break;
        }

        case ImGuiDataType_Float: {
          auto data = const_cast<float *>(vars->floats()->data() + g.ptr());
          ImGui::InputScalarN("##value", g.type(), data, g.numElements());
          nativeVars->floats.at(g.ptr()) = *data;
          break;
        }

        default:
          break;
        }
      }

      ImGui::PopID();
    }

    ImGui::EndTable();
  }
}

static const ImGuiDataType DATATYPE[]{
    ImGuiDataType_U8,    ImGuiDataType_U32, ImGuiDataType_S32,
    ImGuiDataType_Float, ImGuiDataType_U64, ImGuiDataType_S64,
};

void AddVariable(prime::graphics::VariablePoolT *vars,
                 std::vector<prime::graphics::VariableDescription> &descs,
                 prime::graphics::UIFrameCtx &curGlob) {
  const size_t varSizes[]{
      /**/ //
      vars->bools.size(),
      vars->var32.size(),
      vars->var32.size(),
      vars->floats.size(),
      vars->var64.size(),
      vars->var64.size(),
  };
  uint32 hash = JenkinsHash_(curGlob.varName);
  descs.emplace_back(DATATYPE[curGlob.dataType], curGlob.numElements + 1, 0,
                     hash, varSizes[curGlob.dataType]);

  switch (curGlob.dataType) {
  case 0: {
    bool *asBools = reinterpret_cast<bool *>(curGlob.value);
    vars->bools.insert(vars->bools.end(), asBools,
                       asBools + curGlob.numElements + 1);
    break;
  }
  case 1:
  case 2: {
    uint32 *asInts = reinterpret_cast<uint32 *>(curGlob.value);
    vars->var32.insert(vars->var32.end(), asInts,
                       asInts + curGlob.numElements + 1);
    break;
  }
  case 3: {
    float *asFloats = reinterpret_cast<float *>(curGlob.value);
    vars->floats.insert(vars->floats.end(), asFloats,
                        asFloats + curGlob.numElements + 1);
    break;
  }
  case 4:
  case 5:
    vars->var64.insert(vars->var64.end(), curGlob.value,
                       curGlob.value + curGlob.numElements + 1);
    break;
  default:
    break;
  }

  if (!curGlob.strings.contains(hash)) {
    curGlob.strings.emplace(hash, std::move(curGlob.varName));
  }

  curGlob.dataType = 0;
  curGlob.numElements = 0;
  curGlob.value[0] = 0;
  curGlob.value[1] = 0;
}

bool EditVariables(prime::graphics::UIFrameT &native,
                   prime::graphics::UIFrame &frame) {
  bool edited = false;
  auto &curGlob = native.debug->context;

  EditString(&curGlob.varName, "Name");
  ImGui::Combo("Type", &curGlob.dataType,
               "Bool\0UInt32\0Int32\0Float\0UInt64\0Int64\0");
  ImGui::Combo("Count", &curGlob.numElements, "1\0002\0003\0004\0");
  ImGui::InputScalarN("Value", DATATYPE[curGlob.dataType], curGlob.value,
                      curGlob.numElements + 1);

  if (ImGui::Button("Add new global var")) {
    if (!native.varPool) {
      native.varPool = std::make_unique<prime::graphics::VariablePoolT>();
    }

    AddVariable(native.varPool.get(), native.globalVars, curGlob);
    edited = true;
  }

  ImGui::SameLine();

  if (ImGui::Button("Add new local var")) {
    if (!native.varPool) {
      native.varPool = std::make_unique<prime::graphics::VariablePoolT>();
    }

    AddVariable(native.varPool.get(), native.debug->localVarDescs, curGlob);
    edited = true;
  }

  if (!native.globalVars.empty()) {
    ImGui::TextUnformatted("Global Variables");
    ImGui::Separator();
    PopulatVarsTable("globalvars", frame, frame.varPool(), native.varPool.get(),
                     native.globalVars, curGlob.strings);
  }

  ImGui::TextUnformatted("Local Variables");
  ImGui::Separator();
  PopulatVarsTable("localvars", frame, frame.varPool(), native.varPool.get(),
                   native.debug->localVarDescs, curGlob.strings);

  return edited;
}

int main(int, char *argv[]) {
  es::print::AddPrinterFunction(es::Print);

  glfwSetErrorCallback(
      [](int type, const char *msg) { printerror('(' << type << ')' << msg); });

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  int width = 1800;
  int height = 1020;

  GLFWwindow *window =
      glfwCreateWindow(width, height, "GLTex Viewer", nullptr, nullptr);

  if (!window) {
    glfwTerminate();
    return 1;
  }

  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  GLFWwindow *sharedWindow = glfwCreateWindow(1, 1, "", nullptr, window);

  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CONTEXT_RELEASE_BEHAVIOR, GLFW_RELEASE_BEHAVIOR_FLUSH);

  glfwMakeContextCurrent(window);

  GLenum err = glewInit();

  if (GLEW_OK != err) {
    glfwTerminate();
    return 2;
  }

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init();

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);

  prime::graphics::UIFrameT currentFrame;
  currentFrame.meta = prime::common::ClassResource<prime::graphics::UIFrame>();
  auto ext = prime::common::GetClassExtension<prime::graphics::UIFrame>();
  ext.c[4] = 0;
  currentFrame.debug = std::make_unique<prime::graphics::DebugT>();

  flatbuffers::FlatBufferBuilder builder;
  std::string unfixedBuffer;
  prime::graphics::UIFrame *rootPtr;
  time_t nextSaveTime;
  auto &curGlob = currentFrame.debug->context;

  auto Rebuild = [&] {
    if (!curGlob.strings.empty()) {
      if (!currentFrame.debugInfo) {
        currentFrame.debugInfo = std::make_unique<prime::common::DebugInfoT>();
      }

      currentFrame.debugInfo->strings.clear();

      for (auto &[id, str] : curGlob.strings) {
        prime::common::StringHashT strhash;
        strhash.hash = id;
        strhash.data = str;
        currentFrame.debugInfo->strings.emplace_back(
            std::make_unique<prime::common::StringHashT>(std::move(strhash)));
      }
    } else if (currentFrame.debugInfo) {
      es::Dispose(currentFrame.debugInfo);
    }

    builder.Clear();
    builder.ForceDefaults(true);
    builder.Finish(prime::graphics::UIFrame::Pack(builder, &currentFrame),
                   ext.c);
    rootPtr = flatbuffers::GetMutableRoot<prime::graphics::UIFrame>(
        builder.GetBufferPointer());
    unfixedBuffer = {reinterpret_cast<char *>(builder.GetBufferPointer()),
                     builder.GetSize()};
    Fixup(*rootPtr, currentFrame);
    nextSaveTime = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now() + std::chrono::seconds(2));
  };

  Rebuild();
  nextSaveTime = std::numeric_limits<time_t>::max();

  int selectedWidget = 0;
  int selectedItem = -1;

  while (!glfwWindowShouldClose(window)) {
    const time_t curTime =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    if (curTime > nextSaveTime) {
      auto text = prime::utils::ToString(
          prime::common::GetClassHash<prime::graphics::UIFrame>(),
          unfixedBuffer.data());

      printline(text);
      nextSaveTime = std::numeric_limits<time_t>::max();
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ImGui::ShowDemoWindow();

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (ImGui::Begin("EditorDockSpace", nullptr, windowFlags)) {
      ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f),
                       ImGuiDockNodeFlags_None);
      ImGui::PopStyleVar();
      if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
          ImGui::EndMenu();
        }
      }
      ImGui::EndMenuBar();

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    }
    ImGui::End();

    if (ImGui::Begin("View", nullptr)) {
      if (ImGui::BeginChild("RenderArea", ImVec2{}, false,
                            ImGuiWindowFlags_NoMove)) {
        prime::graphics::Draw(*rootPtr);
      }
      ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopStyleVar(3);

    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      static const auto refEnum = GetReflectedEnum<FunctionName>();
      ImGui::Combo("Widgets", &selectedWidget, refEnum->names,
                   refEnum->numMembers);
      ImGui::SameLine();
      if (ImGui::Button("Add")) {
        AddItem(currentFrame, selectedWidget);
        Rebuild();
      }

      if (ImGui::BeginTable("##itemsTbl", 1, ImGuiTableFlags_Borders)) {
        int curItem = 0;
        for (auto &f : currentFrame.stack) {
          ImGui::TableNextColumn();
          ImGui::PushID(&f);
          if (ImGui::Selectable(
                  currentFrame.debug->stackNames.at(curItem).c_str(),
                  selectedItem == curItem)) {
            selectedItem = curItem;
          }
          ImGui::PopID();
          curItem++;
        }

        ImGui::EndTable();
      }

      if (selectedItem > -1) {
        ImGui::TextUnformatted("Widget Settings");
        ImGui::Separator();

        if (EditItem(currentFrame, selectedItem)) {
          Rebuild();
        }
      }

      ImGui::TextUnformatted("Variables");
      ImGui::Separator();
      if (EditVariables(currentFrame, *rootPtr)) {
        Rebuild();
      }
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwDestroyWindow(sharedWindow);
  glfwTerminate();
  return 0;
}
