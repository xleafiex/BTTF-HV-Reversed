# Ice material

The frost overlay uses the original donor geometry and donor frost texture as fallback. IceGeometry.h builds a private copy with area-weighted surface normals and 1.5 mm separation. The original effect normals were unsuitable for triplanar mapping; noisy tessellation and arbitrary vertex displacement are removed.

ice-surface.png is an original AI-generated fused-ice surface texture. ice-surface.rgba is the identical RGBA pixel data prefixed with two little-endian uint32 dimensions. Both are packaged inside delorean.leaf. IceSurface.h uploads it with mipmaps. frost.frag samples it in object space, using broad filtered detail for coverage and finer detail for relief. Windows use a separate low-opacity variant. The normal donor cold timer drives nonuniform thaw.

FrostShader.h attaches the effect at the GL3 pipeline draw callback, preserving the game's delayed alpha drawing. The additional texture is released on module shutdown. If it cannot load, the donor shader is used.

Validation: work/frost-preview.cpp renders the actual donor model and the same geometry helper, detail loader, vertex shader and fragment shader in a hidden standalone librw window. It does not launch reVC. It approximates lighting and component selection; it does not reproduce the game vehicle reflection pipeline, so it is a material check, not an in-game screenshot.

Previous installed version is retained in E:/delorean-smooth-ice.leaf. Native source and resources are rebuilt through the existing package_delorean.py workflow.
