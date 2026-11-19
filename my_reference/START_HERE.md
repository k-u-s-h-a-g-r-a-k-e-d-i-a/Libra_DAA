# Kushagra: only start with this

Open this new C folder by itself in Antigravity. Run `.\run.bat` on Windows or `make run` on Linux.

Practice these two things first:

1. Search: choose **1 -> 1 -> sorting -> 1** (KMP).
2. Recommend: choose **2 -> 1** (book ID 1).

Your first explanation: “I store book records in an array. The program searches their text, scores matching books, sorts the results and recommends related books.”

Read C_AND_LIVE_EDITS.md next. Leave the full algorithm lab until the core flow makes sense. The lab remains part of the submission and can still be questioned.

The root README is the project document. This folder is personal learning material and can stay outside your submission if preferred. Nothing in the application reads it.

For the first code walkthrough, open `src/library.c` and find `search_books`.
Search for **sorting**. Book 1 has one keyword match and one description match,
so its score is `0 × 3 + 1 × 2 + 1 = 3`. Books 2 and 10 each score 2.
Trace that one result through the counting loop, the score line and the call to
`merge_sort`. For a practice edit, double the description weight and predict
which result changes before running it.
