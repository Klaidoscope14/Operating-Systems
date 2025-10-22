# OS Lab 8: Seating Arrangement Orchestrator

## 📘 Assignment Overview

This assignment implements an automated **Seating Arrangement Orchestrator** for exam scheduling. It processes timetable, student enrollment, and room capacity data to allocate seats efficiently using a **Best-Fit algorithm**. The system supports both *dense* and *sparse* allocation modes and ensures proper utilization of available room space.

### 🎯 Objective

Develop a Python-based tool that:

* Reads exam schedules, student roll lists, and room capacities from an Excel workbook.
* Automatically allocates students to rooms based on capacity and mode settings.
* Generates comprehensive output files with seating details and remaining room capacities.

---

## ⚙️ Input Specifications

The program requires **one Excel file** named `input_data_tt.xlsx`, containing the following sheets:

| Sheet Name               | Description                                  | Required Columns               |
| ------------------------ | -------------------------------------------- | ------------------------------ |
| `in_timetable`           | Lists exam dates and subjects for each slot. | Date, Morning, Evening         |
| `in_course_roll_mapping` | Maps course codes to student roll numbers.   | rollno, course_code            |
| `in_roll_name_mapping`   | Maps roll numbers to student names.          | Roll, Name                     |
| `in_room_capacity`       | Lists available rooms and their capacities.  | Room No., Exam Capacity, Block |

---

## 🧮 Algorithm Summary

1. **Input Parsing:** Reads and validates all sheets.
2. **Mapping Construction:** Builds a mapping between course codes and roll numbers.
3. **Allocation:**

   * Implements a *Best-Fit* strategy to assign students to rooms.
   * Prefers clustering within a single block.
   * Adjusts capacity per the selected mode:

     * *Dense Mode*: Full capacity utilization.
     * *Sparse Mode*: Up to 50% of effective capacity per subject.
4. **Output Generation:**

   * `op_overall_seating_arrangement.xlsx`: Room-wise seat assignments.
   * `op_seats_left.xlsx`: Remaining seats in each room.

---

## 🧰 Running the Program

### Prerequisites

Ensure Python 3.10+ is installed with these packages:

```bash
pip install pandas openpyxl
```

### Execution

Run the following in your terminal:

```bash
python3 seating_arrangement.py --input input_data_tt.xlsx --buffer 5 --mode dense
```

* `--buffer`: Number of buffer seats to subtract from each room.
* `--mode`: Either `dense` (full capacity) or `sparse` (half capacity per subject).

---

## 📂 Output Files

| File                                  | Description                                         |
| ------------------------------------- | --------------------------------------------------- |
| `op_overall_seating_arrangement.xlsx` | Detailed seat allocation for each subject and slot. |
| `op_seats_left.xlsx`                  | Remaining capacity summary per room.                |
| `seating.log`                         | Execution log for debugging.                        |
| `errors.txt`                          | Records any runtime errors or parsing issues.       |

---

## 📄 Example Command and Output

```bash
python3 seating_arrangement.py --input input_data_tt.xlsx --buffer 5 --mode dense
```

Example Output Files:

* `Lab8/op_overall_seating_arrangement.xlsx`
* `Lab8/op_seats_left.xlsx`

---

## 🧾 Notes

* If `input_data_tt.xlsx` uses different sheet names, the program automatically detects and adapts.
* If allocation exceeds total capacity, unallocated rolls are marked under `UNALLOCATED` in the output.
* Logs are created in the same directory for traceability.

---