// Classic 5x7 ASCII font (0x20..0x7E), column-major, bit0 = top row.
#pragma once
#include <stdint.h>

namespace qs {
extern const uint8_t kFont5x7[95][5];
}
