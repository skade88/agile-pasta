#!/usr/bin/env python3
"""
Generate a large PSV file with 2 million employee records for testing agile-pasta.

This script creates synthetic employee data that works with the existing 
employee_summary transformation rules.

Usage:
    python3 scripts/generate_large_dataset.py [number_of_records]

Default is 2,000,000 records if no argument provided.
"""

import random
import sys
import os
from datetime import datetime, timedelta

# Get script directory and project root
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.dirname(script_dir)

# Configuration
DEFAULT_NUM_RECORDS = 2_000_000
OUTPUT_FILE = os.path.join(project_root, "examples/input/large_employees.psv")
HEADERS_FILE = os.path.join(project_root, "examples/input/large_employees_Headers.psv")
START_EMP_ID = 100000

# Data for generating realistic records
FIRST_NAMES = [
    "John", "Jane", "Bob", "Alice", "Charlie", "Diana", "Frank", "Grace", "Henry", "Ivy",
    "Jack", "Karen", "Liam", "Mary", "Nathan", "Olivia", "Paul", "Quinn", "Rachel", "Sam",
    "Tom", "Uma", "Victor", "Wendy", "Xander", "Yara", "Zoe", "Adam", "Beth", "Carl",
    "David", "Emma", "Felix", "Gina", "Harry", "Iris", "Jake", "Kate", "Leo", "Mia",
    "Nick", "Opa", "Pete", "Quin", "Ruby", "Steve", "Tina", "Ulrich", "Vera", "Will"
]

LAST_NAMES = [
    "Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia", "Miller", "Davis", "Rodriguez", "Martinez",
    "Hernandez", "Lopez", "Gonzalez", "Wilson", "Anderson", "Thomas", "Taylor", "Moore", "Jackson", "Martin",
    "Lee", "Perez", "Thompson", "White", "Harris", "Sanchez", "Clark", "Ramirez", "Lewis", "Robinson",
    "Walker", "Young", "Allen", "King", "Wright", "Scott", "Torres", "Nguyen", "Hill", "Flores",
    "Green", "Adams", "Nelson", "Baker", "Hall", "Rivera", "Campbell", "Mitchell", "Carter", "Roberts"
]

POSITIONS = [
    "Software Engineer", "Data Analyst", "Project Manager", "Quality Assurance", "UX Designer",
    "DevOps Engineer", "Business Analyst", "Senior Developer", "Marketing Manager", "Product Manager",
    "System Administrator", "Database Administrator", "Technical Writer", "Sales Representative",
    "Customer Support", "Security Analyst", "Network Engineer", "Frontend Developer", "Backend Developer",
    "Full Stack Developer", "Data Scientist", "Machine Learning Engineer", "Cloud Architect"
]

DEPARTMENTS = [
    "Engineering", "Analytics", "Management", "QA", "Design", "Marketing", "Sales", "Support",
    "Operations", "Security", "Finance", "HR", "Legal", "Research", "IT"
]

def generate_hire_date():
    """Generate a random hire date between 2020 and 2024"""
    start_date = datetime(2020, 1, 1)
    end_date = datetime(2024, 12, 31)
    time_between = end_date - start_date
    days_between = time_between.days
    random_days = random.randrange(days_between)
    hire_date = start_date + timedelta(days=random_days)
    return hire_date.strftime("%Y-%m-%d")

def generate_salary():
    """Generate salary that will test the >= 75000 filter (mix above and below)"""
    # 60% above 75000, 40% below to ensure we get good output after filtering
    if random.random() < 0.6:
        return random.randint(75000, 150000)  # Above threshold
    else:
        return random.randint(45000, 74999)   # Below threshold

def generate_employee_record(emp_id):
    """Generate a single employee record"""
    first_name = random.choice(FIRST_NAMES)
    last_name = random.choice(LAST_NAMES)
    name = f"{first_name} {last_name}"
    position = random.choice(POSITIONS)
    hire_date = generate_hire_date()
    salary = generate_salary()
    department = random.choice(DEPARTMENTS)
    
    return f"{emp_id}|{name}|{position}|{hire_date}|{salary}|{department}"

def create_headers_file():
    """Create the headers file"""
    os.makedirs(os.path.dirname(HEADERS_FILE), exist_ok=True)
    with open(HEADERS_FILE, 'w') as f:
        f.write("1.emp_id|name|position|hire_date|salary|department\n")

def main():
    # Parse command line arguments
    num_records = DEFAULT_NUM_RECORDS
    if len(sys.argv) > 1:
        try:
            num_records = int(sys.argv[1])
        except ValueError:
            print(f"Error: Invalid number of records '{sys.argv[1]}'")
            sys.exit(1)
    
    print(f"Generating {num_records:,} employee records...")
    print(f"Output file: {OUTPUT_FILE}")
    print(f"Headers file: {HEADERS_FILE}")
    
    try:
        # Create output directory
        os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)
        
        # Generate headers file
        create_headers_file()
        print(f"Created headers file: {HEADERS_FILE}")
        
        # Generate data file
        with open(OUTPUT_FILE, 'w') as f:
            for i in range(num_records):
                emp_id = START_EMP_ID + i
                record = generate_employee_record(emp_id)
                f.write(record + '\n')
                
                # Progress indicator
                if (i + 1) % 100000 == 0:
                    print(f"Generated {i + 1:,} records...")
        
        print(f"Successfully generated {num_records:,} records in {OUTPUT_FILE}")
        
        # Show file size
        file_size = os.path.getsize(OUTPUT_FILE)
        print(f"Data file size: {file_size:,} bytes ({file_size / 1024 / 1024:.1f} MB)")
        
        # Show first few lines as sample
        print("\nFirst 5 lines of generated data:")
        with open(OUTPUT_FILE, 'r') as f:
            for i, line in enumerate(f):
                if i >= 5:
                    break
                print(f"  {line.strip()}")
                
        print(f"\nLarge dataset created successfully!")
        print(f"Use with: ./build/bin/agile-pasta transform --in examples/input --out examples/output")
                
    except Exception as e:
        print(f"Error generating data: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()