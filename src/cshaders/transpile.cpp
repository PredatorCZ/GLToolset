#include "simplecpp.h"
#include "spike/master_printer.hpp"
#include "spike/util/supercore.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

/*
varying: removed in 2.0 or less, idk
lowp, mediump, highp: unused outside ES (weird anyway)
precise: not an attribute
coherent: image, ssbo and their members
volatile: image, ssbo and their members
restrict: image, ssbo and their members
writeonly: image, ssbo and their members
subroutine: uuhhh..., array var?
invariant: idk

flat: variable interpolation mode
noperspective: variable interpolation mode
*/

struct Attributes {
  bool isConst : 1 = false;
  bool isConstExpr : 1 = false;
  bool isStatic : 1 = false;
  bool isArray : 1 = false;
  bool disable : 1 = false;
  bool isDecl : 1 = false;
  bool isInstanced : 1 = false;
  int binding = -1;
};

struct Literal {
  Literal() = default;
  Literal(int i) : asInt{i} {}
  union {
    int asInt;
    bool asBool;
  };

  bool isInt = false;
  bool isEntry = false;
};

enum class Stage {
  vertex,
  geometry,
  tessControl,
  tessEval,
  fragment,
};

struct References {
  bool vertex : 1 = false;
  bool fragment : 1 = false;
  bool geometry : 1 = false;
  bool tessControl : 1 = false;
  bool tessEval : 1 = false;

  References operator=(Stage s) {
    switch (s) {
    case Stage::vertex:
      vertex = true;
      break;
    case Stage::geometry:
      geometry = true;
      break;
    case Stage::tessControl:
      tessControl = true;
      break;
    case Stage::tessEval:
      tessEval = true;
      break;
    case Stage::fragment:
      fragment = true;
      break;
    }

    return *this;
  }
};

bool operator==(References a, Stage s) {
  switch (s) {
  case Stage::vertex:
    return a.vertex;
  case Stage::geometry:
    return a.geometry;
  case Stage::tessControl:
    return a.tessControl;
  case Stage::tessEval:
    return a.tessEval;
  case Stage::fragment:
    return a.fragment;
  }

  return false;
}

const char *StageToStr(Stage stage) {
  switch (stage) {
  case Stage::vertex:
    return "vertex";
  case Stage::geometry:
    return "geometry";
  case Stage::tessControl:
    return "tessControl";
  case Stage::tessEval:
    return "tessEval";
  case Stage::fragment:
    return "fragment";
  }

  return nullptr;
}

struct Variable {
  std::string typeName;
  Attributes attrs;
  References refs;
  std::vector<int> arraySizes;
  std::vector<Literal> defaultValues;
};

struct StructDecl {
  References refs;
  std::vector<Variable> membersArr;
  std::map<std::string, size_t> members;
};

struct Function {
  const simplecpp::Token *headStart;
  const simplecpp::Token *headEnd;
  const simplecpp::Token *bodyStart;
  const simplecpp::Token *bodyEnd;
  std::map<std::string, Variable> variables;
  std::set<std::string> subCalls;
};

struct Source {
  std::map<std::string, StructDecl> structDecls;
  std::map<std::string, Function> functions;
  std::map<std::string, Variable> variables;

  std::map<std::string, Variable> uniformVariables;
};

struct UnexpectedOp : std::runtime_error {
  static std::string Make(const simplecpp::Token *tok, char op) {
    std::stringstream retval;
    retval << tok->location.file() << ':' << tok->location.line << ':'
           << tok->location.col << ": error: Unexpedted token `" << tok->str()
           << "` (expected `" << op << "`)";

    return retval.str();
  }
  UnexpectedOp(const simplecpp::Token *tok, char op)
      : std::runtime_error(Make(tok, op)) {}
};

struct UnexpectedTokenType : std::runtime_error {
  static std::string Make(const simplecpp::Token *tok, const char *op) {
    std::stringstream retval;
    retval << tok->location.file() << ':' << tok->location.line << ':'
           << tok->location.col << ": error: Unexpedted token `" << tok->str()
           << "` (expected `" << op << "`)";

    return retval.str();
  }
  UnexpectedTokenType(const simplecpp::Token *tok, const char *what)
      : std::runtime_error(Make(tok, what)) {}
};

const simplecpp::Token *ExpectOp(const simplecpp::Token *tok, char op) {
  if (tok->op != op) {
    throw UnexpectedOp(tok, op);
  }

  return tok->next;
}

void ExpectNumber(const simplecpp::Token *tok) {
  if (!tok->number) {
    throw UnexpectedTokenType(tok, "number");
  }
}

void ExpectName(const simplecpp::Token *tok, const char *what) {
  if (!tok->name) {
    throw UnexpectedTokenType(tok, what);
  }
}

const simplecpp::Token *SkipLocalScope(const simplecpp::Token *tok) {
  tok = ExpectOp(tok, '{');
  int level = 0;

  while (tok->op != '}' || level) {
    if (tok->op == '{') {
      level++;
    } else if (tok->op == '}') {
      level--;
    }
    tok = tok->next;
  }

  return tok->next;
}

struct WrongLiteral : std::runtime_error {
  using std::runtime_error::runtime_error;
};

Literal EvaluateConstexpr(const simplecpp::Token *&tok, Source &source) {
  Literal literal{};
  const bool negate = tok->op == '!';

  if (negate) {
    tok = tok->next;
  }

  if (tok->number) {
    literal.asInt = std::atoi(tok->str().c_str());
    literal.isInt = true;
  } else if (tok->name) {
    const std::string &name = tok->str();

    if (name == "false") {
      literal.asBool = false;
    } else if (name == "true") {
      literal.asBool = true;
    } else if (auto foundVar = source.variables.find(name);
               foundVar != source.variables.end()) {
      if (!foundVar->second.attrs.isConstExpr) {
        throw std::runtime_error("Referenced variable must be constexpr");
      }

      if (tok->next->op == '.') {
        tok = tok->next->next;
        auto &strDecl = source.structDecls.at(foundVar->second.typeName);

        if (auto foundMem = strDecl.members.find(tok->str());
            foundMem != strDecl.members.end()) {
          literal =
              strDecl.membersArr.at(foundMem->second).defaultValues.front();
        } else {
          throw std::runtime_error("structure `" + foundVar->second.typeName +
                                   "` doesn't contain member `" + tok->str() +
                                   "`");
        }
      } else {
        literal = foundVar->second.defaultValues.front();
      }
    } else {
      throw WrongLiteral("Expected literal value");
    }
  }

  if (negate) {
    literal.asBool = !literal.asInt;
    literal.isInt = false;
  }

  tok = tok->next;
  return literal;
}

int EvaluateExpression(const simplecpp::Token *&tok, Source &source) {
  Literal left = EvaluateConstexpr(tok, source);
  int result = left.asInt;

  if (!tok->startsWithOneOf("=!><*-+/%")) {
    return result;
  }

  enum OpType {
    EQ, // ==
    NE, // !=
    GT, // >
    GE, // >=
    LT, // <
    LE, // <=
    AD, // +
    SB, // -
    DV, // /
    MT, // *
    MD, // %
  };

  OpType opType;

  if (tok->str() == "==") {
    tok = tok->next;
    opType = EQ;
  } else if (tok->str() == "!=") {
    tok = tok->next;
    opType = NE;
  } else if (tok->op == '>') {
    tok = tok->next;
    opType = GT;
  } else if (tok->str() == ">=") {
    opType = GE;
    tok = tok->next;
  } else if (tok->op == '<') {
    tok = tok->next;
    opType = LT;
  } else if (tok->str() == "<=") {
    opType = LE;
    tok = tok->next;
  } else if (tok->op == '+') {
    tok = tok->next;
    opType = AD;
  } else if (tok->op == '-') {
    tok = tok->next;
    opType = SB;
  } else if (tok->op == '/') {
    tok = tok->next;
    opType = DV;
  } else if (tok->op == '*') {
    tok = tok->next;
    opType = MT;
  } else if (tok->op == '%') {
    tok = tok->next;
    opType = MD;
  }

  Literal right = EvaluateConstexpr(tok, source);

  switch (opType) {
  case EQ:
    result = left.asInt == right.asInt;
    break;
  case NE:
    result = left.asInt != right.asInt;
    break;
  case GT:
    result = left.asInt > right.asInt;
    break;
  case GE:
    result = left.asInt >= right.asInt;
    break;
  case LT:
    result = left.asInt < right.asInt;
    break;
  case LE:
    result = left.asInt <= right.asInt;
    break;
  case AD:
    result = left.asInt + right.asInt;
    break;
  case SB:
    result = left.asInt - right.asInt;
    break;
  case DV:
    result = left.asInt / right.asInt;
    break;
  case MT:
    result = left.asInt * right.asInt;
    break;
  case MD:
    result = left.asInt % right.asInt;
    break;

  default:
    break;
  }

  return result;
}

void ParseAttributes(const simplecpp::Token *&tok, Attributes &attrs,
                     Source &source) {
  if (tok->op == '[') {
    tok = tok->next;

    tok = ExpectOp(tok, '[');

    while (tok->op != ']') {
      ExpectName(tok, "attribute name");

      if (tok->str() == "binding" || tok->str() == "location") {
        tok = tok->next;

        tok = ExpectOp(tok, '(');
        attrs.binding = EvaluateExpression(tok, source);

        tok = ExpectOp(tok, ')');
      } else if (tok->str() == "disable") {
        tok = tok->next;

        tok = ExpectOp(tok, '(');
        attrs.disable = EvaluateExpression(tok, source);

        tok = ExpectOp(tok, ')');
      } else if (tok->str() == "instanced") {
        tok = tok->next;
        attrs.isInstanced =
            source.variables.at("isInstanced").defaultValues.front().asBool;
      } else {
        tok = tok->next;
      }

      if (tok->op != ']') {
        tok = ExpectOp(tok, ',');
      }
    }

    tok = ExpectOp(tok, ']');
    tok = ExpectOp(tok, ']');
  }
}

std::set<std::string_view> opaqueTypes{
    "atomic_uint",
    "sampler1D",
    "sampler2D",
    "sampler3D",
    "sampler2DRect",
    "sampler1DArray",
    "sampler2DArray",
    "samplerBuffer",
    "sampler2DMS",
    "sampler2DMSArray",
    "samplerCube",
    "sampler1DShadow",
    "sampler2DShadow",
    "samplerCubeShadow",
    "samplerCubeArray",
    "sampler1DArrayShadow",
    "sampler2DArrayShadow",
    "sampler2DRectShadow",
    "samplerCubeArrayShadow",
    "image1D",
    "image2D",
    "image3D",
    "image2DRect",
    "imageCube",
    "bufferImage",
    "image1DArray",
    "image2DArray",
    "imageCubeArray",
    "image2DMS",
    "image2DMSArray",
};

std::set<std::string_view> builtinTypes{
    "bool",
    "int",
    "uint",
    "float",
    "double",
    "atomic_uint",
    "vec2",
    "vec3",
    "vec4",
    "mat2",
    "mat3",
    "mat4",
    "mat2x2",
    "mat3x2",
    "mat4x2",
    "mat2x3",
    "mat3x3",
    "mat4x3",
    "mat2x4",
    "mat3x4",
    "mat4x4",
    "sampler1D",
    "sampler2D",
    "sampler3D",
    "sampler2DRect",
    "sampler1DArray",
    "sampler2DArray",
    "samplerBuffer",
    "sampler2DMS",
    "sampler2DMSArray",
    "samplerCube",
    "sampler1DShadow",
    "sampler2DShadow",
    "samplerCubeShadow",
    "samplerCubeArray",
    "sampler1DArrayShadow",
    "sampler2DArrayShadow",
    "sampler2DRectShadow",
    "samplerCubeArrayShadow",
    "bvec2",
    "bvec3",
    "bvec4",
    "image1D",
    "image2D",
    "image3D",
    "image2DRect",
    "imageCube",
    "bufferImage",
    "image1DArray",
    "image2DArray",
    "imageCubeArray",
    "image2DMS",
    "image2DMSArray",
};

bool IsBuildinType(std::string_view typeName) {
  if (builtinTypes.contains(typeName)) {
    return true;
  }

  if (typeName.front() == 'i' || typeName.front() == 'u' ||
      typeName.front() == 'd') {
    typeName.remove_prefix(1);
  }

  return builtinTypes.contains(typeName);
}

bool IsOpaqueType(std::string_view typeName) {
  if (opaqueTypes.contains(typeName)) {
    return true;
  }

  if (typeName.front() == 'i' || typeName.front() == 'u' ||
      typeName.front() == 'd') {
    typeName.remove_prefix(1);
  }

  return opaqueTypes.contains(typeName);
}

std::pair<std::string, Variable> AnalyzeVar(const simplecpp::Token *&tok,
                                            Source &source,
                                            bool needTypeName = true) {
  ExpectName(tok, "typename or `const`");
  Variable var;

  var.typeName = tok->str();
  if (var.typeName == "static") {
    var.attrs.isStatic = true;
    tok = tok->next;
    var.typeName = tok->str();
  }

  if (var.typeName == "const") {
    var.attrs.isConst = true;
    tok = tok->next;
    var.typeName = tok->str();
  } else if (var.typeName == "constexpr") {
    var.attrs.isConstExpr = true;
    tok = tok->next;
    var.typeName = tok->str();
  }

  if (needTypeName && !IsBuildinType(var.typeName)) {
    if (!source.structDecls.contains(var.typeName)) {
      throw UnexpectedTokenType(tok, "typename");
    }
  }
  if (needTypeName) {
    tok = tok->next;
  }

  ParseAttributes(tok, var.attrs, source);
  ExpectName(tok, "identifier name");

  std::string instanceName = tok->str();
  tok = tok->next;

  while (tok->op == '[') {
    tok = tok->next;
    var.attrs.isArray = true;

    if (tok->op != ']') {
      var.arraySizes.emplace_back(EvaluateExpression(tok, source));
    }

    tok = ExpectOp(tok, ']');
  }

  if (tok->op == '=') {
    tok = tok->next;
  }

  while (tok->op != ';') {
    if (tok->isOneOf("{}")) {
      Literal lit;
      lit.isEntry = true;
      var.defaultValues.emplace_back(lit);
      tok = tok->next;
      continue;
    }

    try {
      var.defaultValues.emplace_back(EvaluateExpression(tok, source));
    } catch (const WrongLiteral &) {
      ExpectName(tok, "typename");
      tok = tok->next;
      tok = ExpectOp(tok, '{');
      Literal lit;
      lit.isEntry = true;
      var.defaultValues.emplace_back(lit);
    }

    if (tok->op == ',') {
      tok = tok->next;
    }
  }

  tok = ExpectOp(tok, ';');

  return {instanceName, var};
}

void AnalyzeStruct(const simplecpp::Token *&tok, Source &source) {
  StructDecl decl;
  const simplecpp::Token *startTok = tok;

  std::string typeName;
  if (tok->name) {
    typeName = tok->str();
    tok = tok->next;
  }

  tok = ExpectOp(tok, '{');

  while (tok->op != '}') {
    auto [vname, mvar] = AnalyzeVar(tok, source);
    decl.members.emplace(vname, decl.membersArr.size());
    decl.membersArr.emplace_back(mvar);
  }

  tok = ExpectOp(tok, '}');

  if (tok->op == ';') {
    if (typeName.empty()) {
      throw UnexpectedTokenType(startTok,
                                "unnamed struct declaration is invalid");
    }
    source.structDecls.emplace(typeName, decl);
    tok = tok->next;
    return;
  }

  auto [sname, svar] = AnalyzeVar(tok, source, false);

  if (typeName.empty()) {
    typeName = sname;
    typeName.front() = std::toupper(sname.front());
    svar.typeName = typeName;
  } else {
    svar.typeName = typeName;
  }

  source.structDecls.emplace(typeName, decl);
  source.variables.emplace(sname, svar);
}

Source GatherDeclarations(simplecpp::TokenList &outputTokens) {
  Source source;

  for (const simplecpp::Token *tok = outputTokens.cfront(); tok;) {
    if (tok->str() == "class") {
      throw std::runtime_error("class keyword is unsupported, use struct");
    } else if (tok->str() == "struct") {
      tok = tok->next;
      AnalyzeStruct(tok, source);
    } else {
      const simplecpp::Token *beginTk = tok;
      try {
        source.variables.emplace(AnalyzeVar(tok, source));
      } catch (const std::runtime_error &e) {
        PrintError(e.what());
        tok = beginTk;
        ExpectName(tok, "typename");
        tok = tok->next;
        ExpectName(tok, "identifier name");

        std::string_view name = tok->str();
        tok = tok->next;
        tok = ExpectOp(tok, '(');

        Function func;
        func.headStart = beginTk;

        while (tok->op != ')') {
          tok = tok->next;
        }

        func.headEnd = tok;
        tok = tok->next;

        func.bodyStart = tok->next;
        tok = SkipLocalScope(tok);
        func.bodyEnd = tok ? tok->previous : tok;

        source.functions.emplace(name, func);
      }
    }
  }

  return source;
}

void AnalyzeFunction(Function &func, Source &source) {
  for (const simplecpp::Token *tok = func.bodyStart;
       tok && tok != func.bodyEnd;) {
    while (tok->str() == "if" && tok->next->str() == "constexpr") {
      simplecpp::Token *beginIf = const_cast<simplecpp::Token *>(tok);
      tok = tok->next;
      tok = tok->next;

      tok = ExpectOp(tok, '(');
      bool result = EvaluateExpression(tok, source);
      tok = ExpectOp(tok, ')');
      ExpectOp(tok, '{');

      if (result) {
        simplecpp::Token *mtok = const_cast<simplecpp::Token *>(tok);
        mtok->previous = beginIf->previous;
        beginIf->previous->next = mtok;
        if (beginIf == func.bodyStart) {
          func.bodyStart = tok;
        }

        tok = SkipLocalScope(tok);

        while (tok->str() == "else") {
          simplecpp::Token *elseBegin = const_cast<simplecpp::Token *>(tok);
          tok = tok->next;

          if (tok->str() == "if") {
            tok = tok->next;
            if (tok->str() == "constexpr") {
              tok = tok->next;
            }

            tok = ExpectOp(tok, '(');
            while (tok->op != ')') {
              tok = tok->next;
            }

            tok = tok->next;
          }

          tok = SkipLocalScope(tok);

          simplecpp::Token *mtok = const_cast<simplecpp::Token *>(tok);
          mtok->previous = elseBegin->previous;
          elseBegin->previous->next = mtok;
        }
        tok = mtok;
        break;
      } else {
        tok = SkipLocalScope(tok);

        if (tok->str() == "else") {
          tok = tok->next;
        }
        simplecpp::Token *mtok = const_cast<simplecpp::Token *>(tok);
        mtok->previous = beginIf->previous;
        beginIf->previous->next = mtok;
        if (beginIf == func.bodyStart) {
          func.bodyStart = tok;
        }
      }
    }

    tok = tok->next;
  }

  for (const simplecpp::Token *tok = func.bodyStart;
       tok && tok != func.bodyEnd;) {
    if (tok->str() == "static") {
      const simplecpp::Token *beginTok = tok;
      func.variables.emplace(AnalyzeVar(tok, source));

      simplecpp::Token *mtok = const_cast<simplecpp::Token *>(tok);
      mtok->previous = beginTok->previous;
      beginTok->previous->next = mtok;
      if (beginTok == func.bodyStart) {
        func.bodyStart = tok;
      }
      continue;
    }

    tok = tok->next;
  }
}

void WalkAndMark(Function &func, Source &source,
                 std::map<std::string, Variable> &variables, Stage stage) {
  for (auto &var : func.variables) {
    variables.emplace(var);
  }

  auto ApplyInstance = [](const simplecpp::Token *tok) {
    simplecpp::Token *mtok = const_cast<simplecpp::Token *>(tok);
    mtok->setstr("instances[gl_InstanceID]." + tok->str());
  };

  for (const simplecpp::Token *tok = func.bodyStart; tok != func.bodyEnd;) {
    if (tok->name) {
      // function calls and struct constructors
      if (tok->next->op == '(') {
        if (auto foundFunc = source.functions.find(tok->str());
            foundFunc != source.functions.end()) {
          func.subCalls.emplace(foundFunc->first);

          WalkAndMark(foundFunc->second, source, variables, stage);
        } else if (auto foundStruct = source.structDecls.find(tok->str());
                   foundStruct != source.structDecls.end()) {
          foundStruct->second.refs = stage;
        }
      } else {
        if (auto foundVar = source.variables.find(tok->str());
            foundVar != source.variables.end()) {
          foundVar->second.refs = stage;

          if (foundVar->second.attrs.isInstanced) {
            ApplyInstance(tok);
          }

          if (auto foundStruct =
                  source.structDecls.find(foundVar->second.typeName);
              foundStruct != source.structDecls.end()) {
            foundStruct->second.refs = stage;
          }
        } else if (foundVar = variables.find(tok->str());
                   foundVar != variables.end()) {
          foundVar->second.refs = stage;

          if (foundVar->second.attrs.isInstanced) {
            ApplyInstance(tok);
          }

          if (auto foundStruct =
                  source.structDecls.find(foundVar->second.typeName);
              foundStruct != source.structDecls.end()) {
            foundStruct->second.refs = stage;
          }
        }
      }
    }

    tok = tok->next;
  }
}

void WalkAndDump(Function &func, Source &source, std::ostream &ret,
                 std::set<std::string> &dumped) {
  for (auto &funcName : func.subCalls) {
    if (dumped.count(funcName) == 0) {
      dumped.emplace(funcName);
      WalkAndDump(source.functions.at(funcName), source, ret, dumped);
    }
  }

  for (const simplecpp::Token *tok = func.headStart; tok != func.headEnd;
       tok = tok->next) {
    if (tok->str() == "const") {
      continue;
    }
    if (tok->previous && tok->location.sameline(tok->previous->location)) {
      int thisOP = tok->op ? tok->op : tok->str()[0];
      int prevOP =
          tok->previous->op ? tok->previous->op : tok->previous->str()[0];

      if (std::isalnum(thisOP) && std::isalnum(prevOP)) {
        ret << ' ';
      }
    } else {
      ret << '\n';
    }

    ret << tok->str();
  }

  ret << ") {";

  for (const simplecpp::Token *tok = func.bodyStart; tok != func.bodyEnd;
       tok = tok->next) {
    if (tok->previous && tok->location.sameline(tok->previous->location)) {
      int thisOP = tok->op ? tok->op : tok->str()[0];
      int prevOP =
          tok->previous->op ? tok->previous->op : tok->previous->str()[0];

      if (std::isalnum(thisOP) && std::isalnum(prevOP)) {
        ret << ' ';
      }
    } else {
      ret << '\n';
    }

    ret << tok->str();
  }

  if (func.bodyEnd) {
    ret << "\n}";
  }
}

void DumpStructDecls(StructDecl &str, std::string name, Source &source,
                     std::ostream &ret, std::set<std::string> &dumped,
                     bool keyword = true) {
  for (auto var : str.membersArr) {
    auto foundStr = source.structDecls.find(var.typeName);

    if (dumped.count(var.typeName) || foundStr == source.structDecls.end()) {
      continue;
    }

    dumped.emplace(var.typeName);
    DumpStructDecls(foundStr->second, foundStr->first, source, ret, dumped);
  }

  if (keyword) {
    ret << "struct ";
  }

  ret << name << "{\n";

  for (size_t idx = 0; const Variable &var : str.membersArr) {
    ret << var.typeName << ' ';

    for (auto &[vname, vid] : str.members) {
      if (vid == idx) {
        ret << vname;
        break;
      }
    }

    if (var.attrs.isArray) {
      for (auto &s : var.arraySizes) {
        ret << '[' << s << ']';
      }

      if (var.arraySizes.empty()) {
        ret << "[]";
      }
    }

    ret << ";\n";
    idx++;
  }

  ret << "}";
  if (keyword) {
    ret << ";\n";
  }
}

std::set<std::string>
DumpVariable(const std::string &name, const Variable &var, std::ostream &ret,
             const std::set<std::string> &lastStageOutputs, Source &source,
             std::set<std::string> &dumped) {
  std::set<std::string> stageOutputs;
  auto foundStr = source.structDecls.find(var.typeName);
  const bool isStruct = foundStr != source.structDecls.end();

  if (isStruct) {
    bool isUniform = true;

    for (auto &mem : foundStr->second.membersArr) {
      if (mem.attrs.isArray && mem.arraySizes.empty()) {
        isUniform = false;
        break;
      }
    }

    if (var.attrs.binding > -1) {
      ret << "layout(binding=" << var.attrs.binding << ") ";
    }
    if (var.attrs.binding > -1 || !isUniform) {
      ret << (isUniform ? "uniform " : "readonly buffer ");
    }

    DumpStructDecls(foundStr->second, foundStr->first, source, ret, dumped,
                    var.attrs.binding < 0 && isUniform);
    dumped.emplace(var.typeName);

    if (var.attrs.isConstExpr) {
      ret << "const " << var.typeName << ' ' << name << '=' << var.typeName
          << '(';

      for (auto &mem : foundStr->second.membersArr) {
        if (mem.attrs.isArray) {
          ret << mem.typeName << "[](";

          for (auto &v : mem.defaultValues) {
            if (v.isEntry) {
              continue;
            }

            if (v.isInt) {
              ret << v.asInt;
            } else {
              ret << v.asBool;
            }
            ret << ',';
          }

          if (mem.defaultValues.size()) {
            ret.seekp(size_t(ret.tellp()) - 1);
          }

          ret << ')';
        } else {
          const Literal &v = mem.defaultValues.front();
          if (v.isInt) {
            ret << v.asInt;
          } else {
            ret << v.asBool;
          }
        }

        ret << ',';
      }

      ret.seekp(size_t(ret.tellp()) - 1);
      ret << ");\n";
      return stageOutputs;
    } else if (var.attrs.binding < 0 && isUniform) {
      source.uniformVariables.emplace(name, var);
      return stageOutputs;
    } else {
      ret << name;
      if (var.attrs.isArray) {
        for (int size : var.arraySizes) {
          ret << '[' << size << ']';
        }

        if (var.arraySizes.empty()) {
          ret << "[]";
        }
      }

      ret << ";\n";
      return stageOutputs;
    }
  }

  if (var.attrs.isConstExpr) {
    ret << "const " << var.typeName << ' ' << name;

    if (var.attrs.isArray) {
      ret << "[] = " << var.typeName << "[](";
      for (auto &v : var.defaultValues) {
        if (v.isInt) {
          ret << v.asInt;
        } else {
          ret << v.asBool;
        }
        ret << ',';
      }

      if (var.defaultValues.size()) {
        ret.seekp(size_t(ret.tellp()) - 1);
      }
      ret << ')';
    } else {
      ret << '=';
      const Literal &v = var.defaultValues.front();
      if (v.isInt) {
        ret << v.asInt;
      } else {
        ret << v.asBool;
      }
    }

    ret << ";\n";
  } else if (var.attrs.isStatic) {
    bool isInput = false;
    if (var.attrs.binding > -1) {
      ret << "layout(location=" << var.attrs.binding << ") ";
      isInput = true;
    } else if (var.defaultValues.size()) {
      const Literal &lit = var.defaultValues.front();
      ret << "layout(location=" << lit.asInt << ") ";
      isInput = true;
    } else {
      isInput = lastStageOutputs.contains(name);
    }

    if (var.attrs.isConst) {
      if (isInput) {
        ret << "in ";
      } else if (var.typeName.starts_with("sampler") ||
                 var.typeName.starts_with("isampler") ||
                 var.typeName.starts_with("usampler") ||
                 var.typeName.starts_with("image") ||
                 var.typeName.starts_with("iimage") ||
                 var.typeName.starts_with("uimage") ||
                 var.typeName.starts_with("atomic_uint")) {
        ret << "uniform ";
      } else {
        source.uniformVariables.emplace(name, var);
        return stageOutputs;
      }
    } else {
      ret << "out ";
      stageOutputs.emplace(name);
    }

    ret << var.typeName << ' ' << name;

    if (var.attrs.isArray) {
      for (int size : var.arraySizes) {
        ret << '[' << size << ']';
      }

      if (var.arraySizes.empty()) {
        ret << "[]";
      }
    }

    ret << ";\n";
  }

  return stageOutputs;
}

using Features =
    std::map<std::pair<std::string, std::string>, std::vector<int>>;

void ApplyFeatures(Source &source, const Features &features) {
  for (auto &[name, value] : features) {
    if (auto foundVar = source.variables.find(name.first);
        foundVar != source.variables.end()) {
      const std::string &typeName = foundVar->second.typeName;
      if (auto foundStr = source.structDecls.find(typeName);
          foundStr != source.structDecls.end()) {
        if (auto foundMem = foundStr->second.members.find(name.second);
            foundMem != foundStr->second.members.end()) {
          foundStr->second.membersArr.at(foundMem->second).defaultValues = {
              value.begin(), value.end()};
        }
      } else {
        foundVar->second.defaultValues = {value.begin(), value.end()};
      }
    }
  }
}

std::string MakeName(Source &source) {
  std::stringstream str;

  auto DumpVar = [&str](const Variable &var) {
    if (var.defaultValues.size() == 1) {
      str << var.defaultValues.front().asInt;
    } else {
      str << '[';
      for (auto &lit : var.defaultValues) {
        if (!lit.isEntry) {
          str << lit.asInt << '_';
        }
      }
      str << ']';
    }
  };

  for (auto &[name, var] : source.variables) {
    if (!var.attrs.isConstExpr) {
      continue;
    }
    str << '_' << name;

    if (auto foundStr = source.structDecls.find(var.typeName);
        foundStr != source.structDecls.end()) {
      str << '(';

      for (auto &[name, id] : foundStr->second.members) {
        const Variable &var = foundStr->second.membersArr.at(id);
        str << name << '_';
        DumpVar(var);
        str << '_';
      }

      str << ')';
    } else {
      str << '_';
      DumpVar(var);
    }
  }

  return str.str();
}

void DumpVariables(Source &source, std::ostream &ret, Stage stage) {
  std::vector<std::pair<std::string, Variable>> structUniforms;
  std::vector<std::pair<std::string, Variable>> vec4Uniforms;
  std::vector<std::pair<std::string, Variable>> vec3Uniforms;
  std::vector<std::pair<std::string, Variable>> vec2Uniforms;
  std::vector<std::pair<std::string, Variable>> scalarUniforms;

  auto DumpVar = [&ret](const Variable &var, const std::string &name) {
    ret << var.typeName << ' ' << name;

    if (var.attrs.isArray) {
      for (int size : var.arraySizes) {
        ret << '[' << size << ']';
      }

      if (var.arraySizes.empty()) {
        ret << "[]";
      }
    }

    ret << ";\n";
  };

  uint32_t numUsed = 0;

  for (auto &[name, var] : source.uniformVariables) {
    if (var.attrs.isInstanced) {
      continue;
    } else if (var.typeName == "vec4") {
      vec4Uniforms.emplace_back(name, var);
    } else if (var.typeName == "vec3") {
      vec3Uniforms.emplace_back(name, var);
    } else if (var.typeName == "vec2") {
      vec2Uniforms.emplace_back(name, var);
    } else if (var.typeName == "float" || var.typeName == "uint" ||
               var.typeName == "int") {
      scalarUniforms.emplace_back(name, var);
    } else {
      structUniforms.emplace_back(name, var);
    }

    numUsed += var.refs == stage;
  }

  auto DumpStruct = [&] {
    for (auto &[name, var] : structUniforms) {
      DumpVar(var, name);
    }
    for (auto &[name, var] : vec4Uniforms) {
      DumpVar(var, name);
    }
    for (auto &[name, var] : vec3Uniforms) {
      DumpVar(var, name);

      if (scalarUniforms.size() > 0) {
        auto &[sname, svar] = scalarUniforms.back();
        DumpVar(svar, sname);
        scalarUniforms.pop_back();
      }
    }
    for (auto &[name, var] : vec2Uniforms) {
      DumpVar(var, name);
    }
    for (auto &[name, var] : scalarUniforms) {
      DumpVar(var, name);
    }
  };

  if (numUsed > 0) {
    ret << "uniform ubProperties {\n";
    DumpStruct();
    ret << "};\n";
  }

  structUniforms.clear();
  vec4Uniforms.clear();
  vec3Uniforms.clear();
  vec2Uniforms.clear();
  scalarUniforms.clear();
  numUsed = 0;

  for (auto &[name, var] : source.uniformVariables) {
    if (!var.attrs.isInstanced) {
      continue;
    } else if (var.typeName == "vec4") {
      vec4Uniforms.emplace_back(name, var);
    } else if (var.typeName == "vec3") {
      vec3Uniforms.emplace_back(name, var);
    } else if (var.typeName == "vec2") {
      vec2Uniforms.emplace_back(name, var);
    } else if (var.typeName == "float" || var.typeName == "uint" ||
               var.typeName == "int") {
      scalarUniforms.emplace_back(name, var);
    } else {
      structUniforms.emplace_back(name, var);
    }

    numUsed += var.refs == stage;
  }

  if (numUsed > 0) {
    ret << "struct Instance {\n";
    DumpStruct();
    ret << "};\n";

    ret << "readonly buffer boInstances {\nInstance instances[];\n};\n";
  }
}

int main() {
  es::print::AddPrinterFunction(es::Print);
  const char *sourceName = "simple_model.cpp";

  std::ifstream iStr(sourceName);

  simplecpp::DUI dui;
  simplecpp::OutputList outputList;
  std::vector<std::string> files;

  simplecpp::TokenList rawtokens(iStr, files, sourceName, &outputList);
  auto included = simplecpp::load(rawtokens, files, dui, &outputList);
  simplecpp::TokenList outputTokens(files);
  simplecpp::preprocess(outputTokens, rawtokens, files, included, dui,
                        &outputList);

  for (const simplecpp::Output &output : outputList) {
    es::print::Get(output.type == simplecpp::Output::WARNING
                       ? es::print::MPType::WRN
                       : es::print::MPType::ERR)
        << output.location.file() << ':' << output.location.line << ": ";
    switch (output.type) {
    case simplecpp::Output::ERROR:
    case simplecpp::Output::WARNING:
      break;
    case simplecpp::Output::MISSING_HEADER:
      es::print::Get() << "missing header: ";
      break;
    case simplecpp::Output::INCLUDE_NESTED_TOO_DEEPLY:
      es::print::Get() << "include nested too deeply: ";
      break;
    case simplecpp::Output::SYNTAX_ERROR:
      es::print::Get() << "syntax error: ";
      break;
    case simplecpp::Output::PORTABILITY_BACKSLASH:
      es::print::Get() << "portability: ";
      break;
    case simplecpp::Output::UNHANDLED_CHAR_ERROR:
      es::print::Get() << "unhandled char error: ";
      break;
    case simplecpp::Output::EXPLICIT_INCLUDE_NOT_FOUND:
      es::print::Get() << "explicit include not found: ";
      break;
    }
    es::print::Get() << output.msg;
    es::print::FlushAll();
  }

  Source source = GatherDeclarations(outputTokens);

  ApplyFeatures(source, {
                            {
                                {"isInstanced", ""},
                                {1},
                            },
                            {
                                {"numBones", ""},
                                {128},
                            },
                        });

  for (auto &f : source.functions) {
    AnalyzeFunction(f.second, source);
  }

  std::set<std::string> lastStageOutputs;

  auto DumpStage = [&source, &lastStageOutputs](std::ostream &retHead,
                                                std::ostream &retBody,
                                                Stage stage) {
    std::map<std::string, Variable> variables;
    WalkAndMark(source.functions.at(StageToStr(stage)), source, variables,
                stage);
    std::set<std::string> stageOutputs;
    std::set<std::string> dumped;

    for (auto &[name, var] : source.variables) {
      if (var.refs == stage) {
        stageOutputs.merge(
            DumpVariable(name, var, retHead, lastStageOutputs, source, dumped));
      }
    }

    for (auto &[name, var] : variables) {
      stageOutputs.merge(
          DumpVariable(name, var, retHead, lastStageOutputs, source, dumped));
    }

    for (auto &[name, str] : source.structDecls) {
      if (str.refs == stage && dumped.count(name) == 0) {
        DumpStructDecls(str, name, source, retHead, dumped);
      }
    }

    Function &mainFunc = source.functions.at(StageToStr(stage));
    mainFunc.headStart->next->setstr("main");
    dumped.clear();
    WalkAndDump(mainFunc, source, retBody, dumped);

    return stageOutputs;
  };

  std::ostringstream retHeadVertex;
  std::ostringstream retBodyVertex;
  std::ostringstream retHeadFragment;
  std::ostringstream retBodyFragment;

  lastStageOutputs = DumpStage(retHeadVertex, retBodyVertex, Stage::vertex);
  lastStageOutputs =
      DumpStage(retHeadFragment, retBodyFragment, Stage::fragment);

  DumpVariables(source, retHeadVertex, Stage::vertex);
  DumpVariables(source, retHeadFragment, Stage::fragment);

  std::string generatedName = MakeName(source);

  {
    std::ofstream ost(sourceName + generatedName + ".vert");
    ost << "#version 450 core\n" << retHeadVertex.str() << retBodyVertex.str();
  }

  {
    std::ofstream ost(sourceName + generatedName + ".frag");
    ost << "#version 450 core\n"
        << retHeadFragment.str() << retBodyFragment.str();
  }

  simplecpp::cleanup(included);

  return 0;
}
