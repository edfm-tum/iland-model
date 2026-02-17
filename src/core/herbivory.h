#ifndef HERBIVORY_H
#define HERBIVORY_H

#include <QHash>
#include <bitset>

#include "exception.h"
#include "globalsettings.h"
#include "csvfile.h"

/* Herbivory hack for the work by Miguel, Jonas, Rupert, and me

In a nutshell, we implement results of empirical growth experiments done by Miguel
in a climate chamber, and in Berchtesgaden to study effects of drought and herbivory
and raised temperatere on growth and mortality of saplings.

*/

struct HerbivorySettings {
    bool OpenCanopy { false };
    bool IncreasedTemp { false };
    enum EEventType { WinterOnly, SummerOnly, Both, None };
    EEventType EventType    { None };
    int Replicate {0};
    std::bitset<10> Sequence;

    /**
     * @brief Parses a bit-encoded integer to populate the struct's members.
     * * The encoding scheme is as follows (18 bits total, MSB first):
     * - Bit 1 (pos 17): Stand Type (0=Closed, 1=Open)
     * - Bit 2 (pos 16): Climate Type (0=+0deg, 1=+2deg)
     * - Bits 3-4 (pos 15-14): Event Type (00=None, 01=Summer, 10=Winter, 11=Both)
     * - Bits 5-8 (pos 13-10): Replicate (1-10)
     * - Bits 9-18 (pos 9-0): Event Sequence
     * * @param code The integer code to parse.
     * @return true if parsing was successful, false otherwise (e.g., invalid replicate value).
     */
    bool parse(int code) {
        // --- Stand and Climate Types (1 bit each) ---
        // Bit 1 (highest bit, shifted by 17)
        OpenCanopy = (code >> 17) & 1;
        // Bit 2 (shifted by 16)
        IncreasedTemp = (code >> 16) & 1;

        // --- Event Type (2 bits) ---
        // Isolate bits 3 & 4 by shifting 14 and masking with 0b11 (3)
        int event_bits = (code >> 14) & 0b11;
        switch (event_bits) {
        case 0b01: EventType = SummerOnly; break;
        case 0b10: EventType = WinterOnly; break;
        case 0b11: EventType = Both;       break;
        default:   EventType = None;       break; // 0b00
        }

        // --- Replicate (4 bits) ---
        // Isolate bits 5-8 by shifting 10 and masking with 0b1111 (15)
        int rep_val = (code >> 10) & 0b1111;

        Replicate = rep_val;

        // --- Sequence (10 bits) ---
        // Isolate the lowest 10 bits by masking with 0b1111111111 (1023 or 0x3FF)
        Sequence = std::bitset<10>(code & 0x3FF);

        return true;
    }

    /**
     * @brief Parses a string to its corresponding EEventType enum value.
     * @param treatmentString The string to parse (e.g., "WinterOnly").
     * @return The matching EEventType, or None if no match is found.
     */
    static EEventType parseTreatment(const QString &treatmentString) {
        // This static map is initialized only once, on the first call.
        static const QHash<QString, EEventType> stringToEnumMap = {
            { "WinterOnly", EEventType::WinterOnly },
            { "SummerOnly", EEventType::SummerOnly },
            { "Both",       EEventType::Both },
            { "None",       EEventType::None }
        };

        // .value() performs the lookup and returns the default (second argument) if not found.
        return stringToEnumMap.value(treatmentString, EEventType::None);
    }
};




/* Management of the content */
struct HerbivoryEffect {
    // keys
    QString species;
    HerbivorySettings::EEventType treatment { HerbivorySettings::None };
    bool OpenCanopy;
    bool IncreasedTemp;
    // effects
    double pMortality {0.}; // probabilty of (extra) mortality under these circumstances
    double factorGrowth { 1.};  // multiplier for growth increment under that treatment
    double pTreeBrowsed { 0. }; // probability that a tree is browsed
    double factorBrowsingHeightRemoved { 0. }; // multiplier of height removed in case of browsing
};

// Key that uniquely identifies an herbivory effect
struct HerbivoryEffectKey {
    QString species;
    HerbivorySettings::EEventType treatment;
    bool openCanopy;
    bool increasedTemp;

    // Equality operator (required for QHash)
    bool operator==(const HerbivoryEffectKey &other) const {
        return species == other.species &&
               treatment == other.treatment &&
               openCanopy == other.openCanopy &&
               increasedTemp == other.increasedTemp;
    }
};

inline uint qHash(const HerbivoryEffectKey &key, uint seed = 0) {
    return qHashMulti(seed,
                      key.species,
                      static_cast<int>(key.treatment),
                      key.openCanopy,
                      key.increasedTemp);
}




class Herbivory {
public:
    // In loadHerbivoryData()
    static void loadHerbivoryData(QString path) {
        // ... open and parse CSV ...
        CSVFile init_file(path);
        if (init_file.rowCount() == 0)
            throw IException("Herbivory data file not working!");

        for (int i=0;i<init_file.rowCount();++i) {
            HerbivorySettings::EEventType treatment = HerbivorySettings::parseTreatment( init_file.value(i, "treatment").toString() );
            HerbivoryEffectKey key = {init_file.value(i, "species").toString(),
                treatment,
                init_file.value(i, "openCanopy").toBool(),
                init_file.value(i, "increasedTemp").toBool(),};
            HerbivoryEffect effect = {key.species, key.treatment, key.openCanopy, key.increasedTemp,
                                      init_file.value(i, "pMortality").toDouble(),
                                      init_file.value(i, "factorGrowth").toDouble(),
                                      init_file.value(i, "pTreeBrowsed").toDouble(),
                                      init_file.value(i, "factorBrowsingHeightRemoved").toDouble()};
            herbivoryData.insert(key, effect);


        }
        qDebug() << "Herbivory: Loaded" << herbivoryData.size() << "elements from" << path;

    }

    static void ensureHerbivoryDataLoaded() {
        // The lambda is executed to initialize 'loader'.
        // C++ guarantees this happens exactly once, thread-safely.
        static bool loader = [] {
            loadHerbivoryData(Globals->path("database/herbivory.csv"));
            return true;
        }();
        // The 'loader' variable itself is unused; its only purpose is to trigger the initialization.
        Q_UNUSED(loader)
    }
    static double isEventYear(const HerbivorySettings &settings, int year) {
        ensureHerbivoryDataLoaded();
        return settings.Sequence.test(year - 1);
    }

    static const HerbivoryEffect &herbivoryEffect(const HerbivorySettings &settings, const QString &species) {
        ensureHerbivoryDataLoaded();
        HerbivoryEffectKey key = {species, settings.EventType, settings.OpenCanopy, settings.IncreasedTemp};

        auto it = herbivoryData.find(key);
        if (it != herbivoryData.end()) {
            return it.value(); // Found, return the object
        } else {
            return NoEffect;
            //throw IException("Invalid herbivory settings - no data!");
        }


    }



    /// Get settings for a given resource unit id (or resource unit index)
    static HerbivorySettings &herbivorySettings(int ruId) {
        if (StoreHerbivorySettings.contains(ruId))
            return StoreHerbivorySettings[ruId];

        auto &settings = StoreHerbivorySettings[ruId]; // insert new object (default constructed)

        settings.parse(ruId); // update in hash
        return settings;
    }

private:
    static QHash<int, HerbivorySettings> StoreHerbivorySettings;
    // In your class or global scope
    static QHash<HerbivoryEffectKey, HerbivoryEffect> herbivoryData;
    // empty effect (constructor: no effect)
    static HerbivoryEffect NoEffect;



};



#endif // HERBIVORY_H
