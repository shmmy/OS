all: oslabb03-REPORT-2.pdf
oslabb03-REPORT-2.pdf: oslabb03-REPORT-2.tex title/* files/*
		xelatex -shell-escape oslabb03-REPORT-2.tex
spell:
		aspell -l el_gr --ignore-case=true -c oslabb03-REPORT-2.tex
