# Erstelle eine Kopie ohne Seitenangaben
with open("Book 1 - The Philosopher's Stone.txt", "r", encoding="ANSI") as txt:
	with open("No Pages.txt", "w", encoding="ANSI") as out:
		while True:
			line = txt.readline()
			if not len(line):
				break
			if line[:6] != "Page |":
				out.write(line)

with open("No Pages.txt", "r", encoding="ANSI") as txt:
	counts = [0 for i in range(26)]

	while True:
		letter = txt.read(1)

		# Stoppe wenn das Ende der Datei erreicht wurde
		if not len(letter):
			break

		print(letter, end="")

		# Unterscheide nicht zwischen großen und kleinen Buchstaben
		letter = letter.lower()
		index = ord(letter)

		# Zähle nur Zeichen von A-Z
		if index > 0x60 and index < 0x7b:
			counts[ord(letter) - 0x61] += 1

	print()
	print(" ".join([str(c) for c in counts]))