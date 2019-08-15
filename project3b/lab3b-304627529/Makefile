run:
	python lab3b.py

clean:
	rm lab3b_check.txt

check:
	sort lab3b_check.txt | diff - solution.txt

dist:
	tar -czvf lab3b-304627529.tar.gz lab3b.py Makefile README
