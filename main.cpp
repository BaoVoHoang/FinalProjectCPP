// main.cpp — application entry point + integration layer.
//
// Author: Bao Vo
//
// This is where the finished pieces of the project are wired together into a
// runnable program. It connects the model classes (TrackedFile / StandardCommit
// / Validator, written by Andrei) to the ConsoleView GUI (view/, written by Bao)
// through a small in-memory RepositorySession.
//
// INTEGRATION NOTE:
//   The session below keeps the working files and commit history in memory so
//   the program is fully runnable today. Once Omer's Repository /
//   RepositoryManager / DataManager classes are ready, this session is the seam
//   where they plug in: each menu handler would delegate to Repository instead
//   of the local containers, and DataManager would persist the history. The
//   ConsoleView and the menu flow stay exactly the same.

#include <algorithm>
#include <chrono>
#include <format>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "ConsoleView.h"
#include "StandardCommit.h"
#include "TrackedFile.h"
#include "Validator.h"

using namespace std;

namespace {

// Current wall-clock time as an ISO-8601 "YYYY-MM-DDTHH:MM:SS" string.
string nowTimestamp() {
    const auto now = chrono::floor<chrono::seconds>(chrono::system_clock::now());
    return format("{:%Y-%m-%dT%H:%M:%S}", now);
}

// A short, stable-ish commit id derived from the commit's content + a counter.
// Good enough to identify commits in this demo; a real VCS would hash the tree.
string makeCommitId(int counter, const string& seed) {
    const size_t h = hash<string>{}(seed + to_string(counter));
    ostringstream oss;
    oss << hex << h;
    const string s = oss.str();
    return s.substr(0, min<size_t>(7, s.size()));
}

// Human-readable text for the model's Validator::Error values.
string errorToString(Error e) {
    switch (e) {
        case Error::Empty:         return "value cannot be empty";
        case Error::TooShort:      return "value is too short (min 3 characters)";
        case Error::TooLong:       return "value is too long (max 20 characters)";
        case Error::NoAlpha:       return "value must contain at least one letter";
        case Error::AlreadyExists: return "value already exists";
        case Error::InvalidPath:   return "value is not a valid path";
        case Error::NotExists:     return "value does not exist";
        default:                   return "invalid value";
    }
}

} // namespace

// -------------------------------------------------------------------------
// RepositorySession — the in-memory integration layer.
// -------------------------------------------------------------------------
class RepositorySession {
    ConsoleView view;
    Validator   validator;
    string      repoName;

    // path -> file. The file's Status doubles as its stage state
    // (Modified = working change, Staged = ready to commit, Committed = in history).
    map<string, TrackedFile> workingFiles;
    vector<StandardCommit>   history;
    int commitCounter = 0;

public:
    void run() {
        setupRepo();
        view.showWelcome(repoName);

        bool running = true;
        while (running) {
            view.showMenu();
            const int choice = view.promptMenuChoice();
            if (!cin) break; // input stream closed (e.g. piped EOF) — exit cleanly

            switch (choice) {
                case 1: addOrUpdateFile(); break;
                case 2: stageFiles();      break;
                case 3: showStatus();      break;
                case 4: commitStaged();    break;
                case 5: showLog();         break;
                case 6: showCommitDetail();break;
                case 0: running = false;   break;
                default: view.showError("Unknown option — please choose a number from the menu.");
            }
        }
        view.showGoodbye(repoName);
    }

private:
    // Ask for a repository name, validating it with the model's Validator.
    void setupRepo() {
        for (int attempt = 0; attempt < 3; ++attempt) {
            const string name = view.promptLine("Name your repository (3-20 letters): ");
            if (!cin) break;
            auto result = validator.validateRepoName(name);
            if (result) {
                repoName = *result;
                return;
            }
            view.showError("Invalid name — " + errorToString(result.error()) + ".");
        }
        repoName = "my-repo"; // sensible fallback so the program can still run
        view.showMessage("Using default repository name 'my-repo'.");
    }

    void addOrUpdateFile() {
        const string path = view.promptLine("File path: ");
        if (path.empty()) {
            view.showError("A file path is required.");
            return;
        }
        const string content = view.promptLine("File content: ");

        auto it = workingFiles.find(path);
        if (it == workingFiles.end()) {
            workingFiles.emplace(path, TrackedFile(path, content));
            view.showMessage("Added '" + path + "' (Modified).");
        } else {
            it->second.updateContent(content);
            it->second.setStatus(Status::Modified);
            view.showMessage("Updated '" + path + "' (marked Modified).");
        }
    }

    void stageFiles() {
        if (workingFiles.empty()) {
            view.showError("Nothing to stage — add a file first.");
            return;
        }
        const string target = view.promptLine("File path to stage (or 'all'): ");

        int staged = 0;
        if (target == "all") {
            for (auto& [path, file] : workingFiles) {
                if (file.getStatus() == Status::Modified) {
                    file.setStatus(Status::Staged);
                    ++staged;
                }
            }
        } else {
            auto it = workingFiles.find(target);
            if (it == workingFiles.end()) {
                view.showError("No tracked file at '" + target + "'.");
                return;
            }
            if (it->second.getStatus() != Status::Modified) {
                view.showError("'" + target + "' has no modifications to stage.");
                return;
            }
            it->second.setStatus(Status::Staged);
            ++staged;
        }

        if (staged == 0) view.showMessage("No modified files to stage.");
        else             view.showMessage("Staged " + to_string(staged) + " file(s).");
    }

    void showStatus() {
        vector<TrackedFile> files;
        files.reserve(workingFiles.size());
        for (auto& [path, file] : workingFiles) files.push_back(file);
        view.showStatus(repoName, files);
    }

    void commitStaged() {
        // Collect everything currently staged.
        map<string, string> snapshot;
        vector<TrackedFile*> stagedFiles;
        for (auto& [path, file] : workingFiles) {
            if (file.getStatus() == Status::Staged) {
                snapshot.emplace(file.getPath(), file.getContent());
                stagedFiles.push_back(&file);
            }
        }
        if (stagedFiles.empty()) {
            view.showError("Nothing staged to commit — use option 2 first.");
            return;
        }

        const string author = view.promptLine("Author: ");
        if (author.empty()) {
            view.showError("An author is required.");
            return;
        }
        const string message = view.promptLine("Commit message: ");
        if (message.empty()) {
            view.showError("A commit message is required.");
            return;
        }
        // NOTE: Validator::validateAuthor / validateMessage are still stubs in the
        // model, so we do a lightweight non-empty check here for now. Once those
        // validators are finished this is where they'd be called.

        const string id = makeCommitId(commitCounter, author + message);
        StandardCommit commit(author, message, nowTimestamp(), id);
        commit.setFileSnapshots(snapshot);

        history.push_back(commit);
        ++commitCounter;

        // Move the committed files out of the staging area.
        for (auto* file : stagedFiles) file->setStatus(Status::Committed);

        view.showMessage("Created commit " + id + " with " +
                         to_string(snapshot.size()) + " file(s).");
    }

    void showLog() {
        view.showLog(history);
    }

    void showCommitDetail() {
        if (history.empty()) {
            view.showMessage("No commits yet — nothing to show.");
            return;
        }
        const string key = view.promptLine("Commit id or number (#): ");

        // Try to match by number first (as shown in the log), then by commit id.
        for (size_t i = 0; i < history.size(); ++i) {
            if (key == "#" + to_string(i + 1) ||
                key == to_string(i + 1) ||
                key == history[i].getCommitID()) {
                view.showCommitDetail(history[i]);
                return;
            }
        }
        view.showError("No commit matching '" + key + "'.");
    }
};

int main() {
    RepositorySession session;
    session.run();
    return 0;
}
