#pragma once

#include <stdint.h>

// Change this marker whenever the exported engine ABI or build options change.
#define LEAF_ABI_VERSION 1u
#define LEAF_HOST_BUILD "e19f406-leaf-2"

struct LeafHost {
    uint32_t abiVersion;
    const char *packageDirectory; // UTF-8 absolute path; valid until shutdown.
    void (*log)(const char *message);
    // Optional debug hooks supplied by the reVC host.
    void (*setScmEnabled)(bool enabled);
    void (*spawnPlayerOutside)();
};

struct LeafMod {
    uint32_t abiVersion;
    bool (*initialise)(const LeafHost *host);
    void (*update)();
    void (*draw)();
    void (*shutdown)();
};

typedef const LeafMod *(*LeafGetModFunction)();

enum LeafRenderStage { LEAF_SKY=0, LEAF_WATER_BEGIN=1, LEAF_WATER_END=2, LEAF_WORLD_END=3, LEAF_PRE_RENDER=4,
    LEAF_BEFORE_VEHICLES=5, LEAF_BACKGROUND_EFFECTS=6 };
// Optional export. Return true from LEAF_SKY to replace the stock cloud layer.
typedef bool (*LeafRenderPassFunction)(uint32_t stage);
// Optional pose export: apply=false asks whether this ped needs an override.
// The host restores the complete animated pose immediately after drawing.
typedef bool (*LeafPedPoseFunction)(void *ped, bool apply);

// Every native package exports: extern "C" __declspec(dllexport)
// const LeafMod *LeafGetMod();
