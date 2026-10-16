#include "..\include\db.hpp"
#include <iostream>

namespace {

// runs fixed SQL (schema, pragmas, BEGIN/COMMIT); throws on error
void exec(sqlite3 *db, const char *sql) {
    char *err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw SongDbError(msg);
    }
}

// RAII wrapper: prepare in ctor, finalize in dtor
class Stmt {
    sqlite3 *db_;
    sqlite3_stmt *s_ = nullptr;

public:
    Stmt(sqlite3 *db, const char *sql) : db_(db) {
        if (sqlite3_prepare_v2(db, sql, -1, &s_, nullptr) != SQLITE_OK)
            throw SongDbError(sqlite3_errmsg(db));
    }
    ~Stmt() { sqlite3_finalize(s_); }
    Stmt(const Stmt &) = delete;
    Stmt &operator=(const Stmt &) = delete;

    void bind(int i, const std::string &v) { sqlite3_bind_text(s_, i, v.c_str(), -1, SQLITE_TRANSIENT); }
    void bind(int i, int v) { sqlite3_bind_int(s_, i, v); }

    // true = got a row, false = done; throws on real errors
    bool step() {
        int rc = sqlite3_step(s_);
        if (rc == SQLITE_ROW)
            return true;
        if (rc == SQLITE_DONE)
            return false;
        throw SongDbError(sqlite3_errmsg(db_));
    }

    int colInt(int i) { return sqlite3_column_int(s_, i); }
    std::string colText(int i) {
        auto p = sqlite3_column_text(s_, i);
        return p ? reinterpret_cast<const char *>(p) : "";
    }
};

const char *SELECT_SONG =
    "SELECT s.id, s.title, ar.name, al.title, s.track_no, s.duration_ms, s.file_path, s.ext, s.year "
    "FROM songs s "
    "JOIN artists ar ON ar.id = s.artist_id "
    "JOIN albums  al ON al.id = s.album_id ";

Song rowToSong(Stmt &st) {
    Song s;
    s.id = st.colInt(0);
    s.title = st.colText(1);
    s.artist = st.colText(2);
    s.album = st.colText(3);
    s.track_number = st.colInt(4);
    s.duration_ms = st.colInt(5);
    s.file_path = st.colText(6);
    s.ext = st.colText(7);
    s.year = st.colInt(8);
    return s;
}

} // namespace

SongDB::SongDB(const std::string &path) {
    if (sqlite3_open(path.c_str(), &DB) != SQLITE_OK) {
        std::string msg = sqlite3_errmsg(DB);
        sqlite3_close(DB);
        DB = nullptr;
        throw SongDbError("open failed: " + msg);
    }
    try {
        initSchema();
    } catch (...) {
        sqlite3_close(DB);
        DB = nullptr;
        throw;
    }
}

SongDB::~SongDB() { sqlite3_close(DB); }

SongDB::SongDB(SongDB &&o) noexcept : DB(o.DB) { o.DB = nullptr; }

SongDB &SongDB::operator=(SongDB &&o) noexcept {
    if (this != &o) {
        sqlite3_close(DB);
        DB = o.DB;
        o.DB = nullptr;
    }
    return *this;
}

void SongDB::initSchema() {
    exec(DB, "PRAGMA foreign_keys = ON;");
    exec(DB, R"(
        CREATE TABLE IF NOT EXISTS artists (
            id   INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE
        );)");
    exec(DB, R"(
        CREATE TABLE IF NOT EXISTS albums (
            id        INTEGER PRIMARY KEY,
            title     TEXT NOT NULL,
            artist_id INTEGER NOT NULL REFERENCES artists(id),
            UNIQUE (title, artist_id)
        );)");
    exec(DB, R"(
        CREATE TABLE IF NOT EXISTS songs (
            id          INTEGER PRIMARY KEY,
            title       TEXT NOT NULL,
            duration_ms INTEGER NOT NULL,
            track_no    INTEGER,
            disc_no     INTEGER,
            file_path   TEXT NOT NULL UNIQUE,
            ext         TEXT NOT NULL,
            year        INTEGER NOT NULL,
            artist_id   INTEGER NOT NULL REFERENCES artists(id),
            album_id    INTEGER NOT NULL REFERENCES albums(id)
        );)");
}

int SongDB::add(const Song &s) {
    exec(DB, "BEGIN;");
    try {
        int artist_id = findOrCreateArtist(s.artist);
        int album_id = findOrCreateAlbum(s.album, artist_id);

        Stmt st(DB, "INSERT INTO songs(title, duration_ms, track_no, file_path, ext, year, artist_id, album_id) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?);");
        st.bind(1, s.title);
        st.bind(2, s.duration_ms);
        st.bind(3, s.track_number);
        st.bind(4, s.file_path);
        st.bind(5, s.ext);
        st.bind(6, s.year);
        st.bind(7, artist_id);
        st.bind(8, album_id);
        st.step();

        int id = static_cast<int>(sqlite3_last_insert_rowid(DB));
        exec(DB, "COMMIT;");
        return id;
    } catch (...) {
        sqlite3_exec(DB, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

bool SongDB::remove(int song_id) {
    Stmt st(DB, "DELETE FROM songs WHERE id = ?;");
    st.bind(1, song_id);
    st.step();
    return sqlite3_changes(DB) > 0;
}

bool SongDB::removeByPath(const std::string &file_path) {
    Stmt st(DB, "DELETE FROM songs WHERE file_path = ?;");
    st.bind(1, file_path);
    st.step();
    return sqlite3_changes(DB) > 0;
}

bool SongDB::get(int song_id, Song &out) {
    Stmt st(DB, (std::string(SELECT_SONG) + "WHERE s.id = ?;").c_str());
    st.bind(1, song_id);
    if (!st.step())
        return false;
    out = rowToSong(st);
    return true;
}

std::vector<Song> SongDB::listAll() {
    std::vector<Song> out;
    Stmt st(DB, (std::string(SELECT_SONG) + "ORDER BY ar.name, s.title;").c_str());
    while (st.step())
        out.push_back(rowToSong(st));
    return out;
}