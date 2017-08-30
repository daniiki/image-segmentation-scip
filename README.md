# Image segmentation using SCIP
This is the code for an advanced practical, completed at the Univerity of Heidelberg in 2017.

# Deepndencies
- [SCIP](http://scip.zib.de), which in turn needs [GMP](https://github.com/daniiki/image-segmentation-scip.git)
- [png++](http://www.nongnu.org/pngpp/)
- [VLFeat](http://www.vlfeat.org/)
- OpenCV

If your're a lazy person and happen to be a Nix user, just use the provided `shell.nix` file.

# Installation
Clone this repository using
```
git clone https://github.com/daniiki/image-segmentation-scip.git
```
Then you need to dowlnload the scipoptsuite from http://scip.zib.de/#download and untar the archive.
Make sure to place the `scipoptsuite-4.x.x` directory inside the root folder of this project.
Finally, you can compile SCIP:
```
cd scipoptsuite-4.0.0
make ZIMPL=false READLINE=false
```
and then our program:
```
cd ..
make ZIMPL=false READLINE=false
```

Finally, you can run it:
```
bin/fopra input.png 20
```
where 20 is the desired number of superpixels.

# Documentation
Have a look at https://daniiki.github.io/image-segmentation-scip.
There are also slides about this project at https://github.com/daniiki/image-segmentation-scip/blob/master/presentation/slides.pdf.
