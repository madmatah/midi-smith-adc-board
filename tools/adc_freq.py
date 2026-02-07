#!/usr/bin/env python3
import argparse
from typing import List, Dict, Any

STANDARD_SAMPLING_CYCLES_OPTIONS = [2.5, 8.5, 16.5, 32.5, 64.5, 387.5, 810.5]

def format_value_with_unit(value: float, unit: str = "") -> str:
    formatted_number = f"{value:.2f}"
    if not unit:
        return formatted_number
    return f"{formatted_number} {unit}"

def calculate_adc_conversion_cycles(sampling_cycles: float, resolution_bits: int) -> float:
    return sampling_cycles + (resolution_bits / 2) + 0.5

def calculate_max_frequency_khz(conversion_time_us: float, ranks_count: int) -> float:
    period_seconds = conversion_time_us * 1e-6 * ranks_count
    if period_seconds > 0:
        return (1.0 / period_seconds) / 1000.0
    return 0.0

def calculate_adc_statistics(
    clock_frequency_mhz: float,
    resolution_bits: int,
    ranks_count: int
) -> List[Dict[str, Any]]:
    adc_statistics_list = []
    
    for sampling_cycles in STANDARD_SAMPLING_CYCLES_OPTIONS:
        sampling_time_us = sampling_cycles / clock_frequency_mhz
        
        conversion_cycles = calculate_adc_conversion_cycles(sampling_cycles, resolution_bits)
        conversion_time_us = conversion_cycles / clock_frequency_mhz
        
        max_frequency_khz = calculate_max_frequency_khz(conversion_time_us, ranks_count)
            
        adc_statistics_list.append({
            "sampling_cycles": sampling_cycles,
            "sampling_time_us": sampling_time_us,
            "conversion_time_us": conversion_time_us,
            "max_frequency_khz": max_frequency_khz
        })
        
    return adc_statistics_list

def print_csv_format(adc_statistics_list: List[Dict[str, Any]], ranks_count: int) -> None:
    headers = [
        "Sampling (cycles)",
        "Sampling (time)",
        "Conversion time",
        f"Max freq / channel (ADC {ranks_count} ranks)"
    ]
    print(",".join(f'"{header}"' for header in headers))
    
    for row_dictionary in adc_statistics_list:
        sampling_cycles_str = str(row_dictionary["sampling_cycles"])
        sampling_time_str = format_value_with_unit(row_dictionary['sampling_time_us'])
        conversion_time_str = format_value_with_unit(row_dictionary['conversion_time_us'])
        max_frequency_str = format_value_with_unit(row_dictionary['max_frequency_khz'])
        
        line_items = [sampling_cycles_str, sampling_time_str, conversion_time_str, max_frequency_str]
        print(",".join(f'"{item}"' for item in line_items))

def print_markdown_format(adc_statistics_list: List[Dict[str, Any]], ranks_count: int) -> None:
    headers = [
        "Sampling (cycles)",
        "Sampling (time)",
        "Conversion time",
        f"Max freq / channel (ADC {ranks_count} ranks)"
    ]
    
    print("| " + " | ".join(headers) + " |")
    print("|" + "|".join([":--"] * len(headers)) + "|")
    
    for row_dictionary in adc_statistics_list:
        sampling_cycles_str = str(row_dictionary["sampling_cycles"])
        sampling_time_str = f"\"{format_value_with_unit(row_dictionary['sampling_time_us'], 'μs')}\""
        conversion_time_str = f"\"{format_value_with_unit(row_dictionary['conversion_time_us'], 'μs')}\""
        max_frequency_str = f"\"{format_value_with_unit(row_dictionary['max_frequency_khz'], 'kHz')}\""
        
        print(f"| {sampling_cycles_str}|{sampling_time_str}|{conversion_time_str}|{max_frequency_str}|")

def print_readable_format(adc_statistics_list: List[Dict[str, Any]], ranks_count: int) -> None:
    headers = [
        "Sampling (cycles)",
        "Sampling (time)",
        "Conversion time",
        f"Max freq / channel (ADC {ranks_count} ranks)"
    ]
    
    column_widths = [len(header) for header in headers]
    
    formatted_rows_list = []
    for row_dictionary in adc_statistics_list:
        sampling_cycles_str = str(row_dictionary["sampling_cycles"])
        sampling_time_str = format_value_with_unit(row_dictionary['sampling_time_us'], "μs")
        conversion_time_str = format_value_with_unit(row_dictionary['conversion_time_us'], "μs")
        max_frequency_str = format_value_with_unit(row_dictionary['max_frequency_khz'], "kHz")
        
        formatted_rows_list.append([sampling_cycles_str, sampling_time_str, conversion_time_str, max_frequency_str])
        
        column_widths[0] = max(column_widths[0], len(sampling_cycles_str))
        column_widths[1] = max(column_widths[1], len(sampling_time_str))
        column_widths[2] = max(column_widths[2], len(conversion_time_str))
        column_widths[3] = max(column_widths[3], len(max_frequency_str))
    
    adjusted_column_widths = [width + 2 for width in column_widths]
    
    header_line_string = "".join(header.ljust(width) for header, width in zip(headers, adjusted_column_widths))
    print(header_line_string)
    print("-" * len(header_line_string))
    
    for row_items_list in formatted_rows_list:
        line_string = "".join(item.ljust(width) for item, width in zip(row_items_list, adjusted_column_widths))
        print(line_string)

def run_application():
    argument_parser = argparse.ArgumentParser(description="ADC Frequency Calculator")
    
    argument_parser.add_argument(
        "--clock",
        type=float,
        default=7.0,
        help="ADC Clock frequency in MHz (default: 7)"
    )
    argument_parser.add_argument(
        "--resolution",
        type=int,
        default=16,
        help="ADC Resolution in bits (default: 16)"
    )
    argument_parser.add_argument(
        "--ranks",
        type=int,
        default=8,
        help="Number of ranks per ADC sequence (default: 8)"
    )
    argument_parser.add_argument(
        "--format",
        type=str,
        choices=["csv", "markdown", "readable"],
        default="readable",
        help="Output format (default: readable)"
    )
    
    parsed_arguments = argument_parser.parse_args()
    
    adc_statistics_list = calculate_adc_statistics(
        parsed_arguments.clock,
        parsed_arguments.resolution,
        parsed_arguments.ranks
    )
    
    if parsed_arguments.format == "csv":
        print_csv_format(adc_statistics_list, parsed_arguments.ranks)
    elif parsed_arguments.format == "markdown":
        print_markdown_format(adc_statistics_list, parsed_arguments.ranks)
    else:
        print_readable_format(adc_statistics_list, parsed_arguments.ranks)

if __name__ == "__main__":
    run_application()
