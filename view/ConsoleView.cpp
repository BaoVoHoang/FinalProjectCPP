#include "ConsoleView.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

using namespace std;

// A plain horizontal rule used to frame sections of output.
string ConsoleView::divider() {
    return string(48, '-');
}

void ConsoleView::showWelcome(const string& repoName) const {
    cout << "\n" << divider() << "\n";
    cout << "  MiniVCS — a tiny C++ version control system\n";
    cout << "  Repository: " << repoName << "\n";
    cout << divider() << "\n";
}

void ConsoleView::showMenu() const {
    cout << "\n" << divider() << "\n";
    cout << "  1) Add / update a file\n";
    cout << "  2) Stage file(s)\n";
    cout << "  3) Show status\n";
    cout << "  4) Commit staged files\n";
    cout << "  5) Show commit log\n";
    cout << "  6) Show commit details\n";
    cout << "  0) Exit\n";
    cout << divider() << "\n";
}

void ConsoleView::showMessage(const string& msg) const {
    cout << "  " << msg << "\n";
}

void ConsoleView::showError(const string& msg) const {
    cout << "  [!] " << msg << "\n";
}

void ConsoleView::showStatus(const string& repoName, vector<TrackedFile> files) const {
    cout << "\nStatus of '" << repoName << "':\n";
    if (files.empty()) {
        cout << "  (no files tracked yet — use option 1 to add one)\n";
        return;
    }
    for (auto& file : files) {
        cout << formatStatusLine(file) << "\n";
    }
}

void ConsoleView::showLog(const vector<StandardCommit>& commits) const {
    cout << "\nCommit log (" << commits.size() << " commit(s)):\n";
    if (commits.empty()) {
        cout << "  (no commits yet)\n";
        return;
    }
    // Newest first so the most recent work is at the top, like a real VCS log.
    for (size_t i = commits.size(); i-- > 0; ) {
        cout << "  " << formatCommitSummary(static_cast<int>(i) + 1, commits[i]) << "\n";
    }
}

void ConsoleView::showCommitDetail(StandardCommit& commit) const {
    cout << "\n" << divider() << "\n";
    // displayCommit() is the model's own detailed printer.
    commit.displayCommit();
    cout << "\n" << divider() << "\n";
}

void ConsoleView::showGoodbye(const string& repoName) const {
    cout << "\nClosing repository '" << repoName << "'. Goodbye!\n";
}

int ConsoleView::promptMenuChoice() const {
    const string line = promptLine("Choice> ");
    try {
        size_t pos = 0;
        const int value = stoi(line, &pos);
        // Reject trailing garbage like "3x" so it doesn't silently parse as 3.
        if (pos != line.size()) return -1;
        return value;
    } catch (...) {
        return -1;
    }
}

string ConsoleView::promptLine(const string& prompt) const {
    cout << prompt;
    cout.flush();
    string line;
    if (!getline(cin, line)) return "";
    // getline splits on '\n', so input with Windows CRLF line endings (e.g. a
    // piped file) leaves a trailing '\r'. Drop it so keyword and number matching
    // ("all", "#1", menu choices) behaves the same regardless of line endings.
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return line;
}

string ConsoleView::formatStatusLine(TrackedFile file) {
    ostringstream oss;
    // Left-justify the status tag (widest is "Committed" = 9 chars) so paths align.
    oss << "  [" << left << setw(9) << statusToString(file.getStatus()) << "] "
        << file.getPath() << " (" << file.getSize() << " bytes)";
    return oss.str();
}

string ConsoleView::formatCommitSummary(int index, const StandardCommit& commit) {
    ostringstream oss;
    oss << "#" << index << "  " << commit.getSummary();
    return oss.str();
}
