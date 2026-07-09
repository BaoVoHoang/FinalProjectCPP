#include "TrackedFile.h"

// TODO: Unsure whether incoming file change data will be difference or just new content
void TrackedFile::updateContent (const string& content) {
    this->content = content;
}

void TrackedFile::displayFileInfo() {

}

