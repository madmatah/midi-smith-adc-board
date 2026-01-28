#include "bsp/board.hpp"

#include "bsp/cortex/axi_sram_nocache_mpu.hpp"

namespace bsp {

void Board::init() noexcept {
  bsp::cortex::AxiSramNoCacheMpu::ConfigureRegion();
}


}  // namespace bsp
