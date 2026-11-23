#!/usr/bin/env python3
"""Convert scrollmapper/bible_databases DutSVV.json (Statenvertaling) into data/bible_svv.json.

Usage: make_bible.py DutSVV.json data/bible_svv.json
Keeps the 66 canonical books (drops the apocrypha), adds Dutch names and search aliases.
Aliases are normalised: lowercase, no diacritics, no spaces/dots.
"""
import json, sys, unicodedata

# (source index, Dutch name, reference name, aliases)
BOOKS = [
 (0,"Genesis","Genesis","gen"),(1,"Exodus","Exodus","ex exod"),(2,"Leviticus","Leviticus","lev"),
 (3,"Numeri","Numeri","num numbers"),(4,"Deuteronomium","Deuteronomium","deut deu"),(5,"Jozua","Jozua","joz"),
 (6,"Richteren","Richteren","re richt"),(7,"Ruth","Ruth","rut"),
 (8,"1 Samuël","1 Samuël","1sam"),(9,"2 Samuël","2 Samuël","2sam"),
 (10,"1 Koningen","1 Koningen","1kon"),(11,"2 Koningen","2 Koningen","2kon"),
 (12,"1 Kronieken","1 Kronieken","1kron"),(13,"2 Kronieken","2 Kronieken","2kron"),
 (14,"Ezra","Ezra",""),(15,"Nehemia","Nehemia","neh"),(16,"Esther","Esther","est"),(17,"Job","Job",""),
 (18,"Psalmen","Psalm","ps psalm"),(19,"Spreuken","Spreuken","spr"),(20,"Prediker","Prediker","pred"),
 (21,"Hooglied","Hooglied","hoogl hl"),(22,"Jesaja","Jesaja","jes"),(23,"Jeremia","Jeremia","jer"),
 (24,"Klaagliederen","Klaagliederen","klaagl kl"),(25,"Ezechiël","Ezechiël","ez ezech"),
 (26,"Daniël","Daniël","dan"),(27,"Hosea","Hosea","hos"),(28,"Joël","Joël",""),(29,"Amos","Amos","am"),
 (30,"Obadja","Obadja","ob obad"),(31,"Jona","Jona",""),(32,"Micha","Micha","mi mich"),
 (33,"Nahum","Nahum","nah"),(34,"Habakuk","Habakuk","hab"),(35,"Zefanja","Zefanja","zef"),
 (36,"Haggaï","Haggaï","hag"),(37,"Zacharia","Zacharia","zach"),(38,"Maleachi","Maleachi","mal"),
 (53,"Mattheüs","Mattheüs","mat matt matth"),(54,"Markus","Markus","mar mrk mc"),
 (55,"Lukas","Lukas","luc luk lc"),(56,"Johannes","Johannes","joh"),(57,"Handelingen","Handelingen","hand"),
 (58,"Romeinen","Romeinen","rom"),(59,"1 Korinthe","1 Korinthe","1kor"),(60,"2 Korinthe","2 Korinthe","2kor"),
 (61,"Galaten","Galaten","gal"),(62,"Efeze","Efeze","ef"),(63,"Filippenzen","Filippenzen","fil filip"),
 (64,"Kolossenzen","Kolossenzen","kol"),(65,"1 Thessalonicenzen","1 Thessalonicenzen","1thess 1tess"),
 (66,"2 Thessalonicenzen","2 Thessalonicenzen","2thess 2tess"),(67,"1 Timotheüs","1 Timotheüs","1tim"),
 (68,"2 Timotheüs","2 Timotheüs","2tim"),(69,"Titus","Titus","tit"),(70,"Filemon","Filemon","filem"),
 (71,"Hebreeën","Hebreeën","heb hebr"),(72,"Jakobus","Jakobus","jak"),(73,"1 Petrus","1 Petrus","1pet 1petr"),
 (74,"2 Petrus","2 Petrus","2pet 2petr"),(75,"1 Johannes","1 Johannes","1joh"),
 (76,"2 Johannes","2 Johannes","2joh"),(77,"3 Johannes","3 Johannes","3joh"),
 (78,"Judas","Judas","jud"),(79,"Openbaring","Openbaring","openb apok"),
]

def norm(s):
    s = unicodedata.normalize("NFD", s.lower())
    s = "".join(c for c in s if unicodedata.category(c) != "Mn")
    return s.replace(" ", "").replace(".", "").replace("'", "")

src = json.load(open(sys.argv[1], encoding="utf-8"))
out = {"translation": "Statenvertaling", "books": []}
for idx, name, ref, extra in BOOKS:
    b = src["books"][idx]
    chapters = []
    for ci, c in enumerate(b["chapters"], 1):
        assert c["chapter"] == ci and [v["verse"] for v in c["verses"]] == list(range(1, len(c["verses"]) + 1))
        chapters.append([v["text"].strip() for v in c["verses"]])
    aliases = sorted({norm(name), norm(ref), *[a for a in extra.split() if a]})
    out["books"].append({"name": name, "ref": ref, "aliases": aliases, "chapters": chapters})
json.dump(out, open(sys.argv[2], "w", encoding="utf-8"), ensure_ascii=False, separators=(",", ":"))
print(len(out["books"]), "books,", sum(len(c) for b in out["books"] for c in b["chapters"]), "verses")
