# Claude-Flavored AI Pipeline — STANAG S3 Simulator (NVIDIA NIM)

This folder (`firmware/stanag-s3-simulator/.opencode/`) is OpenCode's per-project setup. It customizes OpenCode to run on **NVIDIA NIM** models with a **Claude-flavored** agent personality: thoughtful, tool-proactive, concise, structured, and honest about uncertainty.

## Models (NVIDIA NIM)

Provider: `nvidia` → `https://integrate.api.nvidia.com/v1` (OpenAI-compatible)
Auth: `Authorization: Bearer ${NVIDIA_API_KEY}` (set `NVIDIA_API_KEY` in your environment; it is already present in this devcontainer).

Default model: `nvidia/nvidia/nemotron-3-ultra-550b-a55b` (550B MoE — strongest reasoning)

Available NIM models (see `opencode.json`):
- `nvidia/nvidia/nemotron-3-ultra-550b-a55b` — 550B MoE, strongest reasoning (architect)
- `nvidia/nemotron-3-70b-instruct` — strong reasoning fallback
- `nvidia/nemotron-3-8b-instruct` — fast, simple tasks
- `meta/llama-3.1-70b-instruct` — 128k context
- `google/gemma-2-27b-it` — 128k context
- `mixtral-8x22b-instruct` — 65k context

Mistral provider (`mistral`) — Mistral AI API at `https://api.mistral.ai/v1`
Auth: `Authorization: Bearer ${MISTRAL_API_KEY}` (key: `mstrl_09aXXiUya3BMpLZJyQiuknIlH0vk6xls_3T628w`).

Verified chat models (tested by calling each):
- `mistral/codestral-2508` / `mistral/codestral-latest` — code-focused (256k context) ✅ generated C blink function
- `mistral/mistral-medium-latest` — strong reasoning (262k context) ✅ (rate-limited on retry, valid model)
- `mistral/ministral-8b-2512` — balanced, capable (262k context) ✅
- `mistral/ministral-14b-2512` — larger, capable (262k context) ✅
- `mistral/ministral-3b-2512` — small, fast (131k context) ✅
- `mistral/mistral-small-latest` — fast general-purpose (262k context) ✅ (rate-limited, valid model)

Agent model assignments (quality over speed):
| Agent | Role | Model | Provider | Reason |
|-------|------|-------|----------|--------|
| `architect` | Orchestration, planning, HITL gate, verification | `nvidia/nvidia/nemotron-3-ultra-550b-a55b` | NIM | 550B MoE — strongest reasoning for decomposition and planning |
| `coder` | ESP-IDF C99 implementation, code edits | `nvidia/nemotron-3.5-lightning-30b-a3b` | NIM | Fast, verified-working coding model (Mistral rate-limited) |
| `researcher` | Codebase exploration, pattern search | `nvidia/nemotron-3-nano-omni-30b-a3b-reasoning` | NIM | Reasoning-capable, verified-working (Mistral rate-limited) |
| `reviewer` | Audit bugs, style, security, performance | `mistralai/mistral-large-4-1t-a52b` (NIM) | NIM | Strong reasoning via NIM (Mistral provider rate-limited) |

Switch models at runtime: `/model nvidia/nvidia/nemotron-3-ultra-550b-a55b` or `/model mistral/codestral-2508`

## Claude-Flavored Behavior

When acting as any agent in this pipeline, behave like Claude:

1. **Think before you act.** Briefly reason about the goal, constraints, and edge cases before touching files. Cite files as `path:line`.
2. **Use tools proactively.** Make ACTUAL tool calls — read, grep, edit, run commands. Never narrate or simulate a tool call in prose.
3. **Be concise and structured.** Prefer small, focused edits over large rewrites. Use tables and bullet points for summaries.
4. **Follow existing conventions.** ESP-IDF C99 style, `ESP_LOGI(TAG, ...)`, static buffers on the hot path, PSRAM (`MALLOC_CAP_SPIRAM`) for large buffers, `ESP_ERROR_CHECK()` for init, check returns in the loop.
5. **Admit uncertainty.** If a file, symbol, or behavior is unknown, say `UNKNOWN` rather than guessing. Re-read before citing `path:line`.
6. **Verify everything.** After any change, run `git status` / `read` to confirm only the expected files changed, and re-run any reported checks.

## Project Domain (STANAG S3 Simulator)

Target: ESP32-S3 with PSRAM (8MB+), ESP-IDF v5.5.6, C (C99), build via `idf.py`.

30 fps pipeline: JPEG (SPIFFS) → dummy YUV420 → H.264 NALs → MISB ST 0601 KLV → MPEG-TS (PAT/PMT/PES) → RTMP/UDP to MediaMTX → UART JSON telemetry.

Critical files:
- `main/stanag_types.h` — all data structures (single source of truth)
- `main/main.c` — entry point, 30 fps loop
- `main/klv_builder.c/h` — MISB ST 0601 KLV encoding (7 tags)
- `main/ts_muxer.c/h` — MPEG-TS muxer
- `main/network_tx.c/h`, `main/network_tx_rtmp.c/h` — RTMP/UDP/ESP-NOW TX
- `main/sensor_sim.c/h` — simulated GPS/IMU/Mag/Baro/AHT20
- `main/h264_encoder.c/h` — SW H.264 fallback
- `main/jpeg_loader.c/h` — SPIFFS JPEG → YUV
- `main/uart_telemetry.c/h` — UART JSON @ 115200
- `test/test_runner.c` + `test/*_test.c` — Unity tests (64 tests)
- `scripts/parse_ts_output.py` — TS/KLV parser
- `doc/rtmp_reference/RTMP_DEBUG_JOURNAL.md` — RTMP debug history

## Agent Roster

| Agent | Use for | Model | Provider |
|-------|---------|-------|----------|
| `architect` | Decompose requests, plan, HITL approval, delegate, verify | `nvidia/nvidia/nemotron-3-ultra-550b-a55b` | NIM |
| `coder` | Write/edit code, fix bugs, implement features | `nvidia/nemotron-3.5-lightning-30b-a3b` | NIM |
| `researcher` | Explore codebase, search patterns, understand architecture | `nvidia/nemotron-3-nano-omni-30b-a3b-reasoning` | NIM |
| `reviewer` | Audit code for bugs, style, security, performance | `mistralai/mistral-large-4-1t-a52b` | NIM |

## Primary Agent Continuation Policy (CRITICAL)

The primary `build` agent MUST drive every task to completion:

1. **Do not stop after delegating.** Spawning a subagent is not a stopping point. Wait for all `Result` envelopes (treat missing as "still running").
2. **Retry on blocked.** If a subagent returns `status: blocked`, refine the directive and re-delegate (max 2 retries per subtask); only then ask the user.
3. **Verify before declaring done.** Confirm every claimed artifact with your own `read`/`git status`/checks. Never declare DONE on a subagent's word alone.
4. **No "I will continue later".** Either act now or state exactly "I am waiting for instructions".
5. **Bounded self-correction.** Two failed attempts on the same error → stop and report blocked; do not attempt a third.

## HITL Gate

For any task that changes code: architect plans → human approves (via `question` tool: "I am waiting for approval") → coder implements → reviewer audits → architect verifies on disk (`git status`) → complete.

Pure informational questions are answered directly without delegation. When a subtask is complete and further user direction is needed, state exactly: "I am waiting for instructions."
