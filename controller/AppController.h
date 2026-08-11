/* AppController - the front end's view of the system.
 *
 * Author: Bao Vo
 *
 * WHY THIS CLASS EXISTS
 * ---------------------
 * controller/RepositoryManager (Omer) is a thin bridge: each method forwards
 * straight to Repository / DataManager / DiffEngine and returns the bool it
 * gets back. That is all the console prototype needed.
 *
 * A GUI needs more than a bool. When "Stage" does nothing the window has to say
 * WHY - not tracked? already staged? no repository yet? It also needs to read
 * the repository to draw its lists, to normalise the paths a user types, and to
 * validate a text field while it is still being typed.
 *
 * Rather than push all of that into Omer's class, it lives here. AppController
 * OWNS a RepositoryManager and delegates every actual repository operation to
 * it, so his code does the real work; this class only adds the layer the
 * interface needs:
 *
 *   - path handling      : what the user types -> a repository-relative path
 *   - explanation        : a readable reason for every outcome, in lastMessage()
 *   - input validation   : via the model's Validator, before the model sees it
 *   - full snapshots     : carries the previous commit's files forward
 *   - disk access        : creating, writing and re-reading working files
 *
 * Everything under model/ and Omer's RepositoryManager stay exactly as their
 * authors wrote them.
 */
#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

// <map> and StandardCommit.h come before Analytics.h on purpose: Analytics.h
// uses map and StandardCommit without including either, so they have to be
// visible by the time the template is parsed.
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Commit.h"
#include "StandardCommit.h"
#include "TrackedFile.h"
#include "Analytics.h"
#include "Repository.h"
#include "Validator.h"
#include "RepositoryManager.h"

using namespace std;

class AppController {
private:
    /* Omer's bridge. Marked mutable because the read-only getters below are
     * const, while his getRepository() is not - reading the repository through
     * a const AppController does not change anything the caller can observe.
     */
    mutable RepositoryManager mur;

    Validator validator;

    /* AnalyticsEngine is instantiated on a REFERENCE type.
     *
     * Two of its methods take "const T" by value. With T = vector<unique_ptr<Commit>>
     * that means copying a vector of unique_ptr, which does not compile. Making T
     * a reference collapses "const T" back to a reference, so the same template
     * works untouched and nothing is copied.
     */
    AnalyticsEngine<vector<unique_ptr<Commit>>&> commitAnalytics;
    AnalyticsEngine<vector<TrackedFile>&>        fileAnalytics;

    // explanation of the most recent operation, success or failure
    string lastMsg;

    // helpers: record a message and return the matching bool
    bool fail(const string message);
    bool succeed(const string message);

    // turns a Validator::Error into wording a user can act on
    static string describe(Error error, const string& subject);

    /* Shorthand for the repository living inside the manager.
     *
     * Callable from the const getters below because 'mur' is mutable. It hands
     * back a non-const Repository& because Omer's getFiles()/getCommits() are
     * not const-qualified; the const getters narrow that to a const& before any
     * caller sees it, so nothing outside this class can write through it.
     */
    Repository& repo() const { return mur.getRepository(); }

public:

    bool initRepository(const string RepoName, const string RepoPath);

    bool saveRepository(const string fileName);

    bool loadRepository(const string fileName);

    vector<string> searchCommits(const string searchText);

    bool addFile(const string filePath);

    // Creates a new file on disk with the given content and starts tracking it.
    // Refuses to overwrite a file that already exists.
    bool createAndTrackFile(const string filePath, const string content);

    // Writes a tracked file's current content back out to disk - this is what
    // makes a restored snapshot show up in the working directory.
    bool writeFileToDisk(const string filePath);

    bool stageFile(const string filePath);

    // Stages every file currently marked Modified. Returns false when there was
    // nothing to stage.
    bool stageAllFiles();

    bool commitChanges(const string message, const string author);

    bool restoreFile(const string commitID, const string filePath);

    // Re-reads a tracked file from disk, picking up edits made outside the app.
    bool refreshFile(const string filePath);

    string getFileStatus(const string filePath);

    /* Live-validation helpers for the GUI.
     *
     * Each returns an empty string when the value is acceptable, or the reason
     * it was rejected. They let the front end highlight a field the moment it
     * becomes invalid WITHOUT the view having to know any of the rules - the
     * answer still comes from the model's Validator.
     */
    string checkRepoName(const string value);
    string checkRepoPath(const string value);
    string checkAuthor(const string value);
    string checkMessage(const string value);

    string getDiff(const string oldContent, const string newContent);

    // Diffs a tracked file's current content against the copy stored in a commit.
    // Returns false (with an explanation in lastMessage) if either side is missing.
    bool diffFileAgainstCommit(const string commitID, const string filePath, string& outDiff);

    /* Paths work the way they do in git: a file is identified by its path
     * RELATIVE TO THE REPOSITORY ROOT, and that relative form is what gets
     * stored, displayed, snapshotted and saved.
     *
     * resolvePath    - relative form -> the path used for actual disk access.
     * toRepoRelative - whatever the user typed -> the stored relative form. An
     *                  absolute path inside the repository is shortened; one
     *                  pointing outside it is kept as-is so files elsewhere on
     *                  the machine can still be tracked.
     */
    string resolvePath(const string& filePath) const;
    string toRepoRelative(const string& filePath) const;

    // ----- read-only access for the view -----
    const string& lastMessage() const { return lastMsg; }

    bool   isInitialized()     const { return repo().isInitialized(); }
    string getRepositoryName() const { return repo().getRepositoryName(); }
    string getRepositoryPath() const { return repo().getRepPath(); }

    const vector<TrackedFile>&        getFiles()   const { return repo().getFiles(); }
    const vector<unique_ptr<Commit>>& getCommits() const { return repo().getCommits(); }

    // Content of a tracked file, or false when the path is not tracked.
    bool getFileContent(const string filePath, string& outContent);

    // Lookup helpers. Return nullptr when there is no match.
    const TrackedFile* findFile(const string filePath) const;
    const Commit*      findCommit(const string commitID) const;

    // True when the file is already tracked by this repository.
    bool isTracked(const string filePath) const;

    // Number of files currently sitting in the staging area.
    int  countStaged() const;

    // ----- analytics -----
    int    getTotalCommits()      { return commitAnalytics.computeTotalCommits(repo().getCommits()); }
    int    getTrackedFileCount()  { return fileAnalytics.computeTrackedFilesCount(repo().getFiles()); }
    string getMostModifiedFile()  { return commitAnalytics.computeMostModifiedFiles(repo().getCommits()); }
    int    getStagedCount() const { return countStaged(); }
};

#endif
