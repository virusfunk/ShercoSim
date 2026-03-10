#include "CORSIKAReader.hh"

#include <cstring>
#include <cmath>
#include <iostream>

// ── file-local constants ───────────────────────────────────────────────────
static const int kSubBlockSize_     = 273;
static const int kSubBlocksPerBlock_= 21;
static const int kBlockBytes        = kSubBlockSize_ * kSubBlocksPerBlock_ * (int)sizeof(float);  // 22932

// ── ctor/dtor ──────────────────────────────────────────────────────────────
CORSIKAReader::CORSIKAReader(const std::string& path)
    : fHasPadding(false), fAtEOF(false), fRunId(0),
      fSubIdx(kSubBlocksPerBlock_)   // force ReadBlock on first call
{
  fFile.open(path, std::ios::binary);
  if (!fFile.is_open()) {
    std::cerr << "CORSIKAReader: cannot open \"" << path << "\"\n";
    fAtEOF = true;
    return;
  }

  // Detect Fortran record padding.
  // If the first 4 bytes equal kBlockBytes (22932) the file has padding;
  // if they spell "RUNH" the file is raw (no padding).
  uint32_t first4 = 0;
  fFile.read(reinterpret_cast<char*>(&first4), 4);
  fFile.seekg(0);
  fHasPadding = (first4 == static_cast<uint32_t>(kBlockBytes));

  // Read RUNH sub-block to get run ID.
  float runh[kSubBlockSize_];
  if (ReadSubBlock(runh)) {
    char id[5]; std::memcpy(id, runh, 4); id[4] = '\0';
    if (std::strncmp(id, "RUNH", 4) == 0) {
      fRunId = static_cast<int>(runh[1]);
    }
  }
  // Rewind so ReadNextEvent starts from the beginning.
  fFile.seekg(0);
  fAtEOF = false;
  fSubIdx = kSubBlocksPerBlock_;
}

CORSIKAReader::~CORSIKAReader() {
  if (fFile.is_open()) fFile.close();
}

bool CORSIKAReader::IsOpen() const {
  return fFile.is_open() && !fAtEOF;
}

// ── private: ReadBlock ─────────────────────────────────────────────────────
// Read one complete Fortran record (21 sub-blocks) into fBlockBuf.
bool CORSIKAReader::ReadBlock() {
  if (fAtEOF) return false;

  if (fHasPadding) {
    uint32_t marker;
    if (!fFile.read(reinterpret_cast<char*>(&marker), 4)) {
      fAtEOF = true;
      return false;
    }
  }

  if (!fFile.read(reinterpret_cast<char*>(fBlockBuf), (std::streamsize)kBlockBytes)) {
    fAtEOF = true;
    return false;
  }

  if (fHasPadding) {
    uint32_t marker;
    fFile.read(reinterpret_cast<char*>(&marker), 4);   // trailing marker (ignore)
  }

  fSubIdx = 0;
  return true;
}

// ── private: ReadSubBlock ──────────────────────────────────────────────────
// Copy next 273-float sub-block into buf, reading a new block when needed.
bool CORSIKAReader::ReadSubBlock(float buf[273]) {
  if (fAtEOF) return false;
  if (fSubIdx >= kSubBlocksPerBlock_) {
    if (!ReadBlock()) return false;
  }
  std::memcpy(buf,
              fBlockBuf + fSubIdx * kSubBlockSize_,
              kSubBlockSize_ * sizeof(float));
  ++fSubIdx;
  return true;
}

// ── ReadNextEvent ──────────────────────────────────────────────────────────
bool CORSIKAReader::ReadNextEvent(Event& evt) {
  float buf[273];
  char  id[5] = {0};

  bool have_evth = false;

  while (ReadSubBlock(buf)) {
    std::memcpy(id, &buf[0], 4);
    id[4] = '\0';

    // ── Run header ──
    if (strncmp(id, "RUNH", 4) == 0) {
      fRunId = static_cast<int>(buf[1]);
      continue;
    }

    // ── Event header ──
    if (strncmp(id, "EVTH", 4) == 0) {
      evt.run_id     = fRunId;
      evt.event_id   = static_cast<int>(buf[1]);
      evt.energy_GeV = buf[3];
      evt.zenith_rad  = buf[10];  // EVTH(11) — zenith angle [rad]
      evt.azimuth_rad = buf[11];  // EVTH(12) — azimuth angle [rad]
      evt.x_core_cm   = buf[97];  // EVTH(98) — shower core x [cm]
      evt.y_core_cm   = buf[98];  // EVTH(99) — shower core y [cm]
      evt.particles.clear();
      have_evth = true;
      continue;
    }

    // ── Event trailer ──  (event complete)
    if (strncmp(id, "EVTE", 4) == 0) {
      return have_evth;
    }

    // ── Run trailer ──  (end of file)
    if (strncmp(id, "RUNE", 4) == 0) {
      fAtEOF = true;
      return false;
    }

    // ── Particle data sub-block ──
    // Unthinned: 39 particle slots × 7 floats = 273.
    // Thinned:   34 particle slots × 8 floats = 272 (+1 pad) = 273.
    // Unused slots have descriptor == 0.
    if (!have_evth) continue;   // skip data before first EVTH

    // 39 particle slots × 7 floats = 273. Unused slots have descriptor == 0.
    // Note: even for THIN-mode CORSIKA 7 files the record length stays 7 floats/particle;
    // the thinning weight is NOT written into the particle records in this format.
    for (int j = 0; j < 39; j++) {
      float desc = buf[j * 7];
      if (desc == 0.f) break;           // end of used slots

      int pcode = static_cast<int>(desc) / 1000;
      if (pcode <= 0) continue;
      // CORSIKA codes 75/76 are sub-shower descriptors, not real particles
      if (pcode == 75 || pcode == 76) continue;

      Particle p;
      p.code = pcode;
      p.px   = buf[j*7 + 1];   // GeV/c  (transverse)
      p.py   = buf[j*7 + 2];   // GeV/c  (transverse)
      p.pz   = buf[j*7 + 3];   // GeV/c  positive = downward (CORSIKA stores |pz|)
      p.x    = buf[j*7 + 4];   // cm  (CORSIKA convention)
      p.y    = buf[j*7 + 5];   // cm
      p.t    = buf[j*7 + 6];   // ns

      evt.particles.push_back(p);
    }
  }

  return false;
}

// ── CorsikaToPDG ──────────────────────────────────────────────────────────
int CORSIKAReader::CorsikaToPDG(int code) {
  switch (code) {
    case  1: return   22;   // gamma
    case  2: return  -11;   // e+
    case  3: return   11;   // e-
    case  5: return  -13;   // mu+
    case  6: return   13;   // mu-
    case  7: return  111;   // pi0
    case  8: return  211;   // pi+
    case  9: return -211;   // pi-
    case 10: return  130;   // K0_L
    case 11: return  321;   // K+
    case 12: return -321;   // K-
    case 13: return 2112;   // neutron
    case 14: return 2212;   // proton
    case 15: return    0;   // reserved
    case 16: return  310;   // K0_S
    default:
      // Heavy nucleus: CORSIKA code = 100*A + Z  (code >= 100)
      if (code >= 100) {
        int A = code / 100;
        int Z = code % 100;
        return 1000000000 + Z * 10000 + A * 10;  // PDG ion code
      }
      return 0;   // unknown — skip
  }
}
