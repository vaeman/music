#include "db.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_set>

#include "taglib/fileref.h"
#include "taglib/tag.h"
#include "taglib/tpropertymap.h"
#include "taglib/tstringlist.h"
#include "taglib/tvariant.h"

// Song has no constructor, so this helper keeps the demo data tidy.
static Song makeSong(const std::string &title, const std::string &artist,
                     const std::string &album, int track, int duration_ms,
                     const std::string &path, const std::string &ext, const int &year) {

    Song s;
    s.title = title;
    s.artist = artist;
    s.album = album;
    s.track_number = track;
    s.duration_ms = duration_ms;
    s.file_path = path;
    s.ext = ext;
    s.year = year;

    return s;
}

std::optional<Song> readMetaData(const std::filesystem::path &path) {
    TagLib::FileRef f(path.c_str());

    if (f.isNull() || !f.tag()) {
        return std::nullopt;
    }

    Song info;
    TagLib::Tag *tag = f.tag();
    TagLib::PropertyMap props = f.properties();

    info.title = tag->title().to8Bit(true);
    info.artist = tag->artist().to8Bit(true);
    info.album = tag->album().to8Bit(true);
    info.file_path = reinterpret_cast<const char *>(path.u8string().c_str());
    // info.disc_no = props["DISCNUMBER"];
    // info.disc_total = tag->properties()[5];
    // info.genre = tag->genre().to8Bit(true);
    info.year = tag->year();
    info.track_number = tag->track();

    if (auto *ap = f.audioProperties()) {
        // info.bitrate = ap->bitrate();
        // info.sampleRate = ap->sampleRate();
        // info.channels = ap->channels();
        info.duration_ms = ap->lengthInMilliseconds();
    }
    return info;
}

using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;

int main() {

    // fetch all shtit from the database that we have, if any
    // check via path first: if path already in our database, skip
    // check for artist first, if artist is not there make new entry of album and artist
    // check for album, if album is not there but artist is, check

    /*
        if (artist in db) {
            if (album in db) {
                add song to album
            } else {
                add song to new album
            }
        }
        else {
            add new album, artist, song.
        }
    */
    //

    auto start = std::chrono::high_resolution_clock::now();

    int c = 0;

    static std::filesystem::path folder_path = "D:/music/";

    std::unordered_set<std::string> exts = {".mp3", ".flac", ".ogg", ".wav", ".m4a"};

    std::vector<std::string> paths;

    if (std::filesystem::exists(folder_path)) {
        for (const auto &i : recursive_directory_iterator(folder_path)) {
            if (i.is_regular_file() && exts.count(i.path().extension().string())) {
                auto song = readMetaData((i.path()));

                if (!song)
                    continue;
                c++;

                // std::cout << song->album << song->artist << '\n';
            }
        }
    }
    std::cout << c << std::endl;

    auto end = std::chrono::high_resolution_clock::now();

    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << time / 12;

    return 0;
}