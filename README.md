# SQZ - Low complexity, scalable lossless and lossy image compression library

[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

Single-file library for C/C++

For documentation, please refer to the [source code](https://github.com/MarcioPais/SQZ/blob/master/sqz.h)

## Description

The SQZ codec provides a low-complexity solution for seamless scalability from lossy to fully lossless image compression, allowing for a single compressed image representation to be decoded at the quality level desired by simply truncating the bitstream at any size, with byte-level granularity. 

## Overview

SQZ is not meant to beat the state-of-the-art extremelly complex image codecs, it was designed with simplicity and practicality in mind. The ideas and methods used were chosen for their suitability to meet the design requirements, and are largely based on previous work.

At its core, SQZ is based on a run-length encoding scheme for discrete wavelet transform tree bitplanes and uses no entropy coding stage.

- The internal pixel data representation is stored in either 8bpp grayscale mode or in one of 3 possible color spaces, one ensuring perfect reconstruction for lossless compression (YCoCg-R), and the others (Oklab, logl1) vying for better subjective perceptual quality for lossy compression.

- The 2D DWT subbands are traversed for run-length encoding in one of 4 possible orders based on different space-filling curves, each providing trade-offs in terms of complexity and locality-preservation.

- The linearized distance between significant subband coefficients at each bitplane is encoded using the Wavelet Difference Reduction (WDR) method.

- The scheduling of the priorities for encoding the subbands and bitplanes is optimized for producing a sharp image at very large compression rates, so that a downscaled low-quality image preview may be obtained on a very strict allocation budget.

All of these components put together allow SQZ to achieve interesting results, far better than could be expected from such a simple method.

Quality assessment for lossy image compression is always subjective, so you are encouraged to always **perform your own evaluation** on a dataset representative of your specific needs, but for the sake of completeness, let us examine some results of a comparative objective study against some state-of-the-art image codecs (JPEG 2000 and JPEG-XL) focusing simply on PSNR, on a small dataset consisting mostly of uncompressed photographic images.

### SQZ vs JPEG 2000
![SQZ vs JPEG 2000](/assets/chart_psnr_jp2.png "PSNR comparison between SQZ and JPEG 2000")

The venerable JPEG 2000 codec is still pretty much the best when it comes to PSNR (consistenly topping even the newer JPEG-XL) and the lack of an entropy coding stage limits SQZ, which trails by about 0.4db at the low end of the PSNR scale and up to about 0.6db at the high end.

However, subjectively, PSNR doesn't always correlate well with perceptual quality (which explains why JPEG-XL is the superior codec of the two), and the Oklab color space provides SQZ with quality benefits that are hard to quantify, especially at high compression ratios.

### SQZ vs JPEG-XL
![SQZ vs JPEG-XL](/assets/chart_psnr_jxl.png "PSNR comparison between SQZ and JPEG-XL")

Compared to a modern next-gen state-of-the-art codec like JPEG-XL, which also uses a perceptual color space (XYB), provides a better view of what SQZ can offer.

SQZ provides very competitive image quality on a very tight byte budget, and even though it eventually gets outclassed, it does so providing best-in-class scalability at much lower complexity and easy integration. The same SQZ image can be decompressed at each of these quality levels by simply choosing how many bytes to use for decompression, there is no need for reencoding.

### Testing methodology
JPEG 2000 and SQZ can compress to an arbitrarily chosen file size, so a direct comparison at each *bpp* rate was made. For JPEG-XL, each image was encoded with the listed *distance* parameter, at *effort 9*, and SQZ decompressed an image matching the size of the resulting *.jxl* image.

