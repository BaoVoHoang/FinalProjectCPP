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

## View / GUI Layer

### `ConsoleView`
**Files:** `view/ConsoleView.h`, `view/ConsoleView.cpp` — *Bao Vo*

The presentation layer — everything the user sees and types goes through here, and
nothing else does. `ConsoleView` renders model objects (a menu, repository status,
the commit log, commit details) and reads raw input; it holds **no business logic
and never mutates model state**. Keeping all I/O in one class means the rest of the
program can run and be tested without a live terminal.

The two `static` formatting helpers — `formatStatusLine()` and
`formatCommitSummary()` — are pure (string in, string out, no I/O), so they are unit
tested directly in `tests/test_main.cpp`.

---

## Application / Integration Layer

### `main.cpp` — *Bao Vo*

The entry point that wires the finished pieces together into a runnable program. It
runs **MiniVCS**, a small interactive version control workflow driven by a
`RepositorySession`:

1. Add / update a file
2. Stage file(s) (`Modified` → `Staged`)
3. Show status
4. Commit staged files (builds a `StandardCommit` with a file snapshot, `Modified`
   files become `Committed`)
5. Show the commit log
6. Show a single commit's details

It exercises the whole model — `TrackedFile`, `StandardCommit`, and `Validator`
(the repository name is validated with `validateRepoName`) — and drives everything
through `ConsoleView`.

**Integration note:** `RepositorySession` currently keeps the working files and
commit history **in memory**, so the program is fully runnable today. That session
is the seam where Omer's `Repository` / `RepositoryManager` / `DataManager` classes
plug in once they are ready — the menu handlers would delegate to `Repository`
instead of the local containers, and the view/menu flow stays unchanged.

---

## Building & Running

The project uses **CMake** (C++23). From the repository root:

```bash
cmake -S . -B build
cmake --build build
```

The main binary is written to `target/` and the test binary to `tests/`.

Run the app:

```bash
./target/FinalProjectCPP        # (target/Debug/FinalProjectCPP.exe with the VS/MSVC generator)
```

---

## target/ vs tests/

-**target/**— where the main application binary (`FinalProjectCPP`) is output when you
  build the project normally.
-**tests/**— contains the test suite (`test_main.cpp`) and the`TestRunner`binary built
  from it. Tests are hand-rolled with a simple`CHECK`/`SECTION`harness and cover
 `TrackedFile`,`StandardCommit`,`ConsoleView`, and the partial`Validator`. Output is
  written to`tests/test_output.txt`when run via the`RunTests`CMake target.

---

## Team & Task Division

| Member | Responsibilities |
|--------|------------------|
| Andrei Cojocaru | Model layer: `Commit`, `StandardCommit`, `TrackedFile`, `DiffEngine`, `Validator` |
| Omer Ozkaya | `Repository`, `RepositoryManager`, `DataManager`, `AnalyticsEngine` |
| Bao Vo | `main.cpp`, GUI (`view/ConsoleView`), `CMakeLists.txt`, `README.md`, testing, integration |
