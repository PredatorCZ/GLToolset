void main() {
    vec3 modelSpace = GetModelSpace();
    GetTSNormal();
    ComputeLights(modelSpace);
    SetPosition(modelSpace);
    SetUVs();
}
