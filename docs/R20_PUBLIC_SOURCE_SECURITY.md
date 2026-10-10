# R20 public source audit scope

The default audit reads every tracked HEAD blob and every unique blob reachable
through HEAD history, including binary and vendored contents, from Git objects.
A shallow checkout, changed tracked working input, unreadable/truncated input,
unsupported tracked mode or blob larger than 16 MiB blocks acceptance; no such
blob may be silently skipped. Reports contain rule IDs and paths/blob identities,
never matched credential values. Filename tokens are redacted as well.

Current non-vendor paths reject ROM/save/APK/archive, model/texture/audio/Unreal
payloads and credential/private-key names. The exact unchanged accepted vendor
tree 5a551f1f9e40184278c57dfb8d25f68a0a1c99dc is the source-snapshot exception
for payload paths; its contents still undergo secret scanning. No new vendor
payload can acquire that exception. Future redistributable binary fixtures need
an explicit reviewed provenance policy rather than a broad asset exemption.

Content signatures cover private-key PEM, GitHub tokens, AWS access-key IDs,
Slack tokens and JWTs. This bounded rule set is not a guarantee against every
possible secret or proprietary representation. Current path leakage is measured;
reachable-history content is measured; other unreachable branches/objects and
hosting account configuration are outside this entry point's scope. CodeQL is
separate evidence, not a credential scan. Findings or unscanned inputs block G1.

Run from a complete clean checkout with full history:

```sh
python tools/audit_public_repository.py --receipt build/r20-security-receipt.json
```

R20 CI fetches full history and runs the same actual default command after
negative regressions and source package generation. Receipts use fresh protected
output destinations. No scanned source/private payload is uploaded.

Ignore proof checks World/Render and whole-package index, all optional private
presentation directories, saves/ROMs/archives, APK and credentials. Ignore rules
do not sanitize already tracked content; the Git-byte scan is mandatory.
The manual self-hosted Unreal workflow is restricted to trusted main and no
longer uploads APKs. Actual execution remains deferred to the project-PC R18
stage; source changes do not certify runner access or a build.

Thirteen explicit temporary-Git regressions cover deleted historical tokens,
binary credentials, redaction, dirty/shallow inputs, oversized blobs, tracked
symlinks, rejected payload names, ignored-output gaps and the immutable vendor
boundary. Fixtures construct artificial signatures at runtime; no credentials
or extracted assets are committed. Real repository counts come only from CI.
