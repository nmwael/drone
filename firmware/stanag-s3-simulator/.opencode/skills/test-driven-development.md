# Test-Driven Development Skill

Use when implementing any feature or bugfix, before writing implementation code.

1. Read the existing test file for the module (`test/*_test.c`) to understand the Unity test patterns (`setUp`/`tearDown`, `TEST_ASSERT`, `RUN_TEST`).
2. Add a failing test case for the new behavior first.
3. Run `idf.py -DTEST_COMPONENT=1 build` then `timeout 60 idf.py -p /dev/null qemu monitor` to confirm the test fails.
4. Implement the feature in the source file, staying within the directive's `files` scope.
5. Re-run the QEMU test suite and confirm the new test passes.
6. Report real command output as evidence in the Result `checks`.

Do not skip step 2 — a test must exist and fail before implementation.
