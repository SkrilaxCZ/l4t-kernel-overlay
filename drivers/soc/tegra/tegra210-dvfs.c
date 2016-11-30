// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2014-2020, NVIDIA CORPORATION.  All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/of_address.h>

#include <soc/tegra/cvb.h>
#include <soc/tegra/tegra-dvfs.h>
#include <soc/tegra/fuse.h>
#include <soc/tegra/tegra_emc.h>

#define KHZ		1000
#define MHZ		1000000
#define VDD_SAFE_STEP	100

static bool tegra_dvfs_cpu_disabled;
static bool tegra_dvfs_core_disabled;
static int cpu_millivolts[MAX_DVFS_FREQS];
static int cpu_dfll_millivolts[MAX_DVFS_FREQS];
static int cpu_lp_millivolts[MAX_DVFS_FREQS];

static const struct dvfs_therm_limits
tegra210_core_therm_floors[MAX_THERMAL_LIMITS] = {
	{15, 950},
	{0, 0},
};

static const struct dvfs_therm_limits
tegra210_core_therm_caps[MAX_THERMAL_LIMITS] = {
	{86, 1132},
	{0, 0},
};

static struct dvfs_rail tegra210_dvfs_rail_vdd_cpu = {
	.reg_id = "vdd-cpu",
	.max_millivolts = 1300,
	.min_millivolts = 800,
	.step = VDD_SAFE_STEP,
	.step_up = 1300,
	.jmp_to_zero = true,
	.dfll_mode = true,
	.alignment = {
		.step_uv = 6250, /* 6.25mV */
	},
	.stats = {
		.bin_uv = 6250, /* 6.25mV */
	},
	.is_ready = false,
};

static struct dvfs_rail tegra210_dvfs_rail_vdd_core = {
	.reg_id = "vdd-core",
	.max_millivolts = 1300,
	.step = VDD_SAFE_STEP,
	.step_up = 1300,
	.alignment = {
		.step_uv = 12500, /* 12.5mV */
	},
	.stats = {
		.bin_uv = 12500, /* 12.5mV */
	},
	.therm_floors = tegra210_core_therm_floors,
	.therm_caps = tegra210_core_therm_caps,
	.is_ready = false,
};

static struct dvfs_rail tegra210_dvfs_rail_vdd_gpu = {
	.reg_id = "vdd-gpu",
	.max_millivolts = 1300,
	.step = VDD_SAFE_STEP,
	.step_up = 1300,
	.alignment = {
		.step_uv = 6250, /* 6.25mV */
	},
	.stats = {
		.bin_uv = 6250, /* 6.25mV */
	},
	.in_band_pm = true,
};

static struct dvfs_rail *tegra210_dvfs_rails[] = {
	&tegra210_dvfs_rail_vdd_cpu,
	&tegra210_dvfs_rail_vdd_core,
	&tegra210_dvfs_rail_vdd_gpu,
};

static struct dvfs cpu_dvfs = {
	.clk_name	= "cclk_g",
	.millivolts	= cpu_millivolts,
	.dfll_millivolts = cpu_dfll_millivolts,
	.auto_dvfs	= true,
	.dvfs_rail	= &tegra210_dvfs_rail_vdd_cpu,
};

/* CPU DVFS tables */
static unsigned long cpu_max_freq[] = {
/* speedo_id	0	 1	  2	   3	    4	     5 */
		1912500, 1912500, 2218500, 1785000, 1632000, 1912500,
/* speedo_id	6	 7	  8	   9	    10 */
		2014500, 1734000, 1683000, 1555500, 1504500
};

#define CPU_FV_TABLE		 \
	.fv_table = {		 \
		{51000,   950},	 \
		{102000,  950},	 \
		{204000,  950},	 \
		{306000,  950},	 \
		{408000,  950},	 \
		{510000,  950},	 \
		{612000,  950},	 \
		{714000,  950},	 \
		{816000,  950},	 \
		{918000,  950},	 \
		{1020000, 950},	 \
		{1122000, 950},	 \
		{1224000, 950},	 \
		{1326000, 993},	 \
		{1428000, 1043}, \
		{1530000, 1093}, \
		{1632000, 1150}, \
		{1734000, 1231}, \
		{1836000, 1362}, \
		{1912500, 1362}, \
		{2014500, 1362}, \
		{2218500, 1362}, \
	}

static struct cpu_dvfs cpu_fv_dvfs_table[] = {
	{
		.speedo_id = 10,
		.process_id = 0,
		.min_mv = 840,
		.max_mv = 1120,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 10,
		.process_id = 1,
		.min_mv = 840,
		.max_mv = 1120,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 9,
		.process_id = 0,
		.min_mv = 900,
		.max_mv = 1162,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 9,
		.process_id = 1,
		.min_mv = 900,
		.max_mv = 1162,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 8,
		.process_id = 0,
		.min_mv = 900,
		.max_mv = 1195,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 8,
		.process_id = 1,
		.min_mv = 900,
		.max_mv = 1195,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 7,
		.process_id = 0,
		.min_mv = 841,
		.max_mv = 1227,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 7,
		.process_id = 1,
		.min_mv = 841,
		.max_mv = 1227,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 6,
		.process_id = 0,
		.min_mv = 870,
		.max_mv = 1150,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 6,
		.process_id = 1,
		.min_mv = 870,
		.max_mv = 1150,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 5,
		.process_id = 0,
		.min_mv = 818,
		.max_mv = 1227,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 5,
		.process_id = 1,
		.min_mv = 818,
		.max_mv = 1227,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 4,
		.process_id = -1,
		.min_mv = 918,
		.max_mv = 1113,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 3,
		.process_id = 0,
		.min_mv = 825,
		.max_mv = 1227,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 3,
		.process_id = 1,
		.min_mv = 825,
		.max_mv = 1227,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 2,
		.process_id = 0,
		.min_mv = 870,
		.max_mv = 1227,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 2,
		.process_id = 1,
		.min_mv = 870,
		.max_mv = 1227,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 1,
		.process_id = 0,
		.min_mv = 837,
		.max_mv = 1227,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 1,
		.process_id = 1,
		.min_mv = 837,
		.max_mv = 1227,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 0,
		.process_id = 0,
		.min_mv = 850,
		.max_mv = 1170,
		CPU_FV_TABLE,
	},
	{
		.speedo_id = 0,
		.process_id = 1,
		.min_mv = 850,
		.max_mv = 1170,
		CPU_FV_TABLE,
	},
};

/* CPU LP DVFS tables */
static unsigned long cpu_lp_max_freq[] = {
/* speedo_id	0	 1        2        3	    4	    5 */
		1132800, 1132800, 1132800, 1132800, 940800, 1132800
};

#define CPU_LP_FV_TABLE		 \
	.fv_table = {		 \
		{51000,   850},	 \
		{102000,  850},	 \
		{204000,  850},	 \
		{307200,  850},	 \
		{403200,  850},	 \
		{518400,  850},	 \
		{614400,  868},	 \
		{710400,  912},	 \
		{825600,  962},	 \
		{921600,  1006}, \
		{1036800, 1062}, \
		{1132800, 1118}, \
		{1228800, 1168}, \
	}

static struct cpu_dvfs cpu_lp_fv_dvfs_table[] = {
	{
		.speedo_id = 5,
		.process_id = -1,
		.min_mv = 818,
		.max_mv = 1227,
		CPU_LP_FV_TABLE,
	},
	{
		.speedo_id = 2,
		.process_id = -1,
		.min_mv = 804,
		.max_mv = 1170,
		CPU_LP_FV_TABLE,
	},
	{
		.speedo_id = 1,
		.process_id = -1,
		.min_mv = 837,
		.max_mv = 1227,
		CPU_LP_FV_TABLE,
	},
	{
		.speedo_id = -1,
		.process_id = -1,
		.min_mv = 850,
		.max_mv = 1170,
		CPU_LP_FV_TABLE,
	},
};

static struct dvfs cpu_lp_dvfs = {
	.clk_name	= "cclk_lp",
	.millivolts	= cpu_lp_millivolts,
	.auto_dvfs	= true,
	.dvfs_rail	= &tegra210_dvfs_rail_vdd_cpu,
	.freqs_mult	= KHZ,
};

/* GPU DVFS tables */
#define NA_FREQ_CVB_TABLE	\
	.freqs_mult = KHZ,	\
	.speedo_scale = 100,	\
	.thermal_scale = 10,	\
	.voltage_scale = 1000,	\
	.cvb_table = {		\
		/* f	   dfll pll:    c0,       c1,       c2,       c3,       c4,       c5 */    \
		{   76800, { }, {   814294,     8144,     -940,      808,   -21583,      226 }, }, \
		{  153600, { }, {   856185,     8144,     -940,      808,   -21583,      226 }, }, \
		{  230400, { }, {   898077,     8144,     -940,      808,   -21583,      226 }, }, \
		{  307200, { }, {   939968,     8144,     -940,      808,   -21583,      226 }, }, \
		{  384000, { }, {   981860,     8144,     -940,      808,   -21583,      226 }, }, \
		{  460800, { }, {  1023751,     8144,     -940,      808,   -21583,      226 }, }, \
		{  537600, { }, {  1065642,     8144,     -940,      808,   -21583,      226 }, }, \
		{  614400, { }, {  1107534,     8144,     -940,      808,   -21583,      226 }, }, \
		{  691200, { }, {  1149425,     8144,     -940,      808,   -21583,      226 }, }, \
		{  768000, { }, {  1191317,     8144,     -940,      808,   -21583,      226 }, }, \
		{  844800, { }, {  1233208,     8144,     -940,      808,   -21583,      226 }, }, \
		{  921600, { }, {  1275100,     8144,     -940,      808,   -21583,      226 }, }, \
		{  998400, { }, {  1316991,     8144,     -940,      808,   -21583,      226 }, }, \
		{ 0,	   { }, { }, }, \
	}

#define NA_FREQ_CVB_TABLE_XA	\
	.freqs_mult = KHZ,	\
	.speedo_scale = 100,	\
	.thermal_scale = 10,	\
	.voltage_scale = 1000,	\
	.cvb_table = {		\
		/* f	   dfll pll:    c0,       c1,       c2,       c3,       c4,       c5 */    \
		{   76800, { }, {  1526811,   -59106,      963,      238,   -11292,      185 }, }, \
		{  153600, { }, {  1543573,   -57798,      910,      179,    -9918,      191 }, }, \
		{  230400, { }, {  1567838,   -56991,      869,       60,    -8545,      203 }, }, \
		{  307200, { }, {  1600241,   -56742,      841,        0,    -7019,      209 }, }, \
		{  384000, { }, {  1635184,   -56501,      813,        0,    -5493,      221 }, }, \
		{  460800, { }, {  1672308,   -56300,      787,     -119,    -3662,      226 }, }, \
		{  537600, { }, {  1712114,   -56093,      759,     -179,    -1526,      238 }, }, \
		{  614400, { }, {  1756009,   -56048,      737,     -298,      610,      244 }, }, \
		{  691200, { }, {  1790251,   -54860,      687,     -358,     3204,      238 }, }, \
		{  768000, { }, {  1783830,   -49449,      532,     -477,     6714,      197 }, }, \
		{  844800, { }, {  1819706,   -45928,      379,     -358,     7019,       89 }, }, \
		{ 0,	   { }, { }, }, \
	}

#define FIXED_FREQ_CVB_TABLE	\
	.freqs_mult = KHZ,	\
	.speedo_scale = 100,	\
	.thermal_scale = 10,	\
	.voltage_scale = 1000,	\
	.cvb_table = {		\
		/* f	   dfll pll:    c0,       c1,       c2 */    \
		{   76800, { }, {  1786666,   -85625,     1632 }, }, \
		{  153600, { }, {  1846729,   -87525,     1632 }, }, \
		{  230400, { }, {  1910480,   -89425,     1632 }, }, \
		{  307200, { }, {  1977920,   -91325,     1632 }, }, \
		{  384000, { }, {  2049049,   -93215,     1632 }, }, \
		{  460800, { }, {  2122872,   -95095,     1632 }, }, \
		{  537600, { }, {  2201331,   -96985,     1632 }, }, \
		{  614400, { }, {  2283479,   -98885,     1632 }, }, \
		{  691200, { }, {  2369315,  -100785,     1632 }, }, \
		{  768000, { }, {  2458841,  -102685,     1632 }, }, \
		{  844800, { }, {  2550821,  -104555,     1632 }, }, \
		{  921600, { }, {  2647676,  -106455,     1632 }, }, \
		{ 0,	   { }, { }, }, \
	}

static struct dvfs gpu_dvfs = {
	.clk_name       = "gpcclk",
	.auto_dvfs      = true,
	.dvfs_rail      = &tegra210_dvfs_rail_vdd_gpu,
};

static struct cvb_dvfs gpu_cvb_dvfs_table[] = {
	{
		.speedo_id = 4,
		.process_id = -1,
		.pll_min_millivolts = 918,
		.max_mv = 1113,
		.max_freq = 844800,
#ifdef CONFIG_TEGRA_USE_NA_GPCPLL
		NA_FREQ_CVB_TABLE_XA,
#else
		FIXED_FREQ_CVB_TABLE,
#endif
	},

	{
		.speedo_id = 3,
		.process_id = -1,
		.pll_min_millivolts = 810,
		.max_mv = 1150,
		.max_freq = 921600,
#ifdef CONFIG_TEGRA_USE_NA_GPCPLL
		NA_FREQ_CVB_TABLE,
#else
		FIXED_FREQ_CVB_TABLE,
#endif
	},

	{
		.speedo_id = 2,
		.process_id = -1,
		.pll_min_millivolts = 818,
		.max_mv = 1150,
		.max_freq = 998400,
#ifdef CONFIG_TEGRA_USE_NA_GPCPLL
		NA_FREQ_CVB_TABLE,
#else
		FIXED_FREQ_CVB_TABLE,
#endif
	},

	{
		.speedo_id = 1,
		.process_id = -1,
		.pll_min_millivolts = 840,
		.max_mv = 1150,
		.max_freq = 998400,
#ifdef CONFIG_TEGRA_USE_NA_GPCPLL
		NA_FREQ_CVB_TABLE,
#else
		FIXED_FREQ_CVB_TABLE,
#endif
	},

	{
		.speedo_id = 0,
		.process_id = -1,
		.pll_min_millivolts = 950,
#ifdef CONFIG_TEGRA_GPU_DVFS
		.max_mv = 1150,
#else
		.max_mv = 1000,
#endif
		.max_freq = 921600,
		FIXED_FREQ_CVB_TABLE,
	},
};

static int gpu_vmin[MAX_THERMAL_RANGES];
static int gpu_peak_millivolts[MAX_DVFS_FREQS];
static int gpu_millivolts[MAX_THERMAL_RANGES][MAX_DVFS_FREQS];

/* Core DVFS tables */
static const int core_millivolts[MAX_DVFS_FREQS] = {
	800, 825, 850, 875, 900, 925, 950, 975, 1000, 1025, 1050, 1062, 1075, 1100, 1125
};

#define CORE_DVFS(_clk_name, _speedo_id, _process_id, _auto, _mult, _freqs...) \
	{							\
		.clk_name	= _clk_name,			\
		.speedo_id	= _speedo_id,			\
		.process_id	= _process_id,			\
		.freqs		= {_freqs},			\
		.freqs_mult	= _mult,			\
		.millivolts	= core_millivolts,		\
		.auto_dvfs	= _auto,			\
		.dvfs_rail	= &tegra210_dvfs_rail_vdd_core,	\
	}

static struct dvfs core_dvfs_table[] = {
/* ID 0 Tables */
	/* Core voltages(mV):				    800,     825,     850,     875,     900,     925,     950,     975,    1000,    1025,    1050,    1062,    1075,    1100,    1125 */
	CORE_DVFS("emc",		0, 0, 1, KHZ,	      1,       1,       1,       1,       1,       1,       1,       1, 1200000, 1200000, 1200000, 1200000, 1200000, 1200000, 1200000),

	CORE_DVFS("vic03",		0, 0, 1, KHZ,	 140800,  140800,  268800,  332800,  371200,  409600,  435200,  473600,  499200,  537600,  563200,  563200,  588800,  601600,  627200),
	CORE_DVFS("nvjpg",		0, 0, 1, KHZ,	 140800,  140800,  268800,  332800,  371200,  409600,  435200,  473600,  499200,  537600,  563200,  563200,  588800,  601600,  627200),
	CORE_DVFS("se",			0, 0, 1, KHZ,	 140800,  140800,  268800,  332800,  371200,  409600,  435200,  473600,  499200,  537600,  563200,  563200,  588800,  601600,  627200),
	CORE_DVFS("tsecb",		0, 0, 1, KHZ,	 140800,  140800,  268800,  332800,  371200,  409600,  435200,  473600,  499200,  537600,  563200,  563200,  588800,  601600,  627200),
	CORE_DVFS("cbus",		0, 0, 1, KHZ,	 217600,  217600,  307200,  307200,  371200,  435200,  499200,  550400,  614400,  678400,  742400,  742400,  793600,  793600,  793600),
	CORE_DVFS("c2bus",		0, 0, 1, KHZ,	 140800,  140800,  268800,  332800,  371200,  409600,  435200,  473600,  499200,  537600,  563200,  563200,  588800,  601600,  627200),

	CORE_DVFS("nvenc",		0, 0, 1, KHZ,	 192000,  192000,  345600,  396800,  435200,  473600,  512000,  563200,  601600,  627200,  652800,  652800,  678400,  691200,  716800),
	CORE_DVFS("nvdec",		0, 0, 1, KHZ,	 192000,  192000,  345600,  396800,  435200,  473600,  512000,  563200,  601600,  627200,  652800,  652800,  678400,  691200,  716800),
	CORE_DVFS("c3bus",		0, 0, 1, KHZ,	 192000,  192000,  345600,  396800,  435200,  473600,  512000,  563200,  601600,  627200,  652800,  652800,  678400,  691200,  716800),

	CORE_DVFS("vi",			0, 0, 1, KHZ,	      1,       1,       1,  307200,  371200,  435200,  499200,  550400,  614400,  678400,  742400,  742400,  793600,  793600,  793600),
	CORE_DVFS("isp",		0, 0, 1, KHZ,	      1,       1,       1,  307200,  371200,  435200,  499200,  550400,  614400,  678400,  742400,  742400,  793600,  793600,  793600),

	CORE_DVFS("ape",		0, 0, 1, KHZ,	 140800,  140800,  230400,  268800,  307200,  345600,  384000,  448000,  486400,  499200,  499200,  499200,  499200,  499200,  499200),

	CORE_DVFS("sbus",		0, 0, 0, KHZ,	 115200,  115200,  179200,  217600,  243200,  268800,  294400,  320000,  345600,  358400,  371200,  371200,  384000,  408000,  408000),
	CORE_DVFS("host1x",		0, 0, 1, KHZ,	  81600,   81600,  140800,  153600,  166400,  192000,  230400,  281600,  320000,  345600,  371200,  371200,  384000,  408000,  408000),
	CORE_DVFS("tsec",		0, 0, 1, KHZ,	 217600,  217600,  384000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000),
	CORE_DVFS("mselect",		0, 0, 1, KHZ,	 204000,  204000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000),

	CORE_DVFS("disp1",		0, 0, 0, KHZ,	 153600,  153600,  332800,  371200,  409600,  422400,  460800,  499200,  537600,  576000,  601600,  601600,  640000,  665600,  665600),
	CORE_DVFS("disp2",		0, 0, 0, KHZ,	 153600,  153600,  332800,  371200,  409600,  422400,  460800,  499200,  537600,  576000,  601600,  601600,  640000,  665600,  665600),

	/* Core voltages(mV):				    800,     825,     850,     875,     900,     925,     950,     975,    1000,    1025,    1050,    1062,    1075,    1100,    1125 */
	CORE_DVFS("emc",		0, 1, 1, KHZ,	      1,       1,       1,       1,       1,       1,       1,       1, 1200000, 1200000, 1200000, 1200000, 1200000, 1200000, 1200000),

	CORE_DVFS("vic03",		0, 1, 1, KHZ,	 192000,  192000,  307200,  345600,  409600,  486400,  524800,  550400,  576000,  588800,  614400,  614400,  627200,  627200,  627200),
	CORE_DVFS("nvjpg",		0, 1, 1, KHZ,	 192000,  192000,  307200,  345600,  409600,  486400,  524800,  550400,  576000,  588800,  614400,  614400,  627200,  627200,  627200),
	CORE_DVFS("se",			0, 1, 1, KHZ,	 192000,  192000,  307200,  345600,  409600,  486400,  524800,  550400,  576000,  588800,  614400,  614400,  627200,  627200,  627200),
	CORE_DVFS("tsecb",		0, 1, 1, KHZ,	 192000,  192000,  307200,  345600,  409600,  486400,  524800,  550400,  576000,  588800,  614400,  614400,  627200,  627200,  627200),
	CORE_DVFS("cbus",		0, 1, 1, KHZ,	 268800,  268800,  473600,  473600,  576000,  588800,  678400,  691200,  691200,  691200,  793600,  793600,  793600,  793600,  793600),
	CORE_DVFS("c2bus",		0, 1, 1, KHZ,	 192000,  192000,  307200,  345600,  409600,  486400,  524800,  550400,  576000,  588800,  614400,  614400,  627200,  627200,  627200),

	CORE_DVFS("nvenc",		0, 1, 1, KHZ,	 268800,  268800,  384000,  448000,  486400,  550400,  576000,  614400,  652800,  678400,  691200,  691200,  716800,  716800,  716800),
	CORE_DVFS("nvdec",		0, 1, 1, KHZ,	 268800,  268800,  384000,  448000,  486400,  550400,  576000,  614400,  652800,  678400,  691200,  691200,  716800,  716800,  716800),
	CORE_DVFS("c3bus",		0, 1, 1, KHZ,	 268800,  268800,  384000,  448000,  486400,  550400,  576000,  614400,  652800,  678400,  691200,  691200,  716800,  716800,  716800),

	CORE_DVFS("vi",			0, 1, 1, KHZ,	      1,       1,       1,  473600,  576000,  588800,  678400,  691200,  691200,  691200,  793600,  793600,  793600,  793600,  793600),
	CORE_DVFS("isp",		0, 1, 1, KHZ,	      1,       1,       1,  473600,  576000,  588800,  678400,  691200,  691200,  691200,  793600,  793600,  793600,  793600,  793600),

	CORE_DVFS("ape",		0, 1, 1, KHZ,	 179200,  179200,  307200,  345600,  371200,  409600,  422400,  460800,  499200,  499200,  499200,  499200,  499200,  499200,  499200),

	CORE_DVFS("sbus",		0, 1, 0, KHZ,	 140800,  140800,  230400,  256000,  281600,  307200,  332800,  358400,  371200,  408000,  408000,  408000,  408000,  408000,  408000),
	CORE_DVFS("host1x",		0, 1, 1, KHZ,	  81600,   81600,  153600,  179200,  192000,  217600,  345600,  358400,  384000,  408000,  408000,  408000,  408000,  408000,  408000),
	CORE_DVFS("tsec",		0, 1, 1, KHZ,	 268800,  268800,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000),
	CORE_DVFS("mselect",		0, 1, 1, KHZ,	 204000,  204000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000),

	CORE_DVFS("disp1",		0, 1, 0, KHZ,	 243200,  243200,  422400,  460800,  499200,  524800,  563200,  576000,  601600,  640000,  665600,  665600,  665600,  665600,  665600),
	CORE_DVFS("disp2",		0, 1, 0, KHZ,	 243200,  243200,  422400,  460800,  499200,  524800,  563200,  576000,  601600,  640000,  665600,  665600,  665600,  665600,  665600),

	/* Core voltages(mV):				    800,     825,     850,     875,     900,     925,     950,     975,    1000,    1025,    1050,    1062,    1075,    1100,    1125 */
	CORE_DVFS("emc",		0, 2, 1, KHZ,	      1,       1,       1,       1,       1,       1,       1,       1, 1200000, 1200000, 1200000, 1200000, 1200000, 1200000, 1200000),

	CORE_DVFS("vic03",		0, 2, 1, KHZ,	 230400,  230400,  371200,  448000,  499200,  563200,  627200,  627200,  627200,  627200,  627200,  627200,  627200,  627200,  627200),
	CORE_DVFS("nvjpg",		0, 2, 1, KHZ,	 230400,  230400,  371200,  448000,  499200,  563200,  627200,  627200,  627200,  627200,  627200,  627200,  627200,  627200,  627200),
	CORE_DVFS("se",			0, 2, 1, KHZ,	 230400,  230400,  371200,  448000,  499200,  563200,  627200,  627200,  627200,  627200,  627200,  627200,  627200,  627200,  627200),
	CORE_DVFS("tsecb",		0, 2, 1, KHZ,	 230400,  230400,  371200,  448000,  499200,  563200,  627200,  627200,  627200,  627200,  627200,  627200,  627200,  627200,  627200),
	CORE_DVFS("cbus",		0, 2, 1, KHZ,	 384000,  384000,  588800,  678400,  691200,  691200,  768000,  793600,  793600,  793600,  793600,  793600,  793600,  793600,  793600),
	CORE_DVFS("c2bus",		0, 2, 1, KHZ,	 230400,  230400,  371200,  448000,  499200,  563200,  627200,  627200,  627200,  627200,  627200,  627200,  627200,  627200,  627200),

	CORE_DVFS("nvenc",		0, 2, 1, KHZ,	 307200,  307200,  486400,  563200,  614400,  640000,  652800,  678400,  716800,  716800,  716800,  716800,  716800,  716800,  716800),
	CORE_DVFS("nvdec",		0, 2, 1, KHZ,	 307200,  307200,  486400,  563200,  614400,  640000,  652800,  678400,  716800,  716800,  716800,  716800,  716800,  716800,  716800),
	CORE_DVFS("c3bus",		0, 2, 1, KHZ,	 307200,  307200,  486400,  563200,  614400,  640000,  652800,  678400,  716800,  716800,  716800,  716800,  716800,  716800,  716800),

	CORE_DVFS("vi",			0, 2, 1, KHZ,	      1,       1,  588800,  678400,  691200,  691200,  768000,  793600,  793600,  793600,  793600,  793600,  793600,  793600,  793600),
	CORE_DVFS("isp",		0, 2, 1, KHZ,	      1,       1,  588800,  678400,  691200,  691200,  768000,  793600,  793600,  793600,  793600,  793600,  793600,  793600,  793600),

	CORE_DVFS("ape",		0, 2, 1, KHZ,	 230400,  230400,  358400,  396800,  422400,  486400,  499200,  499200,  499200,  499200,  499200,  499200,  499200,  499200,  499200),

	CORE_DVFS("sbus",		0, 2, 0, KHZ,	 204800,  204800,  307200,  332800,  371200,  384000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000),
	CORE_DVFS("host1x",		0, 2, 1, KHZ,	 128000,  128000,  217600,  345600,  384000,  384000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000),
	CORE_DVFS("tsec",		0, 2, 1, KHZ,	 345600,  345600,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000),
	CORE_DVFS("mselect",		0, 2, 1, KHZ,	 204000,  204000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000,  408000),

	CORE_DVFS("disp1",		0, 2, 0, KHZ,	 345600,  345600,  486400,  524800,  563200,  576000,  627200,  640000,  665600,  665600,  665600,  665600,  665600,  665600,  665600),
	CORE_DVFS("disp2",		0, 2, 0, KHZ,	 345600,  345600,  486400,  524800,  563200,  576000,  627200,  640000,  665600,  665600,  665600,  665600,  665600,  665600,  665600),

	/* Core voltages(mV):				    800,     825,     850,     875,     900,     925,     950,     975,    1000,    1025,    1050,    1062,    1075,    1100,    1125 */
	CORE_DVFS("pll_a",		0, -1, 1, KHZ,	1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000),
	CORE_DVFS("pll_c",		0, -1, 1, KHZ,	1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000),
	CORE_DVFS("pll_c2",		0, -1, 1, KHZ,	1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000),
	CORE_DVFS("pll_c3",		0, -1, 1, KHZ,	1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000),
	CORE_DVFS("pll_c4_out0",	0, -1, 1, KHZ,	1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000),
	CORE_DVFS("pll_d_out0",		0, -1, 1, KHZ,	1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000),
	CORE_DVFS("pll_d2",		0, -1, 1, KHZ,	1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000),
	CORE_DVFS("pll_dp",		0, -1, 1, KHZ,	1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000, 1130000),

/* Common Tables */
	/* Core voltages(mV):				    800,     825,     850,     875,     900,     925,     950,     975,    1000,    1025,    1050,    1062,    1075,    1100,    1125 */
	CORE_DVFS("csi",		-1, -1, 1, KHZ,	 750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000),
	CORE_DVFS("cilab",		-1, -1, 1, KHZ,	 102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000),
	CORE_DVFS("cilcd",		-1, -1, 1, KHZ,	 102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000),
	CORE_DVFS("cile",		-1, -1, 1, KHZ,	 102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000),

	CORE_DVFS("dsia",		-1, -1, 1, KHZ,	 500000,  500000,  500000,  500000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000),
	CORE_DVFS("dsib",		-1, -1, 1, KHZ,	 500000,  500000,  500000,  500000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000,  750000),

	CORE_DVFS("dsialp",		-1, -1, 1, KHZ,	 102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000),
	CORE_DVFS("dsiblp",		-1, -1, 1, KHZ,	 102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000),

	CORE_DVFS("sor0",		-1, -1, 1, KHZ,	 162000,  162000,  270000,  270000,  270000,  540000,  540000,  540000,  540000,  540000,  540000,  540000,  540000,  540000,  540000),
	CORE_DVFS("sor1",		-1, -1, 1, KHZ,	 148500,  148500,  297000,  297000,  297000,  297000,  297000,  594000,  594000,  594000,  594000,  594000,  594000,  594000,  594000),

	CORE_DVFS("i2s0",		-1, -1, 1, KHZ,	  24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576),
	CORE_DVFS("i2s1",		-1, -1, 1, KHZ,	  24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576),
	CORE_DVFS("i2s2",		-1, -1, 1, KHZ,	  24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576),
	CORE_DVFS("i2s3",		-1, -1, 1, KHZ,	  24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576),
	CORE_DVFS("i2s4",		-1, -1, 1, KHZ,	  24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576,   24576),

	CORE_DVFS("d_audio",		-1, -1, 1, KHZ,	  49152,   49152,   98304,   98304,   98304,   98304,   98304,   98304,   98304,   98304,   98304,   98304,   98304,   98304,   98304),
	CORE_DVFS("spdif_out",		-1, -1, 1, KHZ,	  24576,   24576,   24576,   24576,   24576,   24576,   24728,   24728,   24728,   24728,   24728,   24728,   24728,   24728,   24728),
	CORE_DVFS("dmic1",		-1, -1, 1, KHZ,	  12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190),
	CORE_DVFS("dmic2",		-1, -1, 1, KHZ,	  12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190),
	CORE_DVFS("dmic3",		-1, -1, 1, KHZ,	  12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190,   12190),

	CORE_DVFS("hda",		-1, -1, 1, KHZ,	  51000,   51000,   51000,   51000,   51000,   51000,   51000,   51000,   51000,   51000,   51000,   51000,   51000,   51000,   51000),
	CORE_DVFS("hda2codec_2x",	-1, -1, 1, KHZ,	  48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000),

	CORE_DVFS("sdmmc2",		-1, -1, 1, KHZ,	      1,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000),
	CORE_DVFS("sdmmc4",		-1, -1, 1, KHZ,	      1,  200000,  266000,  266000,  266000,  266000,  266000,  266000,  266000,  266000,  266000,  266000,  266000,  266000,  266000),
	CORE_DVFS("sdmmc2_ddr",		-1, -1, 1, KHZ,	 102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000),
	CORE_DVFS("sdmmc4_ddr",		-1, -1, 1, KHZ,	 102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000),

	CORE_DVFS("sdmmc1",		-1, -1, 1, KHZ,	 136000,  136000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000),
	CORE_DVFS("sdmmc3",		-1, -1, 1, KHZ,	 136000,  136000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000),

	CORE_DVFS("sdmmc1_ddr",		-1, -1, 1, KHZ,	  96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000),
	CORE_DVFS("sdmmc3_ddr",		-1, -1, 1, KHZ,	  96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000,   96000),

	CORE_DVFS("xusb_falcon_src",	-1, -1, 1, KHZ,	      1,       1,  336000,  336000,  336000,  336000,  336000,  336000,  336000,  336000,  336000,  336000,  336000,  336000,  336000),
	CORE_DVFS("xusb_host_src",	-1, -1, 1, KHZ,	      1,       1,  112000,  112000,  112000,  112000,  112000,  112000,  112000,  112000,  112000,  112000,  112000,  112000,  112000),
	CORE_DVFS("xusb_dev_src",	-1, -1, 1, KHZ,	      1,       1,  112000,  112000,  112000,  112000,  112000,  112000,  112000,  112000,  112000,  112000,  112000,  112000,  112000),
	CORE_DVFS("xusb_ssp_src",	-1, -1, 1, KHZ,	      1,       1,  120000,  120000,  120000,  120000,  120000,  120000,  120000,  120000,  120000,  120000,  120000,  120000,  120000),
	CORE_DVFS("xusb_fs_src",	-1, -1, 1, KHZ,	      1,       1,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000),
	CORE_DVFS("xusb_hs_src",	-1, -1, 1, KHZ,	      1,       1,  120000,  120000,  120000,  120000,  120000,  120000,  120000,  120000,  120000,  120000,  120000,  120000,  120000),
	CORE_DVFS("usbd",		-1, -1, 1, KHZ,	 480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000),
	CORE_DVFS("usb2",		-1, -1, 1, KHZ,	 480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000,  480000),

	CORE_DVFS("sata",		-1, -1, 1, KHZ,	      1,       1,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000,  102000),
	CORE_DVFS("sata_oob",		-1, -1, 1, KHZ,	      1,       1,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000,  204000),
	CORE_DVFS("pcie",		-1, -1, 1, KHZ,	      1,       1,  500000,  500000,  500000,  500000,  500000,  500000,  500000,  500000,  500000,  500000,  500000,  500000,  500000),

	CORE_DVFS("i2c1",		-1, -1, 1, KHZ,	 136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000),
	CORE_DVFS("i2c2",		-1, -1, 1, KHZ,	 136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000),
	CORE_DVFS("i2c3",		-1, -1, 1, KHZ,	 136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000),
	CORE_DVFS("i2c4",		-1, -1, 1, KHZ,	 136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000),
	CORE_DVFS("i2c5",		-1, -1, 1, KHZ,	 136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000),
	CORE_DVFS("i2c6",		-1, -1, 1, KHZ,	 136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000),
	CORE_DVFS("vii2c",		-1, -1, 1, KHZ,	 136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000),

	CORE_DVFS("pwm",		-1, -1, 1, KHZ,	  48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000,   48000),

	CORE_DVFS("soc_therm",		-1, -1, 1, KHZ,	 136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000,  136000),
	CORE_DVFS("tsensor",		-1, -1, 1, KHZ,	  19200,   19200,   19200,   19200,   19200,   19200,   19200,   19200,   19200,   19200,   19200,   19200,   19200,   19200,   19200),

	CORE_DVFS("sbc1",		-1, -1, 1, KHZ,	  12000,   35000,   50000,   50000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000),
	CORE_DVFS("sbc2",		-1, -1, 1, KHZ,	  12000,   35000,   50000,   50000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000),
	CORE_DVFS("sbc3",		-1, -1, 1, KHZ,	  12000,   35000,   50000,   50000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000),
	CORE_DVFS("sbc4",		-1, -1, 1, KHZ,	  12000,   35000,   50000,   50000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000,   65000),
};

static struct dvfs qspi_sdr_dvfs_table[] = {
	CORE_DVFS("qspi",		-1, -1, 1, KHZ,	  81600,   81600,  116600,  116600,  116600,  116600,  116600,  116600,  116600,  116600,  116600,  116600,  116600,  116600,  116600),
};

static struct dvfs qspi_ddr_dvfs_table[] = {
	CORE_DVFS("qspi",		-1, -1, 1, KHZ,	  81600,   81600,   81600,   81600,   81600,   81600,   81600,   81600,   81600,   81600,   81600,   81600,   81600,   81600,   81600),
};

static struct dvfs sor1_dp_dvfs_table[] = {
	CORE_DVFS("sor1",		-1, -1, 1, KHZ,	 162000,  162000,  270000,  270000,  270000,  540000,  540000,  540000,  540000,  540000,  540000,  540000,  540000,  540000,  540000),
};

int tegra_dvfs_disable_core_set(const char *arg, const struct kernel_param *kp)
{
	int ret;

	ret = param_set_bool(arg, kp);
	if (ret)
		return ret;

	if (tegra_dvfs_core_disabled)
		tegra_dvfs_rail_disable(&tegra210_dvfs_rail_vdd_core);
	else
		tegra_dvfs_rail_enable(&tegra210_dvfs_rail_vdd_core);

	return 0;
}

int tegra_dvfs_disable_cpu_set(const char *arg, const struct kernel_param *kp)
{
	int ret;

	ret = param_set_bool(arg, kp);
	if (ret)
		return ret;

	if (tegra_dvfs_cpu_disabled)
		tegra_dvfs_rail_disable(&tegra210_dvfs_rail_vdd_cpu);
	else
		tegra_dvfs_rail_enable(&tegra210_dvfs_rail_vdd_cpu);

	return 0;
}

int tegra_dvfs_disable_get(char *buffer, const struct kernel_param *kp)
{
	return param_get_bool(buffer, kp);
}

static struct kernel_param_ops tegra_dvfs_disable_core_ops = {
	.set = tegra_dvfs_disable_core_set,
	.get = tegra_dvfs_disable_get,
};

static struct kernel_param_ops tegra_dvfs_disable_cpu_ops = {
	.set = tegra_dvfs_disable_cpu_set,
	.get = tegra_dvfs_disable_get,
};

module_param_cb(disable_core, &tegra_dvfs_disable_core_ops,
	&tegra_dvfs_core_disabled, 0644);
module_param_cb(disable_cpu, &tegra_dvfs_disable_cpu_ops,
	&tegra_dvfs_cpu_disabled, 0644);

static void init_dvfs_one(struct dvfs *d, int max_freq_index)
{
	int ret;
	struct clk *c = clk_get_sys(d->clk_name, d->clk_name);

	if (IS_ERR(c)) {
		pr_info("tegra210_dvfs: no clock found for %s\n",
			d->clk_name);
		return;
	}

	d->max_millivolts = d->dvfs_rail->nominal_millivolts;

	ret = tegra_setup_dvfs(c, d);
	if (ret)
		pr_err("tegra210_dvfs: failed to enable dvfs on %s\n",
				__clk_get_name(c));
}

static bool match_dvfs_one(const char *name, int dvfs_speedo_id,
			   int dvfs_process_id, int speedo_id, int process_id)
{
	if ((dvfs_process_id != -1 && dvfs_process_id != process_id) ||
		(dvfs_speedo_id != -1 && dvfs_speedo_id != speedo_id)) {
		pr_debug("tegra210_dvfs: rejected %s speedo %d, process %d\n",
			 name, dvfs_speedo_id, dvfs_process_id);
		return false;
	}
	return true;
}

static int set_cpu_dvfs_data(unsigned long max_freq, struct cpu_dvfs *d,
			     struct dvfs *cpu_dvfs, int *max_freq_index)
{
	int i, mv, dfll_mv, min_dfll_mv, num_freqs;
	unsigned long fmax_at_vmin = 0;
	unsigned long fmin_use_dfll = 0;
	unsigned long *freqs;
	int *dfll_millivolts;
	struct rail_alignment *align = &tegra210_dvfs_rail_vdd_cpu.alignment;

	min_dfll_mv = d->min_mv;
	if (min_dfll_mv < tegra210_dvfs_rail_vdd_cpu.min_millivolts) {
		pr_debug("tegra210_dvfs: dfll min %dmV below rail min %dmV\n",
			 min_dfll_mv, tegra210_dvfs_rail_vdd_cpu.min_millivolts);
		min_dfll_mv = tegra210_dvfs_rail_vdd_cpu.min_millivolts;
	}
	min_dfll_mv = tegra_round_voltage(min_dfll_mv, align, true);
	d->max_mv = tegra_round_voltage(d->max_mv, align, false);

	if (tegra_get_cpu_fv_table(&num_freqs, &freqs, &dfll_millivolts))
		return -EPROBE_DEFER;

	for (i = 0; i < num_freqs; i++) {
		if (freqs[i] / KHZ != d->fv_table[i].freq) {
			pr_err("Err: DFLL freq ladder does not match PLL's\n");
			return -EINVAL;
		}

		if (d->fv_table[i].freq > max_freq)
			break;

		mv = d->fv_table[i].volt;
		/*
		 * Check maximum frequency at minimum voltage for dfll source;
		 * round down unless all table entries are above Vmin, then use
		 * the 1st entry as is.
		 */
		dfll_mv = max(dfll_millivolts[i] / 1000, min_dfll_mv);
		if (dfll_mv > min_dfll_mv) {
			if (!i)
				fmax_at_vmin = freqs[i];
			if (!fmax_at_vmin)
				fmax_at_vmin = freqs[i - 1];
		}

		/* Clip maximum frequency at maximum voltage for pll source */
		if ((mv > d->max_mv) && !i) {
			pr_err("Err: volt of 1st entry is higher than Vmax\n");
			return -EINVAL;
		}

		/* Minimum rate with pll source voltage above dfll Vmin */
		if ((mv >= min_dfll_mv) && !fmin_use_dfll)
			fmin_use_dfll = freqs[i];

		/* fill in dvfs tables */
		cpu_dvfs->freqs[i] = freqs[i];
		cpu_millivolts[i] = mv;
		cpu_dfll_millivolts[i] = min(dfll_mv, d->max_mv);
	}

	/*
	 * In the dfll operating range dfll voltage at any rate should be
	 * better (below) than pll voltage
	 */
	if (!fmin_use_dfll || (fmin_use_dfll > fmax_at_vmin))
		fmin_use_dfll = fmax_at_vmin;

	/* dvfs tables are successfully populated - fill in the rest */
	cpu_dvfs->speedo_id = d->speedo_id;
	cpu_dvfs->process_id = d->process_id;
	cpu_dvfs->dvfs_rail->nominal_millivolts = min(d->max_mv,
		max(cpu_millivolts[i - 1], cpu_dfll_millivolts[i - 1]));
	*max_freq_index = i - 1;

	cpu_dvfs->use_dfll_rate_min = fmin_use_dfll;

	return 0;
}

/*
 * Setup slow CPU (a.k.a LP CPU) DVFS table from FV data. Only PLL is used as
 * a clock source for slow CPU. Its maximum frequency must be reached within
 * nominal voltage -- FV frequency list is cut off at rate that exceeds either
 * sku-based maximum limit or requires voltage above nominal. Error when DVFS
 * table can not be constructed must never happen.
 *
 * Final CPU rail nominal voltage is set as maximum of fast and slow CPUs
 * nominal voltages.
 */
static int set_cpu_lp_dvfs_data(unsigned long max_freq, struct cpu_dvfs *d,
				struct dvfs *cpu_lp_dvfs, int *max_freq_index)
{
	int i, mv, min_mv;
	struct rail_alignment *align = &tegra210_dvfs_rail_vdd_cpu.alignment;

	min_mv = d->min_mv;
	if (min_mv < tegra210_dvfs_rail_vdd_cpu.min_millivolts) {
		pr_debug("tegra210_dvfs: scpu min %dmV below rail min %dmV\n",
			 min_mv, tegra210_dvfs_rail_vdd_cpu.min_millivolts);
		min_mv = tegra210_dvfs_rail_vdd_cpu.min_millivolts;
	}
	min_mv = tegra_round_voltage(min_mv, align, true);

	d->max_mv = tegra_round_voltage(d->max_mv, align, false);
	BUG_ON(d->max_mv > tegra210_dvfs_rail_vdd_cpu.max_millivolts);
	cpu_lp_dvfs->dvfs_rail->nominal_millivolts =
		max(cpu_lp_dvfs->dvfs_rail->nominal_millivolts, d->max_mv);

	for (i = 0; i < MAX_DVFS_FREQS; i++) {
		struct cpu_pll_fv_table *t = &d->fv_table[i];
		if (!t->freq || t->freq > max_freq)
			break;

		mv = t->volt;
		mv = max(mv, min_mv);
		if (mv > d->max_mv) {
			pr_warn("tegra210_dvfs: %dmV for %s rate %d above limit %dmV\n",
			     mv, cpu_lp_dvfs->clk_name, t->freq, d->max_mv);
			break;
		}

		/* fill in dvfs tables */
		cpu_lp_dvfs->freqs[i] = t->freq;
		cpu_lp_millivolts[i] = mv;
	}

	/* Table must not be empty */
	if (!i) {
		pr_err("tegra210_dvfs: invalid cpu lp dvfs table\n");
		return -ENOENT;
	}

	/* dvfs tables are successfully populated - fill in the rest */
	cpu_lp_dvfs->speedo_id = d->speedo_id;
	cpu_lp_dvfs->process_id = d->process_id;
	*max_freq_index = i - 1;

	return 0;
}

int of_tegra_dvfs_init(const struct of_device_id *matches)
{
	int ret;
	struct device_node *np;

	for_each_matching_node(np, matches) {
		const struct of_device_id *match = of_match_node(matches, np);
		int (*dvfs_init_cb)(struct device_node *) = match->data;

		ret = dvfs_init_cb(np);
		if (ret) {
			pr_err("dt: Failed to read %s data from DT\n",
			       match->compatible);
			return ret;
		}
	}
	return 0;
}

/*
 * QSPI DVFS tables are different in SDR and DDR modes. Use SDR tables by
 * default. Check if DDR mode is specified for enabled QSPI device in DT,
 * and overwrite DVFS table, respectively.
 */

static struct dvfs *qspi_dvfs = &qspi_sdr_dvfs_table[0];

static int of_update_qspi_dvfs(struct device_node *dn)
{
	if (of_device_is_available(dn)) {
		if (of_get_property(dn, "nvidia,x4-is-ddr", NULL))
			qspi_dvfs = &qspi_ddr_dvfs_table[0];
	}
	return 0;
}

static struct of_device_id tegra210_dvfs_qspi_of_match[] = {
	{ .compatible = "nvidia,tegra210-qspi", .data = of_update_qspi_dvfs, },
	{ },
};

static void init_qspi_dvfs(int soc_speedo_id, int core_process_id,
				  int core_nominal_mv_index)
{
	of_tegra_dvfs_init(tegra210_dvfs_qspi_of_match);

	if (match_dvfs_one(qspi_dvfs->clk_name, qspi_dvfs->speedo_id,
		qspi_dvfs->process_id, soc_speedo_id, core_process_id))
		init_dvfs_one(qspi_dvfs, core_nominal_mv_index);
}

static void __init init_sor1_dvfs(int soc_speedo_id, int core_process_id,
				  int core_nominal_mv_index)
{
	struct dvfs *sor1_dp_dvfs = &sor1_dp_dvfs_table[0];
	struct clk *c;

	c = clk_get_sys(sor1_dp_dvfs->clk_name, sor1_dp_dvfs->clk_name);
	if (IS_ERR(c)) {
		pr_debug("init_sor1_dvfs: no clock found for %s\n",
			sor1_dp_dvfs->clk_name);
		return;
	}

	if (match_dvfs_one(sor1_dp_dvfs->clk_name, sor1_dp_dvfs->speedo_id,
		sor1_dp_dvfs->process_id, soc_speedo_id, core_process_id))
		tegra_dvfs_add_alt_freqs(c, sor1_dp_dvfs);

	return;

}

static int get_core_speedo_mv(void)
{
	int speedo_rev = tegra_sku_info.revision;

	switch (tegra_sku_info.soc_process_id) {
	case 0:
		if (speedo_rev <= 1)
			return 1000;

		return 1125;
	case 1:
		return 1075;
	case 2:
		return 1000;
	default:
		pr_err("Un-supported Tegra210 speedo %d\n",
				tegra_sku_info.soc_speedo_id);
		return -EINVAL;
	}
}

static int get_core_nominal_mv_index(int speedo_id)
{
	int i;
	int mv = get_core_speedo_mv();

	if (mv < 0)
		return mv;

	/* Round nominal level down to the nearest core scaling step */
	for (i = 0; i < MAX_DVFS_FREQS; i++) {
		if ((core_millivolts[i] == 0) || (mv < core_millivolts[i]))
			break;
	}

	if (i == 0) {
		pr_err("tegra210-dvfs: failed to get nominal idx at volt %d\n",
		       mv);
		return -ENOSYS;
	}

	return i - 1;
}

static int init_cpu_dvfs_table(int *cpu_max_freq_index)
{
	int i, ret;
	int cpu_speedo_id = tegra_sku_info.cpu_speedo_id;
	int cpu_process_id = tegra_sku_info.cpu_process_id;

	BUG_ON(cpu_speedo_id >= ARRAY_SIZE(cpu_max_freq));
	for (ret = 0, i = 0; i <  ARRAY_SIZE(cpu_fv_dvfs_table); i++) {
		struct cpu_dvfs *d = &cpu_fv_dvfs_table[i];
		unsigned long max_freq = cpu_max_freq[cpu_speedo_id];
		if (match_dvfs_one("cpu dvfs", d->speedo_id, d->process_id,
				   cpu_speedo_id, cpu_process_id)) {
			ret = set_cpu_dvfs_data(max_freq,
				d, &cpu_dvfs, cpu_max_freq_index);
			break;
		}
	}
	BUG_ON(i == ARRAY_SIZE(cpu_fv_dvfs_table));

	return ret;
}

static int init_cpu_lp_dvfs_table(int *cpu_lp_max_freq_index)
{
	int i, ret;
	int cpu_lp_speedo_id = tegra_sku_info.cpu_speedo_id;
	int cpu_lp_process_id = tegra_sku_info.cpu_process_id;

	for (ret = 0, i = 0; i <  ARRAY_SIZE(cpu_lp_fv_dvfs_table); i++) {
		struct cpu_dvfs *d = &cpu_lp_fv_dvfs_table[i];
		unsigned long max_freq = cpu_lp_max_freq[cpu_lp_speedo_id];
		if (match_dvfs_one("cpu lp dvfs", d->speedo_id, d->process_id,
				   cpu_lp_speedo_id, cpu_lp_process_id)) {
			ret = set_cpu_lp_dvfs_data(max_freq,
				d, &cpu_lp_dvfs, cpu_lp_max_freq_index);
			break;
		}
	}

	return ret;
}

static void adjust_emc_dvfs_table(struct dvfs *d)
{
	unsigned long rate;
	int i;

	for (i = 0; i < ARRAY_SIZE(core_millivolts); i++) {
		if (core_millivolts[i] == 0)
			return;

		rate = tegra210_predict_emc_rate(core_millivolts[i]);
		if (IS_ERR_VALUE(rate))
			return;

		if (rate)
			d->freqs[i] = rate;
	}
}

/*
 * Find maximum GPU frequency that can be reached at minimum voltage across all
 * temperature ranges.
 */
static unsigned long find_gpu_fmax_at_vmin(
	struct dvfs *gpu_dvfs, int thermal_ranges, int freqs_num)
{
	int i, j;
	unsigned long fmax = ULONG_MAX;

	/*
	 * For voltage scaling row in each temperature range, as well as peak
	 * voltage row find maximum frequency at lowest voltage, and return
	 * minimax. On Tegra21 all GPU DVFS thermal dependencies are integrated
	 * into thermal DVFS table (i.e., there is no separate thermal floors
	 * applied in the rail level). Hence, returned frequency specifies max
	 * frequency safe at minimum voltage across all temperature ranges.
	 */
	for (j = 0; j < thermal_ranges; j++) {
		for (i = 1; i < freqs_num; i++) {
			if (gpu_millivolts[j][i] > gpu_millivolts[j][0])
				break;
		}
		fmax = min(fmax, gpu_dvfs->freqs[i - 1]);
	}

	for (i = 1; i < freqs_num; i++) {
		if (gpu_peak_millivolts[i] > gpu_peak_millivolts[0])
			break;
	}
	fmax = min(fmax, gpu_dvfs->freqs[i - 1]);

	return fmax;
}

/*
 * Determine minimum voltage safe at maximum frequency across all temperature
 * ranges.
 */
static int find_gpu_vmin_at_fmax(
	struct dvfs *gpu_dvfs, int thermal_ranges, int freqs_num)
{
	int j, vmin;

	/*
	 * For voltage scaling row in each temperature range find minimum
	 * voltage at maximum frequency and return max Vmin across ranges.
	 */
	for (vmin = 0, j = 0; j < thermal_ranges; j++)
		vmin = max(vmin, gpu_millivolts[j][freqs_num-1]);

	return vmin;
}

static int init_gpu_rail_thermal_scaling(struct dvfs_rail *rail,
					 struct cvb_dvfs *d)
{
	return 1;
}

static int init_gpu_rail_thermal_caps(struct dvfs *dvfs,
		struct dvfs_rail *rail, int thermal_ranges, int freqs_num)
{
	if (thermal_ranges <= 1 )
		return 0;

	WARN_ON(1);
}

/*
 * Setup gpu dvfs tables from cvb data, determine nominal voltage for gpu rail,
 * and gpu maximum frequency. Error when gpu dvfs table can not be constructed
 * must never happen.
 */
static int set_gpu_dvfs_data(unsigned long max_freq,
	struct cvb_dvfs *d, struct dvfs *gpu_dvfs, int *max_freq_index)
{
	int i, j, thermal_ranges, mv, min_mv, err;
	struct cvb_dvfs_table *table = NULL;
	int speedo = tegra_sku_info.gpu_speedo_value;
	struct dvfs_rail *rail = &tegra210_dvfs_rail_vdd_gpu;
	struct rail_alignment *align = &rail->alignment;

	d->max_mv = tegra_round_voltage(d->max_mv, align, false);
	min_mv = d->pll_min_millivolts;
	if (min_mv < rail->min_millivolts) {
		pr_debug("tegra21_dvfs: gpu min %dmV below rail min %dmV\n",
			 min_mv, rail->min_millivolts);
		min_mv = rail->min_millivolts;
	}

	/*
	 * Get scaling thermal ranges; 1 range implies no thermal dependency.
	 * Invalidate scaling cooling device in the latter case.
	 */
	thermal_ranges = init_gpu_rail_thermal_scaling(rail, d);
	if (thermal_ranges == 1)
		rail->vts_cdev = NULL;

	/*
	 * Apply fixed thermal floor for each temperature range
	 */
	for (j = 0; j < thermal_ranges; j++) {
		mv = max(min_mv, d->therm_floors_table[j]);
		gpu_vmin[j] = tegra_round_voltage(mv, align, true);
	}

	/*
	 * Use CVB table to fill in gpu dvfs frequencies and voltages. Each
	 * CVB entry specifies gpu frequency and CVB coefficients to calculate
	 * the respective voltage.
	 */
	for (i = 0; i < MAX_DVFS_FREQS; i++) {
		table = &d->cvb_table[i];
		if (!table->freq || (table->freq > max_freq))
			break;

		mv = tegra_get_cvb_voltage(
			speedo, d->speedo_scale, &table->cvb_pll_param);

		for (j = 0; j < thermal_ranges; j++) {
			int mvj = mv;
			int t = 0;

			if (thermal_ranges > 1)
				t = d->vts_trips_table[j];

			/* get thermal offset for this trip-point */
			mvj += tegra_get_cvb_t_voltage(speedo, d->speedo_scale,
				t, d->thermal_scale, &table->cvb_pll_param);
			mvj = tegra_round_cvb_voltage(mvj, d->voltage_scale, align);

			/* clip to minimum, abort if above maximum */
			mvj = max(mvj, gpu_vmin[j]);
			if (mvj > d->max_mv)
				break;

			/*
			 * Update voltage for adjacent ranges bounded by this
			 * trip-point (cvb & dvfs are transpose matrices, and
			 * cvb freq row index is column index for dvfs matrix)
			 */
			gpu_millivolts[j][i] = mvj;
			if (j && (gpu_millivolts[j-1][i] < mvj))
				gpu_millivolts[j-1][i] = mvj;
		}
		/* Make sure all voltages for this frequency are below max */
		if (j < thermal_ranges)
			break;

		/* fill in gpu dvfs tables */
		gpu_dvfs->freqs[i] = table->freq;
	}

	gpu_dvfs->millivolts = &gpu_millivolts[0][0];

	/*
	 * Table must not be empty, must have at least one entry in range, and
	 * must specify monotonically increasing voltage on frequency dependency
	 * in each temperature range.
	 */
	err = tegra_dvfs_init_thermal_dvfs_voltages(&gpu_millivolts[0][0],
			gpu_peak_millivolts, i, thermal_ranges, gpu_dvfs);

	if (err || !i) {
		pr_err("tegra21_dvfs: invalid gpu dvfs table\n");
		return -ENOENT;
	}

	/* Shift out the 1st trip-point */
	for (j = 1; j < thermal_ranges; j++)
		d->vts_trips_table[j - 1] = d->vts_trips_table[j];

	/* dvfs tables are successfully populated - fill in the gpu dvfs */
	gpu_dvfs->speedo_id = d->speedo_id;
	gpu_dvfs->process_id = d->process_id;
	gpu_dvfs->freqs_mult = d->freqs_mult;

	*max_freq_index = i - 1;

	gpu_dvfs->dvfs_rail->nominal_millivolts = min(d->max_mv,
		find_gpu_vmin_at_fmax(gpu_dvfs, thermal_ranges, i));

	gpu_dvfs->fmax_at_vmin_safe_t = d->freqs_mult *
		find_gpu_fmax_at_vmin(gpu_dvfs, thermal_ranges, i);

	/* Initialize thermal capping */
	init_gpu_rail_thermal_caps(gpu_dvfs, rail, thermal_ranges, i);

#ifdef CONFIG_TEGRA_USE_NA_GPCPLL
	/*
	 * Set NA DVFS flag, if GPCPLL NA mode is enabled. This is necessary to
	 * make sure that GPCPLL configuration is updated by tegra core DVFS
	 * when thermal DVFS cooling device state is changed. Since tegra core
	 * DVFS does not support NA operations for Vmin cooling device, GPU Vmin
	 * thermal floors have been integrated with thermal DVFS, and no Vmin
	 * cooling device is installed.
	 */
	if (tegra_fuse_can_use_na_gpcpll())
		gpu_dvfs->na_dvfs = 1;
#endif
	return 0;
}

static void init_gpu_dvfs_table(int *gpu_max_freq_index)
{
	int i, ret;
	int gpu_speedo_id = tegra_sku_info.gpu_speedo_id;
	int gpu_process_id = tegra_sku_info.gpu_process_id;

	for (ret = 0, i = 0; i < ARRAY_SIZE(gpu_cvb_dvfs_table); i++) {
		struct cvb_dvfs *d = &gpu_cvb_dvfs_table[i];
		unsigned long max_freq = d->max_freq;
		if (match_dvfs_one("gpu cvb", d->speedo_id, d->process_id,
				   gpu_speedo_id, gpu_process_id)) {
			ret = set_gpu_dvfs_data(max_freq,
				d, &gpu_dvfs, gpu_max_freq_index);
			break;
		}
	}
	BUG_ON((i == ARRAY_SIZE(gpu_cvb_dvfs_table)) || ret);
}

int tegra210_init_dvfs(void)
{
	int soc_speedo_id = tegra_sku_info.soc_speedo_id;
	int core_process_id = tegra_sku_info.soc_process_id;
	int i, ret;
	int core_nominal_mv_index;
	int cpu_max_freq_index = 0;
	int cpu_lp_max_freq_index = 0;
	int gpu_max_freq_index = 0;

	/*
	 * Find nominal voltages for core (1st) and cpu rails before rail
	 * init. Nominal voltage index in core scaling ladder can also be
	 * used to determine max dvfs frequencies for all core clocks. In
	 * case of error disable core scaling and set index to 0, so that
	 * core clocks would not exceed rates allowed at minimum voltage.
	 */
	core_nominal_mv_index = get_core_nominal_mv_index(soc_speedo_id);
	if (core_nominal_mv_index < 0) {
		tegra210_dvfs_rail_vdd_core.disabled = true;
		tegra_dvfs_core_disabled = true;
		core_nominal_mv_index = 0;
	}
	tegra210_dvfs_rail_vdd_core.nominal_millivolts =
		core_millivolts[core_nominal_mv_index];
	tegra210_dvfs_rail_vdd_core.min_millivolts =
		max(tegra210_dvfs_rail_vdd_core.min_millivolts,
		    core_millivolts[0]);

	/*
	 * Construct fast and slow CPU DVFS tables from FV data; find maximum
	 * frequency, minimum  and nominal voltage for each CPU cluster, and
	 * combined rail limits (fast CPU should be initialized 1st).
	 */
	ret = init_cpu_dvfs_table(&cpu_max_freq_index);
	if (ret)
		goto out;

	init_cpu_lp_dvfs_table(&cpu_lp_max_freq_index);
	if (ret)
		goto out;

	/*
	 * Construct GPU DVFS table from CVB data; find GPU maximum frequency,
	 * and nominal voltage.
	 */
	init_gpu_dvfs_table(&gpu_max_freq_index);

	/* Init core thermal floors */
	tegra_dvfs_init_therm_limits(&tegra210_dvfs_rail_vdd_core);

	/* Init rail structures and dependencies */
	tegra_dvfs_init_rails(tegra210_dvfs_rails,
			      ARRAY_SIZE(tegra210_dvfs_rails));

	/*
	 * Search core dvfs table for speedo/process matching entries and
	 * initialize dvfs-ed clocks
	 */
	for (i = 0; i < ARRAY_SIZE(core_dvfs_table); i++) {
		struct dvfs *d = &core_dvfs_table[i];
		if (!match_dvfs_one(d->clk_name, d->speedo_id,
			d->process_id, soc_speedo_id, core_process_id))
			continue;
		init_dvfs_one(d, core_nominal_mv_index);

		/*
		 * EMC dvfs is board dependent, the EMC scaling frequencies are
		 * determined by the Tegra BCT and the board specific EMC DFS
		 * table owned by EMC driver.
		 */
		if (!strcmp(d->clk_name, "emc") && tegra210_emc_is_ready())
			adjust_emc_dvfs_table(d);
	}

	init_qspi_dvfs(soc_speedo_id, core_process_id, core_nominal_mv_index);
	init_sor1_dvfs(soc_speedo_id, core_process_id, core_nominal_mv_index);

	/*
	 * Initialize matching cpu dvfs entry already found when nominal
	 * voltage was determined
	 */
	init_dvfs_one(&cpu_dvfs, cpu_max_freq_index);
	init_dvfs_one(&cpu_lp_dvfs, cpu_lp_max_freq_index);
	init_dvfs_one(&gpu_dvfs, gpu_max_freq_index);

	for (i = 0; i < ARRAY_SIZE(tegra210_dvfs_rails); i++) {
		struct dvfs_rail *rail = tegra210_dvfs_rails[i];
		pr_info("tegra dvfs: %s: nominal %dmV, offset %duV, step %duV, scaling %s\n",
			rail->reg_id, rail->nominal_millivolts,
			rail->alignment.offset_uv, rail->alignment.step_uv,
			rail->disabled ? "disabled" : "enabled");
	}
	return 0;
out:
	return ret;
}
