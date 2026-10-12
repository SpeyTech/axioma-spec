# Vector harness definition for SRS-004-AMD-002 v0.2 (test harness, not normative)

This file is pure ASCII: every non-ASCII character is written as a Python-style `\uXXXX` escape.

Implement `AX:OBS:v2` from AMENDMENT.md (SRS-004-AMD-002) text only. Then print the lines below to stdout:
- ASCII, LF line endings, no trailing spaces;
- lowercase hex;
- no other output.

For every case, start from the BASE record below and apply the listed changes.
1. Run admission validation as SHALL-020 orders it, steps 1 to 7 and 10. Every output in this harness is NFC under any Unicode version, so steps 8 and 9 never fire, and implementations MAY skip them here.
2. If a check fails, print the REJECT line for the first failing check.
3. Otherwise compute `input_hash` (SHALL-009) and `obs_hash` (SHALL-010), build the stored record (canonical form with `obs_hash` present), and print the OBS line.

## Notation

- Strings are Unicode text, encoded as UTF-8.
- `"x"*N` means N copies.
- `\n`, `\"`, `\\` and `\uXXXX` / `\U0001XXXX` here are Python-style escapes describing the text. They are not JSON.
- `bytes(...)` gives raw bytes that need not be valid UTF-8.
- `NULL(len=k)` means input absent while a length k is claimed.

## BASE

| Field | Value |
|---|---|
| completion_state | COMPLETE |
| failure_type | null |
| input | `"prompt"` |
| ledger_seq | 1 |
| model_id | `"m"` |
| oracle_id | `"o"` |
| output | `"ok"` |
| output_size | byte length of output, unless a case sets it |
| params | max_tokens null, seed null, temperature null, top_p null |
| rejected_record_size | absent |

## Output lines

```
OBS case=<name> len=<byte length of stored record> record_sha256=<SHA-256 of stored record bytes> obs_hash=<obs_hash>
REJECT case=<name> result=REJECTED(<reason>)
```

## Section 1: admissible records (in this order)

| # | Name | Changes from BASE |
|---|---|---|
| 1 | base | none |
| 2 | empty_input | input `""` |
| 3 | lf_output | output `"a\nb"` |
| 4 | quote_backslash | output `"say \"hi\" \\ done"` |
| 5 | non_ascii | output `"caf\u00e9 \u00c5 \U0001F600"` |
| 6 | params_full | max_tokens 256, seed 9007199254740991, temperature 45875, top_p 62259 |
| 7 | params_negative | seed -9007199254740991, temperature -65536, top_p 0 |
| 8 | truncated | completion_state TRUNCATED, output `"abc"`, output_size 100000 |
| 9 | error_timeout | completion_state ERROR, failure_type TIMEOUT, output `""` |
| 10 | error_invalid_output | completion_state ERROR, failure_type INVALID_OUTPUT, output `""` |
| 11 | error_transport | completion_state ERROR, failure_type TRANSPORT_ERROR, output `""` |
| 12 | ledger_max | ledger_seq 9007199254740991 |
| 13 | ledger_zero | ledger_seq 0 |
| 14 | identifiers_utf8 | model_id `"mod\u00e8le-1"`, oracle_id `"\u03c9-oracle"` |
| 15 | identifier_256 | model_id `"m"*256` |
| 16 | quote_heavy_fits | output `"\""*32000` |
| 17 | del_free_unicode | output `"\u00a0\u2028\u2029"` (no-break space, line separator, paragraph separator: not controls, written unescaped) |
| 18 | max_tokens_zero | max_tokens 0 |
| 19 | max_tokens_max | max_tokens 2147483647 |
| 20 | size_rejection_record | completion_state ERROR, failure_type INVALID_OUTPUT, output `""`, rejected_record_size 70000 |
| 21 | record_exact_limit | output `"\""*32000 + "a"*X`, with X the value that makes the stored record exactly 65536 bytes (the stored length is linear in X, and output_size keeps 5 digits) |

## Section 2: rejection cases (in this order)

| # | Name | Changes from BASE | Expected reason |
|---|---|---|---|
| 1 | ledger_over | ledger_seq 9007199254740992 | integer_range |
| 2 | seed_over | seed 9007199254740992 | integer_range |
| 3 | seed_under | seed -9007199254740992 | integer_range |
| 4 | output_size_over | completion_state TRUNCATED, output_size 9007199254740992 | integer_range |
| 5 | complete_with_timeout | failure_type TIMEOUT | state_pair |
| 6 | error_without_failure | completion_state ERROR, output `""` | state_pair |
| 7 | truncated_with_failure | completion_state TRUNCATED, failure_type INVALID_OUTPUT | state_pair |
| 8 | unknown_state | completion_state PARTIAL | state_pair |
| 9 | empty_model_id | model_id `""` | identifier |
| 10 | control_in_oracle_id | oracle_id `"o\u0001"` | identifier |
| 11 | c1_in_model_id | model_id `"m\u0085"` | identifier |
| 12 | model_id_257 | model_id `"m"*257` | identifier |
| 13 | null_input | input NULL(len=5) | input |
| 14 | control_in_output | output `"a\u0001b"` | output_encoding |
| 15 | cr_in_output | output `"a\rb"` | output_encoding |
| 16 | del_in_output | output `"x\u007fy"` | output_encoding |
| 17 | invalid_utf8_output | output bytes(0x61, 0xff, 0x62) | output_encoding |
| 18 | size_mismatch | output `"ok"`, output_size 3 | output_size |
| 19 | truncated_size_under | completion_state TRUNCATED, output `"abc"`, output_size 2 | output_size |
| 20 | quote_heavy_over | output `"\""*33000` | record_size |
| 21 | order_state_before_range | failure_type TIMEOUT, ledger_seq 9007199254740992 | state_pair |
| 22 | order_identifier_before_output | model_id `""`, output `"a\u0001b"` | identifier |
| 23 | max_tokens_negative | max_tokens -1 | integer_range |
| 24 | max_tokens_over | max_tokens 2147483648 | integer_range |
| 25 | rejected_size_on_complete | rejected_record_size 70000 | member_not_permitted |
| 26 | rejected_size_on_timeout | completion_state ERROR, failure_type TIMEOUT, output `""`, rejected_record_size 70000 | member_not_permitted |
| 27 | rejected_size_with_output | completion_state ERROR, failure_type INVALID_OUTPUT, output `"x"`, rejected_record_size 70000 | member_not_permitted |
| 28 | rejected_size_too_small | completion_state ERROR, failure_type INVALID_OUTPUT, output `""`, rejected_record_size 65536 | integer_range |
| 29 | record_exact_limit_plus_one | as Section 1 case 21, with X + 1 | record_size |
| 30 | order_member_before_range | rejected_record_size 70000, ledger_seq 9007199254740992 | member_not_permitted |

The expected-reason column states what the text requires. The harness output is what each implementation actually computes.
