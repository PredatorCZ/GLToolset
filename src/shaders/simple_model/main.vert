void main() {
    vec3 modelSpace = GetModelSpace(inPos);
    GetTSNormal();
    ComputeLights(modelSpace);
    SetPosition(modelSpace);
    SetUVs();
}
