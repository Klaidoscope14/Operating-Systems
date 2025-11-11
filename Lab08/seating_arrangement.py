import argparse, logging, traceback, sys, copy
from collections import defaultdict, deque
import pandas as pd

def setup_logging():
    logging.basicConfig(filename="seating.log", level=logging.DEBUG, format="%(asctime)s %(levelname)s: %(message)s")

def log_exception(e):
    with open("errors.txt", "a", encoding="utf-8") as f:
        f.write(f"{str(e)}\n{traceback.format_exc()}\n\n")
    logging.exception(e)

def norm_list(cell):
    if pd.isna(cell): return []
    s = str(cell)
    for sep in [";", "\n", ","]:
        if sep in s:
            return [x.strip() for x in s.split(sep) if x.strip()]
    return [s.strip()] if s.strip() else []

def build_rooms(df, cols, buf):
    d = defaultdict(list)
    tot = 0
    for _, r in df.iterrows():
        try:
            room, cap = str(r[cols["room"]]).strip(), int(r[cols["capacity"]])
            blk = str(r.get(cols.get("block", ""), "")).strip()
        except: continue
        eff = max(0, cap - buf)
        d[blk].append({"block": blk, "room": room, "capacity": cap, "eff": eff, "rem": eff, "sub": []})
        tot += eff
    for b in d:
        d[b].sort(key=lambda x: (0, int("".join(c for c in x["room"] if c.isdigit()) or 0)))
    return d, tot

def allocate(subs, rooms, mode):
    res = []
    lim = lambda e: e if mode == "dense" else e // 2
    subs.sort(key=lambda s: len(s["Rolls"]), reverse=True)
    for s in subs:
        rolls, need, recs = deque(s["Rolls"]), len(s["Rolls"]), []
        blks = sorted([(b, sum(r["rem"] for r in rooms[b])) for b in rooms], key=lambda x: x[1], reverse=True)
        for b, _ in blks:
            if sum(min(lim(r["eff"]), r["rem"]) for r in rooms[b]) >= need:
                for r in rooms[b]:
                    can = min(lim(r["eff"]), r["rem"], need)
                    if can <= 0: continue
                    a = [rolls.popleft() for _ in range(min(can, len(rolls)))]
                    r["rem"] -= len(a)
                    r["sub"].append({"subject": s["Subject"], "rolls": a})
                    recs.append({"subject": s["Subject"], "block": b, "room": r["room"], "rolls": a, "alloc": len(a)})
                    need = len(rolls)
                    if not need: break
                break
        if need:
            allr = [r for b in rooms for r in rooms[b]]
            for r in allr:
                can = min(lim(r["eff"]), r["rem"], need)
                if can <= 0: continue
                a = [rolls.popleft() for _ in range(min(can, len(rolls)))]
                r["rem"] -= len(a)
                r["sub"].append({"subject": s["Subject"], "rolls": a})
                recs.append({"subject": s["Subject"], "block": r["block"], "room": r["room"], "rolls": a, "alloc": len(a)})
                need = len(rolls)
                if not need: break
        if rolls:
            recs.append({"subject": s["Subject"], "block": "UNALLOCATED", "room": "", "rolls": list(rolls), "alloc": len(rolls)})
        res.extend(recs)
    return res

def write_out(allc, rooms, rollmap):
    rows = []
    for (d, s), a in allc.items():
        for x in a:
            names = [f"{r}:{rollmap.get(r, 'Unknown Name')}" for r in x.get("rolls", [])]
            rows.append({"Date": d, "Slot": s, "Subject": x.get("subject",""), "Block": x.get("block",""),
                         "Room": x.get("room",""), "AssignedRolls": ";".join(x.get("rolls", [])),
                         "AssignedRollsWithNames": ";".join(names), "SeatsAllocated": x.get("alloc", len(x.get("rolls", [])))})
    pd.DataFrame(rows).to_excel("op_overall_seating_arrangement.xlsx", index=False)
    r2 = [{"Block": b, "Room": r["room"], "Capacity": r["capacity"], "EffectiveCapacity": r["eff"], "RemainingSeats": r["rem"]}
          for b in rooms for r in rooms[b]]
    pd.DataFrame(r2).to_excel("op_seats_left.xlsx", index=False)

def detect_layout(path, buf):
    xl, sheets = pd.ExcelFile(path), pd.ExcelFile(path).sheet_names
    if "in_room_capacity" in sheets:
        r, cr, tt, rn = xl.parse("in_room_capacity"), xl.parse("in_course_roll_mapping"), xl.parse("in_timetable"), xl.parse("in_roll_name_mapping")
        c2r = defaultdict(list)
        for _, x in cr.iterrows():
            try: c2r[str(x["course_code"]).strip()].append(str(x["rollno"]).strip())
            except: pass
        rollmap = {str(x["Roll"]).strip(): str(x["Name"]).strip() for _, x in rn.iterrows() if str(x["Roll"]).strip()}
        sched = []
        for _, x in tt.iterrows():
            d = x.get("Date", "")
            for s, sl in [("Morning","Morning"),("Evening","Evening")]:
                for sub in norm_list(x.get(s, "")):
                    if sub and sub.upper() != "NO EXAM":
                        sched.append({"Date": d, "Slot": sl, "Subject": sub, "Rolls": c2r.get(sub, [])})
        rooms, tot = build_rooms(r, {"room": "Room No.", "capacity": "Exam Capacity", "block": "Block"}, buf)
        logging.info("Using in_* layout, total capacity %d", tot)
        return rooms, sched, rollmap
    raise Exception("Workbook format not recognized.")

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--input","-i",default="input_data_tt.xlsx")
    p.add_argument("--buffer","-b",type=int,default=5)
    p.add_argument("--mode","-m",choices=["dense","sparse"],default="dense")
    a = p.parse_args()
    setup_logging()
    try:
        rooms_t, sched, rollmap = detect_layout(a.input, a.buffer)
        grp = defaultdict(list)
        for s in sched: grp[(s["Date"], s["Slot"])].append({"Subject": s["Subject"], "Rolls": s["Rolls"]})
        allc = {}
        for (d, s), sub in grp.items():
            rc = {b:[copy.deepcopy(r) for r in rooms_t[b]] for b in rooms_t}
            allc[(d, s)] = allocate(sub, rc, a.mode)
        write_out(allc, rooms_t, rollmap)
        print("✅ Seating arrangement complete. Check generated Excel files.")
    except Exception as e:
        log_exception(e)
        print("❌ Error occurred. See errors.txt and seating.log for details.")
        sys.exit(1)

if __name__ == "__main__":
    main()