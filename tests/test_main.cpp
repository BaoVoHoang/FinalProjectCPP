#include <iostream>
#include <sstream>
#include <map>
#include "TrackedFile.h"
#include "StandardCommit.h"
#include "Validator.h"

using namespace std;

// ---- minimal test harness ----
static int g_passed = 0, g_failed = 0;

#define CHECK(expr) \
    do { \
        if (expr) { cout << "[PASS] " << #expr << "\n"; ++g_passed; } \
        else      { cout << "[FAIL] " << #expr << "\n"; ++g_failed; } \
    } while (0)

#define SECTION(name) cout << "\n=== " << (name) << " ===\n"

static void printSummary() {
    cout << "\n----------------------------------------\n";
    cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
}

// ---- TrackedFile tests ----
static void testTrackedFile() {
    SECTION("TrackedFile: construction");
    {
        TrackedFile f("src/main.cpp", "int main() {}");
        CHECK(f.getPath()    == "src/main.cpp");
        CHECK(f.getContent() == "int main() {}");
        CHECK(f.getSize()    == (int)string("int main() {}").size());
        CHECK(f.getStatus()  == Status::Modified);
    }

    SECTION("TrackedFile: empty content");
    {
        TrackedFile f("empty.txt", "");
        CHECK(f.getSize()    == 0);
        CHECK(f.getContent() == "");
    }

    SECTION("TrackedFile: setStatus");
    {
        TrackedFile f("a.cpp", "x");
        f.setStatus(Status::Staged);
        CHECK(f.getStatus() == Status::Staged);
        f.setStatus(Status::Committed);
        CHECK(f.getStatus() == Status::Committed);
        f.setStatus(Status::Error);
        CHECK(f.getStatus() == Status::Error);
    }

    SECTION("TrackedFile: setContent updates content and size");
    {
        TrackedFile f("a.cpp", "hello");
        f.setContent("hello world");
        CHECK(f.getContent() == "hello world");
        CHECK(f.getSize()    == (int)string("hello world").size());
    }

    SECTION("TrackedFile: updateContent updates content but NOT size (known inconsistency)");
    {
        TrackedFile f("a.cpp", "hello");
        int originalSize = f.getSize();
        f.updateContent("hello world");
        CHECK(f.getContent() == "hello world");
        // updateContent does not recalculate size — this test documents the behaviour
        CHECK(f.getSize() == originalSize);
    }

    SECTION("TrackedFile: setPath");
    {
        TrackedFile f("old/path.cpp", "code");
        f.setPath("new/path.cpp");
        CHECK(f.getPath() == "new/path.cpp");
    }

    SECTION("TrackedFile: statusToString");
    {
        CHECK(statusToString(Status::Modified)  == "Modified");
        CHECK(statusToString(Status::Staged)    == "Staged");
        CHECK(statusToString(Status::Committed) == "Committed");
        CHECK(statusToString(Status::Error)     == "Error");
    }

    SECTION("TrackedFile: statusFromString");
    {
        CHECK(statusFromString("Modified")  == Status::Modified);
        CHECK(statusFromString("Staged")    == Status::Staged);
        CHECK(statusFromString("Committed") == Status::Committed);
        CHECK(statusFromString("garbage")   == Status::Error);
        CHECK(statusFromString("")          == Status::Error);
    }

    SECTION("TrackedFile: operator<< for Status");
    {
        ostringstream oss;
        oss << Status::Modified;
        CHECK(oss.str() == "Modified");
        oss.str("");
        oss << Status::Staged;
        CHECK(oss.str() == "Staged");
    }
}

// ---- Commit / StandardCommit tests ----
static void testCommit() {
    SECTION("StandardCommit: construction and getters");
    {
        StandardCommit c("Alice", "Initial commit", "2026-01-01T00:00:00", "abc123");
        CHECK(c.getAuthor()    == "Alice");
        CHECK(c.getMessage()   == "Initial commit");
        CHECK(c.getTimestamp() == "2026-01-01T00:00:00");
        CHECK(c.getCommitID()  == "abc123");
    }

    SECTION("StandardCommit: setters");
    {
        StandardCommit c("Alice", "msg", "ts", "id");
        c.setAuthor("Bob");
        CHECK(c.getAuthor() == "Bob");
        c.setMessage("Updated message");
        CHECK(c.getMessage() == "Updated message");
        c.setTimestamp("2026-06-01T12:00:00");
        CHECK(c.getTimestamp() == "2026-06-01T12:00:00");
        c.setCommitID("def456");
        CHECK(c.getCommitID() == "def456");
    }

    SECTION("StandardCommit: equality operator (based on commitID)");
    {
        StandardCommit c1("Alice", "msg", "ts", "same-id");
        StandardCommit c2("Bob",   "different msg", "different ts", "same-id");
        StandardCommit c3("Alice", "msg", "ts", "other-id");
        CHECK(c1 == c2);
        CHECK(!(c1 == c3));
    }

    SECTION("StandardCommit: fileSnapshots empty by default");
    {
        StandardCommit c("Alice", "msg", "ts", "id");
        CHECK(c.getFileSnapshots().empty());
    }

    SECTION("StandardCommit: setFileSnapshots / getFileSnapshots roundtrip");
    {
        StandardCommit c("Alice", "msg", "ts", "id");
        map<string, string> snap = {
            {"src/main.cpp", "int main() { return 0; }"},
            {"README.md",    "# Project"},
        };
        c.setFileSnapshots(snap);
        CHECK(c.getFileSnapshots().size() == 2);
        CHECK(c.getFileSnapshots().at("src/main.cpp") == "int main() { return 0; }");
        CHECK(c.getFileSnapshots().at("README.md")    == "# Project");
    }

    SECTION("StandardCommit: overwriting fileSnapshots replaces old data");
    {
        StandardCommit c("Alice", "msg", "ts", "id");
        c.setFileSnapshots({{"a.cpp", "old"}});
        c.setFileSnapshots({{"b.cpp", "new"}});
        CHECK(c.getFileSnapshots().count("a.cpp") == 0);
        CHECK(c.getFileSnapshots().count("b.cpp") == 1);
    }

    SECTION("StandardCommit: getSummary format");
    {
        StandardCommit c("Alice", "Fix bug", "ts", "abc123");
        map<string, string> snap = {{"a.cpp", ""}, {"b.cpp", ""}};
        c.setFileSnapshots(snap);
        string summary = c.getSummary();
        CHECK(summary.find("abc123") != string::npos);
        CHECK(summary.find("Alice")  != string::npos);
        CHECK(summary.find("Fix bug") != string::npos);
        CHECK(summary.find("2") != string::npos); // 2 files
    }

    SECTION("StandardCommit: getSummary with no files");
    {
        StandardCommit c("Alice", "Empty commit", "ts", "xyz");
        string summary = c.getSummary();
        CHECK(summary.find("0") != string::npos);
    }

    SECTION("StandardCommit: operator<< delegates to getSummary");
    {
        StandardCommit c("Alice", "msg", "ts", "id99");
        ostringstream oss;
        oss << c;
        CHECK(oss.str() == c.getSummary());
    }

    SECTION("StandardCommit: displayCommit smoke test (no crash)");
    {
        StandardCommit c("Alice", "Display test", "2026-01-01", "smoke1");
        c.setFileSnapshots({{"main.cpp", "code"}, {"util.cpp", "util"}});
        // Redirect cout so it doesn't pollute test output
        ostringstream sink;
        streambuf* old = cout.rdbuf(sink.rdbuf());
        c.displayCommit();
        cout.rdbuf(old);
        CHECK(sink.str().find("Alice") != string::npos);
        CHECK(sink.str().find("main.cpp") != string::npos);
    }
}

// ---- Validator tests ----
static void testValidator() {
    Validator v;

    SECTION("Validator: validateRepoName - empty string");
    {
        auto result = v.validateRepoName("");
        CHECK(!result.has_value());
        CHECK(result.error() == Error::Empty);
    }

    SECTION("Validator: validateRepoName - too short (< 3 chars)");
    {
        auto result = v.validateRepoName("ab");
        CHECK(!result.has_value());
        CHECK(result.error() == Error::TooShort);
    }

    SECTION("Validator: validateRepoName - single char");
    {
        auto result = v.validateRepoName("x");
        CHECK(!result.has_value());
        CHECK(result.error() == Error::TooShort);
    }

    SECTION("Validator: validateRepoName - too long (> 20 chars)");
    {
        auto result = v.validateRepoName("this-name-is-way-too-long");
        CHECK(!result.has_value());
        CHECK(result.error() == Error::TooLong);
    }

    SECTION("Validator: validateRepoName - valid name");
    {
        auto result = v.validateRepoName("my-repo");
        CHECK(result.has_value());
        CHECK(result.value() == "my-repo");
    }

    SECTION("Validator: validateRepoName - trims leading/trailing spaces");
    {
        auto result = v.validateRepoName("  my-repo  ");
        CHECK(result.has_value());
        CHECK(result.value() == "my-repo");
    }

    SECTION("Validator: validateRepoName - exactly 3 chars");
    {
        auto result = v.validateRepoName("abc");
        CHECK(result.has_value());
    }

    SECTION("Validator: validateRepoName - exactly 20 chars");
    {
        auto result = v.validateRepoName("abcdefghij1234567890");
        CHECK(result.has_value());
    }
}

int main() {
    cout << "========================================\n";
    cout << "  FinalProject Test Suite\n";
    cout << "========================================\n";

    testTrackedFile();
    testCommit();
    testValidator();

    printSummary();
    return g_failed == 0 ? 0 : 1;
}
