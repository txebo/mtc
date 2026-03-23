#include "core/analytics/AnalyticsStore.h"

namespace mtc {

std::string AnalyticsStore::status() const {
    return "AnalyticsStore: pendiente de persistencia SQLite y eventos observados.";
}

}  // namespace mtc

