void main() {
    vec3 modelSpace = GetModelSpace(inPos);
#ifdef NUM_LIGHTS
    ComputeLights(modelSpace);
#endif
    SetPosition(modelSpace);
    SetUVs();
}
