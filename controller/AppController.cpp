/* Implementation of AppController - see AppController.h for why it exists.
 *
 * Author: Bao Vo
 *
 * Every repository operation below ends up calling the matching method on the
 * RepositoryManager member (Omer's code), which forwards it to the model. What
 * this file adds around those calls is the checking, path handling and wording
 * that a graphical front end needs and a bool cannot carry.
 */
#include "AppController.h"

#include <filesystem>
#include <fstream>

namespace {

// Reads a whole file off disk into a string. Returns false when the file cannot
// be opened. Lines are re-joined with '\n' so a file written on Windows and one
// written on Linux produce identical tracked content.
bool readWholeFile(const string& filePath, string& outContent) {
    ifstream inFile;
    inFile.open(filePath);

    if (!inFile) {
        return false;
    }

    string content;
    string line;

    while (getline(inFile, line)) {

        // strip the '\r' left behind by a CRLF ending
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        content += line;
        content += "\n";
    }

    inFile.close();
    outContent = content;

    return true;
}

// Writes content out to a file, creating the containing folder if needed so
// that tracking "src/notes.txt" in a fresh repository works without the user
// having to make the folder first.
bool writeWholeFile(const string& filePath, const string& content) {
    const std::filesystem::path target(filePath);

    if (target.has_parent_path()) {
        error_code ignored;
        std::filesystem::create_directories(target.parent_path(), ignored);
    }

    ofstream outFile;
    outFile.open(target);

    if (!outFile) {
        return false;
    }

    outFile << content;
    outFile.close();

    return !outFile.fail();
}

} // namespace

/* Structural check of a save file, run BEFORE DataManager is allowed near it.
 *
 * DataManager::loadData assumes a well-formed file: it feeds each count line
 * straight to stoi() and clears the repository before it starts reading. A
 * truncated or hand-edited file therefore throws part-way through, after the
 * caller's data has already been discarded.
 *
 * Rather than change DataManager, the file is parsed once here, reading nothing
 * into the repository. Only a file that survives this dry run is handed over,
 * so a bad file is reported as a failed load and the repository is left alone.
 *
 * The layout mirrors DataManager::saveData exactly:
 *   name / path / fileCount   then per file: path, status, <size>, <content>
 *   commitCount               then per commit: id, author, message, timestamp,
 *                             snapshotCount, then per snapshot: path, <size>, <content>
 */
namespace {

// One line that must parse as a non-negative count.
bool scanCount(ifstream& in, int& outCount) {
    string line;

    if (!getline(in, line)) {
        return false;
    }

    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    if (line.empty()) {
        return false;
    }

    for (char character : line) {

        if (!isdigit(static_cast<unsigned char>(character))) {
            return false;
        }
    }

    outCount = stoi(line);

    return true;
}

// A size line followed by exactly that many bytes, then the separating newline.
bool scanTextBlock(ifstream& in) {
    int size = 0;

    if (!scanCount(in, size)) {
        return false;
    }

    if (size > 0) {
        string buffer;
        buffer.resize(size);
        in.read(&buffer[0], size);

        if (in.gcount() != size) {
            return false;
        }
    }

    string discard;
    getline(in, discard);

    return true;
}

bool saveFileIsWellFormed(const string& fileName) {
    ifstream in(fileName);

    if (!in) {
        return false;
    }

    string repoName;
    string repoPath;

    if (!getline(in, repoName) || !getline(in, repoPath)) {
        return false;
    }

    int fileCount = 0;

    if (!scanCount(in, fileCount)) {
        return false;
    }

    for (int i = 0; i < fileCount; i++) {
        string path;
        string status;

        if (!getline(in, path) || !getline(in, status)) {
            return false;
        }

        if (!scanTextBlock(in)) {
            return false;
        }
    }

    int commitCount = 0;

    if (!scanCount(in, commitCount)) {
        return false;
    }

    for (int i = 0; i < commitCount; i++) {
        string id;
        string author;
        string message;
        string timestamp;

        if (!getline(in, id) || !getline(in, author) ||
            !getline(in, message) || !getline(in, timestamp)) {
            return false;
        }

        int snapshotCount = 0;

        if (!scanCount(in, snapshotCount)) {
            return false;
        }

        for (int j = 0; j < snapshotCount; j++) {
            string snapshotPath;

            if (!getline(in, snapshotPath)) {
                return false;
            }

            if (!scanTextBlock(in)) {
                return false;
            }
        }
    }

    return true;
}

} // namespace

bool AppController::fail(const string message) {
    lastMsg = message;
    return false;
}

bool AppController::succeed(const string message) {
    lastMsg = message;
    return true;
}

string AppController::describe(Error error, const string& subject) {
    switch (error) {
        case Error::Empty:         return subject + " cannot be empty";
        case Error::TooShort:      return subject + " is too short";
        case Error::TooLong:       return subject + " is too long";
        case Error::NoAlpha:       return subject + " must contain at least one letter";
        case Error::AlreadyExists: return subject + " already exists";
        case Error::InvalidPath:   return subject + " is not valid";
        case Error::NotExists:     return subject + " does not exist";
        default:                   return subject + " is invalid";
    }
}

// ---------------------------------------------------------------------------
// Path handling
// ---------------------------------------------------------------------------

string AppController::resolvePath(const string& filePath) const {
    namespace fs = std::filesystem;

    const fs::path given(filePath);
    const string   base = repo().getRepPath();

    // an absolute path is already the real location; a repository without a
    // path has nothing to resolve against
    if (given.is_absolute() || base.empty()) {
        return filePath;
    }

    return (fs::path(base) / given).generic_string();
}

string AppController::toRepoRelative(const string& filePath) const {
    namespace fs = std::filesystem;

    const fs::path given(filePath);
    const string   basePath = repo().getRepPath();

    if (!given.is_absolute() || basePath.empty()) {
        return filePath;
    }

    error_code errorCode;
    const fs::path base = fs::absolute(fs::path(basePath), errorCode);

    if (errorCode) {
        return filePath;
    }

    const fs::path relative = fs::relative(given, base, errorCode);

    if (errorCode || relative.empty()) {
        return filePath;
    }

    const string relativeText = relative.generic_string();

    // a leading ".." means the file sits outside the repository, so keep the
    // absolute path rather than pretending it is inside
    if (relativeText.starts_with("..")) {
        return filePath;
    }

    return relativeText;
}

// ---------------------------------------------------------------------------
// Lookup helpers
// ---------------------------------------------------------------------------

const TrackedFile* AppController::findFile(const string filePath) const {

    // accepts either form: what the user typed or the stored relative path
    const string storedPath = toRepoRelative(filePath);

    for (const auto& file : repo().getFiles()) {

        if (file.getPath() == storedPath) {
            return &file;
        }
    }

    return nullptr;
}

const Commit* AppController::findCommit(const string commitID) const {

    for (const auto& commit : repo().getCommits()) {

        if (commit->getCommitID() == commitID) {
            return commit.get();
        }
    }

    return nullptr;
}

bool AppController::isTracked(const string filePath) const {
    return findFile(filePath) != nullptr;
}

int AppController::countStaged() const {
    int staged = 0;

    for (const auto& file : repo().getFiles()) {

        if (file.getStatus() == Status::Staged) {
            staged++;
        }
    }

    return staged;
}

// ---------------------------------------------------------------------------
// Repository operations
// ---------------------------------------------------------------------------

bool AppController::initRepository(const string RepoName, const string RepoPath) {

    // the raw input is validated here so the model only ever stores clean values
    auto checkedName = validator.validateRepoName(RepoName);

    if (!checkedName) {
        return fail("Repository name rejected: " + describe(checkedName.error(), "the name") + ".");
    }

    auto checkedPath = validator.validateRepoPath(RepoPath);

    if (!checkedPath) {
        return fail("Repository path rejected: " + describe(checkedPath.error(), "the path") + ".");
    }

    if (!mur.initRepository(*checkedName, *checkedPath)) {
        return fail("Repository could not be initialized.");
    }

    return succeed("Repository '" + *checkedName + "' initialized at " + *checkedPath + ".");
}

bool AppController::addFile(const string filePath) {

    if (!isInitialized()) {
        return fail("Initialize a repository first.");
    }

    auto checkedPath = validator.validateRepoPath(filePath);

    if (!checkedPath) {
        return fail("File path rejected: " + describe(checkedPath.error(), "the path") + ".");
    }

    // paths are repository-relative, so report and probe the same location the
    // model will actually read from
    const string storedPath = toRepoRelative(*checkedPath);
    const string diskPath   = resolvePath(storedPath);

    if (isTracked(storedPath)) {
        return fail("'" + storedPath + "' is already tracked.");
    }

    /* Repository::addFile opens the path it is given, so it has to be handed the
     * resolved location. It then stores that same string as the file's path, so
     * for a relative path the tracked entry is renamed to the stored form below
     * - that keeps every later lookup working off the repository-relative name.
     */
    ifstream probe(diskPath);

    if (!probe) {
        return fail("Cannot open '" + diskPath + "' - check the file exists and the path is right.");
    }

    probe.close();

    if (!mur.addFile(diskPath)) {
        return fail("'" + storedPath + "' could not be added.");
    }

    for (auto& file : repo().getFiles()) {

        if (file.getPath() == diskPath) {
            file.setPath(storedPath);
            break;
        }
    }

    return succeed("Tracking '" + storedPath + "' (Modified).");
}

bool AppController::createAndTrackFile(const string filePath, const string content) {

    if (!isInitialized()) {
        return fail("Initialize a repository first.");
    }

    auto checkedPath = validator.validateRepoPath(filePath);

    if (!checkedPath) {
        return fail("File path rejected: " + describe(checkedPath.error(), "the path") + ".");
    }

    const string storedPath = toRepoRelative(*checkedPath);
    const string diskPath   = resolvePath(storedPath);

    // never clobber something that is already on disk
    ifstream probe(diskPath);

    if (probe) {
        probe.close();
        return fail("'" + diskPath + "' already exists - use 'Track a file' instead.");
    }

    if (!writeWholeFile(diskPath, content)) {
        return fail("Could not write '" + diskPath + "'.");
    }

    if (!mur.addFile(diskPath)) {
        return fail("'" + storedPath + "' was written but could not be tracked.");
    }

    for (auto& file : repo().getFiles()) {

        if (file.getPath() == diskPath) {
            file.setPath(storedPath);
            break;
        }
    }

    return succeed("Created and now tracking '" + storedPath + "' at " + diskPath + ".");
}

bool AppController::writeFileToDisk(const string filePath) {

    const TrackedFile* file = findFile(filePath);

    if (file == nullptr) {
        return fail("'" + filePath + "' is not tracked.");
    }

    if (!writeWholeFile(resolvePath(toRepoRelative(filePath)), file->getContent())) {
        return fail("Could not write '" + filePath + "' to disk.");
    }

    return succeed("Wrote '" + filePath + "' to disk (" + to_string(file->getSize()) + " bytes).");
}

bool AppController::stageFile(const string filePath) {

    if (!isInitialized()) {
        return fail("Initialize a repository first.");
    }

    const TrackedFile* file = findFile(filePath);

    if (file == nullptr) {
        return fail("'" + filePath + "' is not tracked - add it first.");
    }

    if (file->getStatus() == Status::Staged) {
        return fail("'" + filePath + "' is already staged.");
    }

    // Omer's stageFile compares the stored path exactly, so hand it that form
    if (!mur.stageFile(toRepoRelative(filePath))) {
        return fail("'" + filePath + "' could not be staged.");
    }

    return succeed("Staged '" + filePath + "'.");
}

bool AppController::stageAllFiles() {

    if (!isInitialized()) {
        return fail("Initialize a repository first.");
    }

    int staged = 0;

    // collected first: staging mutates the entries being walked
    vector<string> toStage;

    for (const auto& file : repo().getFiles()) {

        if (file.getStatus() == Status::Modified) {
            toStage.push_back(file.getPath());
        }
    }

    for (const auto& path : toStage) {

        if (mur.stageFile(path)) {
            staged++;
        }
    }

    if (staged == 0) {
        return fail("No modified files to stage.");
    }

    return succeed("Staged " + to_string(staged) + " file(s).");
}

bool AppController::commitChanges(const string message, const string author) {

    if (!isInitialized()) {
        return fail("Initialize a repository first.");
    }

    if (countStaged() == 0) {
        return fail("Nothing staged to commit - stage a file first.");
    }

    auto checkedAuthor = validator.validateAuthor(author);

    if (!checkedAuthor) {
        return fail("Author rejected: " + describe(checkedAuthor.error(), "the author") + ".");
    }

    auto checkedMessage = validator.validateMessage(message);

    if (!checkedMessage) {
        return fail("Message rejected: " + describe(checkedMessage.error(), "the message") + ".");
    }

    /* A StandardCommit is documented to hold a FULL snapshot of the repository,
     * but Repository::commitChanges records only the files staged this time
     * round. The previous commit's snapshot is captured here and merged back in
     * afterwards, so files committed earlier stay reachable by restoreFile
     * instead of silently disappearing from later commits.
     */
    auto& commits = repo().getCommits();

    map<string, string> carried;

    if (!commits.empty()) {
        StandardCommit* previous = dynamic_cast<StandardCommit*>(commits.back().get());

        if (previous != nullptr) {
            carried = previous->getFileSnapshots();
        }
    }

    if (!mur.commitChanges(*checkedMessage, *checkedAuthor)) {
        return fail("Commit failed.");
    }

    StandardCommit* created = dynamic_cast<StandardCommit*>(commits.back().get());

    if (created != nullptr && !carried.empty()) {

        // newly staged files win over the copies carried forward
        map<string, string> merged = carried;

        for (const auto& snapshot : created->getFileSnapshots()) {
            merged[snapshot.first] = snapshot.second;
        }

        created->setFileSnapshots(merged);
    }

    const Commit* newest = commits.back().get();

    return succeed("Created commit " + newest->getCommitID() + " - " + newest->getSummary() + ".");
}

bool AppController::restoreFile(const string commitID, const string filePath) {

    if (!isInitialized()) {
        return fail("Initialize a repository first.");
    }

    auto checkedID = validator.validateCommitID(commitID);

    if (!checkedID) {
        return fail("Commit id rejected: " + describe(checkedID.error(), "the commit id") + ".");
    }

    if (findCommit(*checkedID) == nullptr) {
        return fail("No commit with id '" + *checkedID + "'.");
    }

    if (!isTracked(filePath)) {
        return fail("'" + filePath + "' is not tracked.");
    }

    if (!mur.restoreFile(*checkedID, toRepoRelative(filePath))) {
        return fail("Commit " + *checkedID + " has no snapshot of '" + filePath + "'.");
    }

    return succeed("Restored '" + filePath + "' from commit " + *checkedID + " (now Modified).");
}

bool AppController::refreshFile(const string filePath) {

    if (!isInitialized()) {
        return fail("Initialize a repository first.");
    }

    if (!isTracked(filePath)) {
        return fail("'" + filePath + "' is not tracked.");
    }

    const string storedPath = toRepoRelative(filePath);

    string content;

    if (!readWholeFile(resolvePath(storedPath), content)) {
        return fail("Cannot re-read '" + filePath + "' from disk.");
    }

    for (auto& file : repo().getFiles()) {

        if (file.getPath() == storedPath) {

            // an unchanged file keeps whatever status it already had
            if (content != file.getContent()) {
                file.setContent(content);
                file.setStatus(Status::Modified);
            }

            break;
        }
    }

    return succeed("Re-read '" + filePath + "' from disk (status: " + getFileStatus(filePath) + ").");
}

// ---------------------------------------------------------------------------
// Persistence and queries - all handled by Omer's manager
// ---------------------------------------------------------------------------

bool AppController::saveRepository(const string fileName) {

    if (!isInitialized()) {
        return fail("Initialize a repository first.");
    }

    if (fileName.empty()) {
        return fail("A save file name is required.");
    }

    if (!mur.saveRepository(fileName)) {
        return fail("Could not write to '" + fileName + "'.");
    }

    return succeed("Saved " + to_string(repo().getFiles().size()) + " file(s) and " +
                   to_string(repo().getCommits().size()) + " commit(s) to '" + fileName + "'.");
}

bool AppController::loadRepository(const string fileName) {

    if (fileName.empty()) {
        return fail("A save file name is required.");
    }

    // checked before DataManager sees it, so a bad file cannot leave the
    // repository half-cleared (see saveFileIsWellFormed above)
    if (!saveFileIsWellFormed(fileName)) {
        return fail("Could not load '" + fileName + "' - the file is missing or not a valid save file.");
    }

    if (!mur.loadRepository(fileName)) {
        return fail("Could not load '" + fileName + "' - the file is missing or not a valid save file.");
    }

    return succeed("Loaded repository '" + repo().getRepositoryName() + "' - " +
                   to_string(repo().getFiles().size()) + " file(s), " +
                   to_string(repo().getCommits().size()) + " commit(s).");
}

vector<string> AppController::searchCommits(const string searchText) {

    return mur.searchCommits(searchText);
}

string AppController::getFileStatus(const string filePath) {

    // hand Omer's exact-match lookup the stored repository-relative form
    return mur.getFileStatus(toRepoRelative(filePath));
}

// ---------------------------------------------------------------------------
// Live validation for the GUI
// ---------------------------------------------------------------------------

string AppController::checkRepoName(const string value) {
    auto checked = validator.validateRepoName(value);
    return checked ? "" : describe(checked.error(), "the name");
}

string AppController::checkRepoPath(const string value) {
    auto checked = validator.validateRepoPath(value);
    return checked ? "" : describe(checked.error(), "the path");
}

string AppController::checkAuthor(const string value) {
    auto checked = validator.validateAuthor(value);
    return checked ? "" : describe(checked.error(), "the author");
}

string AppController::checkMessage(const string value) {
    auto checked = validator.validateMessage(value);
    return checked ? "" : describe(checked.error(), "the message");
}

// ---------------------------------------------------------------------------
// Diffing
// ---------------------------------------------------------------------------

string AppController::getDiff(const string oldContent, const string newContent) {

    return mur.getDiff(oldContent, newContent);
}

bool AppController::diffFileAgainstCommit(const string commitID, const string filePath, string& outDiff) {

    if (!isInitialized()) {
        return fail("Initialize a repository first.");
    }

    const Commit* commit = findCommit(commitID);

    if (commit == nullptr) {
        return fail("No commit with id '" + commitID + "'.");
    }

    // the snapshot map only exists on the concrete commit type
    const StandardCommit* standard = dynamic_cast<const StandardCommit*>(commit);

    if (standard == nullptr) {
        return fail("Commit " + commitID + " stores no file snapshots.");
    }

    const map<string, string>& snapshots = standard->getFileSnapshots();
    auto snapshot = snapshots.find(toRepoRelative(filePath));

    if (snapshot == snapshots.end()) {
        return fail("Commit " + commitID + " has no snapshot of '" + filePath + "'.");
    }

    const TrackedFile* current = findFile(filePath);

    if (current == nullptr) {
        return fail("'" + filePath + "' is not tracked.");
    }

    outDiff = mur.getDiff(snapshot->second, current->getContent());

    return succeed("Diff of '" + filePath + "': commit " + commitID + " -> working copy.");
}

bool AppController::getFileContent(const string filePath, string& outContent) {

    const TrackedFile* file = findFile(filePath);

    if (file == nullptr) {
        return fail("'" + filePath + "' is not tracked.");
    }

    outContent = file->getContent();

    return succeed("Content of '" + filePath + "'.");
}
