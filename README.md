# FinalProject — C++ Version Control System

A C++ project building out a version control system.

---

## Model Classes

### `Commit` — abstract base class
**Files:**`model/Commit.h`,`model/Commit.cpp`

The backbone of the commit system. Every commit stores four fields:`author`,`message`,
`timestamp`, and`commitID`. It's abstract —`displayCommit()`and`getSummary()`are pure
virtual, so concrete subclasses must implement them.

A few design notes:
- Two`Commit`s are considered **equal if and only if their`commitID`s match**, regardless of
  author, message, or anything else. This is enforced via an overloaded`operator==`.
-`operator<<`is overloaded and delegates to`getSummary()`, so you can stream any commit
  directly to`cout`.

---

### `StandardCommit` — extends `Commit`
**Files:**`model/StandardCommit.h`,`model/StandardCommit.cpp`

The concrete commit implementation. Adds`fileSnapshots`— a`map<string, string>`storing
a full snapshot of every tracked file's content at commit time (path → content).

`displayCommit()`prints the standard commit metadata followed by a list of snapshotted file
paths (content is intentionally omitted — nobody wants that dumped to the terminal).

`getSummary()`returns a compact one-liner in this format:
```
COMMIT-<id>-<N> files-<message>-<author>
```

TODO:`getSummary()`should eventually make use of the extra snapshot data rather than
behaving identically to the base class version.

---

### `TrackedFile`
**Files:**`model/TrackedFile.h`,`model/TrackedFile.cpp`

Represents a single file under version control. Holds the file's`path`,`content`, byte`size`,
and a`Status`tracking where it sits in the commit workflow.

**`Status`enum:**
-`Modified`— changed but not staged
-`Staged`— staged and ready to commit
-`Committed`— part of a commit
-`Error`— unrecognized state (also the fallback for`statusFromString`on unknown input)

A freshly constructed`TrackedFile`starts as`Modified`, with`size`derived automatically
from the initial content.

Content can be updated via`setContent()`or`updateContent()`— both currently do the same
thing (replace content and recalculate size). The separation exists in case one of them is
eventually reworked to accept a delta rather than the full new content.

`statusToString`/`statusFromString`handle round-tripping the enum to and from strings.
`operator<<`is overloaded for`Status`as a convenience wrapper around`statusToString`.

---

### `Validator` *(unfinished)*
**Files:**`model/Validator.h`,`model/Validator.cpp`

IN PROGRESS

bare base level has been started, but responsibility of this class
was unceremoniously removed from me by Surbhi.

---

## target/ vs tests/

-**target/**— where the main application binary (`FinalProjectCPP`) is output when you
  build the project normally.
-**tests/**— contains the test suite (`test_main.cpp`) and the`TestRunner`binary built
  from it. Tests are hand-rolled with a simple`CHECK`/`SECTION`harness and cover
 `TrackedFile`,`StandardCommit`, and the partial`Validator`. Output is written to
 `tests/test_output.txt`when run via the`RunTests`CMake target.
