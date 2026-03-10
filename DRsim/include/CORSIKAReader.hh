#ifndef CORSIKAReader_h
#define CORSIKAReader_h 1

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

// Reader for CORSIKA 7 binary (unthinned) ground-particle output.
//
// Format: Fortran unformatted binary, one "block" per Fortran WRITE.
// Each block = 21 sub-blocks x 273 floats = 5733 floats (22932 bytes).
// With Fortran padding:  [uint32_t 22932] [22932 bytes] [uint32_t 22932]
// Sub-block identifier:  first 4 bytes interpreted as char[4]:
//   "RUNH" – run header,  "EVTH" – event header,
//   "EVTE" – event end,   "RUNE" – run end,  else particle data.
// Particle record (7 floats): [descriptor, px, py, pz, x, y, t]
//   particle_code = int(descriptor) / 1000

class CORSIKAReader {
public:
  struct Particle {
    int   code;        // CORSIKA particle code  (1=γ, 3=e⁻, 6=μ⁻, 14=p…)
    float px, py, pz;  // momentum  [GeV/c]   (pz < 0 = downward in real world)
    float x,  y;       // position  [cm]
    float t;           // arrival time [ns]
  };

  struct Event {
    int   run_id;
    int   event_id;
    float energy_GeV;
    float zenith_rad;   // zenith angle of primary [rad]
    float azimuth_rad;  // azimuth angle of primary [rad]
    float x_core_cm;   // shower core x position [cm]  (EVTH(98))
    float y_core_cm;   // shower core y position [cm]  (EVTH(99))
    std::vector<Particle> particles;
  };

  explicit CORSIKAReader(const std::string& path);
  ~CORSIKAReader();

  bool IsOpen()  const;
  bool ReadNextEvent(Event& evt);   // false at EOF or error

  // Skip n events (for selecting which shower to simulate in single-event jobs)
  void SkipEvents(int n) { Event dummy; for (int i=0; i<n; ++i) if (!ReadNextEvent(dummy)) break; }

  // Translate CORSIKA particle code to PDG code (0 = unknown).
  static int CorsikaToPDG(int code);

private:
  bool ReadBlock();
  bool ReadSubBlock(float buf[273]);

  std::ifstream fFile;
  bool  fHasPadding;   // Fortran record markers present
  bool  fAtEOF;
  int   fRunId;

  // Block-level buffer (21 sub-blocks × 273 floats each)
  float fBlockBuf[273 * 21];   // raw block data
  int   fSubIdx;               // next sub-block to serve [0, 21)
};

#endif
