#include "DiffEngine.h"
#include <iostream>

string DiffEngine::computeDiff(const string oldContent, const string newContent) {

    string result = "";

    // if same content, no diff
    if (oldContent == newContent) {
        return "No differences found";
    }

    // basic version for now
    // later we can make line by line diff
    result += "Old Content:\n";
    result += oldContent;

    result += "\n\nNew Content:\n";
    result += newContent;

    return result;
}

void DiffEngine::displayDiff(const string diffText) {

    // simple console display for now
    cout << diffText << endl;
}