#include "TrackedFile.h"

#include <utility>

string statusToString(const Status status) {
    switch (status) {
        case Status::Modified : return "Modified";
        case Status::Staged   : return "Staged";
        case Status::Committed: return "Committed";
        default               : return "Error";
    }
}

Status statusFromString(const string& status) {
    if (status == "Modified") return Status::Modified;
    if (status == "Staged") return Status::Staged;
    if (status == "Committed") return Status::Committed;
    return Status::Error;
}

// size is derived from content; a freshly tracked file starts out Modified.
TrackedFile::TrackedFile(string path, const string& content)
    : path(std::move(path)),
      content(content),
      size(static_cast<int>(content.size())),
      status(Status::Modified) {}

// TODO: Unsure whether incoming file change data will be difference or just new content
void TrackedFile::updateContent (const string& content) {
    this->content = content;
}

void TrackedFile::displayFileInfo() {}

void TrackedFile::setContent(const string& nContent) {
    this->content = nContent;
    // NOTE: Explicitly converts unsigned double into signed int, which could cause problems.
    // Too bad! (realistically shouldn't encounter this problem but I can fix if needed)
    this->setSize(nContent.size());
}

