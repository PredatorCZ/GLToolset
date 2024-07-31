void main() {
    vec3 modelSpace = GetModelSpace(inPos);
    ComputeLights(modelSpace);
    SetPosition(modelSpace);
    SetUVs();
}
