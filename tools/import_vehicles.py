#!/usr/bin/env python3
"""
Vehicle Specs Import Script for Frontier Mining Tracker
Converts Excel spreadsheet to JSON and imports into SQLite database.

Usage:
    python import_vehicles.py Vehicle_Specs.xlsx --json vehicles.json --db frontier_mining.db
    python import_vehicles.py Vehicle_Specs.xlsx --summary  # Preview only
"""

import json
import sqlite3
import re
import argparse
from pathlib import Path

try:
    import pandas as pd
    HAS_PANDAS = True
except ImportError:
    HAS_PANDAS = False
    print("Warning: pandas not installed. Excel import disabled.")


def generate_vehicle_id(name: str) -> str:
    """Generate a unique ID from vehicle name."""
    clean = re.sub(r'[^a-zA-Z0-9\s]', '', name)
    clean = re.sub(r'\s+', '_', clean.strip())
    return clean.upper()


def parse_category(category: str) -> tuple:
    """Parse category string into main and sub categories."""
    if pd.isna(category):
        return ("Unknown", "")
    
    main = category.replace("Vehicles - ", "").strip()
    
    category_map = {
        "Rock Trucks": ("Rock Truck", "Haul Truck"),
        "Excavators": ("Excavator", "Crawler"),
        "Loaders": ("Loader", "Wheel Loader"),
        "Dozers": ("Dozer", "Crawler Dozer"),
        "Graders": ("Grader", "Motor Grader"),
        "Paving": ("Paving", "Road Equipment"),
        "Trucks": ("Truck", "Utility Truck"),
        "Fuel Trucks": ("Truck", "Fuel Truck"),
        "Flying": ("Drone", "UAV"),
        "Tunneler": ("Tunneler", "Underground"),
    }
    
    return category_map.get(main, (main, ""))


def determine_capacity_type(category_main: str) -> str:
    """Determine if capacity should go to bucket or truck field."""
    truck_types = ["Rock Truck", "Truck"]
    return "truck" if category_main in truck_types else "bucket"


def convert_excel_to_vehicles(excel_path: str) -> list:
    """Convert Excel file to list of vehicle dictionaries."""
    if not HAS_PANDAS:
        raise RuntimeError("pandas required for Excel import")
    
    df = pd.read_excel(excel_path)
    
    vehicles = []
    seen_ids = set()
    
    for _, row in df.iterrows():
        name = str(row['Vehicle Name']).strip()
        vehicle_id = generate_vehicle_id(name)
        
        # Handle duplicate IDs
        original_id = vehicle_id
        counter = 2
        while vehicle_id in seen_ids:
            vehicle_id = f"{original_id}_{counter}"
            counter += 1
        seen_ids.add(vehicle_id)
        
        category_main, category_sub = parse_category(row['Category'])
        capacity_type = determine_capacity_type(category_main)
        
        capacity_m3 = float(row['Capacity (m3)']) if pd.notna(row['Capacity (m3)']) else 0.0
        
        vehicle = {
            "id": vehicle_id,
            "name": name,
            "category_main": category_main,
            "category_sub": category_sub,
            "bucket_capacity_m3": capacity_m3 if capacity_type == "bucket" else 0.0,
            "truck_capacity_m3": capacity_m3 if capacity_type == "truck" else 0.0,
            "tank_capacity_l": float(row['Fuel Capacity (L)']) if pd.notna(row['Fuel Capacity (L)']) else 0.0,
            "fuel_use_l_per_hour": float(row['Fuel Use (L/Hour)']) if pd.notna(row['Fuel Use (L/Hour)']) else 0.0,
            "purchase_price": float(row['Purchase Price']) if pd.notna(row['Purchase Price']) else 0.0,
            "active": 1,
            "notes": str(row['Notes']) if pd.notna(row['Notes']) else ""
        }
        
        vehicles.append(vehicle)
    
    return vehicles


def load_from_json(json_path: str) -> list:
    """Load vehicles from JSON file."""
    with open(json_path, 'r', encoding='utf-8') as f:
        return json.load(f)


def export_to_json(vehicles: list, output_path: str):
    """Export vehicles to JSON file."""
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(vehicles, f, indent=2, ensure_ascii=False)
    print(f"Exported {len(vehicles)} vehicles to {output_path}")


def import_to_database(vehicles: list, db_path: str, clear_existing: bool = True):
    """Import vehicles into SQLite database."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    # Create table with correct schema
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS vehicles (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            category_main TEXT,
            category_sub TEXT,
            bucket_capacity_m3 REAL DEFAULT 0,
            truck_capacity_m3 REAL DEFAULT 0,
            tank_capacity_l REAL DEFAULT 0,
            fuel_use_l_per_hour REAL DEFAULT 0,
            purchase_price REAL DEFAULT 0,
            active INTEGER DEFAULT 1,
            notes TEXT
        )
    """)
    
    if clear_existing:
        cursor.execute("DELETE FROM vehicles")
        print("Cleared existing vehicles")
    
    # Insert vehicles
    inserted = 0
    for v in vehicles:
        try:
            cursor.execute("""
                INSERT OR REPLACE INTO vehicles (
                    id, name, category_main, category_sub,
                    bucket_capacity_m3, truck_capacity_m3,
                    tank_capacity_l, fuel_use_l_per_hour,
                    purchase_price, active, notes
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                v['id'], v['name'], v['category_main'], v['category_sub'],
                v['bucket_capacity_m3'], v['truck_capacity_m3'],
                v['tank_capacity_l'], v['fuel_use_l_per_hour'],
                v['purchase_price'], v.get('active', 1), v['notes']
            ))
            inserted += 1
        except sqlite3.Error as e:
            print(f"  Error inserting '{v['name']}': {e}")
    
    conn.commit()
    conn.close()
    print(f"Imported {inserted} vehicles to {db_path}")
    return inserted


def print_summary(vehicles: list):
    """Print summary of vehicles."""
    print("\n" + "=" * 60)
    print("VEHICLE IMPORT SUMMARY")
    print("=" * 60)
    
    # Count by category
    categories = {}
    for v in vehicles:
        cat = v['category_main']
        categories[cat] = categories.get(cat, 0) + 1
    
    print("\nVehicles by Category:")
    print("-" * 40)
    for cat, count in sorted(categories.items()):
        print(f"  {cat:20} {count:3}")
    print("-" * 40)
    print(f"  {'TOTAL':20} {len(vehicles):3}")
    
    # Capacity summary
    loaders = [v for v in vehicles if v['bucket_capacity_m3'] > 0]
    trucks = [v for v in vehicles if v['truck_capacity_m3'] > 0]
    
    print("\nCapacity Ranges:")
    if loaders:
        min_b = min(v['bucket_capacity_m3'] for v in loaders)
        max_b = max(v['bucket_capacity_m3'] for v in loaders)
        print(f"  Bucket: {min_b:.1f} - {max_b:.1f} m³")
    if trucks:
        min_t = min(v['truck_capacity_m3'] for v in trucks)
        max_t = max(v['truck_capacity_m3'] for v in trucks)
        print(f"  Truck:  {min_t:.1f} - {max_t:.1f} m³")
    
    # Sample
    print("\n" + "=" * 60)
    print("SAMPLE VEHICLES")
    print("=" * 60)
    
    shown = set()
    for v in vehicles:
        if v['category_main'] not in shown:
            shown.add(v['category_main'])
            print(f"\n[{v['category_main']}]")
            print(f"  ID:    {v['id']}")
            print(f"  Name:  {v['name']}")
            if v['bucket_capacity_m3'] > 0:
                print(f"  Bucket: {v['bucket_capacity_m3']} m³")
            if v['truck_capacity_m3'] > 0:
                print(f"  Truck:  {v['truck_capacity_m3']} m³")
            print(f"  Fuel:  {v['fuel_use_l_per_hour']} L/hr")
            print(f"  Price: ${v['purchase_price']:,.0f}")


def main():
    parser = argparse.ArgumentParser(
        description='Import vehicle specs into Frontier Mining Tracker database'
    )
    parser.add_argument('input_file', help='Input file (Excel .xlsx or JSON .json)')
    parser.add_argument('--db', help='SQLite database path')
    parser.add_argument('--json', help='Output JSON file path')
    parser.add_argument('--summary', action='store_true', help='Print summary only')
    parser.add_argument('--no-clear', action='store_true', help='Do not clear existing vehicles')
    
    args = parser.parse_args()
    
    input_path = Path(args.input_file)
    
    print(f"Loading from {input_path}...")
    
    if input_path.suffix.lower() in ('.xlsx', '.xls'):
        vehicles = convert_excel_to_vehicles(str(input_path))
    elif input_path.suffix.lower() == '.json':
        vehicles = load_from_json(str(input_path))
    else:
        print(f"Unsupported file type: {input_path.suffix}")
        return 1
    
    print(f"Loaded {len(vehicles)} vehicles")
    
    print_summary(vehicles)
    
    if args.summary:
        return 0
    
    if args.json:
        export_to_json(vehicles, args.json)
    
    if args.db:
        clear_existing = not args.no_clear
        import_to_database(vehicles, args.db, clear_existing)
    
    if not args.json and not args.db:
        print("\nNo output specified. Use --db and/or --json to save data.")
    
    return 0


if __name__ == '__main__':
    exit(main())
