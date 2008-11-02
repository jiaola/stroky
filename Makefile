CFLAGS	= -g -Wall

.PHONY: sorderlib sorderlibgen clean
sorderlib: sorderlib_5000 sorderlib_10000 sorderlib_all
	@echo "**************************************************"
	@echo "chareters stroke order lib generated:"
	@echo "FILEs: sorderlib_5000 sorderlib_10000 sorderlib_all"
	@echo ""
	@echo "use the C code in DIR modules to query those lib"
	@echo "they are all yours, enjoy!"
	@echo "**************************************************"
sorderlib_all: strokeorder.freq.txt sorderlibgen
	@echo "generating lib with all GBK chareters ......"
	./sorderlib_gen/sorderlibgen strokeorder.freq.txt sorderlib_all

sorderlibgen:
	(cd sorderlib_gen; make)

strokeorder.freq.txt:
	@echo "**************************************************"
	@echo "creating stroke order data file with chareter freq"
	@echo "this may take a few minutes"
	@echo "**************************************************"
	(cd freqtab_gen; awk -f combine.awk data/strokeorder.txt > ../strokeorder.freq.txt)

sorderlib_5000: strokeorder.freq.txt sorderlibgen
	@echo "generating lib with 5000 chareters ......"
	sort -k5nr strokeorder.freq.txt  | head -n 5000 | sort > strokorder.5000.tmp
	./sorderlib_gen/sorderlibgen  strokorder.5000.tmp sorderlib_5000
	rm -f strokorder.5000.tmp

sorderlib_10000: strokeorder.freq.txt sorderlibgen
	@echo "generating lib with 10000 chareters ......"
	sort -k5nr strokeorder.freq.txt  | head -n 10000 | sort > strokorder.10000.tmp
	./sorderlib_gen/sorderlibgen  strokorder.10000.tmp sorderlib_10000
	rm -f strokorder.10000.tmp
	
clean:
	(cd sorderlib_gen; make clean)
	rm -f *.tmp sorderlib_all strokeorder.freq.txt sorderlib_5000 sorderlib_10000