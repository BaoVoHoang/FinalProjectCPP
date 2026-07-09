#ifndef TRACKEDFILE_H
#define TRACKEDFILE_H
#include "string"
using namespace std;

enum class Status {
    Modified,
    Staged,
    Committed,
    Error
};

inline string statusToString(const Status status) {
    switch (status) {
        case Status::Modified : return "Modified";
        case Status::Staged   : return "Staged";
        case Status::Committed: return "Committed";
        case Status::Error    : return "Error";
        default               : return "Error";
    }
}

inline Status statusFromString(const string status) {
    if (status.compare("Modified") == 0) return Status::Modified;
    if (status.compare("Staged") == 0) return Status::Modified;
    if (status.compare("Committed") == 0) return Status::Modified;
    return Status::Error;
}

class TrackedFile {
    string path, content;
    int size;
    Status status;

public:
    TrackedFile(string path, string content) : path(path), content(content), status() {}

    void   updateContent(const string& content);
    void   displayFileInfo();

    // getters
    string getPath()         { return path; }
    string getContent()      { return content; }
    int    getSize() const   { return size; }
    Status getStatus() const { return status; }

    // setters
    void   setStatus  (const Status status) { this->status = status; }
    void   setSize    (int size)            { this->size = size; }
    void   setPath    (string path)         { this->path = path; }
    void   setContent (string content) {
        this->content = content;
        // implicitly converts unsigned double into signed int, which could cause problems. Too bad!
        this->setSize(content.size());
    }
};
#endif