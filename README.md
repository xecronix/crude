# CRUDE

CRUDE is a simple source snapshot tool written in C with Pelles C.

It creates full numbered snapshots of a project folder and stores a notes file with 2 sections:

- description: what was completed
- direction: current thinking, next steps, or design notes

CRUDE is intentionally simple:
- no database
- no built-in diff UI
- full copies per revision
- external diff tools like WinMerge work naturally