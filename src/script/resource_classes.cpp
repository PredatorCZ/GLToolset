#include "resource_classes.hpp"
#include "common/transform.hpp"
#include "squirrel.h"

const glm::vec2 *GetVec2(HSQUIRRELVM v, SQInteger idx) {
  SQUserPointer self = nullptr;
  sq_getinstanceup(v, idx, &self, SQVEC2_TAG, SQFalse);
  return static_cast<const glm::vec2 *>(self);
}

const glm::vec4 *GetVec4(HSQUIRRELVM v, SQInteger idx) {
  SQUserPointer self = nullptr;
  sq_getinstanceup(v, idx, &self, SQVEC4_TAG, SQFalse);
  return static_cast<const glm::vec4 *>(self);
}

const glm::vec3 *GetVec3(HSQUIRRELVM v, SQInteger idx) {
  SQUserPointer self = nullptr;
  sq_getinstanceup(v, idx, &self, SQVEC3_TAG, SQFalse);
  return static_cast<const glm::vec3 *>(self);
}

const glm::quat *GetQuat(HSQUIRRELVM v, SQInteger idx) {
  SQUserPointer self = nullptr;
  sq_getinstanceup(v, idx, &self, SQROTATION_TAG, SQFalse);
  return static_cast<const glm::quat *>(self);
}

namespace color {
SQInteger destructor(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size)) {
  delete (Color *)p;
  return 1;
}

SQInteger constructor(HSQUIRRELVM v) {
  SQUserPointer self = nullptr;
  if (SQ_FAILED(sq_getinstanceup(v, 1, &self, SQCOLOR_TAG, SQFalse))) {
    return sq_throwerror(v, _SC("invalid type tag"));
  }
  if (self != nullptr) {
    return sq_throwerror(v, _SC("invalid Color object"));
  }

  Color *rex = new Color{.value = -1U};
  const SQInteger numOps = sq_gettop(v);

  if (numOps > 1) {
    SQInteger tmp;
    sq_getinteger(v, 2, &tmp);
    rex->r = tmp;
    sq_getinteger(v, 3, &tmp);
    rex->g = tmp;
    sq_getinteger(v, 4, &tmp);
    rex->b = tmp;
    sq_getinteger(v, 5, &tmp);
    rex->a = tmp;
  }

  sq_setinstanceup(v, 1, rex);
  sq_setreleasehook(v, 1, destructor);
  return SQ_OK;
}

void Register(HSQUIRRELVM v) {
  /* +1 */ sq_pushstring(v, _SC(SQCOLOR_TAG), -1);
  /* +1 */ sq_newclass(v, SQFalse);
  /* +0 */ sq_settypetag(v, -1, SQCOLOR_TAG);

  /* +1 */ sq_pushstring(v, "constructor", 11);
  /* +1 */ sq_newclosure(v, constructor, 0);
  /* +0 */ sq_setparamscheck(v, 0, "xiiii");
  /* +0 */ sq_setnativeclosurename(v, -1, "constructor");
  /* -2 */ sq_newslot(v, -3, SQFalse);

  sq_pushstring(v, "COLOR_WHITE", -1);
  sq_push(v, -2);
  sq_call(v, 0, SQTrue, SQTrue);
  sq_remove(v, -2);
  sq_newslot(v, -5, SQTrue);

  /* -2 */ sq_newslot(v, -3, SQFalse);
}
} // namespace color

namespace quat {
SQInteger destructor(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size)) {
  delete (glm::quat *)p;
  return 1;
}

SQInteger constructor(HSQUIRRELVM v) {
  SQUserPointer self = nullptr;
  if (SQ_FAILED(sq_getinstanceup(v, 1, &self, SQROTATION_TAG, SQFalse))) {
    return sq_throwerror(v, _SC("invalid type tag"));
  }
  if (self != nullptr) {
    return sq_throwerror(v, _SC("invalid rotation object"));
  }

  glm::quat *rex = new glm::quat{};
  const SQInteger numOps = sq_gettop(v);

  if (numOps > 1) {
    glm::vec3 euler;
    sq_getfloat(v, 2, &euler.x);
    sq_getfloat(v, 3, &euler.y);
    sq_getfloat(v, 4, &euler.z);
    *rex = glm::quat(glm::radians(euler));
  }

  sq_setinstanceup(v, 1, rex);
  sq_setreleasehook(v, 1, destructor);
  return SQ_OK;
}

void Register(HSQUIRRELVM v) {
  /* +1 */ sq_pushstring(v, _SC(SQROTATION_TAG), -1);
  /* +1 */ sq_newclass(v, SQFalse);
  /* +0 */ sq_settypetag(v, -1, SQROTATION_TAG);

  /* +1 */ sq_pushstring(v, "constructor", 11);
  /* +1 */ sq_newclosure(v, constructor, 0);
  /* +0 */ sq_setparamscheck(v, 0, "xnnn");
  /* +0 */ sq_setnativeclosurename(v, -1, "constructor");
  /* -2 */ sq_newslot(v, -3, SQFalse);

  sq_pushstring(v, "DEFAULT_ROTATION", -1);
  sq_push(v, -2);
  sq_call(v, 0, SQTrue, SQTrue);
  sq_remove(v, -2);
  sq_newslot(v, -5, SQTrue);

  /* -2 */ sq_newslot(v, -3, SQFalse);
}
} // namespace quat

namespace vec2 {
SQInteger destructor(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size)) {
  delete (glm::vec2 *)p;
  return 1;
}

SQInteger constructor(HSQUIRRELVM v) {
  SQUserPointer self = nullptr;
  if (SQ_FAILED(sq_getinstanceup(v, 1, &self, SQVEC2_TAG, SQFalse))) {
    return sq_throwerror(v, _SC("invalid type tag"));
  }
  if (self != nullptr) {
    return sq_throwerror(v, _SC("invalid vec2 object"));
  }

  glm::vec2 *rex = new glm::vec2{};
  const SQInteger numOps = sq_gettop(v);

  if (numOps == 2) {
    sq_getfloat(v, 2, &rex->x);
    rex->y = rex->x;
  } else if (numOps > 2) {
    sq_getfloat(v, 2, &rex->x);
    sq_getfloat(v, 3, &rex->y);
  }

  sq_setinstanceup(v, 1, rex);
  sq_setreleasehook(v, 1, destructor);
  return SQ_OK;
}

void Register(HSQUIRRELVM v) {
  /* +1 */ sq_pushstring(v, _SC(SQVEC2_TAG), -1);
  /* +1 */ sq_newclass(v, SQFalse);
  /* +0 */ sq_settypetag(v, -1, SQVEC2_TAG);

  /* +1 */ sq_pushstring(v, "constructor", 11);
  /* +1 */ sq_newclosure(v, constructor, 0);
  /* +0 */ sq_setparamscheck(v, 0, "xnn");
  /* +0 */ sq_setnativeclosurename(v, -1, "constructor");
  /* -2 */ sq_newslot(v, -3, SQFalse);

  sq_pushstring(v, "DEFAULT_VECTOR2", -1);
  sq_push(v, -2);
  sq_call(v, 0, SQTrue, SQTrue);
  sq_remove(v, -2);
  sq_newslot(v, -5, SQTrue);

  /* -2 */ sq_newslot(v, -3, SQFalse);
}
} // namespace vec2

namespace vec3 {
SQInteger destructor(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size)) {
  delete (glm::vec3 *)p;
  return 1;
}

SQInteger constructor(HSQUIRRELVM v) {
  SQUserPointer self = nullptr;
  if (SQ_FAILED(sq_getinstanceup(v, 1, &self, SQVEC3_TAG, SQFalse))) {
    return sq_throwerror(v, _SC("invalid type tag"));
  }
  if (self != nullptr) {
    return sq_throwerror(v, _SC("invalid vec3 object"));
  }

  glm::vec3 *rex = new glm::vec3{};
  const SQInteger numOps = sq_gettop(v);

  if (numOps == 2) {
    sq_getfloat(v, 2, &rex->x);
    rex->y = rex->x;
    rex->z = rex->x;
  } else if (numOps > 2) {
    sq_getfloat(v, 2, &rex->x);
    sq_getfloat(v, 3, &rex->y);
    sq_getfloat(v, 4, &rex->z);
  }

  sq_setinstanceup(v, 1, rex);
  sq_setreleasehook(v, 1, destructor);
  return SQ_OK;
}

void Register(HSQUIRRELVM v) {
  /* +1 */ sq_pushstring(v, _SC(SQVEC3_TAG), -1);
  /* +1 */ sq_newclass(v, SQFalse);
  /* +0 */ sq_settypetag(v, -1, SQVEC3_TAG);

  /* +1 */ sq_pushstring(v, "constructor", 11);
  /* +1 */ sq_newclosure(v, constructor, 0);
  /* +0 */ sq_setparamscheck(v, 0, "xnnn");
  /* +0 */ sq_setnativeclosurename(v, -1, "constructor");
  /* -2 */ sq_newslot(v, -3, SQFalse);

  sq_pushstring(v, "DEFAULT_VECTOR3", -1);
  sq_push(v, -2);
  sq_call(v, 0, SQTrue, SQTrue);
  sq_remove(v, -2);
  sq_newslot(v, -5, SQTrue);

  /* -2 */ sq_newslot(v, -3, SQFalse);
}
} // namespace vec3

namespace vec4 {
SQInteger destructor(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size)) {
  delete (glm::vec4 *)p;
  return 1;
}

SQInteger constructor(HSQUIRRELVM v) {
  SQUserPointer self = nullptr;
  if (SQ_FAILED(sq_getinstanceup(v, 1, &self, SQVEC4_TAG, SQFalse))) {
    return sq_throwerror(v, _SC("invalid type tag"));
  }
  if (self != nullptr) {
    return sq_throwerror(v, _SC("invalid vec4 object"));
  }

  glm::vec4 *rex = new glm::vec4{};
  const SQInteger numOps = sq_gettop(v);

  if (numOps > 1) {
    sq_getfloat(v, 2, &rex->x);
    rex->y = rex->x;
    rex->z = rex->x;
    rex->w = rex->x;
  } else if (numOps > 2) {
    sq_getfloat(v, 2, &rex->x);
    sq_getfloat(v, 3, &rex->y);
    sq_getfloat(v, 4, &rex->z);
    sq_getfloat(v, 5, &rex->w);
  }

  sq_setinstanceup(v, 1, rex);
  sq_setreleasehook(v, 1, destructor);
  return SQ_OK;
}

void Register(HSQUIRRELVM v) {
  /* +1 */ sq_pushstring(v, _SC(SQVEC4_TAG), -1);
  /* +1 */ sq_newclass(v, SQFalse);
  /* +0 */ sq_settypetag(v, -1, SQVEC4_TAG);

  /* +1 */ sq_pushstring(v, "constructor", 11);
  /* +1 */ sq_newclosure(v, constructor, 0);
  /* +0 */ sq_setparamscheck(v, 0, "xnnnn");
  /* +0 */ sq_setnativeclosurename(v, -1, "constructor");
  /* -2 */ sq_newslot(v, -3, SQFalse);

  sq_pushstring(v, "DEFAULT_VECTOR4", -1);
  sq_push(v, -2);
  sq_call(v, 0, SQTrue, SQTrue);
  sq_remove(v, -2);
  sq_newslot(v, -5, SQTrue);

  /* -2 */ sq_newslot(v, -3, SQFalse);
}
} // namespace vec4

namespace dualquat {
static SQInteger destructor(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size)) {
  delete (glm::dualquat *)p;
  return 1;
}

static SQInteger constructor(HSQUIRRELVM v) {
  SQUserPointer self = nullptr;
  if (SQ_FAILED(sq_getinstanceup(v, 1, &self, SQTRANSFORM_TAG, SQFalse))) {
    return sq_throwerror(v, _SC("invalid type tag"));
  }
  if (self != nullptr) {
    return sq_throwerror(v, _SC("invalid transform object"));
  }

  glm::dualquat *rex = new glm::dualquat{glm::quat(1, 0, 0, 0)};
  const SQInteger numOps = sq_gettop(v);

  if (numOps > 1) {
    glm::dualquat(*GetQuat(v, 2), *GetVec3(v, 3));
  }

  sq_setinstanceup(v, 1, rex);
  sq_setreleasehook(v, 1, destructor);
  return SQ_OK;
}

void Register(HSQUIRRELVM v) {
  /* +1 */ sq_pushstring(v, _SC(SQTRANSFORM_TAG), -1);
  /* +1 */ sq_newclass(v, SQFalse);
  /* +0 */ sq_settypetag(v, -1, SQTRANSFORM_TAG);

  /* +1 */ sq_pushstring(v, "constructor", 11);
  /* +1 */ sq_newclosure(v, constructor, 0);
  /* +0 */ sq_setparamscheck(v, 0, "xxx");
  /* +0 */ sq_setnativeclosurename(v, -1, "constructor");
  /* -2 */ sq_newslot(v, -3, SQFalse);

  sq_pushstring(v, "DEFAULT_TRANSFORM", -1);
  sq_push(v, -2);
  sq_call(v, 0, SQTrue, SQTrue);
  sq_remove(v, -2);
  sq_newslot(v, -5, SQTrue);

  /* -2 */ sq_newslot(v, -3, SQFalse);
}
} // namespace dualquat

void RegisterResourceClasses(HSQUIRRELVM v) {
  color::Register(v);
  vec2::Register(v);
  vec3::Register(v);
  vec4::Register(v);
  quat::Register(v);
  dualquat::Register(v);
}
