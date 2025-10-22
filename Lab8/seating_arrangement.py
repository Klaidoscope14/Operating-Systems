#!/usr/bin/env python3
"""
Robust Seating Arrangement Orchestrator (auto-detects sheet layout)

Drop this file into your Lab8 folder and run:
  python3 seating_arrangement.py --input input_data_tt.xlsx --buffer 5 --mode dense

It will:
 - try multiple expected sheet/column layouts (legacy 'rooms/schedule/roll_name_map'
   and 'in_*' layout used in your workbook)
 - if neither exact layout exists, it will scan sheets to infer likely columns
 - produce op_overall_seating_arrangement.xlsx and op_seats_left.xlsx
 - write logs to seating.log and errors to errors.txt
"""
import argparse
import logging
import traceback
import sys
from collections import defaultdict, deque
import pandas as pd
import copy

# -------------------------------------------------------------------------
# Default candidate layouts (the script will try these in order)
LAYOUTS = [
    # layout A (original script)
    {
        "rooms_sheet": "rooms",
        "schedule_sheet": "schedule",
        "rollname_sheet": "roll_name_map",
        "rooms_cols": {"building": "Building", "room": "Room", "capacity": "Capacity"},
        "schedule_cols": {"date": "Date", "slot": "Slot", "subject": "Subject", "rolls": "Rolls"},
        "rollmap_cols": {"roll": "Roll", "name": "Name"}
    },
    # layout B (the 'in_*' layout detected in your workbook)
    {
        "rooms_sheet": "in_room_capacity",
        "schedule_sheet": "in_timetable",
        "rollname_sheet": "in_roll_name_mapping",
        "rooms_cols": {"room": "Room No.", "capacity": "Exam Capacity", "block": "Block"},
        # note: timetable is expanded via in_course_roll_mapping
        "course_roll_sheet": "in_course_roll_mapping",
        "course_roll_cols": {"roll": "rollno", "course": "course_code"},
        "timetable_cols": {"date": "Date", "morning": "Morning", "evening": "Evening"},
        "rollmap_cols": {"roll": "Roll", "name": "Name"}
    }
]
# -------------------------------------------------------------------------

def setup_logging():
    logging.basicConfig(filename="seating.log", level=logging.DEBUG,
                        format="%(asctime)s %(levelname)s: %(message)s")

def log_exception(e):
    with open("errors.txt", "a", encoding="utf-8") as ef:
        ef.write(f"Exception: {str(e)}\n")
        ef.write(traceback.format_exc())
        ef.write("\n\n")
    logging.exception("Exception occurred: %s", e)

def normalize_list_cell(cell):
    if pd.isna(cell): return []
    s = str(cell)
    for sep in [";", "\n", ","]:
        if sep in s:
            return [x.strip() for x in s.split(sep) if x.strip()]
    return [s.strip()] if s.strip() else []

# ---------- allocation primitives (unchanged core logic) -------------------
def build_rooms_generic(rooms_df, cols_map, buffer):
    rooms_by_block = defaultdict(list)
    total_capacity = 0
    for _, row in rooms_df.iterrows():
        try:
            # support both 'building' + 'room' + 'capacity' or 'Room No.' + 'Exam Capacity' + 'Block'
            room = str(row[cols_map.get("room")]).strip()
            cap = int(row[cols_map.get("capacity")])
            block = str(row.get(cols_map.get("block", ""), "")).strip()
        except Exception:
            logging.warning("Skipping malformed room row: %s", row.to_dict())
            continue
        eff = max(0, cap - buffer)
        room_obj = {
            "block": block,
            "room": room,
            "capacity": cap,
            "eff_capacity": eff,
            "remaining": eff,
            "assigned_subjects": []
        }
        rooms_by_block[block].append(room_obj)
        total_capacity += eff

    # sort rooms inside blocks by numeric part if possible
    def sort_key(r):
        try:
            num = int("".join([c for c in r["room"] if c.isdigit()]) or 0)
            return (0, num)
        except:
            return (1, r["room"])
    for b in rooms_by_block:
        rooms_by_block[b].sort(key=sort_key)
    return rooms_by_block, total_capacity

def allocate(subjects, rooms_by_block, mode="dense"):
    allocations = []
    def per_room_limit(eff):
        return eff if mode == "dense" else eff // 2
    subjects_sorted = sorted(subjects, key=lambda s: len(s["Rolls"]), reverse=True)

    for subj in subjects_sorted:
        rolls = deque(subj["Rolls"])
        needed = len(rolls)
        subj_allocs = []
        blocks = sorted([(b, sum(r["remaining"] for r in rooms_by_block[b])) for b in rooms_by_block], key=lambda x: x[1], reverse=True)

        # try single block
        for b, cap in blocks:
            if cap <= 0: continue
            free = sum(min(per_room_limit(r["eff_capacity"]), r["remaining"]) for r in rooms_by_block[b])
            if free >= needed:
                for r in rooms_by_block[b]:
                    can = min(per_room_limit(r["eff_capacity"]), r["remaining"], needed)
                    if can <= 0: continue
                    assigned = [rolls.popleft() for _ in range(min(can, len(rolls)))]
                    r["remaining"] -= len(assigned)
                    r["assigned_subjects"].append({"subject": subj["Subject"], "assigned_rolls": assigned})
                    subj_allocs.append({"subject": subj["Subject"], "block": b, "room": r["room"], "assigned_rolls": assigned, "seats_allocated": len(assigned)})
                    needed = len(rolls)
                    if needed == 0: break
                break

        # multi-block if needed
        if needed > 0:
            all_rooms = [r for b in rooms_by_block for r in rooms_by_block[b]]
            for r in all_rooms:
                can = min(per_room_limit(r["eff_capacity"]), r["remaining"], needed)
                if can <= 0: continue
                assigned = [rolls.popleft() for _ in range(min(can, len(rolls)))]
                r["remaining"] -= len(assigned)
                r["assigned_subjects"].append({"subject": subj["Subject"], "assigned_rolls": assigned})
                subj_allocs.append({"subject": subj["Subject"], "block": r["block"], "room": r["room"], "assigned_rolls": assigned, "seats_allocated": len(assigned)})
                needed = len(rolls)
                if needed == 0: break

        if rolls:
            subj_allocs.append({"subject": subj["Subject"], "block": "UNALLOCATED", "room": "", "assigned_rolls": list(rolls), "seats_allocated": len(rolls)})
        allocations.extend(subj_allocs)
    return allocations

def write_outputs(all_allocs, rooms_by_block, roll_name_map):
    rows = []
    for (date, slot), allocs in all_allocs.items():
        for a in allocs:
            names = [f"{r}:{roll_name_map.get(r, 'Unknown Name')}" for r in a.get("assigned_rolls",[])]
            rows.append({
                "Date": date, "Slot": slot, "Subject": a.get("subject",""),
                "Block": a.get("block",""), "Room": a.get("room",""),
                "AssignedRolls": ";".join(a.get("assigned_rolls",[])),
                "AssignedRollsWithNames": ";".join(names),
                "SeatsAllocated": a.get("seats_allocated", len(a.get("assigned_rolls",[])))
            })
    pd.DataFrame(rows).to_excel("op_overall_seating_arrangement.xlsx", index=False)

    rem_rows = []
    for b, rooms in rooms_by_block.items():
        for r in rooms:
            rem_rows.append({
                "Block": b, "Room": r["room"], "Capacity": r["capacity"],
                "EffectiveCapacity": r["eff_capacity"], "RemainingSeats": r["remaining"]
            })
    pd.DataFrame(rem_rows).to_excel("op_seats_left.xlsx", index=False)

# ---------- auto-detection utilities --------------------------------------
def find_layout_and_build_schedule(path, buffer, mode):
    """
    Try the known layouts; if none exactly match, attempt to infer mapping.
    Returns (rooms_by_block_template, schedule_grouped, roll_name_map, used_layout_description)
    """
    xl = pd.ExcelFile(path)
    sheets = xl.sheet_names
    logging.info("Workbook sheets: %s", sheets)

    # helper: try Layout entries
    for layout in LAYOUTS:
        try:
            # layout B uses different path: in_room_capacity + in_timetable + in_course_roll_mapping
            if layout.get("rooms_sheet") in sheets:
                # attempt to parse the expected sheets for this layout
                if "in_timetable" in sheets and layout.get("course_roll_sheet","") in sheets:
                    logging.info("Trying layout (in_*) detection")
                    rooms_df = xl.parse(layout["rooms_sheet"])
                    cr_df = xl.parse(layout["course_roll_sheet"])
                    tt_df = xl.parse(layout["schedule_sheet"])
                    rn_df = xl.parse(layout["rollmap_cols"].get("sheet", layout.get("rollname_sheet","")) ) if layout.get("rollmap_cols") else (xl.parse(layout["rollname_sheet"]) if layout.get("rollname_sheet") in sheets else pd.DataFrame())
                    # Build course->roll mapping
                    course_to_rolls = defaultdict(list)
                    for _, r in cr_df.iterrows():
                        try:
                            roll = str(r[layout["course_roll_cols"]["roll"]]).strip()
                            course = str(r[layout["course_roll_cols"]["course"]]).strip()
                            if roll and course:
                                course_to_rolls[course].append(roll)
                        except Exception:
                            continue
                    # roll->name
                    roll_name_map = {}
                    if layout.get("rollmap_cols"):
                        rn_sheet = layout.get("rollname_sheet")
                        if rn_sheet in sheets:
                            rn_df = xl.parse(rn_sheet)
                            for _, r in rn_df.iterrows():
                                try:
                                    roll = str(r[layout["rollmap_cols"]["roll"]]).strip()
                                    name = str(r[layout["rollmap_cols"]["name"]]).strip()
                                    if roll:
                                        roll_name_map[roll] = name
                                except Exception:
                                    continue
                    # build schedule rows from timetable
                    schedule_rows = []
                    for _, row in tt_df.iterrows():
                        date = row.get(layout["timetable_cols"]["date"], "")
                        for subj in normalize_list_cell(row.get(layout["timetable_cols"]["morning"], "")):
                            if subj and subj.upper() != "NO EXAM":
                                schedule_rows.append({"Date": date, "Slot": "Morning", "Subject": subj, "Rolls": course_to_rolls.get(subj, [])})
                        for subj in normalize_list_cell(row.get(layout["timetable_cols"]["evening"], "")):
                            if subj and subj.upper() != "NO EXAM":
                                schedule_rows.append({"Date": date, "Slot": "Evening", "Subject": subj, "Rolls": course_to_rolls.get(subj, [])})
                    rooms_by_block_template, total_cap = build_rooms_generic(rooms_df, layout["rooms_cols"], buffer)
                    logging.info("Using detected 'in_*' layout. Effective capacity: %d", total_cap)
                    return rooms_by_block_template, schedule_rows, roll_name_map, "in_* layout"
                # fallback: try layout A (rooms + schedule + roll_name_map)
                if layout.get("rooms_sheet") in sheets and layout.get("schedule_sheet") in sheets:
                    logging.info("Trying legacy layout (rooms,schedule,roll_name_map)")
                    rooms_df = xl.parse(layout["rooms_sheet"])
                    schedule_df = xl.parse(layout["schedule_sheet"])
                    rn_df = xl.parse(layout["rollname_sheet"]) if layout.get("rollname_sheet") in sheets else pd.DataFrame()
                    # build roll->name
                    roll_name_map = {}
                    if not rn_df.empty:
                        for _, r in rn_df.iterrows():
                            try:
                                roll = str(r[layout["rollmap_cols"]["roll"]]).strip()
                                name = str(r[layout["rollmap_cols"]["name"]]).strip()
                                if roll:
                                    roll_name_map[roll] = name
                            except Exception:
                                continue
                    # convert schedule_df rows into schedule_rows
                    schedule_rows = []
                    if layout["schedule_cols"]["date"] not in schedule_df.columns:
                        logging.warning("Expected schedule column 'Date' not found in schedule sheet for this layout.")
                        continue
                    for _, r in schedule_df.iterrows():
                        date = r[layout["schedule_cols"]["date"]]
                        slot = r[layout["schedule_cols"]["slot"]]
                        subject = r[layout["schedule_cols"]["subject"]]
                        rolls = normalize_list_cell(r[layout["schedule_cols"]["rolls"]])
                        schedule_rows.append({"Date": date, "Slot": slot, "Subject": str(subject).strip(), "Rolls": rolls})
                    rooms_by_block_template, total_cap = build_rooms_generic(rooms_df, layout["rooms_cols"], buffer)
                    logging.info("Using legacy layout. Effective capacity: %d", total_cap)
                    return rooms_by_block_template, schedule_rows, roll_name_map, "legacy layout"
        except Exception as e:
            logging.warning("Layout attempt failed: %s", e)
            continue

    # If neither layout matched exactly: attempt inference
    logging.info("No exact layout match. Attempting inference over workbook sheets & columns.")
    # heuristics: find sheet with Date + Morning/Evening -> treat as timetable
    tt_sheet = None
    cr_sheet = None
    rooms_sheet = None
    rn_sheet = None
    for s in sheets:
        cols = [c.strip() for c in pd.ExcelFile(path).parse(s, nrows=0).columns.tolist()]
        cols_lower = [c.lower() for c in cols]
        # timetable candidate
        if ("date" in cols_lower) and (("morning" in cols_lower) or ("slot" in cols_lower)):
            tt_sheet = s
        # course-roll mapping candidate
        if ("roll" in "".join(cols_lower) or "rollno" in cols_lower) and ("course" in "".join(cols_lower) or "course_code" in cols_lower):
            cr_sheet = s if cr_sheet is None else cr_sheet
        # rooms candidate
        if ("exam capacity" in " ".join(cols_lower) or "capacity" in " ".join(cols_lower)) and ("room" in " ".join(cols_lower)):
            rooms_sheet = s if rooms_sheet is None else rooms_sheet
        # roll->name candidate
        if ("roll" in " ".join(cols_lower) and "name" in " ".join(cols_lower)):
            rn_sheet = s if rn_sheet is None else rn_sheet

    # If we found components, parse them with flexible column names
    if rooms_sheet is None:
        raise Exception("Could not find a rooms sheet automatically. Expected sheet with Room & Capacity columns.")
    rooms_df = pd.ExcelFile(path).parse(rooms_sheet)
    # find best columns in rooms_df
    colnames = rooms_df.columns.tolist()
    def pick_col(possible):
        for p in possible:
            for c in colnames:
                if c.strip().lower() == p.lower():
                    return c
        return None
    room_col = pick_col(["Room No.","Room","Room No","room","room no"])
    cap_col = pick_col(["Exam Capacity","Capacity","exam capacity","capacity","Seats"])
    block_col = pick_col(["Block","block","Building","building","Block Name"])
    if room_col is None or cap_col is None:
        raise Exception(f"Rooms sheet '{rooms_sheet}' does not contain obvious Room/Capacity columns. Found: {colnames}")

    rooms_cols_map = {"room": room_col, "capacity": cap_col, "block": block_col}
    rooms_by_block_template, total_cap = build_rooms_generic(rooms_df, rooms_cols_map, buffer)

    # build course->roll mapping if we found cr_sheet
    course_to_rolls = defaultdict(list)
    if cr_sheet:
        cr_df = pd.ExcelFile(path).parse(cr_sheet)
        cr_cols = cr_df.columns.tolist()
        roll_col = pick_col(["rollno","roll","roll_no","roll number","rollno"])
        course_col = pick_col(["course_code","course","course code","subject code"])
        if roll_col and course_col:
            for _, r in cr_df.iterrows():
                try:
                    roll = str(r[roll_col]).strip()
                    course = str(r[course_col]).strip()
                    if roll and course:
                        course_to_rolls[course].append(roll)
                except Exception:
                    continue

    # roll->name map if found rn_sheet
    roll_name_map = {}
    if rn_sheet:
        rn_df = pd.ExcelFile(path).parse(rn_sheet)
        rn_cols = rn_df.columns.tolist()
        rn_roll = pick_col(["Roll","roll","rollno","roll_no"])
        rn_name = pick_col(["Name","name","Student Name","student name"])
        if rn_roll and rn_name:
            for _, r in rn_df.iterrows():
                try:
                    roll = str(r[rn_roll]).strip()
                    name = str(r[rn_name]).strip()
                    if roll:
                        roll_name_map[roll] = name
                except Exception:
                    continue

    # build schedule rows from tt_sheet if found and course_to_rolls present
    schedule_rows = []
    if tt_sheet:
        tt_df = pd.ExcelFile(path).parse(tt_sheet)
        tt_cols = tt_df.columns.tolist()
        date_col = pick_col(["Date","date"])
        morning_col = pick_col(["Morning","morning"])
        evening_col = pick_col(["Evening","evening"])
        slot_col = pick_col(["Slot","slot","Time","time"])
        subj_col = None
        if (date_col is None):
            raise Exception(f"Timetable sheet '{tt_sheet}' found but no Date column.")
        # If timetable uses per-row Subject+Slot format (subject,slot,date), use that:
        # detect subject+slot columns
        subj_col = pick_col(["Subject","subject","course","Course","Course Code","course_code"])
        if subj_col and slot_col:
            # simple rows -> expand directly
            for _, r in tt_df.iterrows():
                date = r[date_col]
                slot = r[slot_col]
                subject = str(r[subj_col]).strip()
                rolls = course_to_rolls.get(subject, []) if course_to_rolls else normalize_list_cell(r.get("Rolls",""))
                schedule_rows.append({"Date": date, "Slot": slot, "Subject": subject, "Rolls": rolls})
        else:
            # likely two columns Morning/Evening that contain subject lists
            for _, r in tt_df.iterrows():
                date = r.get(date_col, "")
                if morning_col:
                    for subj in normalize_list_cell(r.get(morning_col,"")):
                        schedule_rows.append({"Date": date, "Slot": "Morning", "Subject": subj, "Rolls": course_to_rolls.get(subj, [])})
                if evening_col:
                    for subj in normalize_list_cell(r.get(evening_col,"")):
                        schedule_rows.append({"Date": date, "Slot": "Evening", "Subject": subj, "Rolls": course_to_rolls.get(subj, [])})
    else:
        raise Exception("Could not find a timetable sheet automatically (no sheet with Date + Morning/Evening or Subject+Slot).")

    logging.info("Auto-inferred layout. Rooms sheet: %s. Timetable sheet: %s. Course-roll sheet: %s. roll-name sheet: %s",
                 rooms_sheet, tt_sheet, cr_sheet, rn_sheet)
    return rooms_by_block_template, schedule_rows, roll_name_map, f"inferred from sheets: rooms={rooms_sheet}, tt={tt_sheet}"

# ------------------------------- main ------------------------------------
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", "-i", default="input_data_tt.xlsx")
    parser.add_argument("--buffer", "-b", type=int, default=5)
    parser.add_argument("--mode", "-m", choices=["dense","sparse"], default="dense")
    args = parser.parse_args()

    setup_logging()
    logging.info("Started seating arrangement with input=%s buffer=%d mode=%s", args.input, args.buffer, args.mode)

    try:
        rooms_template, schedule_rows, roll_name_map, layout_desc = find_layout_and_build_schedule(args.input, args.buffer, args.mode)
        logging.info("Layout detected: %s", layout_desc)

        if not schedule_rows:
            logging.warning("No schedule rows detected. Nothing to allocate.")
            print("No schedule rows found in the workbook. Check your timetable sheet.")
            sys.exit(1)

        # Group schedule rows by (Date, Slot)
        grouped = defaultdict(list)
        for s in schedule_rows:
            grouped[(s["Date"], s["Slot"])].append({"Subject": s["Subject"], "Rolls": s["Rolls"]})

        all_allocs = {}
        for (date, slot), subj_list in grouped.items():
            rooms_copy = {b: [copy.deepcopy(r) for r in rooms_template[b]] for b in rooms_template}
            allocations = allocate(subj_list, rooms_copy, args.mode)
            all_allocs[(date, slot)] = allocations

        write_outputs(all_allocs, rooms_template, roll_name_map)
        print("Seating arrangement complete — outputs: op_overall_seating_arrangement.xlsx and op_seats_left.xlsx")
        logging.info("Completed seating arrangement successfully.")
    except Exception as e:
        log_exception(e)
        print("An error occurred. See errors.txt and seating.log for details.")
        sys.exit(1)

if __name__ == "__main__":
    main()