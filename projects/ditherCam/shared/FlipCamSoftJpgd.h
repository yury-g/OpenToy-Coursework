/*----------------------------------------------------------------------------/
/ TJpgDec - Tiny JPEG Decompressor include file               (C)ChaN, 2019
/-----------------------------------------------------------------------------/
/ original source is here : http://elm-chan.org/fsw/tjpgd/00index.html
/
/ Modified for LGFX  by lovyan03, 2020
/ add support grayscale jpeg
/ add bayer pattern
/ tweak for 32bit processor
/----------------------------------------------------------------------------*/
#ifndef FLIPCAM_SOFT_JPGD_H
#define FLIPCAM_SOFT_JPGD_H
/*---------------------------------------------------------------------------*/
/* System Configurations */

#define	FC_JD_SZBUF		512	/* Size of stream input buffer */
#define FC_JD_FORMAT		0	/* Output pixel format 0:RGB888 (3 BYTE/pix), 1:RGB565 (1 WORD/pix) */
#define	FC_JD_USE_SCALE	1	/* Use descaling feature for output */
#define FC_JD_TBLCLIP		0	/* Use table for saturation (might be a bit faster but increases 1K bytes of code size) */
#define FC_JD_BAYER		0	/* Use bayer pattern table */

/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

#if __has_include (<stdint.h>)
 #include <stdint.h>
#elif defined(_WIN32)	/* Main development platform */
 typedef unsigned char	uint8_t;
 typedef unsigned short	uint16_t;
 typedef short			int16_t;
 typedef unsigned long	uint32_t;
 typedef long			int32_t;
#endif

/* Error code */
typedef enum {
	FC_JDR_OK = 0,	/* 0: Succeeded */
	FC_JDR_INTR,	/* 1: Interrupted by output function */	
	FC_JDR_INP,	/* 2: Device error or wrong termination of input stream */
	FC_JDR_MEM1,	/* 3: Insufficient memory pool for the image */
	FC_JDR_MEM2,	/* 4: Insufficient stream input buffer */
	FC_JDR_PAR,	/* 5: Parameter error */
	FC_JDR_FMT1,	/* 6: Data format error (may be damaged data) */
	FC_JDR_FMT2,	/* 7: Right format but not supported */
	FC_JDR_FMT3	/* 8: Not supported JPEG standard */
} FC_JRESULT;



/* Rectangular structure */
typedef struct {
	uint32_t left, right, top, bottom;
} FC_JRECT;



/* Decompressor object structure */
typedef struct FC_JDEC FC_JDEC;
struct FC_JDEC {
	uint8_t* dptr;				/* Current data read ptr */
	uint8_t* dpend;				/* data end ptr */
	uint8_t* inbuf;				/* Bit stream input buffer */
	uint8_t dbit;			/* Current bit in the current read byte */
	uint8_t scale;			/* Output scaling ratio */
	uint8_t msx, msy;		/* MCU size in unit of block (width, height) */
	uint8_t qtid[3];		/* Quantization table ID of each component */
	int32_t dcv[3];				/* Previous DC element of each component */
	uint16_t nrst;				/* Restart inverval */
	uint16_t width, height;/* Size of the input image (pixel) */
	uint8_t* huffbits[2][2];	/* Huffman bit distribution tables [id][dcac] */
	uint16_t* huffcode[2][2];	/* Huffman code word tables [id][dcac] */
	uint8_t* huffdata[2][2];	/* Huffman decoded data tables [id][dcac] */
	int32_t* qttbl[4];			/* Dequantizer tables [id] */
	void* workbuf;				/* Working buffer for IDCT and RGB output */
	int16_t* mcubuf;			/* Working buffer for the MCU */
	uint8_t* pool;				/* Pointer to available memory pool */
	uint16_t sz_pool;			/* Size of momory pool (bytes available) */
	uint32_t (*infunc)(void*, uint8_t*, uint32_t);/* Pointer to jpeg stream input function */
	void* device;				/* Pointer to I/O device identifiler for the session */
	uint8_t comps_in_frame;		/* 1=Y(grayscale)  3=YCrCb */
};



/* TJpgDec API functions */
FC_JRESULT fc_jd_prepare (FC_JDEC*, uint32_t(*)(void*,uint8_t*,uint32_t), void*, uint_fast16_t, void*);
FC_JRESULT fc_jd_decomp (FC_JDEC*, uint32_t(*)(void*,void*,FC_JRECT*), uint_fast8_t);


#ifdef __cplusplus
}
#endif

#endif /* _TJPGDEC */
