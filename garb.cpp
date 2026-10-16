// #include <db.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <sqlite3.h>
#include <string>
#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;
using namespace std;
namespace tl = TagLib;

sqlite3 *db;
int exit = 0;

static int callback(void *data, int argc, char **argv, char **azColName) {
    int i;
    fprintf(stderr, "%s: ", (const char *)data);

    for (i = 0; i < argc; i++) {

        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");
    return 0;
}

int initDb() {

    if (sqlite3_open("data.db", &db) != SQLITE_OK) {
        cerr << "failed to open db" << sqlite3_errmsg(db) << "\n";
        return 1;
    }

    // checking if schema already exists
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT name FROM sqlite_master WHERE type='table' AND name='songs';", -1, &stmt, nullptr);
    bool schema_exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    if (!schema_exists) {
        char *errormsg = nullptr;
        const char *schema =
            "CREATE TABLE artists(id INTEGER PRIMARY KEY, artist TEXT);"
            "CREATE TABLE albums(id INTEGER PRIMARY KEY, album TEXT, artist_id INT);"
            "CREATE TABLE songs(id INTEGER PRIMARY KEY, title TEXT, length INT, "
            "path TEXT, year INT, genre TEXT, artist_id INT, album_id INT,"
            " FOREIGN KEY(artist_id) REFERENCES artists(id),"
            " FOREIGN KEY(album_id) REFERENCES albums(id));";

        if (sqlite3_exec(db, schema, nullptr, nullptr, &errormsg) != SQLITE_OK) {
            cerr << "schema error" << errormsg << "\n";
            sqlite3_free(errormsg);
        }
    }
    return 0;
}

struct Metadata {
    std::string title;
    std::string genre;
    std::string artist;
    std::string album;
    int length;
    int year;
    int track;
};

int c;

Metadata getMetadata(fs::path track_path) {

    tl::FileRef track(fs::path(track_path).c_str());
    Metadata data;

    if (!track.isNull() && track.tag()) {

        TagLib::Tag *tag = track.tag();

        data.title = tag->title().to8Bit(true);
        data.artist = tag->artist().to8Bit(true);
        data.album = tag->album().to8Bit(true);
        data.genre = tag->genre().to8Bit(true);
        data.year = tag->year();
        data.track = tag->track();

        if (track.audioProperties()) {
            data.length = track.audioProperties()->lengthInSeconds();
            c += 1;
        }
    }

    return data;
}

struct DbCache {
    unordered_map<string, int> artist_ids;
    unordered_map<string, int> album_ids;
    unordered_map<string, int> song_paths;
};

DbCache initDbCache() {
    DbCache cache;
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(db, "SELECT path from songs;", -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        cache.artist_ids[(const char *)sqlite3_column_text(stmt, 0)] = sqlite3_column_int(stmt, 1);
    }
}

int scan(fs::path folder) {

    vector<string> SUPPORTED = {
        ".mp3",
        ".flac",
        ".wav",
        ".ogg",
        ".m4a",
    };

    if (fs::is_directory(folder)) { // checking if given folder is a directory
        // create data.db if doesn't exist

        initDb();

        // walk
        for (const auto &file : fs::recursive_directory_iterator(folder)) {

            if (fs::is_directory(file.path())) {
                continue;
            }

            string ext = file.path().extension().string();

            if (find(SUPPORTED.begin(), SUPPORTED.end(), ext) != SUPPORTED.end()) {
                getMetadata(file.path().c_str());
            }
        }
    }
    cout << c;

    return 0;
}

int main() {

    scan("D:/music");

    return 0;
}

int main() {
    // Optional: pass a db path as the first argument, otherwise use the default.
    std::string db_path = "D:/vibhu/code/c++/music/data.db";

    try {
        SongDB db(db_path);
        std::cout << "Opened " << db_path << "\n\n";

        // ---- 1. INSERT ----
        std::cout << "== Adding songs ==\n";
        std::vector<Song> to_add = {
            makeSong("Time", "Pink Floyd", "The Dark Side of the Moon", 4, 413000, "D:/Music/pf/time.flac", "flac"),
            makeSong("Money", "Pink Floyd", "The Dark Side of the Moon", 6, 382000, "D:/Music/pf/money.flac", "flac"),
            makeSong("Comfortably Numb", "Pink Floyd", "The Wall", 6, 383000, "D:/Music/pf/numb.flac", "flac"),
            makeSong("Get Lucky", "Daft Punk", "Random Access Memories", 8, 369000, "D:/Music/dp/get_lucky.mp3", "mp3"),
        };
        for (const auto &s : to_add) {
            try {
                int id = db.add(s);
                std::cout << "added  id=" << id << "  " << s.title << '\n';
            } catch (const SongDbError &e) {
                // Running the demo twice lands here: file_path is UNIQUE.
                std::cout << "skipped " << s.title << "  (" << e.what() << ")\n";
            }
        }
        std::cout << '\n';

        // ---- 2. READ ALL ----
        std::cout << "== All songs (ordered by artist, title) ==\n";
        auto all = db.listAll();
        printSongs(all);

        // ---- 3. READ ONE ----
        std::cout << "== Get by id ==\n";
        if (!all.empty()) {
            Song one;
            if (db.get(all.front().id, one))
                std::cout << "id " << one.id << ": " << one.artist << " - " << one.title
                          << " [" << one.album << "] " << one.file_path << "\n";
        }
        Song missing;
        std::cout << "id 99999 found? " << (db.get(99999, missing) ? "yes" : "no") << "\n\n";

        // ---- 4. DELETE ----
        std::cout << "== Deleting ==\n";
        if (all.size() >= 2) {
            int victim = all.back().id;
            std::cout << "remove(" << victim << ") -> " << (db.remove(victim) ? "deleted" : "not found") << '\n';
            std::cout << "remove(" << victim << ") again -> " << (db.remove(victim) ? "deleted" : "not found") << '\n';
        }
        std::cout << "removeByPath(\"D:/Music/pf/money.flac\") -> "
                  << (db.removeByPath("D:/Music/pf/money.flac") ? "deleted" : "not found") << "\n\n";

        // ---- 5. READ AGAIN ----
        std::cout << "== After deletes ==\n";
        printSongs(db.listAll());

    } catch (const SongDbError &e) {
        std::cerr << "Database error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
