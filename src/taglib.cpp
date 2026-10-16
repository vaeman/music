#include <cstdio>
#include <iomanip>
#include <iostream>
#include <optional>

#include "taglib/fileref.h"
#include "taglib/tag.h"
#include "taglib/tpropertymap.h"
#include "taglib/tstringlist.h"
#include "taglib/tvariant.h"

struct TrackInfo {

    std::string title, artist, album, path, genre;
    unsigned int track, length, track_total, year, disc_total, disc_no;
};

std::optional<TrackInfo> readMetaData(const std::string &path) {
    TagLib::FileRef f(path.c_str());

    TrackInfo info;
    TagLib::Tag *tag = f.tag();
    TagLib::PropertyMap props = f.properties();
    info.title = tag->title().to8Bit(true);
    info.artist = tag->artist().to8Bit(true);
    info.album = tag->album().to8Bit(true);
    info.path = path;
    // info.disc_no = props["DISCNUMBER"];
    // info.disc_total = tag->properties()[5];
    info.genre = tag->genre().to8Bit(true);
    info.year = tag->year();
    info.track = tag->track();

    if (auto *ap = f.audioProperties()) {
        // info.bitrate = ap->bitrate();
        // info.sampleRate = ap->sampleRate();
        // info.channels = ap->channels();
        info.length = ap->lengthInSeconds();
    }
    return info;
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {

        std::cout << "******************** \"" << argv[i] << "\" ********************" << std::endl;

        TagLib::FileRef f(argv[i]);

        if (!f.isNull() && f.tag()) {

            TagLib::Tag *tag = f.tag();

            std::cout << "-- TAG (basic) --" << std::endl;
            std::cout << "title   - \"" << tag->title() << "\"" << std::endl;
            std::cout << "artist  - \"" << tag->artist() << "\"" << std::endl;
            std::cout << "album   - \"" << tag->album() << "\"" << std::endl;
            std::cout << "year    - \"" << tag->year() << "\"" << std::endl;
            std::cout << "comment - \"" << tag->comment() << "\"" << std::endl;
            std::cout << "track   - \"" << tag->track() << "\"" << std::endl;
            std::cout << "genre   - \"" << tag->genre() << "\"" << std::endl;

            TagLib::PropertyMap tags = f.properties();

            if (!tags.isEmpty()) {
                unsigned int longest = 0;
                for (auto j = tags.cbegin(); j != tags.cend(); ++j) {
                    if (j->first.size() > longest) {
                        longest = j->first.size();
                    }
                }

                std::cout << "-- TAG (properties) --" << std::endl;
                for (auto j = tags.cbegin(); j != tags.cend(); ++j) {
                    for (auto k = j->second.begin(); k != j->second.end(); ++k) {
                        std::cout << std::left << std::setfill(' ') << std::setw(longest) << j->first << " - " << '"' << *k << '"' << std::endl;
                    }
                }
            }

            TagLib::StringList names = f.complexPropertyKeys();
            for (const auto &name : names) {
                const auto &properties = f.complexProperties(name);
                for (const auto &property : properties) {
                    std::cout << name << ":" << std::endl;
                    for (const auto &[key, value] : property) {
                        std::cout << "  " << std::left << std::setfill(' ') << std::setw(11) << key << " - ";
                        if (value.type() == TagLib::Variant::ByteVector) {
                            std::cout << "(" << value.value<TagLib::ByteVector>().size() << " bytes)" << std::endl;
                            /* The picture could be extracted using:
                            std::ofstream picture;
                            TagLib::String fn(argv[i]);
                            int slashPos = fn.rfind('/');
                            int dotPos = fn.rfind('.');
                            if(slashPos >= 0 && dotPos > slashPos) {
                              fn = fn.substr(slashPos + 1, dotPos - slashPos - 1);
                            }
                            fn += ".jpg";
                            picture.open(fn.toCString(), std::ios_base::out | std::ios_base::binary);
                            picture << value.value<TagLib::ByteVector>();
                            picture.close();
                            */
                        } else {
                            std::cout << value << std::endl;
                        }
                    }
                }
            }
        }

        if (!f.isNull() && f.audioProperties()) {

            TagLib::AudioProperties *properties = f.audioProperties();

            int seconds = properties->lengthInSeconds() % 60;
            int minutes = (properties->lengthInSeconds() - seconds) / 60;

            std::cout << "-- AUDIO --" << std::endl;
            std::cout << "bitrate     - " << properties->bitrate() << std::endl;
            std::cout << "sample rate - " << properties->sampleRate() << std::endl;
            std::cout << "channels    - " << properties->channels() << std::endl;
            std::cout << "length      - " << minutes << ":" << std::setfill('0') << std::setw(2) << std::right << seconds << std::endl;
        }
    }
}