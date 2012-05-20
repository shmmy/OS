all: oslabb03-REPORT-3.pdf
oslabb03-REPORT-3.pdf: oslabb03-REPORT-3.tex title/* files/*
		xelatex -shell-escape oslabb03-REPORT-3.tex
spell:
		aspell -l el_gr --ignore-case=true -c oslabb03-REPORT-3.tex
