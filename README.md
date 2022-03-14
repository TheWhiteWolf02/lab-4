# Enhancing Library Robustness

In the previous task you built a tool for testing a C-Library function. The first step was to build a routine for a single test on a single function. Step 2 was to generate test values for testing one function.

The current task is to build a robustness wrapper for the non-robust library functions.
You shall build a robustness wrapper for the function `fputs` and test the wrapper with your
robustness test from the previous task. `fputs` must not crash for any input. Instead `fputs` has to
return EOF for each "non-robust" input.
