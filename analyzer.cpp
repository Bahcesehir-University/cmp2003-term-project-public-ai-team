#include "analyzer.h"

#include <fstream>
#include <algorithm>
#include <cctype>

void TripAnalyzer::ingestFile(const std::string& csvPath) {
    std::ifstream in(csvPath);
    if (!in.is_open()) return;

    std::string line;

    // Skipping header
    if (!std::getline(in, line)) return;

    while (std::getline(in, line)) {
        if (line.empty()) continue;

        // Handle Windows CRLF
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        // TripID,PickupZoneID,DropoffZoneID,PickupDateTime,...
        std::size_t c1 = line.find(',');
        if (c1 == std::string::npos) continue;

        std::size_t c2 = line.find(',', c1 + 1);
        if (c2 == std::string::npos) continue;

        std::size_t c3 = line.find(',', c2 + 1);
        if (c3 == std::string::npos) continue;

        std::size_t c4 = line.find(',', c3 + 1);
        if (c4 == std::string::npos) continue;

        // PickupZoneID
        std::string zone = line.substr(c1 + 1, c2 - (c1 + 1));
        if (zone.empty()) continue;

        // PickupDateTime is field [c3+1, c4)
        std::size_t t0 = c3 + 1;
        std::size_t tlen = c4 - t0;
        if (tlen < 13) continue;               // YYYY-MM-DD HH::MM" minimal check
        if (line[t0 + 10] != ' ') continue;

        char h1 = line[t0 + 11];
        char h2 = line[t0 + 12];
        if (!std::isdigit((unsigned char)h1) || !std::isdigit((unsigned char)h2)) continue;

        int hour = (h1 - '0') * 10 + (h2 - '0');
        if (hour < 0 || hour > 23) continue;

        // Aggregate
        ++zoneCounts_[zone];
        ++zoneHourCounts_[zone][static_cast<std::size_t>(hour)];
    }
}

std::vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    std::vector<ZoneCount> out;
    if (k <= 0) return out;

    out.reserve(zoneCounts_.size());
    for (const auto& it : zoneCounts_) {
        out.push_back(ZoneCount{it.first, it.second});
    }

    auto comp = [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    };

    if (static_cast<std::size_t>(k) < out.size()) {
        std::nth_element(out.begin(), out.begin() + k, out.end(), comp);
        out.resize(static_cast<std::size_t>(k));
    }
    std::sort(out.begin(), out.end(), comp);
    return out;
}

std::vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    std::vector<SlotCount> out;
    if (k <= 0) return out;

    // Upper bound: where each zone can contribute up to 24 slots
    out.reserve(zoneHourCounts_.size() * 24);

    for (const auto& [zone, hours] : zoneHourCounts_) {
        for (int h = 0; h < 24; ++h) {
            long long cnt = hours[static_cast<std::size_t>(h)];
            if (cnt > 0) out.push_back(SlotCount{zone, h, cnt});
        }
    }

    auto comp = [](const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone)   return a.zone < b.zone;
        return a.hour < b.hour;
    };

    if (static_cast<std::size_t>(k) < out.size()) {
        std::nth_element(out.begin(), out.begin() + k, out.end(), comp);
        out.resize(static_cast<std::size_t>(k));
    }
    std::sort(out.begin(), out.end(), comp);
    return out;
}
