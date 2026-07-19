# Immediate-Mode GUI System

Sloth needs a GUI system powerful enough to carry a Kenshi/X4-style sandbox
(dense data tables, inventories, faction/trade UIs, HUD, world-space
labels) — this is meant to be one of the most load-bearing parts of the
engine, not an afterthought. It's immediate-mode: no retained widget
objects, but persistent per-widget state (hover/active/focus, scroll
position, etc.) survives across frames via a hashed ID, the same shape as
Dear ImGui / Casey Muratori's original IMGUI articles.

This doc tracks what's built, what's next, and the design decisions behind
each piece, so future work (by a human or by Claude in a fresh session) has
the reasoning, not just the code.

## Status

### Done

1. **Quad batcher + orthographic projection shader**
   `renderer/sloth_gui_renderer.h/.cpp` — `GuiRenderer` batches screen-space
   quads into one instanced draw call per `Flush()`. `MakeScreenProjection()`
   builds the y-down orthographic matrix every screen-space draw call uses
   (text, shapes, images all share this convention).

2. **SDF rounded-rect / circle shapes**
   Same file. Every shape is a rounded-box signed distance field (Inigo
   Quilez's `sdRoundBox`) evaluated per-fragment — resolution-independent
   edges, corner radius, and an inset-SDF border/stroke, all from one
   analytic distance. A circle is just a square quad with
   `cornerRadius == half its size`. `DrawRect()` / `DrawCircle()`.

3. **Texture class + textured quads**
   `renderer/sloth_texture.h/.cpp` — `Texture` wraps a GL 2D texture,
   normalized to RGBA8, loadable from file (`stb_image`) or from an in-memory
   pixel buffer. `GuiRenderer::DrawImage()` reuses the same rounded-box SDF
   clip as `DrawRect()` so icons/portraits can be rounded/circular for free.
   Images batch separately from shapes (one bound texture per draw call);
   `Flush()` groups *consecutive* same-texture `DrawImage()` calls into one
   instanced draw, breaking to a new draw whenever the texture changes.
   Known limitation: shapes flush before images, so strict call-order
   interleaving between a rect and an image isn't respected yet — needs the
   layered draw list from item 8 below.

4. **Widget ID system + persistent state table**
   `gui/sloth_gui_id.h/.cpp` — `GuiId` (32-bit FNV-1a), `HashGuiId()` for
   scoped hashing, `StripGuiLabelHash()` for the Dear-ImGui-style
   `"Save##File"` display-vs-hash convention.
   `gui/sloth_gui_context.h/.cpp` — `GuiContext`: the ID stack
   (`PushId`/`PopId`/`GetId`), the three interaction flags every widget will
   be built on (`hotId`/`activeId`/`focusedId` — active locks out hot, last
   `SetHot()` in a frame wins so later-drawn/topmost widgets win ties), mouse
   state snapshotted from `Input`, and `GuiStorage` (a generic per-`GuiId`
   bool/int/float/vec2 table for widget-specific persistent state: scroll
   offsets, drag anchors, open/closed flags, ...).
   `GuiContext` is an explicit, non-singleton object (constructed and owned
   by the caller, like `GuiRenderer`/`TextRenderer`) — deliberate, matches
   how the rest of the renderer stack is built, no hidden globals.

5. **Clip-rect / scissor**
   `renderer/sloth_gui_rect.h` — `GuiRect`, `IntersectGuiRect()`,
   `UnboundedGuiRect()`. Rather than a real `glScissor` (which would force a
   batch break on every clip change, like a texture switch does), each
   queued shape/image instance carries its own `ClipMin`/`ClipMax` and the
   fragment shader discards outside it — clipping is free per-instance, no
   batching penalty. `GuiRenderer::PushClipRect`/`PopClipRect` (rendering)
   and `GuiContext::PushClipRect`/`PopClipRect`/`IsPointVisible()`
   (hit-testing) are two parallel stacks widget code drives together —
   deliberately not merged into one shared stack, since one's a rendering
   concern and the other's an interaction concern living in different
   classes. `GuiContext` asserts its clip stack balances at `EndFrame`;
   `GuiRenderer`'s clip stack is persistent state independent of `Flush()`
   (a widget may flush its own background mid-region while a panel clip is
   still pushed - see item 6b), with over-pop caught by `PopClipRect` and
   overall balance riding on the lockstep `GuiContext` side.

All five are demoed by hand in `src/sandbox/src/main.cpp` and have been
visually verified running.

6. **Button, Checkbox, Label widgets**
   `gui/sloth_gui_widgets.h/.cpp` — the hot/active/click pattern that used to
   be hand-rolled inline in `main.cpp` is now extracted into reusable
   functions:
   - `bool Button(GuiContext&, GuiRenderer&, TextRenderer&, const Font&, GlyphCache&, StringView label, glm::vec2 min, glm::vec2 max, const glm::mat4& viewProjection)`
     — returns true on the frame it's released while still hovered (a
     click, not a press).
   - `bool Checkbox(GuiContext&, GuiRenderer&, TextRenderer&, const Font&, GlyphCache&, StringView label, glm::vec2 pos, bool& value, const glm::mat4& viewProjection)`
     — toggles `value` in place and returns true the frame it changed;
     `value` lives in caller state, not `GuiStorage`.
   - `void Label(TextRenderer&, const Font&, GlyphCache&, StringView text, glm::vec2 baselinePos, f32 pixelHeight, const glm::vec4& color, const glm::mat4& viewProjection)`
     — thin wrapper around `TextRenderer::DrawText`, no `GuiId`/interaction
     state of its own (dropped the `GuiRenderer&` param the original sketch
     had, since nothing in it needs to draw a shape).
   - Both `Button`/`Checkbox` internally `Flush()` the `GuiRenderer` after
     queuing their background rect(s) and before drawing their label —
     `TextRenderer::DrawText` issues its own immediate draw call (unlike
     `GuiRenderer`'s queued shapes), so without an internal Flush the label
     could render before the background rect that's supposed to sit behind
     it. This is the same ordering caveat item 8 (draw-layer sorting) is
     meant to solve properly; widgets paper over it locally for now rather
     than waiting on that.
   - No text-measurement/centering yet — labels are left-aligned with a
     fixed padding, vertically offset by a hardcoded baseline-from-midpoint
     constant (`TextBaselineMidpointOffset` in the .cpp) rather than real
     font ascent/descent math. Revisit once `Font`/`TextRenderer` expose
     per-call metrics, or once item 7's layout solver needs real intrinsic
     sizes anyway.
   - Colors/sizing are hardcoded constants in `sloth_gui_widgets.cpp`
     (no `GuiStyle` yet — that's item 12); expect these signatures to
     shrink once a style stack exists.
   - `main.cpp`'s demo button block now calls `Button()`/`Label()`, plus a
     new `Checkbox()` demo, exercising all three against the sandbox's real
     font/text-renderer.
6b. **Panel/window (`BeginPanel`/`EndPanel`)**
   `gui/sloth_gui_widgets.h/.cpp` — a draggable, clippable container, the
   remaining piece of item 6. `BeginPanel(... label, defaultPos, size ...)`
   returns a `PanelResult{ contentMin, contentMax }` (the padded, title-bar-
   excluded content rect the caller places widgets into); `EndPanel()` closes
   it. Paired like `PushClipRect`/`PopClipRect`, both once per frame.
   - **Persistent position**: the panel's top-left lives in `GuiStorage` as a
     `vec2` keyed by the panel's `GuiId`, so a dragged panel stays put across
     frames; `defaultPos` only seeds the slot the first frame that ID is seen.
   - **Dragging**: grabbed on the title bar (`hot` + press within the bar →
     `active`, storing a mouse-to-origin `dragAnchor` vec2 in `GuiStorage`
     under `HashGuiId("##drag", id)` so the corner doesn't snap to the
     cursor). A live drag stays `active` wherever the pointer wanders —
     `activeId` locks out every other widget's `SetHot`, so a fast drag can't
     slip off the bar, and child widgets inside can't be hovered mid-drag.
     Position-drag only; no resize handles yet.
   - **Input capture**: hovering anywhere over the panel claims `hot` (so
     `WantsMouseInput` is true and gameplay code below won't react to a click
     that lands on the panel). A child widget drawn later overrides this via
     last-`SetHot`-wins, so it only captures the panel's empty regions.
   - **Clipping**: pushes the content rect on both the `GuiRenderer` clip
     stack (per-fragment) and the `GuiContext` clip stack (hit-testing), kept
     in lockstep so an overflowing widget is neither drawn nor clickable past
     the edge. `EndPanel` flushes queued content *before* popping so it
     inherits the panel's clip, then pops both.
   - **Draw ordering**: `BeginPanel` draws its body + title-bar rects and
     `Flush()`es immediately, so content (and `TextRenderer`, which draws
     immediately) lands on top — same local workaround Button/Checkbox use
     pending draw layers (item 8).
   - **Known gaps**: `TextRenderer` doesn't clip, so a widget label
     overflowing the panel isn't cut off (only its background rect is);
     no click-to-front z-ordering between overlapping panels (needs draw
     layers, item 8); no resize, no collapse/close button.
   - The sandbox's inventory window (`main.cpp`, toggled with `I`) is now a
     real `BeginPanel`/`EndPanel` wrapping the slot-button grid — draggable by
     its title bar, exercising the panel against live content.

### Not started

Everything below is ordered by dependency, not necessarily priority — items
within a step can be reordered based on what Dust actually needs first.

## 7. Auto-layout stack (rows/columns, padding, spacing)

Dense Kenshi/X4-style data screens (crew lists, cargo manifests, faction
tables) need real flex-ish layout, not manual pixel math per widget.

- A layout cursor pushed/popped like the clip-rect stack: `BeginRow()` /
  `BeginColumn()` / `EndLayout()`, `Spacing()`, `SameLine()`.
- Widgets query their own intrinsic size (text extents, fixed icon size)
  and the layout solver assigns rects — avoids the classic immediate-mode
  "one frame of flicker" on auto-sized content that naive
  cursor-advances-immediately designs get.
- Anchoring to screen edges for HUD elements that need to survive window
  resize independent of layout flow (hook `Window`'s resize callback).

## 8. Draw-layer sorting

Fixes the ordering caveat called out in `GuiRenderer`'s class comment:
shapes and images currently flush in two fixed passes, so true call-order
interleaving between them doesn't work, and there's no way for a tooltip or
popup drawn "at the end" of a frame to guarantee it renders over
everything else if a panel below it was drawn later.

- A small set of draw layers (e.g. Background / Normal / Overlay / Tooltip),
  each independently batched internally, drained in a fixed layer order at
  `Flush()` time regardless of submission order within a layer.
- This is also where cross-primitive (shape vs. image) ordering gets solved
  properly, if it turns out to matter in practice — worth checking against
  real widget usage before over-building it.

## 9. Text input widget

The fiddliest widget: cursor position, selection range, blinking caret,
click-to-position, arrow-key navigation, backspace/delete, clipboard
paste (needs a GLFW clipboard call `Window` doesn't wrap yet).

- Edit state (cursor index, selection anchor, blink timer) lives in
  `GuiStorage` keyed by the input field's `GuiId` — exactly the kind of
  state `GuiStorage` was built for.
- `Input` doesn't currently expose text/character callbacks (only key
  codes) — will need a GLFW char callback wired through `Input` for proper
  UTF-8/IME-friendly typing, or an ASCII-only first pass keyed off
  `IsKeyPressed()` if that's acceptable short-term.

## 10. Scrolling, tabs, tree/collapsible headers, tooltips, modal/popup

Builds directly on clip-rect (item 5), layout (item 7), and draw layers
(item 8):

- **Scrolling**: a clipped region + a scroll offset in `GuiStorage`,
  mouse-wheel input (`Input::GetScrollDeltaY()` already exists) adjusting it,
  clamped to content size.
- **Tabs**: persisted "active tab index" in `GuiStorage`, otherwise mostly a
  layout problem.
- **Tree/collapsible header**: persisted open/closed bool in `GuiStorage`
  per node.
- **Tooltips/modal/popup**: need the Overlay/Tooltip draw layers from item 8
  to guarantee they render on top regardless of submission order, plus
  modal needs to claim exclusive input (block `WantsMouseInput()`-style
  checks from reaching anything below it).

## 11. Data table/grid, drag-and-drop, world-space UI, minimap

The Kenshi/X4-specific payoff features:

- **Data table/grid**: sortable columns, virtualized row rendering (only
  clip-test + draw visible rows) for long lists (market goods, crew,
  faction relations).
- **Drag-and-drop**: a generic typed payload a source widget stashes and a
  drop-target widget queries — cargo/inventory slots dragged between grids.
- **World-space-anchored UI**: health bars / nameplates / interaction
  prompts projected from a world position via `Camera::GetViewProjectionMatrix()`
  into screen space, then drawn through the normal `GuiRenderer` path.
- **Minimap**: needs a render-target/FBO abstraction the engine doesn't have
  yet (nothing in `renderer/` wraps `glFramebuffer` today) — render the
  world top-down into a texture, then `DrawImage()` it like any other icon.
  This is the one item here with an engine-level dependency, not just a GUI
  one.

## 12. Theming/style stack, multi-font support

- `GuiStyle`: colors, corner radius, padding, font sizes, pushed/popped like
  the clip-rect stack so a panel or one-off widget can override style
  without global mutation. Worth making data-driven (loaded from file) once
  the widget set stabilizes, rather than hardcoding constants indefinitely.
- `GlyphCache` currently looks built around a single font at a time — check
  whether it needs to become keyed by `(font, size)` so widgets can mix
  heading/body/monospace-data-table fonts and sizes cheaply per draw call
  without juggling multiple `GlyphCache` instances by hand.

## Open questions to revisit

- **GuiContext lifetime/ownership**: still one explicit object per caller,
  matching `GuiRenderer`. Revisit only if a second simultaneous GUI context
  is ever actually needed (unlikely given Sloth's single-window model).
- **Draw-order caveat (item 3)**: may turn out not to matter in practice once
  real widgets are built — don't build the full layered draw list (item 8)
  speculatively before hitting a case that needs it.
- **ASCII-only text**: `TextRenderer`/`Font` are ASCII-only today; item 9
  (text input) is the first place this will really bite for player-typed
  content (item/settlement naming). Worth deciding whether to invest in
  UTF-8 before or after building the text input widget.
