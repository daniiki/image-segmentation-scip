PDFLATEX = lualatex -synctex=1 -interaction=nonstopmode -shell-escape
LATEXMKOPT = -bibtex -pdf -pdflatex="$(PDFLATEX)"

PDF_TEX = $(shell for file in *.svg; do echo $${file%.svg}.pdf_tex; done)

.PHONY: slides.pdf report.pdf all

all: $(PDF_TEX) slides.pdf

clean:
	latexmk -c $(LATEXMKOPT); \
	rm -f $(PDF_TEX); \
	rm -f traffic.csv; \
	for e in $(PDF_TEX); do rm -f $${e%_tex}; done

cleanall: clean
	latexmk -C $(LATEXMKOPT)

slides.pdf: slides.tex
	latexmk $(LATEXMKOPT) $<

%.pdf_tex: %.svg
	file=$@; inkscape -D -z --file=$+ --export-pdf=$${file%_tex} --export-latex
