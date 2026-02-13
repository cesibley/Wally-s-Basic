# PRINT USING Phase 3 Test Staging

These test files are **Phase 3 prep scaffolds** for upcoming `PRINT USING`
string-field support.

They are intentionally kept in a subdirectory so they do not run as part of the
current `tests/run_all.sh` torture sweep (which only includes top-level
`WBASIC_TT_*.bas` files).

When Phase 3 implementation lands, promote finalized variants into top-level
`WBASIC_TT_*` files (or extend the harness) with locked golden output.
