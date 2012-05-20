all: oslabb03-REPORT-1.pdf
oslabb03-REPORT-1.pdf: oslabb03-REPORT-1.tex title/* 
		xelatex -shell-escape oslabb03-REPORT-1.tex
spell:
		aspell -l el_gr --ignore-case=true -c oslabb03-REPORT-1.tex
