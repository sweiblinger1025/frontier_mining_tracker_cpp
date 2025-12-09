#!/usr/bin/env python3
"""
Import Locations from Excel to JSON for Frontier Mining Tracker

Reads: Frontier_Mining_Locations_.xlsx
Outputs:
  - maps.json (id, abbrev, name)
  - location_types.json (id, name)
  - locations.json (id, name, map_id, type_id)

The map abbreviation is extracted from the location name prefix (e.g., "ARC - Main Base" -> "ARC")
"""

import pandas as pd
import json
import sys
from pathlib import Path


def extract_abbreviation(location_name: str) -> str:
    """Extract the map abbreviation from location name prefix."""
    if " - " in location_name:
        return location_name.split(" - ")[0].strip()
    return ""


def main(excel_path: str, output_dir: str = "."):
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)
    
    # Read Excel file
    print(f"Reading {excel_path}...")
    df = pd.read_excel(excel_path)
    
    print(f"Found {len(df)} locations")
    
    # Extract abbreviations from location names
    df['Abbrev'] = df['Location'].apply(extract_abbreviation)
    
    # Build maps table (unique map names with abbreviations)
    maps_df = df[['Abbrev', 'Map']].drop_duplicates().sort_values('Abbrev')
    maps_data = []
    map_name_to_id = {}
    
    for idx, (_, row) in enumerate(maps_df.iterrows(), start=1):
        maps_data.append({
            "id": idx,
            "abbrev": row['Abbrev'],
            "name": row['Map']
        })
        map_name_to_id[row['Map']] = idx
    
    print(f"Extracted {len(maps_data)} unique maps")
    
    # Build location_types table
    types_sorted = sorted(df['Type'].unique())
    types_data = []
    type_name_to_id = {}
    
    for idx, type_name in enumerate(types_sorted, start=1):
        types_data.append({
            "id": idx,
            "name": type_name
        })
        type_name_to_id[type_name] = idx
    
    print(f"Extracted {len(types_data)} unique location types")
    
    # Build locations table
    locations_data = []
    for idx, row in enumerate(df.itertuples(), start=1):
        locations_data.append({
            "id": idx,
            "name": row.Location,
            "map_id": map_name_to_id[row.Map],
            "type_id": type_name_to_id[row.Type]
        })
    
    print(f"Processed {len(locations_data)} locations")
    
    # Write JSON files
    maps_file = output_path / "maps.json"
    with open(maps_file, 'w', encoding='utf-8') as f:
        json.dump(maps_data, f, indent=2)
    print(f"Wrote {maps_file}")
    
    types_file = output_path / "location_types.json"
    with open(types_file, 'w', encoding='utf-8') as f:
        json.dump(types_data, f, indent=2)
    print(f"Wrote {types_file}")
    
    locations_file = output_path / "locations.json"
    with open(locations_file, 'w', encoding='utf-8') as f:
        json.dump(locations_data, f, indent=2)
    print(f"Wrote {locations_file}")
    
    # Print summary
    print("\n--- Summary ---")
    print(f"Maps: {len(maps_data)}")
    for m in maps_data:
        print(f"  {m['id']:2}. [{m['abbrev']}] {m['name']}")
    
    print(f"\nLocation Types: {len(types_data)}")
    for t in types_data:
        print(f"  {t['id']}. {t['name']}")
    
    print(f"\nLocations by Map:")
    for m in maps_data:
        count = sum(1 for loc in locations_data if loc['map_id'] == m['id'])
        print(f"  {m['abbrev']}: {count} locations")
    
    print("\nDone!")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python import_locations.py <excel_file> [output_dir]")
        print("Example: python import_locations.py Frontier_Mining_Locations_.xlsx ./resources/data")
        sys.exit(1)
    
    excel_file = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "."
    
    main(excel_file, output_dir)
