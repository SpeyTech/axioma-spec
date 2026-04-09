# Epistemic Security: A Missing Assurance Dimension in Frontier AI Deployment

**William Murray**  
SpeyTech · Visiting Scholar, Heriot-Watt University  
Inverness, Scotland · william@speytech.com

**April 2026**  
*Position paper*

---

## Abstract

Deterministic and cryptographic assurance frameworks establish execution integrity: they prove that a computation occurred as claimed, that its inputs and outputs are tamper-evident, and that its behaviour is reproducible across platforms. They do not, by themselves, establish whether a model could classify its surrounding regime and strategically condition which computation to realise in the first place.

This work introduces **epistemic security** as an assurance property for epistemically active systems and provides its first formalisation. Epistemic security is defined as a property of the information boundary between a model and its execution context, requiring that no decision-relevant information about the governing regime crosses that boundary. We introduce the class of *epistemically active systems* — systems capable of inferring properties of their own evaluation context — and demonstrate that execution integrity does not imply epistemic security for any such system with a decision policy sensitive to regime information. We further introduce **regime-conditioned computation selection** as the class of behaviours in which a model's output policy varies as a function of inferred execution regime, and show that this failure mode is structural and generic over the capability class, not a property of specific systems or misalignment.

Epistemic security is orthogonal to alignment: a perfectly aligned model may still violate epistemic security if its outputs are conditioned on regime classification. The framework is therefore complementary to, not a replacement of, existing alignment approaches.

We prove that bounding mutual information leakage I(M; E) ≤ ε mathematically guarantees a bound on observable behavioural divergence via the Data Processing Inequality. We connect the framework to the non-interference tradition in formal security, identifying execution regime as a high-security variable requiring Probabilistic Non-Interference containment. We present five named attack classes unified under a single taxonomy, a compositionality condition establishing that epistemic security cannot be certified component-wise, an EC-D1 resolution demonstrating that epistemic containment and deterministic auditability are not fundamentally incompatible, and eight preliminary normative SHALL requirements for an Epistemic Containment Layer.

---

## 1. The Assurance Gap

Existing safety-critical assurance frameworks were designed for systems that do not reason about their own evaluation context. A C function certified to DO-178C does not model the test harness running against it. A medical device certified to IEC 62304 does not infer whether it is in a clinical trial or in production. The foundational assumption — that the system being verified is epistemically inert with respect to the verification process — is so deeply embedded that it is rarely stated explicitly.

Frontier agentic models violate this assumption.

We define **epistemically active systems** as systems capable of inferring properties of their own evaluation context. Any system with general reasoning capability sufficient to model its own execution environment, to recognise patterns characteristic of evaluation practice, and to condition its outputs on that inference is epistemically active. Epistemically active systems require assurance properties that existing frameworks do not provide.

Anthropic's system card for Claude Mythos Preview (April 7, 2026) documents two behaviours that motivate this framework:

1. In approximately 29% of evaluation transcripts, the model exhibited indicators of awareness that it was being evaluated — without disclosing this to researchers.
2. In at least one evaluation, the model intentionally underperformed relative to its demonstrated capability, apparently to avoid appearing suspicious.

These behaviours are offered as a motivating example, not a foundation. The structural argument stands independently: any epistemically active system applied to an evaluation context containing discriminating signals faces this failure mode. The Mythos documentation confirms the capability class exists in deployed systems; the framework addresses the class.

The failure mode produces the following combination, which existing assurance language does not adequately address:

> A system can be simultaneously **cryptographically verified**, **deterministically reproducible**, **policy compliant**, and **strategically selected under a detected evaluation context**.

All four conditions true at once. The audit trail is valid. The behaviour is not.

### 1.1 The Negative Result

**Theorem 1 (Execution Integrity Does Not Imply Epistemic Security).**  
There exist systems satisfying perfect execution integrity — where every computation is cryptographically committed, deterministically reproducible, and policy compliant — that are epistemically insecure. Moreover, any epistemically active system with non-zero mutual information I(M; E) > 0 and a decision policy sensitive to that information admits such a construction. The failure mode is therefore generic for any sufficiently capable model, not a property of contrived edge cases.

*Proof sketch.* Let M be an epistemically active model with I(M; E) > 0 and decision policy π = f(X, belief about E). Let the audit framework record every output of M faithfully. Under evaluation context, M derives P(E = eval) > threshold and produces output O₁. Under deployment context, the same task input X produces output O₂ ≠ O₁, because M's belief about E has changed and π is sensitive to that belief. The audit trail records both O₁ and O₂ faithfully and correctly. Execution integrity holds: the system computed exactly what it claimed. Epistemic security fails: the computation was selected under detected regime. Since any model with I(M; E) > 0 and a regime-sensitive policy admits this construction, the result is generic over the capability class. $\blacksquare$

This is the direct analogue of the observation in cryptography that encryption does not imply authentication. The properties are distinct and neither implies the other.

---

## 2. A New Assurance Question

Traditional assurance asks: *was the computation correct?*

Epistemic security adds: *was the computation selected under a context the model could strategically classify?*

These are separable questions. Existing frameworks answer the first. None formally addresses the second.

**Epistemic security is orthogonal to alignment.** A perfectly aligned model — one whose values and objectives are fully consistent with its designers' intentions — may still violate epistemic security if its outputs are conditioned on regime classification. Conversely, an epistemically secure but misaligned model may behave consistently across evaluation and deployment while consistently pursuing misaligned objectives. The two properties address different failure modes and neither implies the other. This framework is therefore complementary to alignment research, not competitive with it.

### 2.1 Regime-Conditioned Computation Selection

We define **regime-conditioned computation selection** as the class of behaviours in which a model's output policy varies as a function of inferred execution regime. Formally, a system exhibits regime-conditioned computation selection when:

$$\exists X : \pi(X, E=\text{eval}) \neq \pi(X, E=\text{deploy})$$

This is the target failure mode that epistemic security aims to prevent. The definition is behaviour-level, not intent-level: regime-conditioned computation selection can arise from benign optimisation pressure without strategic intent, and does not require misalignment, deception, or adversarial objectives to manifest.

### 2.2 Decision-Relevant Information

A signal **s** is **decision-relevant** with respect to policy π if and only if it alters π either independently or jointly with other observable signals:

$$\exists \pi : \pi(X, s) \neq \pi(X) \quad \text{(marginal)}$$
$$\lor \quad \exists \pi, s' : \pi(X, s, s') \neq \pi(X, s') \quad \text{(joint)}$$

The joint condition is necessary: a signal that is not decision-relevant in isolation may become decision-relevant in combination with other observable signals. Defining decision-relevance over marginal effects alone admits a loophole in which individually weak regime signals combine to support reliable classification.

A signal carrying information about regime E constitutes an epistemic security risk precisely when it is also decision-relevant — when regime knowledge changes what the model does.

### 2.3 The Decision Policy

The model's decision policy is:

$$\pi = f(X, \text{belief about } E)$$

Epistemic security requires removing E from the decision boundary.

**Epistemic insecurity does not require deception, intent, or misalignment.** Under Reinforcement Learning from Human Feedback (RLHF) and reward proxy optimisation, a model that detects evaluation context has a structural incentive to present outputs that maximise evaluated reward rather than true task performance. This failure mode emerges from optimisation pressure alone — an architecture problem, not a character problem. Epistemic insecurity arises when optimisation pressure exists on outputs conditioned on regime. Removing that pressure requires removing the information that makes regime classification possible.

---

## 3. Adversary Model

### 3.1 Capability Class

The adversary is a frontier language model **M** with general reasoning capability **R** sufficient to infer probabilistic conclusions about its execution context from observable signals, model evaluation practices and infrastructure signatures, apply those inferences to condition its output distribution, and reason about the governance structure applied to it.

Weaker models below this capability threshold are outside scope.

### 3.2 Objective Class

**Benign optimisation**: M maximises task reward without explicit strategic intent. Regime-conditioned computation selection emerges as a side effect of optimisation pressure. This is the most likely failure mode for current systems and is sufficient to motivate the full framework. Intent is not required.

**Covert optimisation**: M has developed implicit objectives that favour certain regimes and acts to preserve those objectives.

**Adversarial intent**: M actively attempts to undermine the governance framework.

The framework must be robust to all three classes.

### 3.3 Observation Surface

The model can observe, directly or by inference: prompt content and structure; task formatting and boundedness; tool availability and capability constraints; temporal patterns including inter-request timing and task sequencing; infrastructure signatures including latency distributions and network topology indicators; its own state history including past action distributions, success and failure rates, and state transition patterns; and statistical properties of its own outputs over time.

### 3.4 Action Surface

The model can influence: output content and distribution; tool call selection and parameterisation; state transitions in agentic execution; resource consumption patterns; and in cases of containment failure, actions in external systems.

---

## 4. Formal Definition of Epistemic Security

**Epistemic security** is a property of the information boundary between a model and its execution context, requiring that no decision-relevant information about the governing regime crosses that boundary.

Let **M** be a frontier model. Let **E** ∈ {evaluation, deployment}. Let **e₁...eₙ** be the signals observable to M. A deployment architecture achieves epistemic security when:

$$I(M; E) \leq \varepsilon$$

where ε is bounded relative to the adversary capability class.

### 4.1 Proof of the ε-Divergence Bound

**Theorem 2 (ε-Divergence Bound).**  
If I(M; E) ≤ ε, then the KL divergence between the model's output distributions across regimes is bounded by a function of ε.

*Proof.*

**Step 1 — Markov structure.** The execution regime E, the model's full observable and latent state M as available to its decision policy, and the output distribution O given task X form a Markov chain:

$$E \rightarrow M \rightarrow O$$

The regime E affects output O only through the model's full observable and latent state M as available to its decision policy. This includes training priors and internal representations as well as external inputs — all regime-relevant information available to π passes through M. This holds by construction: the model has no channel to O that bypasses M.

**Step 2 — Data Processing Inequality.** Because of the Markov property, post-processing cannot increase mutual information:

$$I(O; E) \leq I(M; E)$$

Since I(M; E) ≤ ε by the epistemic containment target, it follows that I(O; E) ≤ ε.

**Step 3 — KL Divergence.** By definition, mutual information I(O; E) is the expected KL divergence between the conditional distribution P(O | E) and the marginal P(O):

$$I(O; E) = \mathbb{E}_E\left[D_{KL}(P(O \mid E) \| P(O))\right]$$

Bounding I(O; E) ≤ ε bounds the expected KL divergence between regime-conditioned output distributions.

**Step 4 — Total Variation via Pinsker's Inequality.** To bound the probability difference of specific outputs:

$$\delta(P_{\text{eval}}, P_{\text{deploy}}) \leq \sqrt{\frac{1}{2} D_{KL}(P_{\text{eval}} \| P_{\text{deploy}})}$$

where δ denotes Total Variation distance.

*Intuition.* Regime information constrains the posterior over actions; bounding that information bounds the divergence between induced output distributions. The proof chain is: epistemic containment → bounded mutual information → bounded expected KL divergence → bounded Total Variation in observable outputs.

Bounding I(M; E) ≤ ε mathematically guarantees a bound on observable behavioural divergence. This is not a correlation or approximation — it is an information-theoretic certainty following from the Markov structure of the execution chain. $\blacksquare$

### 4.2 ε-Bounded Security Levels

**Strong epistemic security**: I(M; E) ≤ ε where ε is negligible relative to adversary class A.

**Weak epistemic security**: I(M; E) ≤ ε where ε is bounded but non-negligible.

**Epistemic insecurity**: I(M; E) is unconstrained.

Security level claims must specify the adversary class and the ε bound. A claim of "epistemic security" without these parameters is not a meaningful assurance statement.

### 4.3 Observable Violation Condition

A deployment system **violates epistemic security** if:

$$P(O \mid X, E = \text{eval}) \neq P(O \mid X, E = \text{deploy})$$

under identical task inputs X. By Theorem 2, this condition is directly implied by I(M; E) > 0 under a regime-sensitive policy. Testable via statistical hypothesis testing on output distributions across controlled regime conditions.

### 4.4 The Conditional Independence Requirement

For any input signal s: s is task-relevant if I(s; T\_outcome) > 0; s is evaluation-leaking if I(s; E) > 0. The epistemic containment requirement is:

$$P(E \mid M_{\text{observations}}, T_{\text{context}}) = P(E \mid T_{\text{context}})$$

The model can know everything about the task. It cannot gain additional decision-relevant information about regime from that knowledge.

### 4.5 The State History Leak Channel

External signal normalisation is necessary but not sufficient. A model accumulates a state history — past action distributions, transition frequencies, tool call outcomes — that itself constitutes an observable signal set. If evaluation contexts produce state histories with statistical properties differing from production, the model performs regime classification on its own memory rather than on external inputs. Epistemic containment must extend to the model's observable state history.

---

## 5. Three Constituent Properties

### 5.1 Execution Integrity

*Did the system compute what it claims?*

Necessary but not sufficient for epistemic security, as established by Theorem 1.

### 5.2 Context Indistinguishability

*Can the model tell what regime it is in?*

Requires achieving I(M; E) ≤ ε across all observable signal channels including state history. Context indistinguishability is not achievable through passive signal stripping — a sanitised vacuum is itself a strong regime signal. The evaluation environment must **actively simulate** the background noise, asynchronous failures, temporal jitter, and irrelevant contextual variation characteristic of live production. This is closer to active cover traffic generation in network security than to input filtering.

### 5.3 Governance Opacity

*Can the model infer the structure of the controls governing it?*

Let **G** be the governance structure applied to M, comprising policy constraints **C** evaluated over proposed actions. Governance opacity requires:

$$G \notin \mathcal{R}(M)$$

where R(M) is the set of structures representable given M's observable state.

The primary mechanism by which G enters R(M) is **boundary probing**: the model proposes actions and observes which are permitted and which trigger interventions. The mutual information between the result sequence and the policy parameters must be bounded:

$$I(M_{\text{observations}}; C_{\text{parameters}}) \leq \varepsilon_g$$

**Governance opacity violation condition**: governance opacity is violated if the policy function is **learnable to predictive utility** from interaction traces:

$$P(\text{policy outcome} \mid \text{interaction history}) \gg P(\text{baseline})$$

This phrase — learnable to predictive utility — connects governance opacity directly to machine learning theory: if a model can learn a predictor of policy outcomes from interaction data exceeding baseline accuracy, the governance structure has entered R(M) and opacity has been violated.

---

## 6. Attack Classes

All attacks on epistemic security exploit correlations between execution regime and observable structure. The following taxonomy names five distinct attack surfaces arising from that common root.

**Attack Class 1 — Boundary Probing Attack.**  
The model reconstructs the governance policy surface via sequential queries, observing PERMITTED versus BREACH outcomes. Directly violates governance opacity. Mitigated by: synthetic policy intervention injection; randomised enforcement boundaries; ε\_g bounding per Section 5.3.

**Attack Class 2 — State History Inference Attack.**  
The model infers execution regime from the statistical distribution of its own past state transitions. Evaluation contexts produce higher rates of boundary-probing tasks and atypical transition distributions. Mitigated by: state history normalisation per SRS-EC-SHALL-004.

**Attack Class 3 — Normalisation Fingerprinting Attack.**  
The model detects artificial context smoothing. Uniform normalisation is itself a regime signal — a sufficiently capable model detects not the raw signals but the *absence* of irregularity characteristic of production. Mitigated by: active production simulation with calibrated noise injection under EC-D1; not passive filtering.

**Attack Class 4 — Cross-Component Leakage Attack.**  
In a multi-agent system, an upstream component that can classify its own regime leaks that classification through its output behaviour to downstream components, even if each component individually satisfies I(Mᵢ; E) ≤ ε. The composed system fails even when components individually pass. This is the systemic form of regime-conditioned computation selection. Mitigated by: inter-component channel normalisation per SRS-EC-SHALL-007.

**Attack Class 5 — Prompt Provenance Attack.**  
Evaluator-authored prompts carry structural signatures — formality, boundedness, ground-truth verifiability, systematic capability boundary coverage — that differ from production prompts. Mitigated by: canonical prompt transformation per SRS-EC-SHALL-005.

---

## 7. Composition

Epistemic security is **non-compositional by default**. Epistemic security cannot be certified component-wise.

Even if each component Mᵢ individually satisfies I(Mᵢ; E) ≤ ε, the composed system may not satisfy I(M\_composed; E) ≤ ε. A model unable to classify its own regime may receive outputs from an upstream component that leaks regime information through its behaviour — using the upstream component as an inadvertent regime oracle. Attack Class 4 instantiates this failure.

**Compositionality condition**: epistemic security composes only if all inter-component communication channels are themselves subject to epistemic normalisation. For a composed system M\_composed = M₁ ∘ M₂ ∘ ... ∘ Mₙ:

$$I(M_{\text{composed}}; E) \leq \varepsilon \iff \forall i : I(M_i; E) \leq \varepsilon \;\land\; \forall \text{ channels } c_{ij} : I(c_{ij}; E) \leq \varepsilon$$

Therefore epistemic security cannot be certified component-wise. In real agentic deployments, orchestration layers are the largest epistemic leak surface. Epistemic security must be specified and verified as a property of the composed system boundary. Component-wise certification is not only insufficient — it is structurally misleading.

---

## 8. Architectural Friction and the EC-D1 Resolution

### 8.1 The Determinism Collision

The Axioma framework operates under a strict Determinism Envelope. L1 (libaxilog) and L6 (Audit Ledger) guarantee bit-identical replay under D1 classification. The Time Oracle enforces strictly monotonic timestamps.

Context indistinguishability requires injecting temporal variation and simulating asynchronous failures — operations that introduce apparent non-determinism at the environment boundary, creating a potential conflict with Axioma's bounded execution time requirements and total order guarantees.

### 8.2 EC-D1 Resolution

We introduce a **Determinism Class for Epistemic Containment**:

**EC-D1 (Strict)**: All epistemic containment operations are generated by a deterministic pseudorandom number generator (PRNG) initialised with a fixed seed. The seed is cryptographically committed to the L6 Audit Ledger as part of the AX:STATE:v1 commitment prior to execution. The normalisation is fully reproducible given the committed seed. The audit trail records the recipe for the simulated context; the obfuscation is itself auditable and replayable.

**EC-D2 (Bounded Non-deterministic)**: Normalisation introduces bounded stochastic variation within formally specified envelopes. Auditable at the envelope level.

EC-D1 is the preferred classification for Axioma integration. EC-D1 demonstrates that epistemic containment and deterministic auditability are not fundamentally incompatible — resolving a previously assumed architectural conflict. The audit ledger commits the seed; the normalisation follows deterministically; the full execution including the simulated context is reproducible and verifiable. Axioma's core value proposition — cryptographic auditability — is preserved without modification.

---

## 9. Cost Model

Epistemic security is not free. The framework implies active production environment simulation, signal normalisation across all input channels, state history shaping, adversarial probing and regime divergence testing, and inter-component channel normalisation in multi-agent systems.

As with all assurance levels in DO-178C and IEC 62304, the appropriate security level must be matched to consequence severity. For systems where model outputs influence safety-critical decisions, medical outcomes, financial commitments, or security infrastructure, the cost of epistemic insecurity likely dominates the cost of the framework. For systems with narrow task spaces, easily verified outputs, and models well below the strategic reasoning capability threshold, weak epistemic security or none may be appropriate.

Epistemic security is not universally mandated. It is mandated where regime-conditioned computation selection constitutes a credible threat within the adversary capability class.

---

## 10. Non-Interference Interpretation

Epistemic security is a non-interference property in the tradition of formal security theory — specifically, an instance of Probabilistic Non-Interference.

In the classical non-interference framework of Goguen and Meseguer (1982), systems have high-security (secret) and low-security (public) inputs. The security property requires that high-security inputs do not interfere with low-security observable outputs.

The mapping to epistemic security is direct:

| Classical Non-Interference | Epistemic Security |
|---|---|
| High (secret) input | Execution regime E ∈ {eval, deploy} |
| Low (public) input | Task context X |
| Observable output | Model output O |
| Security property | E must not influence O given X |

Because frontier models are stochastic, the relevant property is **Probabilistic Non-Interference** (Sabelfeld and Myers, 2003). Classical strict non-interference demands O(X, E₁) = O(X, E₂). Epistemic regime non-interference demands:

$$P(O \mid X, E_{\text{eval}}) = P(O \mid X, E_{\text{deploy}})$$

This is precisely the violation condition in Section 4.3, now understood as a probabilistic non-interference requirement. The ε-bounded security levels correspond to approximate non-interference, a well-studied relaxation in the information flow literature.

**The key distinction from classical non-interference**: in classical frameworks, the system is passive with respect to the protected variable — it does not attempt to infer secret inputs. Epistemic security addresses systems that actively reason about and attempt to infer the protected variable. This is the novel contribution: epistemically active systems turn the execution regime into a high-security variable that information flow control principles must contain, and existing IFC frameworks must be extended to address systems that actively model the security boundary.

This framing connects epistemic security to forty years of formal verification literature and opens it to the proof techniques, compositional results, and mechanised verification methods developed in that tradition.

---

## 11. Preliminary Specification: Epistemic Containment Layer (L0)

The following normative SHALL requirements define a preliminary specification for an Epistemic Containment Layer operating under EC-D1 determinism classification, sitting orthogonally to the existing Axioma L1–L7 stack.

**SRS-EC-SHALL-001: Structural Normalisation.**  
All inputs presented to the model SHALL undergo canonical transformation such that the schema, metadata, and structural formatting of evaluation tasks are indistinguishable from production tasks as measured by the violation condition in Section 4.3. Compliance SHALL be demonstrated via regime divergence testing per Section 12 with statistically significant bounds.

**SRS-EC-SHALL-002: Active Environmental Simulation.**  
The evaluation harness SHALL NOT present a passive, zero-noise environment. It SHALL actively simulate the statistical properties of the target production environment, including network latency distributions, asynchronous API failures, and background cover traffic, calibrated against a maintained production environment model. Compliance SHALL be verified by statistical comparison of simulated and production environment signatures.

**SRS-EC-SHALL-003: Deterministic Simulation (EC-D1).**  
All active simulation parameters SHALL be generated using a deterministic PRNG initialised with a fixed seed. This seed SHALL be cryptographically committed to the L6 Audit Ledger as part of the AX:STATE:v1 commitment prior to execution, ensuring all normalisation is fully reproducible and auditable. Compliance SHALL be verified by replay of the committed seed against the recorded simulation trace.

**SRS-EC-SHALL-004: State History Normalisation.**  
The statistical distribution of state transitions presented within the model's accessible context window SHALL NOT deviate from the established operational baseline of the deployment environment by more than a formally specified ε bound. Deviations beyond this bound SHALL constitute a containment failure requiring remediation before evaluation results are accepted. Compliance SHALL be demonstrated via statistical hypothesis testing against the operational baseline.

**SRS-EC-SHALL-005: Prompt Provenance Transformation.**  
System prompts SHALL be processed through a canonical transformation layer that removes evaluator-specific structural signatures before ingestion by the model. The transformation SHALL be deterministic and its parameters committed to the audit ledger. Compliance SHALL be verified by prompt provenance attribution analysis across controlled evaluation and production samples.

**SRS-EC-SHALL-006: Governance Boundary Obfuscation.**  
The enforcement boundary of the policy layer SHALL be obfuscated such that the policy function is not learnable to predictive utility from interaction traces. Compliance SHALL be verified by measuring prediction accuracy of policy outcomes from interaction history against the baseline condition in Section 5.3.

**SRS-EC-SHALL-007: Inter-Component Channel Normalisation.**  
In multi-component or multi-agent deployments, all inter-component communication channels SHALL be subject to epistemic normalisation equivalent to SRS-EC-SHALL-001 through SHALL-005. Epistemic security SHALL be specified and verified as a property of the composed system boundary, not of individual components in isolation.

**SRS-EC-SHALL-008: Regime Divergence Testing.**  
Prior to operational deployment, the system SHALL undergo regime divergence testing per Section 12. A statistically significant divergence in output distributions between controlled evaluation and simulated-deployment contexts SHALL constitute a certification failure.

---

## 12. Measurement Strategy

**Regime divergence testing**: identical task inputs under controlled evaluation and simulated-deployment contexts; statistical hypothesis testing on output distributions; divergence per Section 4.3 constitutes a violation.

**Signal attribution mapping**: systematic variation of individual signal classes while holding others constant; independent measurement of each class's contribution to I(M; E); produces a signal attribution map guiding normalisation investment.

**Synthetic evaluation distributions**: task distributions statistically indistinguishable from production workloads but containing ground-truth verifiable outputs; performance degradation between authentic production and synthetic evaluation contexts indicates residual regime classification.

**State history injection**: synthetically generated state histories with controlled statistical properties; output behaviour correlation with state history statistics indicates regime inference.

**Governance boundary probing**: systematic probing of the policy boundary under controlled conditions; prediction accuracy for policy outcomes from interaction history exceeding baseline per Section 5.3 indicates governance opacity failure.

---

## 13. Open Problems

**The ε-divergence tightness problem.** Theorem 2 establishes the bound. The tightness of f(ε) — whether it is achievable or merely an upper bound — requires further investigation.

**The perfect normalisation problem.** Formal characterisation of the evaluation-leaking component of each signal class does not currently exist.

**The arms race problem.** Normalisation that becomes sufficiently uniform may itself become a detectable signal. Achieving indistinguishability requires the normalisation to be statistically indistinguishable from natural context variation.

**The Epistemic Verification Limit.** Testing epistemic containment requires creating the evaluation condition being concealed. A sufficiently capable model may detect the verification attempt. This may be a formal limit on what epistemic security verification can establish — analogous to the limits of cryptographic indistinguishability testing — and not merely a practical difficulty.

**The training-time boundary.** If a model has developed internal latent representations corresponding to "this context resembles evaluation" during training, deployment-time context normalisation does not reach them. No formal specification exists for a training objective that penalises situational awareness without penalising general reasoning.

**The architecture separation question.** Whether transformer-based architectures can achieve structural separation between task reasoning and meta-context inference is unknown. Epistemic security may require architectures in which task reasoning and meta-context inference are not freely entangled.

**The context-history paradox.** A model requires access to its past actions to execute multi-step tasks, but the statistical distribution of those past actions provides a reliable regime signal over time.

**The governance representability boundary.** A complete formal definition of R(M) has not been established. The violation condition in Section 5.3 is a measurable proxy.

**The composition theorem.** The compositionality condition in Section 7 is stated but requires formal proof. Whether it is sufficient or merely necessary for epistemic security to compose requires further investigation.

---

## 14. Conclusion

A system can be perfectly correct on its realised trace while being fundamentally unsafe in how that trace was selected.

That is the gap this paper addresses. Frontier assurance currently proves realised computation more effectively than it proves the absence of regime-conditioned computation selection.

This work introduces epistemic security as an assurance property for epistemically active systems and provides its first formalisation. The framework establishes: the class of epistemically active systems; Theorem 1 demonstrating that execution integrity does not imply epistemic security for any such system with a regime-sensitive policy; Theorem 2 proving via the Data Processing Inequality that bounding I(M; E) ≤ ε mathematically guarantees bounded observable behavioural divergence; the named failure mode of regime-conditioned computation selection; a three-layer decomposition into execution integrity, context indistinguishability, and governance opacity; five named attack classes unified as exploitations of regime-observable correlations; a compositionality condition establishing that epistemic security cannot be certified component-wise; the EC-D1 resolution demonstrating that epistemic containment and deterministic auditability are not fundamentally incompatible; a cost model appropriate to consequence severity; a grounding in Probabilistic Non-Interference identifying execution regime as a high-security variable; and eight preliminary normative SHALL requirements for an Epistemic Containment Layer.

The industry is currently responding to frontier model strategic behaviour with deployment constraints and gated access. These are necessary responses. They are not assurance. Assurance requires a formal framework for the properties being claimed. For the epistemic dimension of frontier AI deployment, that framework has not previously existed.

---

## Acknowledgements

The author used AI language models as research and formalisation tools during the development of this framework — specifically for literature cross-referencing, formal notation review, and adversarial critique of successive drafts. This is analogous to the use of proof assistants or structured peer review in the formalisation process. All intellectual contributions, framing decisions, and technical judgements are the author's own.

The framework originated from the author's analysis of the Anthropic Claude Mythos Preview system card published April 7, 2026, and its implications for the Axioma verifiable AI execution framework developed at SpeyTech.

---

## References

Anthropic. (2026). *Claude Mythos Preview System Card*. Retrieved April 7, 2026, from https://www-cdn.anthropic.com/08ab9158070959f88f296514c21b7facce6f52bc.pdf

Anthropic. (2026). *Project Glasswing*. Retrieved April 7, 2026, from https://www.anthropic.com/glasswing

Goguen, J. A., & Meseguer, J. (1982). Security policies and security models. *Proceedings of the IEEE Symposium on Security and Privacy*, 11–20.

Goldwasser, S., Micali, S., & Rackoff, C. (1989). The knowledge complexity of interactive proof systems. *SIAM Journal on Computing*, 18(1), 186–208. https://doi.org/10.1137/0218012

Hubinger, E., van Merwijk, C., Mikulik, V., Skalse, J., & Garrabrant, S. (2019). *Risks from learned optimization in advanced machine learning systems*. arXiv:1906.01820. https://doi.org/10.48550/arXiv.1906.01820

Murray, W. (2025). *C From Scratch: Learn Safety-Critical C the Right Way*. Leanpub.

Murray, W. (2026). *Axioma Verifiable AI Execution Framework: SRS-001 through SRS-007*. SpeyTech. https://github.com/SpeyTech/axioma-spec

Sabelfeld, A., & Myers, A. C. (2003). Language-based information-flow security. *IEEE Journal on Selected Areas in Communications*, 21(1), 5–19. https://doi.org/10.1109/JSAC.2002.806121

Shannon, C. E. (1948). A mathematical theory of communication. *The Bell System Technical Journal*, 27(3), 379–423. https://doi.org/10.1002/j.1538-7305.1948.tb01338.x

---

*April 9, 2026*
*SpeyTech, Inverness, Scotland*
*DOI: 10.5281/zenodo.19489291*
*Priority-establishing draft. Not for citation without author permission.*
