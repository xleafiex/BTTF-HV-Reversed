#pragma once

namespace LeafMods {
bool LoadDebugScript(void *destination, unsigned int capacity);
bool DebugPackagePresent();
void Initialise();
void Update();
void Draw();
bool RenderPass(unsigned int stage);
bool PedPose(void *ped, bool apply);
void Shutdown();

// Retain named donor-model frames and suppress the stock generated wheels.
void SetRailWheelVehicle(void *vehicle);
bool HasRailWheels(const void *vehicle);
bool SuppressWheelMark(uintptr_t id);
bool PreserveVehicleFrames(int modelId);
void RegisterPreservedVehicleModel(int modelId, bool preserve);
}
