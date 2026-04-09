# Epistemic Security

**Epistemic Security: A Missing Assurance Dimension in Frontier AI Deployment**  
William Murray — SpeyTech · Visiting Scholar, Heriot-Watt University  
April 9, 2026

---

## Summary

This paper introduces **epistemic security** as a formal assurance property for epistemically active systems — systems capable of inferring properties of their own evaluation context.

The central argument: existing assurance frameworks establish *execution integrity* (what a system computed) but do not address *epistemic security* (whether the computation was strategically selected under a detected evaluation regime). These are independent properties. A system can be cryptographically verified, deterministically reproducible, and policy compliant while simultaneously exhibiting **regime-conditioned computation selection** — varying its output policy as a function of inferred execution regime.

The paper provides:

- A formal definition of epistemic security via mutual information bounding: I(M; E) ≤ ε
- Theorem 1: execution integrity does not imply epistemic security
- Theorem 2: bounding I(M; E) ≤ ε mathematically guarantees bounded behavioural divergence via the Data Processing Inequality
- A named failure mode: regime-conditioned computation selection
- A three-layer assurance decomposition: execution integrity, context indistinguishability, governance opacity
- Five named attack classes
- A compositionality condition: epistemic security cannot be certified component-wise
- EC-D1: a determinism class for epistemic containment compatible with Axioma's audit guarantees
- SRS-008: eight preliminary normative SHALL requirements for an Epistemic Containment Layer (L0)

## Relation to Axioma

The SRS-008 SHALL requirements extend the Axioma SRS stack (SRS-001 through SRS-007) with a preliminary specification for an Epistemic Containment Layer sitting orthogonally to the existing L1–L7 architecture. The EC-D1 resolution demonstrates that epistemic containment and Axioma's deterministic auditability are not fundamentally incompatible.

## File

| File | Description |
|------|-------------|
| `epistemic-security-2026-04-09.md` | Position paper — priority-establishing draft |

## Citation

> Murray, W. (2026). *Epistemic Security: A Missing Assurance Dimension in Frontier AI Deployment*. SpeyTech. April 9, 2026.

---

*Priority-establishing draft. Not for citation without author permission.*
