#ifndef SONG_DB_HPP
#define SONG_DB_HPP

#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <vector>

struct Song {
    int id = 0; // set by db on insert/read, ignored on insert input
    std::string title;
    std::string artist;
    std::string album;
    int track_number = 0;
    int duration_ms = 0;
    int year = 0;
    std::string file_path;
    std::string ext; // "mp3", "flac", etc.
};

// Thrown on any database error (open failure, bad SQL, etc).
class SongDbError : public std::runtime_error {
public:
    explicit SongDbError(const std::string &msg) : std::runtime_error(msg) {}
};

// RAII wrapper around a SQLite-backed song library.
// Opens the database (creating the schema if needed) on construction,
// closes it automatically on destruction.
class SongDB {
public:
    explicit SongDB(const std::string &db_path); // throws SongDbError
    ~SongDB();

    // Non-copyable (owns a raw sqlite3* handle), movable.
    SongDB(const SongDB &) = delete;
    SongDB &operator=(const SongDB &) = delete;
    SongDB(SongDB &&other) noexcept;
    SongDB &operator=(SongDB &&other) noexcept;

    // adds all the songs at the very end
    int addSongs(const std::vector<Song> &songs);

    // Deletes a song by id. Returns true if a row was deleted.
    bool remove(int song_id);

    // Check Artist
    int checkArtist(const std::string &artist_name);

    // Check Album
    int checkAlbum(const std::string &album_name, const int &album_id);

    // adds artist to artists table
    int addArtist(const std::string &name);

    // adds album to albums table
    int addAlbum(const std::string &name, const int &artist_id);

    // Deletes a song by exact file_path. Returns true if a row was deleted.
    bool removeByPath(const std::string &file_path);

    // Fetches a single song by id. Returns true if found (out is filled).
    bool get(int song_id, Song &out);

    // Returns every song in the library, ordered by artist then title.
    std::vector<Song> listAll();

private:
    sqlite3 *DB = nullptr;
    void initSchema();
};

#endif // SONG_DB_HPP