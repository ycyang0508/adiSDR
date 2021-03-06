// adiSDR.cpp: 定義主控台應用程式的進入點。
//

#include "stdafx.h"
#include <stdbool.h>
#include <stdio.h>
#include "AD9361Init.h"

/* simple configuration and streaming */
int main(int argc, char **argv)
{
	// Streaming devices
    ad9361Param_t ad9361Param;
    size_t nrx = 0;
	size_t ntx = 0;

    ad9361Init(&ad9361Param);

	printf("* Creating non-cyclic IIO buffers with 1 MiS\n");
	ad9361Param.rxbuf = iio_device_create_buffer(ad9361Param.rx, 1024 * 1024, false);
	if (!ad9361Param.rxbuf) {
		perror("Could not create RX buffer");
		ad9361_shutdown();
	}
	ad9361Param.txbuf = iio_device_create_buffer(ad9361Param.tx, 1024 * 1024, false);
	if (!ad9361Param.txbuf) {
		perror("Could not create TX buffer");
		ad9361_shutdown();
	}

	printf("* Starting IO streaming (press CTRL+C to cancel)\n");
	while (!isAD9361Stop())
	{
		ssize_t nbytes_rx, nbytes_tx;
		char *p_dat, *p_end;
		ptrdiff_t p_inc;

		// Schedule TX buffer
		nbytes_tx = iio_buffer_push(ad9361Param.txbuf);
		if (nbytes_tx < 0) { printf("Error pushing buf %d\n", (int)nbytes_tx); ad9361_shutdown(); }

		// Refill RX buffer
		nbytes_rx = iio_buffer_refill(ad9361Param.rxbuf);
		if (nbytes_rx < 0) { printf("Error refilling buf %d\n", (int)nbytes_rx); ad9361_shutdown(); }

		// READ: Get pointers to RX buf and read IQ from RX buf port 0
		p_inc = iio_buffer_step(ad9361Param.rxbuf);
		p_end = (char *)iio_buffer_end(ad9361Param.rxbuf);
		for (p_dat = (char *)iio_buffer_first(ad9361Param.rxbuf, ad9361Param.rx0_i); p_dat < p_end; p_dat += p_inc) {
			// Example: swap I and Q
			const int16_t i = ((int16_t*)p_dat)[0]; // Real (I)
			const int16_t q = ((int16_t*)p_dat)[1]; // Imag (Q)
			((int16_t*)p_dat)[0] = q;
			((int16_t*)p_dat)[1] = i;
		}

		// WRITE: Get pointers to TX buf and write IQ to TX buf port 0
		p_inc = iio_buffer_step(ad9361Param.txbuf);
		p_end = (char *)iio_buffer_end(ad9361Param.txbuf);
		for (p_dat = (char *)iio_buffer_first(ad9361Param.txbuf, ad9361Param.tx0_i); p_dat < p_end; p_dat += p_inc) {
			// Example: fill with zeros
			// 12-bit sample needs to be MSB alligned so shift by 4
			// https://wiki.analog.com/resources/eval/user-guides/ad-fmcomms2-ebz/software/basic_iq_datafiles#binary_format
			((int16_t*)p_dat)[0] = 0 << 4; // Real (I)
			((int16_t*)p_dat)[1] = 0 << 4; // Imag (Q)
		}

		// Sample counter increment and status output
		nrx += nbytes_rx / iio_device_get_sample_size(ad9361Param.rx);
		ntx += nbytes_tx / iio_device_get_sample_size(ad9361Param.tx);
		printf("\tRX %8.2f MSmp, TX %8.2f MSmp\n", nrx / 1e6, ntx / 1e6);

	}

	ad9361_shutdown();


	return 0;
}

