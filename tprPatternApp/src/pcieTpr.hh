/*
PCIe TPR Queue buffer structure

  ------------ HEADER ----------------
                 +------------------+  M (mode): bit 22, 1: LCLS1, 0:LCLS2
32 bit word 0    |  M TAG        CHM|  TAG (tag): bits[19,16], 0: event, 1: bsa ctrl, 2: bsa event
                 +------------------+  CHM (channel mask): bits[15, 0]
                 +------------------+
32 bit word 1    |                  |  size of payload
                 +------------------+
   ------------ Payload ----------------
                 +------------------+
32 bit word 2    |       ns         | \
                 +------------------+  | LCLS 2: pulse id
                 +------------------+  | LCLS 1: timestamp 1
32 bit word 3    |       sec        | /
                 +------------------+
                 +------------------+
32 bit word 4    |       ns         | \
                 +------------------+  |
                 +------------------+  | timestamp 2
32 bit word 5    |       sec        | /
                 +------------------+
                 /// skip /// skip ///
                 remarks) in LCLS2 mode, the pulse id occupies the first timestamp (64bit, timestamp 1)
                 +------------------+
32 bit word 6    |                  | LCLS 2:fixed rate 10, ac rate 6, ac timeslot 3, ac phase 12, rsvd 1
                 +------------------+ LCLS 1:      rsvd 10, ac rate 6, ac timeslot 3,     rsvd 12, rsvd 1
                 +------------------+
32 bit word 7    |                  | LCLS 2: beam present 1, rsvd 3, destination 4, rsvd 8, charge 16
                 +------------------+ LCLS 1: beam present 1, rsvd 3, destination 4, rsvd 8,   rsvd 16
                 remarks) beam present, destination from modifier bit in LCLS 1
                 +------------------+
32 bit word 8    |                  | LCLS 2: energy0 16
                 +------------------+ LCLS 1: modifier0 32
                 +------------------+
32 bit word 9    |                  | LCLS 2: energy1 16
                 +------------------+ LCLS 1: modifier1 32
                 +------------------+
32 bit    |                  | LCLS 2: energy2 16
          +------------------+ LCLS 1: modifier2 32
          +------------------+
32 bit    |                  | LCLS 2: energy3 16
          +------------------+ LCLS 1: modifier3 32
          +------------------+
32 bit    |                  | LCLS 2: wavelength0 16
          +------------------+ LCLS 1: modifier4 32
          +------------------+
32 bit    |                  | LCLS 2: wavelength1 16
          +------------------+ LCLS 1: modifier5 32
          +------------------+
32 bit    |                  | LCLS 2: rsvd 16, mps limit 16
          +------------------+ LCLS 1: rsvd 32
          +------------------+
32 bit    |                  | LCLS 2: mps class0 to mps class 15 for each 4 bits
          +------------------+ LCLS 1: rsvd 32
          +------------------+
32 bit    |                  | LCLS 2: control seq0 16, control seq1
          +------------------+ LCLS 1: event mask 256 bits
          +------------------+             |
32 bit    |                  |             |
          +------------------+             |
          +------------------+             |
32 bit    |                  |             |
          +------------------+             |
          +------------------+             |
32 bit    |                  |             |
          +------------------+             |
          +------------------+             |
32 bit    |                  |             |
          +------------------+             |
          +------------------+             |
32 bit    |                  |             |
          +------------------+             |
          +------------------+             |
32 bit    |                  |             |
          +------------------+             |
          +------------------+             |
32 bit    |                  |             |
          +------------------+             |
          +------------------+             |
32 bit    |                  | LCLS 2: control seq16 16, control seq17 16
          +------------------+ LCLS 1: rsvd 32

*/
