#default builds n executable named lab0 from lab0.c
lab0: lab0.c
	gcc -g -o lab0 lab0.c

check:
	if [ -f "lab0" ]; then \
		echo "lab0 exists."; \
	if cmp -s a.txt b.txt; then \
		echo "Test 1: The files match"; \
	else \
		echo "Test 1: The files don't match"; \
	fi; \
	echo "I'm changing b's contents." > b.txt; \
	./lab0 --input=a.txt --output=b.txt; \
        if cmp -s a.txt b.txt; then \
                echo "Test 2: The files match"; \
        else \
                echo "Test 2: The files don't match"; \
        fi; \
	touch c.txt; \
	./lab0 --input=c.txt --output=b.txt; \
        if cmp -s c.txt b.txt; then \
                echo "Test 3: The files match"; \
        else \
                echo "Test 3: The files don't match"; \
        fi; \
	if ./lab0 --input=d.txt --output=b.txt > /dev/null 2>&1 || [ $$? -eq 1 ]; then \
		echo "Test 4: Success."; \
	else \
		echo "Test 4: Failure."; \
	fi; \
        if ./lab0 --input=a.txt --output=b.txt --segfault --catch > /dev/null 2>&1 || [ $$? -eq 3 ]; then \
                echo "Test 5: Success."; \
        else \
                echo "Test 5: Failure."; \
        fi; \
        if ./lab0 --segfault --catch > /dev/null 2>&1 || [ $$? -eq 3 ]; then \
                echo "Test 6: Success."; \
        else \
                echo "Test 6: Failure."; \
        fi; \
	chmod 000 b.txt; \
	if ./lab0 --input=a.txt --output=b.txt 2>/dev/null || [ $$? -eq 2 ]; then \
		echo "Test 7: Success."; \
	else \
		echo "Test 7: Failure."; \
	fi; \
	chmod 777 b.txt; \
	./lab0 --catch --input=a.txt --output=b.txt; \
        if cmp -s a.txt b.txt; then \
                echo "Test 8: Nothing unusual happened"; \
        else \
                echo "Test 8: Catch has a problem."; \
        fi; \
	< /dev/urandom tr -dc "[:alnum:]" | head -c5000 > d.txt; \
	./lab0 --input=d.txt --output=b.txt; \
	if cmp -s d.txt b.txt; then \
		echo "Test 9: Nothing unusual happened"; \
	else \
		echo "Test 9: There was a problem with a file 5000 bytes big."; \
	fi; \
	rm a.txt; \
	rm b.txt; \
	rm c.txt; \
	rm d.txt; \
	else \
		echo "Test failed: lab0 doesn't exist yet."; \
	fi;

clean:
	rm lab0

dist:
	tar -czvf lab0-304627529.tar.gz lab0.c pic1.png pic2.png Makefile README
