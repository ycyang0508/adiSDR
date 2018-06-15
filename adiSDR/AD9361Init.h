#pragma once

#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <iio.h>


/* helper macros */
#define MHZ(x) ((long long)(x*1000000.0 + .5))
#define GHZ(x) ((long long)(x*1000000000.0 + .5))

#define ASSERT(expr) { \
	if (!(expr)) { \
		(void) fprintf(stderr, "assertion failed (%s:%d)\n", __FILE__, __LINE__); \
		(void) abort(); \
	} \
}

/* RX is input, TX is output */
enum iodev { RX, TX };

/* common RX and TX streaming params */
struct stream_cfg {
	long long bw_hz; // Analog banwidth in Hz
	long long fs_hz; // Baseband sample rate in Hz
	long long lo_hz; // Local oscillator frequency in Hz
	const char* rfport; // Port name
};


struct ad9361Param_t {

struct iio_context *ctx   ;
struct iio_channel *rx0_i ;
struct iio_channel *rx0_q ;
struct iio_channel *tx0_i ;
struct iio_channel *tx0_q ;
struct iio_buffer  *rxbuf ;
struct iio_buffer  *txbuf ;
struct iio_device *tx;
struct iio_device *rx;

    ad9361Param_t()
    {
        ctx   = nullptr;
        rx0_i = nullptr;  
        rx0_q = nullptr;  
        tx0_i = nullptr;  
        tx0_q = nullptr;  
        rxbuf = nullptr;  
        txbuf = nullptr;
        tx    = nullptr;
        rx    = nullptr ;
    }
};

bool isAD9361Stop();
void ad9361_shutdown();
void ad9361Init(ad9361Param_t *pAD9361ParamIn);

