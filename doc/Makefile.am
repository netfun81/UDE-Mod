TEXSOURCES= \
	l2h.conf \
	manual.tex 
TEXOUTPUT= \
	manual.pdf

DIR_CONTENTS= ${TEXSOURCES} ${TEXOUTPUT} README
udedocdir= ${prefix}/doc/${PACKAGE}-${VERSION}
udedoc_DATA= ${DIR_CONTENTS}
EXTRA_DIST= ${DIR_CONTENTS}

manual.pdf: manual.tex
	pdflatex manual.tex
	pdflatex manual.tex
	pdflatex manual.tex

clean-local:
	rm -rf *.log *.aux *.toc *.pdf
