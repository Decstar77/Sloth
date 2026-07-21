# Agent Task Queue

Backlog for autonomous/scheduled agents. Each run: pick the first task under
**Open** with no `claimed:` line, add a `claimed: <branch-name> (<date>)` line
under it, do the work on that branch, push it, then move the task to
**Done** with a one-line summary and the branch name (leave PR review to a
human — do not merge to `main` from an unattended run).

Keep tasks small enough to land in a single sitting (a few hours of agent
time). Split anything bigger into multiple entries here rather than one
open-ended task.

Scope for now: **engine only** (`src/engine`, `sloth` namespace). Nothing
in `src/game`/`dust` unless a task says otherwise.

## Open

- [ ] Extend `Vertex` (`renderer/sloth_static_mesh.h`) with normal and UV
  attributes (currently position + color only), and update every
  `Geometry::Create*` builder (`renderer/sloth_geometry.h/.cpp`) to emit
  correct normals/UVs. This is a prerequisite for lighting (shadow mapping
  needs normals) and for textured/model rendering (needs UVs) — do this
  first, before either of those tasks. Touches every existing call site
  that constructs a `Vertex` or `MeshData`, so expect churn in `DustGame`'s
  mesh setup too even though the task is engine-scoped.

- [ ] Basic material/texture support for `StaticMesh`/`RenderModel`
  rendering — bind a `Texture` (`renderer/sloth_texture.h`, already exists)
  per draw and sample it in the shader instead of (or blended with)
  vertex color. Depends on the Vertex UV task above.

- [ ] Basic shadow mapping (single directional light, one shadow map,
  depth-only render pass + PCF or similar filtering in the main shader).
  Depends on the Vertex normal task above (need normals for N·L lighting
  to make shadowing visible/meaningful). Lives alongside the existing
  `renderer/` code (`sloth_shader`, `sloth_static_mesh`, `sloth_camera`).
  Follow-up task below upgrades this to cascaded shadow maps — keep the
  single-map version simple and correct first rather than building
  CSM-ready abstractions prematurely.

- [ ] Cascaded shadow mapping, building on the basic shadow mapping task
  above (depends on it landing first).

- [ ] Model loading via Assimp (new dependency — vendor it the same way
  existing third-party libs under the engine are integrated, wire into
  premake5.lua). Should produce `sloth::MeshData`-compatible data (see
  `renderer/sloth_geometry.h`) including normals/UVs, for the follow-up
  rendering task to consume. No renderer changes in this task — loading
  only.

- [ ] Model rendering, building on the Assimp loading task above (depends
  on it landing first). Wire loaded meshes into the existing
  `StaticMesh`/`RenderModel` path (`renderer/sloth_static_mesh.h`,
  `renderer/sloth_render_model.h`).

- [ ] Skybox rendering — basic cube skybox (static cubemap or procedural,
  rendered behind everything via depth trick / drawn last with depth
  writes disabled). No dynamic sky/atmospheric scattering yet, just get a
  box on screen.

- [ ] Frustum culling — derive the view frustum from `Camera`
  (`renderer/sloth_camera.h`) and cull entities/draw calls outside it
  before submitting them. Needed before the world gets big; keep it
  simple (per-object AABB vs. frustum planes), no spatial acceleration
  structure in this task.

- [ ] Instanced rendering path for `StaticMesh` — a way to draw many
  copies of the same mesh (different transforms) in one draw call
  (`glDrawElementsInstanced` + a per-instance transform buffer), for
  repeated geometry like crates/rocks/debris. Additive to the existing
  single-draw `StaticMesh::Draw()`, not a replacement.

- [ ] Basic post-processing pipeline — render the scene to an offscreen
  framebuffer, then a full-screen-quad pass applies at least tonemapping
  (existing rendering is presumably writing straight to the default
  framebuffer with no color grading step). Structure it so more passes
  (bloom, etc.) can be added later without redesigning the framebuffer
  setup, but only implement tonemapping now.

- [ ] Basic serialization — JSON (or similar) read/write helpers in the
  engine (`core/`), following the same style as `sloth_string.h`/
  `sloth_arena.h` (arena-friendly where practical, `SL_ASSERT`/`SL_LOG_*`
  for error handling, no exceptions). Scope this task to the
  parse/serialize primitives only (load a file into a DOM-like structure,
  read typed fields out of it, write one back out) — no game-specific
  schema or save/load system yet, that's a future `dust`-scoped task.

## Done

_(none yet)_
