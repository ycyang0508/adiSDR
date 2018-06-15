#include "stdafx.h"
#include "AD9361Init.h"


/* static scratch mem for strings */
static char tmpstr[64];


static ad9361Param_t *pAD9361Param;

static bool stop;

/* cleanup and exit */
static void shutdown()
{
	printf("* Destroying buffers\n");
	if (pAD9361Param->rxbuf) { iio_buffer_destroy(pAD9361Param->rxbuf); }
	if (pAD9361Param->txbuf) { iio_buffer_destroy(pAD9361Param->txbuf); }

	printf("* Disabling streaming channels\n");
	if (pAD9361Param->rx0_i) { iio_channel_disable(pAD9361Param->rx0_i); }
	if (pAD9361Param->rx0_q) { iio_channel_disable(pAD9361Param->rx0_q); }
	if (pAD9361Param->tx0_i) { iio_channel_disable(pAD9361Param->tx0_i); }
	if (pAD9361Param->tx0_q) { iio_channel_disable(pAD9361Param->tx0_q); }

	printf("* Destroying context\n");
	if (pAD9361Param->ctx) { iio_context_destroy(pAD9361Param->ctx); }
	exit(0);
}

static void handle_sig(int sig)
{
	printf("Waiting for process to finish...\n");
	stop = true;
}


/* check return value of attr_write function */
static void errchk(int v, const char* what) {
	if (v < 0) { fprintf(stderr, "Error %d writing to channel \"%s\"\nvalue may not be supported.\n", v, what); shutdown(); }
}

/* write attribute: long long int */
static void wr_ch_lli(struct iio_channel *chn, const char* what, long long val)
{
	errchk(iio_channel_attr_write_longlong(chn, what, val), what);
}

/* write attribute: string */
static void wr_ch_str(struct iio_channel *chn, const char* what, const char* str)
{
	errchk(iio_channel_attr_write(chn, what, str), what);
}

/* helper function generating channel names */
static char* get_ch_name(const char* type, int id)
{
	snprintf(tmpstr, sizeof(tmpstr), "%s%d", type, id);
	return tmpstr;
}

/* returns ad9361 phy device */
static struct iio_device* get_ad9361_phy(struct iio_context *ctx)
{
	struct iio_device *dev = iio_context_find_device(ctx, "ad9361-phy");
	ASSERT(dev && "No ad9361-phy found");
	return dev;
}

/* finds AD9361 streaming IIO devices */
static bool get_ad9361_stream_dev(struct iio_context *ctx, enum iodev d, struct iio_device **dev)
{
	switch (d) {
	case TX: *dev = iio_context_find_device(ctx, "cf-ad9361-dds-core-lpc"); return *dev != NULL;
	case RX: *dev = iio_context_find_device(ctx, "cf-ad9361-lpc");  return *dev != NULL;
	default: ASSERT(0); return false;
	}
}

/* finds AD9361 streaming IIO channels */
static bool get_ad9361_stream_ch(struct iio_context *ctx, enum iodev d, struct iio_device *dev, int chid, struct iio_channel **chn)
{
	*chn = iio_device_find_channel(dev, get_ch_name("voltage", chid), d == TX);
	if (!*chn)
		*chn = iio_device_find_channel(dev, get_ch_name("altvoltage", chid), d == TX);
	return *chn != NULL;
}

/* finds AD9361 phy IIO configuration channel with id chid */
static bool get_phy_chan(struct iio_context *ctx, enum iodev d, int chid, struct iio_channel **chn)
{
	switch (d) {
	case RX: *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("voltage", chid), false); return *chn != NULL;
	case TX: *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("voltage", chid), true);  return *chn != NULL;
	default: ASSERT(0); return false;
	}
}

/* finds AD9361 local oscillator IIO configuration channels */
static bool get_lo_chan(struct iio_context *ctx, enum iodev d, struct iio_channel **chn)
{
	switch (d) {
		// LO chan is always output, i.e. true
	case RX: *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("altvoltage", 0), true); return *chn != NULL;
	case TX: *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("altvoltage", 1), true); return *chn != NULL;
	default: ASSERT(0); return false;
	}
}

/* applies streaming configuration through IIO */
bool cfg_ad9361_streaming_ch(struct iio_context *ctx, struct stream_cfg *cfg, enum iodev type, int chid)
{
	struct iio_channel *chn = NULL;

	// Configure phy and lo channels
	printf("* Acquiring AD9361 phy channel %d\n", chid);
	if (!get_phy_chan(ctx, type, chid, &chn)) { return false; }
	wr_ch_str(chn, "rf_port_select", cfg->rfport);
	wr_ch_lli(chn, "rf_bandwidth", cfg->bw_hz);
	wr_ch_lli(chn, "sampling_frequency", cfg->fs_hz);

	// Configure LO channel
	printf("* Acquiring AD9361 %s lo channel\n", type == TX ? "TX" : "RX");
	if (!get_lo_chan(ctx, type, &chn)) { return false; }
	wr_ch_lli(chn, "frequency", cfg->lo_hz);
	return true;
}

/* cleanup and exit */
void ad9361_shutdown()
{
	printf("* Destroying buffers\n");
	if (pAD9361Param->rxbuf) { iio_buffer_destroy(pAD9361Param->rxbuf); }
	if (pAD9361Param->txbuf) { iio_buffer_destroy(pAD9361Param->txbuf); }

	printf("* Disabling streaming channels\n");
	if (pAD9361Param->rx0_i) { iio_channel_disable(pAD9361Param->rx0_i); }
	if (pAD9361Param->rx0_q) { iio_channel_disable(pAD9361Param->rx0_q); }
	if (pAD9361Param->tx0_i) { iio_channel_disable(pAD9361Param->tx0_i); }
	if (pAD9361Param->tx0_q) { iio_channel_disable(pAD9361Param->tx0_q); }

	printf("* Destroying context\n");
	if (pAD9361Param->ctx) { iio_context_destroy(pAD9361Param->ctx); }
	exit(0);
}

bool isAD9361Stop()
{

    return stop;

}
void ad9361Init(ad9361Param_t *pAD9361ParamIn)
{

    pAD9361Param = pAD9361ParamIn;
	// RX and TX sample counters

	// Stream configurations
	struct stream_cfg rxcfg;
	struct stream_cfg txcfg;

	// Listen to ctrl+c and ASSERT
	signal(SIGINT, handle_sig);

	// RX stream config
	rxcfg.bw_hz = MHZ(2);   // 2 MHz rf bandwidth
	rxcfg.fs_hz = MHZ(2.5);   // 2.5 MS/s rx sample rate
	rxcfg.lo_hz = GHZ(2.5); // 2.5 GHz rf frequency
	rxcfg.rfport = "A_BALANCED"; // port A (select for rf freq.)

								 // TX stream config
	txcfg.bw_hz = MHZ(1.5); // 1.5 MHz rf bandwidth
	txcfg.fs_hz = MHZ(2.5);   // 2.5 MS/s tx sample rate
	txcfg.lo_hz = GHZ(2.5); // 2.5 GHz rf frequency
	txcfg.rfport = "A"; // port A (select for rf freq.)

	printf("* Acquiring IIO context\n");
	//ASSERT((ctx = iio_create_network_context("192.168.2.1")) && "No context");
    ASSERT((pAD9361Param->ctx = iio_create_context_from_uri("usb:3.2.5")) && "No context");
	//ASSERT((ctx = iio_create_default_context()) && "No context");
	ASSERT(iio_context_get_devices_count(pAD9361Param->ctx) > 0 && "No devices");

	printf("* Acquiring AD9361 streaming devices\n");
	ASSERT(get_ad9361_stream_dev(pAD9361Param->ctx, TX, &pAD9361Param->tx) && "No tx dev found");
	ASSERT(get_ad9361_stream_dev(pAD9361Param->ctx, RX, &pAD9361Param->rx) && "No rx dev found");

	printf("* Configuring AD9361 for streaming\n");
	ASSERT(cfg_ad9361_streaming_ch(pAD9361Param->ctx, &rxcfg, RX, 0) && "RX port 0 not found");
	ASSERT(cfg_ad9361_streaming_ch(pAD9361Param->ctx, &txcfg, TX, 0) && "TX port 0 not found");

	printf("* Initializing AD9361 IIO streaming channels\n");
	ASSERT(get_ad9361_stream_ch(pAD9361Param->ctx, RX, pAD9361Param->rx, 0, &pAD9361Param->rx0_i) && "RX chan i not found");
	ASSERT(get_ad9361_stream_ch(pAD9361Param->ctx, RX, pAD9361Param->rx, 1, &pAD9361Param->rx0_q) && "RX chan q not found");
	ASSERT(get_ad9361_stream_ch(pAD9361Param->ctx, TX, pAD9361Param->tx, 0, &pAD9361Param->tx0_i) && "TX chan i not found");
	ASSERT(get_ad9361_stream_ch(pAD9361Param->ctx, TX, pAD9361Param->tx, 1, &pAD9361Param->tx0_q) && "TX chan q not found");

	printf("* Enabling IIO streaming channels\n");
	iio_channel_enable(pAD9361Param->rx0_i);
	iio_channel_enable(pAD9361Param->rx0_q);
	iio_channel_enable(pAD9361Param->tx0_i);
	iio_channel_enable(pAD9361Param->tx0_q);

}