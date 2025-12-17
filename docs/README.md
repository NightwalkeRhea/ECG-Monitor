# Simulation Strategy
As the rational thing is not to simulate everything at once, we go bottom-up, and if any of the steps below fails, we stop. 

## Simulation Order
| Step | Module               | Goal                |
| ---- | -------------------- | ------------------- |
| 1    | `spi_slave_rx`       | CDC correctness     |
| 2    | `fir_filter`         | Numeric stability   |
| 3    | `spi_byte_shifter`   | SPI timing          |
| 4    | `w25q64_page_logger` | Flash protocol      |
| 5    | `dsp_pipeline_qrs`   | End-to-end dataflow |
| 6    | `ecg_top`            | Integration sanity  |

