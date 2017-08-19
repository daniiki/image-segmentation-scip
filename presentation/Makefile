PDFLATEX = lualatex -synctex=1 -interaction=nonstopmode
LATEXMKOPT = -bibtex -pdf -pdflatex="$(PDFLATEX)"

PDF_TEX = data_image.pdf_tex data_superpixels.pdf_tex data_segments.pdf_tex

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

data_image.pdf_tex: data.svg
	file=$@; inkscape -D -z -i layer1 --file=$+ --export-pdf=$${file%_tex} --export-latex
data_superpixels.pdf_tex: data.svg
	file=$@; inkscape -D -z -i layer2 --file=$+ --export-pdf=$${file%_tex} --export-latex
data_segments.pdf_tex: data.svg
	file=$@; inkscape -D -z -i layer3 --file=$+ --export-pdf=$${file%_tex} --export-latex
