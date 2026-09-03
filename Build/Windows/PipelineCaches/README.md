# Build/Windows/PipelineCaches

Drop the built **`BattlegroundClone_<ShaderPlatform>.stable.upipelinecache`** here (e.g.
`BattlegroundClone_SF_D3D_SM6.stable.upipelinecache`).

`RunUAT BuildCookRun` detects any `*.stable.upipelinecache` in this folder, converts it to a
binary `*.upipelinecache` and stages it into the pak under
`<Staged>/BattlegroundClone/Content/PipelineCaches/Windows/`. At runtime
`r.ShaderPipelineCache.Enabled=1` loads it and precompiles the PSOs.

**This `.stable.upipelinecache` IS committed** — it is the shippable artifact. Only the raw
recordings under `Saved/CollectedPSOs/` are throwaway.

Regenerate with `Tools/PSOCache/Build-PSOCache.ps1`. Full workflow:
`docs/2026-09-03-bundled-pso-cache-setup.md`.
