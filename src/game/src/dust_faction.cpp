#include "dust_faction.h"


namespace dust {

    const char * ToString( FactionType type ) {
        switch ( type ) {
            case FACTION_TYPE_NEUTRAL:  return "Neutral";
            case FACTION_TYPE_REMNANT:  return "Remnant";
            case FACTION_TYPE_RUSTBORN: return "Rustborn";
            case FACTION_TYPE_ZENITH:   return "Zenith";
        }
        return "Unknown";
    }

}