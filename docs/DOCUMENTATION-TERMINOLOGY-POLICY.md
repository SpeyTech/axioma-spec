# Documentation Terminology Policy

Status: project-wide, non-normative guidance. Approving authority: William Murray, Director, Spey Systems LTD (owner decision D6, 13 October 2026). Applies to new and changed documentation; existing lines are triaged separately (normative text changes only through amendment).

Legal / intellectual property owner: Spey Systems LTD (SC889983). Documentation owner: William Murray, Director, Spey Systems LTD.

## Principle

Make Axioma's architecture and product documentation provider-neutral without altering technical truth, historical records, test evidence or reproducibility.

## Applies to

READMEs, architecture and design documents, specifications (new text and amendments), product and commercial material, user-facing interface text, and audit or review reports.

## Preferred terms

| Avoid in architectural prose | Prefer |
|---|---|
| Names of model families or vendors | inference provider, model implementation |
| Generic product-category acronyms for language models | non-deterministic inference system |
| "model output" in a protocol sense | oracle response, inference output |
| "AI agent", where unnecessary | automated actor, execution client |
| "prompt" | inference request, where technically appropriate |

## Must keep original wording

Rewording these would compromise accuracy or reproducibility:

- historical evidence: committed records, ledgers, frames, anchors, experiment outputs and their manifests;
- test fixtures, golden vectors and captured error strings;
- provider-specific integration code and its configuration names (for example, an API key variable or a protocol header name in an adapter);
- exact technical quotations and external citations;
- git history and commit metadata.

Where such a term must appear in a provider-neutral document, refer to it indirectly ("the adapter's provider key variable"), or quote it once in a clearly marked technical appendix.

## Identifiers in code

New public identifiers use neutral names (for example `MODEL_OBS` or `INFERENCE_OBS`). Renaming an existing public identifier is an API change, so it goes through the normal proposal process with a compatibility note. Known existing cases:

- the model-observation input enumerator in axioma-agent `include/axilog/agent.h` (a rename candidate);
- the provider API-key environment variable in the axioma-oracle gateway, which is provider-specific by nature, belongs to an adapter, and may stay.

## Enforcement

An informational CI check greps documentation paths (`*.md`, `docs/`) for a denylist of provider and model-family names. It excludes fixture, evidence and adapter directories, and flags matches for review. It reports and never blocks.
