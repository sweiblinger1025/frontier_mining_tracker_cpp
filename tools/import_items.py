#!/usr/bin/env python3
"""
Item Import Script for Frontier Mining Tracker
Imports items from Excel/JSON into SQLite database with the new schema.

New Schema Columns:
    - id (auto), code, name, category_main, category_sub
    - buy_price_internal, buy_price_display, sell_price_internal, sell_price_display
    - weight, is_purchasable, is_sellable, is_craftable, pricing_group, notes

Usage:
    python import_items.py Items.xlsx --db frontier_mining.db
    python import_items.py items.json --db frontier_mining.db
    python import_items.py Items.xlsx --summary  # Preview only
"""

import json
import sqlite3
import argparse
import math
from pathlib import Path

# Try to import pandas for Excel support
try:
    import pandas as pd
    HAS_PANDAS = True
except ImportError:
    HAS_PANDAS = False
    print("Warning: pandas not installed. Excel import disabled.")
    print("Install with: pip install pandas openpyxl")


def calculate_sell_price(buy_price: float, sell_rate: float = 0.70) -> float:
    """Calculate sell price from buy price using game's sell rate (default 70%)."""
    return buy_price * sell_rate


def round_display_price(price: float) -> int:
    """Round price for single-item display (game's rounding behavior)."""
    return int(round(price))


def load_from_excel(excel_path: str) -> list:
    """
    Load items from Excel file.
    
    Expected columns (flexible naming):
        - Name / Item Name / ItemName
        - Category / Category Main / CategoryMain
        - Sub Category / Category Sub / CategorySub (optional)
        - Buy Price / BuyPrice / Price
        - Sell Price / SellPrice (optional - calculated if missing)
        - Weight (optional)
        - Can Buy / Purchasable / IsPurchasable (optional)
        - Can Sell / Sellable / IsSellable (optional)
        - Craftable / IsCraftable (optional)
        - Code / Item Code / ItemCode (optional)
        - Notes (optional)
    """
    if not HAS_PANDAS:
        raise RuntimeError("pandas required for Excel import")
    
    df = pd.read_excel(excel_path)
    
    # Normalize column names (lowercase, no spaces)
    df.columns = [col.lower().replace(' ', '_').replace('-', '_') for col in df.columns]
    
    # Column name mappings (try multiple variations)
    def get_column(df, names, default=None):
        for name in names:
            normalized = name.lower().replace(' ', '_')
            if normalized in df.columns:
                return df[normalized]
        return pd.Series([default] * len(df))
    
    items = []
    
    for idx, row in df.iterrows():
        # Required fields
        name = str(get_column(df, ['name', 'item_name', 'itemname']).iloc[idx])
        if pd.isna(name) or name == 'nan' or not name.strip():
            continue
            
        # Category
        category_main = str(get_column(df, ['category', 'category_main', 'categorymain', 'main_category']).iloc[idx])
        if pd.isna(category_main) or category_main == 'nan':
            category_main = ''
            
        category_sub = str(get_column(df, ['sub_category', 'category_sub', 'categorysub', 'subcategory']).iloc[idx])
        if pd.isna(category_sub) or category_sub == 'nan':
            category_sub = ''
        
        # Prices
        buy_price_val = get_column(df, ['buy_price', 'buyprice', 'price', 'cost']).iloc[idx]
        buy_price_internal = float(buy_price_val) if pd.notna(buy_price_val) else 0.0
        
        sell_price_val = get_column(df, ['sell_price', 'sellprice']).iloc[idx]
        if pd.notna(sell_price_val):
            sell_price_internal = float(sell_price_val)
        else:
            sell_price_internal = calculate_sell_price(buy_price_internal)
        
        # Display prices (rounded for single item)
        buy_price_display = round_display_price(buy_price_internal)
        sell_price_display = round_display_price(sell_price_internal)
        
        # Weight
        weight_val = get_column(df, ['weight', 'item_weight']).iloc[idx]
        weight = float(weight_val) if pd.notna(weight_val) else 0.0
        
        # Boolean flags
        def parse_bool(val, default=True):
            if pd.isna(val):
                return default
            if isinstance(val, bool):
                return val
            if isinstance(val, (int, float)):
                return bool(val)
            val_str = str(val).lower().strip()
            return val_str in ('true', 'yes', '1', 'y', '✓', 'x')
        
        is_purchasable = parse_bool(
            get_column(df, ['can_buy', 'purchasable', 'is_purchasable', 'ispurchasable', 'buyable']).iloc[idx], 
            default=True
        )
        is_sellable = parse_bool(
            get_column(df, ['can_sell', 'sellable', 'is_sellable', 'issellable']).iloc[idx],
            default=True
        )
        is_craftable = parse_bool(
            get_column(df, ['craftable', 'is_craftable', 'iscraftable', 'can_craft']).iloc[idx],
            default=False
        )
        
        # Optional fields
        code_val = get_column(df, ['code', 'item_code', 'itemcode', 'id']).iloc[idx]
        code = str(code_val) if pd.notna(code_val) and str(code_val) != 'nan' else ''
        
        notes_val = get_column(df, ['notes', 'note', 'description', 'desc']).iloc[idx]
        notes = str(notes_val) if pd.notna(notes_val) and str(notes_val) != 'nan' else ''
        
        item = {
            'code': code,
            'name': name.strip(),
            'category_main': category_main.strip(),
            'category_sub': category_sub.strip(),
            'buy_price_internal': buy_price_internal,
            'buy_price_display': buy_price_display,
            'sell_price_internal': sell_price_internal,
            'sell_price_display': sell_price_display,
            'weight': weight,
            'is_purchasable': is_purchasable,
            'is_sellable': is_sellable,
            'is_craftable': is_craftable,
            'pricing_group': 'Base70',
            'notes': notes.strip()
        }
        
        items.append(item)
    
    return items


def load_from_json(json_path: str) -> list:
    """Load items from JSON file."""
    with open(json_path, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    items = []
    
    for item_data in data:
        # Handle both old and new field names
        buy_internal = item_data.get('buy_price_internal') or item_data.get('buyPrice') or item_data.get('buy_price', 0)
        sell_internal = item_data.get('sell_price_internal') or item_data.get('sellPriceInternal') or item_data.get('sell_price')
        
        if sell_internal is None:
            sell_internal = calculate_sell_price(buy_internal)
        
        item = {
            'code': item_data.get('code', ''),
            'name': item_data.get('name', ''),
            'category_main': item_data.get('category_main') or item_data.get('categoryMain') or item_data.get('category', ''),
            'category_sub': item_data.get('category_sub') or item_data.get('categorySub', ''),
            'buy_price_internal': float(buy_internal),
            'buy_price_display': round_display_price(buy_internal),
            'sell_price_internal': float(sell_internal),
            'sell_price_display': round_display_price(sell_internal),
            'weight': float(item_data.get('weight', 0)),
            'is_purchasable': item_data.get('is_purchasable', item_data.get('isPurchasable', True)),
            'is_sellable': item_data.get('is_sellable', item_data.get('isSellable', True)),
            'is_craftable': item_data.get('is_craftable', item_data.get('isCraftable', False)),
            'pricing_group': item_data.get('pricing_group', item_data.get('pricingGroup', 'Base70')),
            'notes': item_data.get('notes', '')
        }
        
        items.append(item)
    
    return items


def export_to_json(items: list, output_path: str):
    """Export items to JSON file."""
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(items, f, indent=2, ensure_ascii=False)
    print(f"Exported {len(items)} items to {output_path}")


def import_to_database(items: list, db_path: str, clear_existing: bool = True):
    """Import items into SQLite database."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    # Create table with new schema
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            code TEXT,
            name TEXT NOT NULL,
            category_main TEXT,
            category_sub TEXT,
            buy_price_internal REAL DEFAULT 0,
            buy_price_display REAL DEFAULT 0,
            sell_price_internal REAL DEFAULT 0,
            sell_price_display REAL DEFAULT 0,
            weight REAL DEFAULT 0,
            is_purchasable INTEGER DEFAULT 1,
            is_sellable INTEGER DEFAULT 1,
            is_craftable INTEGER DEFAULT 0,
            pricing_group TEXT DEFAULT 'Base70',
            notes TEXT
        )
    """)
    
    if clear_existing:
        cursor.execute("DELETE FROM items")
        print("Cleared existing items")
    
    # Insert items
    inserted = 0
    for item in items:
        try:
            cursor.execute("""
                INSERT INTO items (
                    code, name, category_main, category_sub,
                    buy_price_internal, buy_price_display,
                    sell_price_internal, sell_price_display,
                    weight, is_purchasable, is_sellable, is_craftable,
                    pricing_group, notes
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                item['code'],
                item['name'],
                item['category_main'],
                item['category_sub'],
                item['buy_price_internal'],
                item['buy_price_display'],
                item['sell_price_internal'],
                item['sell_price_display'],
                item['weight'],
                1 if item['is_purchasable'] else 0,
                1 if item['is_sellable'] else 0,
                1 if item['is_craftable'] else 0,
                item['pricing_group'],
                item['notes']
            ))
            inserted += 1
        except sqlite3.Error as e:
            print(f"  Error inserting '{item['name']}': {e}")
    
    conn.commit()
    conn.close()
    print(f"Imported {inserted} items to {db_path}")
    return inserted


def print_summary(items: list):
    """Print summary of items."""
    print("\n" + "=" * 60)
    print("ITEM IMPORT SUMMARY")
    print("=" * 60)
    
    # Count by category
    categories = {}
    for item in items:
        cat = item['category_main'] or '(No Category)'
        categories[cat] = categories.get(cat, 0) + 1
    
    print("\nItems by Category:")
    print("-" * 40)
    for cat, count in sorted(categories.items()):
        print(f"  {cat:30} {count:4}")
    print("-" * 40)
    print(f"  {'TOTAL':30} {len(items):4}")
    
    # Price summary
    prices = [item['buy_price_internal'] for item in items if item['buy_price_internal'] > 0]
    if prices:
        print(f"\nPrice Range: ${min(prices):,.0f} - ${max(prices):,.0f}")
    
    # Flags summary
    purchasable = sum(1 for item in items if item['is_purchasable'])
    sellable = sum(1 for item in items if item['is_sellable'])
    craftable = sum(1 for item in items if item['is_craftable'])
    
    print(f"\nFlags:")
    print(f"  Purchasable: {purchasable}")
    print(f"  Sellable:    {sellable}")
    print(f"  Craftable:   {craftable}")
    
    # Sample items
    print("\n" + "=" * 60)
    print("SAMPLE ITEMS")
    print("=" * 60)
    
    for item in items[:5]:
        print(f"\n  Name:     {item['name']}")
        print(f"  Category: {item['category_main']}" + (f" - {item['category_sub']}" if item['category_sub'] else ""))
        print(f"  Buy:      ${item['buy_price_internal']:,.2f} (display: ${item['buy_price_display']:,})")
        print(f"  Sell:     ${item['sell_price_internal']:,.2f} (display: ${item['sell_price_display']:,})")
        margin = item['sell_price_internal'] - item['buy_price_internal']
        roi = (margin / item['buy_price_internal'] * 100) if item['buy_price_internal'] > 0 else 0
        print(f"  Margin:   ${margin:,.2f} ({roi:.1f}%)")


def main():
    parser = argparse.ArgumentParser(
        description='Import items into Frontier Mining Tracker database',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python import_items.py Items.xlsx --summary
    python import_items.py Items.xlsx --db frontier_mining.db
    python import_items.py Items.xlsx --json items.json --db frontier_mining.db
    python import_items.py items.json --db frontier_mining.db --no-clear
        """
    )
    
    parser.add_argument('input_file', help='Input file (Excel .xlsx or JSON .json)')
    parser.add_argument('--db', help='SQLite database path')
    parser.add_argument('--json', help='Output JSON file path')
    parser.add_argument('--summary', action='store_true', help='Print summary only')
    parser.add_argument('--no-clear', action='store_true', help='Do not clear existing items before import')
    
    args = parser.parse_args()
    
    input_path = Path(args.input_file)
    
    # Load items based on file type
    print(f"Loading from {input_path}...")
    
    if input_path.suffix.lower() in ('.xlsx', '.xls'):
        items = load_from_excel(str(input_path))
    elif input_path.suffix.lower() == '.json':
        items = load_from_json(str(input_path))
    else:
        print(f"Unsupported file type: {input_path.suffix}")
        return 1
    
    print(f"Loaded {len(items)} items")
    
    # Print summary
    print_summary(items)
    
    if args.summary:
        return 0
    
    # Export to JSON if requested
    if args.json:
        export_to_json(items, args.json)
    
    # Import to database if requested
    if args.db:
        clear_existing = not args.no_clear
        import_to_database(items, args.db, clear_existing)
    
    if not args.json and not args.db:
        print("\nNo output specified. Use --db and/or --json to save data.")
    
    return 0


if __name__ == '__main__':
    exit(main())
