#ifndef REPOSITORYMANAGER_H
#define REPOSITORYMANAGER_H

#include <string>
#include <vector>

#include "Repository.h"
#include "DataManager.h"
#include "DiffEngine.h"

using namespace std;

class RepositoryManager {
private:
    Repository repo;
    DataManager dataManager;
    DiffEngine diffEngine;

public:

    bool initRepository(const string RepoName, const string RepoPath);

    bool addFile(const string filePath);

    bool stageFile(const string filePath);

    bool commitChanges(const string message, const string author);

    bool restoreFile(const string commitID, const string filePath);

    bool saveRepository(const string fileName);

    bool loadRepository(const string fileName);

    vector<string> searchCommits(const string searchText);

    string getFileStatus(const string filePath);

    string getDiff(const string oldContent, const string newContent);
};

#endif