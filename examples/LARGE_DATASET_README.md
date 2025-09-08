# Large Dataset Example

This directory contains a large-scale example input file designed to test the agile-pasta data transformation tool with 2 million employee records.

## Files

- `large_employees.psv` - Input data file with 2,000,000 employee records (~120MB)
- `large_employees_Headers.psv` - Header file defining the column structure

## Data Structure

The large dataset follows the same structure as the original `employees.psv` example but with 2 million records:

### Headers (same as original):
```
emp_id|name|position|hire_date|salary|department
```

### Data Format:
```
100000|Jack Jones|Quality Assurance|2022-08-09|133934|Analytics
100001|Iris Green|UX Designer|2024-11-26|147369|Support
100002|Will Baker|Customer Support|2023-07-14|142460|Marketing
...
```

## Data Characteristics

- **Employee IDs**: Start from 100000 and increment sequentially
- **Names**: Randomly generated combinations of 50 first names and 50 last names
- **Positions**: 23 different job positions including Software Engineer, Data Analyst, etc.
- **Hire Dates**: Random dates between 2020-01-01 and 2024-12-31
- **Salaries**: 
  - 60% above $75,000 (range: $75,000 - $150,000)
  - 40% below $75,000 (range: $45,000 - $74,999)
  - This distribution ensures good test coverage for the salary >= 75000 filter
- **Departments**: 15 different departments (Engineering, Analytics, Management, etc.)

## Usage with Existing Transformation Rules

This large dataset is designed to work seamlessly with the existing `employee_summary` transformation rules:

```bash
# Transform the large dataset (along with other files)
./build/bin/agile-pasta transform --in examples/input --out examples/output

# To test ONLY the large dataset, temporarily move other files:
mkdir -p /tmp/backup
mv examples/input/employees.psv examples/input/employees_Headers.psv /tmp/backup/
mv examples/input/departments.psv examples/input/departments_Headers.psv /tmp/backup/
./build/bin/agile-pasta transform --in examples/input --out examples/output
# Restore files when done:
mv /tmp/backup/* examples/input/
```

### Expected Output

With the existing transformation rules:
- **Global Filter**: `salary >= '75000'` - filters to approximately 1.2 million records (60%)
- **Field Transformations**:
  - `employee_name` → `UPPER(name)` - converts names to uppercase
  - `annual_salary` → `salary` - copies salary value
  - `department_name` → `LOWER(department)` - converts department to lowercase
  - `years_since_hire` → `2` - default value for demo

## Performance Notes

- **Loading**: Multi-threaded loading takes advantage of available CPU cores
- **Processing**: Memory-efficient processing handles large datasets
- **Output**: Generates approximately 1.2M records in the final CSV

## Use Cases

This large dataset enables testing of:

1. **Performance**: Processing speed with large datasets
2. **Memory Usage**: Memory efficiency during transformation
3. **Scalability**: System behavior under load
4. **End-to-End Workflow**: Complete data pipeline validation

## File Generation

The large dataset was generated using a Python script that creates realistic but synthetic employee data. The generation process ensures:

- Consistent data format matching the original schema
- Realistic value distributions
- Proper filtering test coverage
- Large enough scale for performance testing

## Integration

The large dataset integrates seamlessly with the existing agile-pasta examples:

- Uses the same headers structure as `employees.psv`
- Works with existing `employee_summary` transformation rules
- Produces output in the same format as smaller examples
- Demonstrates scalability of the transformation engine

This provides a comprehensive end-to-end example of processing very large datasets using the agile-pasta transformation tool.