#include "db.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
using namespace std;

int isExtMusic(std::filesystem::directory_entry path) {
    std::filesystem::path path1 = path.path();
    std::filesystem::path ext = path1.extension();

    string ext1 = ext.string();

    const string exts[] = {".mp3", ".flac", ".ogg", ".wav", ".m4a"};

    for (string i : exts) {
        if (ext == i)
            return 1;
    }

    return 0;
}

void addToDB(std::filesystem::directory_entry path, string name, string artist, string album, double duration, double date) {
}

int main() {
    string path = "D:/Music/";
    int c = 0;

    for (const auto &file : recursive_directory_iterator(path)) { // iterating through path folder
        if (isExtMusic(file)) {
            c++;
            // cout << file << std::endl;
        }
    }
    cout << c;
}
