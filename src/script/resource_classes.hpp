inline char SQCOLOR_TAG[] = "color";
inline char SQTRANSFORM_TAG[] = "transform";
inline char SQROTATION_TAG[] = "rotation";
inline char SQVEC2_TAG[] = "vec2";
inline char SQVEC3_TAG[] = "vec3";
inline char SQVEC4_TAG[] = "vec4";

typedef struct SQVM* HSQUIRRELVM;

void RegisterResourceClasses(HSQUIRRELVM v);
