/*
 * Tegra234 MC StreamID configuration
 *
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define pr_fmt(fmt)	"%s(): " fmt, __func__

#include <linux/err.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#include <linux/platform/tegra/tegra-mc-sid.h>
#include <dt-bindings/memory/tegra234-smmu-streamid.h>

enum override_id {
	HDAR,
	HOST1XDMAR,
	NVENCSRD,
	PCIE6AR,
	PCIE6AW,
	PCIE7AR,
	NVENCSWR,
	DLA0RDB,
	DLA0RDB1,
	DLA0WRB,
	DLA1RDB,
	PCIE7AW,
	PCIE8AR,
	PSCR,
	PSCW,
	HDAW,
	OFAR1,
	PCIE8AW,
	PCIE9AR,
	PCIE6AR1,
	PCIE9AW,
	PCIE10AR,
	PCIE10AW,
	ISPRA,
	ISPFALR,
	ISPWA,
	ISPWB,
	PCIE10AR1,
	PCIE7AR1,
	XUSB_HOSTR,
	XUSB_HOSTW,
	XUSB_DEVR,
	XUSB_DEVW,
	TSECSRD,
	TSECSWR,
	XSPI1W,
	MGBEARD,
	MGBEBRD,
	MGBECRD,
	MGBEDRD,
	MGBEAWR,
	OFAR,
	OFAW,
	MGBEBWR,
	SDMMCRA,
	MGBECWR,
	SDMMCRAB,
	SDMMCWA,
	MGBEDWR,
	SDMMCWAB,
	SEU1RD,
	SEU1WR,
	DCER,
	DCEW,
	VICSRD,
	VICSWR,
	DLA1RDB1,
	DLA1WRB,
	VI2W,
	VI2FALR,
	VIW,
	XSPI0R,
	XSPI0W,
	XSPI1R,
	NVDECSRD,
	NVDECSWR,
	APER,
	APEW,
	VI2FALW,
	NVJPGSRD,
	NVJPGSWR,
	SESRD,
	SESWR,
	AXIAPR,
	AXIAPW,
	ETRR,
	ETRW,
	DCEDMAR,
	DCEDMAW,
	AXISR,
	AXISW,
	EQOSR,
	EQOSW,
	UFSHCR,
	UFSHCW,
	BPMPR,
	BPMPW,
	BPMPDMAR,
	BPMPDMAW,
	AONR,
	AONW,
	AONDMAR,
	AONDMAW,
	SCER,
	SCEW,
	SCEDMAR,
	SCEDMAW,
	APEDMAR,
	APEDMAW,
	VICSRD1,
	VIFALR,
	VIFALW,
	DLA0RDA,
	DLA0FALRDB,
	DLA0WRA,
	DLA0FALWRB,
	DLA1RDA,
	DLA1FALRDB,
	DLA1WRA,
	DLA1FALWRB,
	PVA0RDA,
	PVA0RDB,
	PVA0RDC,
	PVA0WRA,
	PVA0WRB,
	PVA0WRC,
	RCER,
	RCEW,
	RCEDMAR,
	RCEDMAW,
	PCIE0R,
	PCIE0W,
	PCIE1R,
	PCIE1W,
	PCIE2AR,
	PCIE2AW,
	PCIE3R,
	PCIE3W,
	PCIE4R,
	PCIE4W,
	PCIE5R,
	PCIE5W,
	ISPFALW,
	DLA0RDA1,
	DLA1RDA1,
	PVA0RDA1,
	PVA0RDB1,
	PCIE5R1,
	NVENCSRD1,
	ISPRA1,
	JESD204TXIF0RD,
	JESD204TXIF1RD,
	JESD204RXIF0WR,
	JESD204RXIF1WR,
	JESD204RXIF2WR,
	JESD204RXIF3WR,
	JESD204TXCTRLRD,
	JESD204TXCTRLWR,
	JESD204RXCTRLRD,
	JESD204RXCTRLWR,
	JESD204TXIF0RD1,
	JESD204TXIF1RD1,
	LDPCDECAR,
	LDPCDECAR1,
	LDPCDECAW,
	LDPCDECBR,
	LDPCDECBW,
	LDPCENCAR,
	LDPCENCAW,
	LDPCENCBR,
	LDPCENCBW,
	PMA0AWR,
	NVJPG1SRD,
	NVJPG1SWR,
	MAX_OID,
};

static struct sid_override_reg sid_override_reg[] = {
	DEFREG(HDAR,        0xa8),
	DEFREG(HOST1XDMAR,  0xb0),
	DEFREG(NVENCSRD,    0xe0),
	DEFREG(PCIE6AR,     0x140),
	DEFREG(PCIE6AW,     0x148),
	DEFREG(PCIE7AR,     0x150),
	DEFREG(NVENCSWR,    0x158),
	DEFREG(DLA0RDB,     0x160),
	DEFREG(DLA0RDB1,    0x168),
	DEFREG(DLA0WRB,     0x170),
	DEFREG(DLA1RDB,     0x178),
	DEFREG(PCIE7AW,     0x180),
	DEFREG(PCIE8AR,     0x190),
	DEFREG(PSCR,        0x198),
	DEFREG(PSCW,        0x1a0),
	DEFREG(HDAW,        0x1a8),
	DEFREG(OFAR1,       0x1d0),
	DEFREG(PCIE8AW,     0x1d8),
	DEFREG(PCIE9AR,     0x1e0),
	DEFREG(PCIE6AR1,    0x1e8),
	DEFREG(PCIE9AW,     0x1f0),
	DEFREG(PCIE10AR,    0x1f8),
	DEFREG(PCIE10AW,    0x200),
	DEFREG(ISPRA,       0x220),
	DEFREG(ISPFALR,     0x228),
	DEFREG(ISPWA,       0x230),
	DEFREG(ISPWB,       0x238),
	DEFREG(PCIE10AR1,   0x240),
	DEFREG(PCIE7AR1,    0x248),
	DEFREG(XUSB_HOSTR,  0x250),
	DEFREG(XUSB_HOSTW,  0x258),
	DEFREG(XUSB_DEVR,   0x260),
	DEFREG(XUSB_DEVW,   0x268),
	DEFREG(TSECSRD,     0x2a0),
	DEFREG(TSECSWR,     0x2a8),
	DEFREG(XSPI1W,      0x2b0),
	DEFREG(MGBEARD,     0x2c0),
	DEFREG(MGBEBRD,     0x2c8),
	DEFREG(MGBECRD,     0x2d0),
	DEFREG(MGBEDRD,     0x2d8),
	DEFREG(MGBEAWR,     0x2e0),
	DEFREG(OFAR,        0x2e8),
	DEFREG(OFAW,        0x2f0),
	DEFREG(MGBEBWR,     0x2f8),
	DEFREG(SDMMCRA,     0x300),
	DEFREG(MGBECWR,     0x308),
	DEFREG(SDMMCRAB,    0x318),
	DEFREG(SDMMCWA,     0x320),
	DEFREG(MGBEDWR,     0x328),
	DEFREG(SDMMCWAB,    0x338),
	DEFREG(SEU1RD,      0x340),
	DEFREG(SEU1WR,      0x348),
	DEFREG(DCER,        0x350),
	DEFREG(DCEW,        0x358),
	DEFREG(VICSRD,      0x360),
	DEFREG(VICSWR,      0x368),
	DEFREG(DLA1RDB1,    0x370),
	DEFREG(DLA1WRB,     0x378),
	DEFREG(VI2W,        0x380),
	DEFREG(VI2FALR,     0x388),
	DEFREG(VIW,         0x390),
	DEFREG(XSPI0R,      0x3a8),
	DEFREG(XSPI0W,      0x3b0),
	DEFREG(XSPI1R,      0x3b8),
	DEFREG(NVDECSRD,    0x3c0),
	DEFREG(NVDECSWR,    0x3c8),
	DEFREG(APER,        0x3d0),
	DEFREG(APEW,        0x3d8),
	DEFREG(VI2FALW,     0x3e0),
	DEFREG(NVJPGSRD,    0x3f0),
	DEFREG(NVJPGSWR,    0x3f8),
	DEFREG(SESRD,       0x400),
	DEFREG(SESWR,       0x408),
	DEFREG(AXIAPR,      0x410),
	DEFREG(AXIAPW,      0x418),
	DEFREG(ETRR,        0x420),
	DEFREG(ETRW,        0x428),
	DEFREG(DCEDMAR,     0x440),
	DEFREG(DCEDMAW,     0x448),
	DEFREG(AXISR,       0x460),
	DEFREG(AXISW,       0x468),
	DEFREG(EQOSR,       0x470),
	DEFREG(EQOSW,       0x478),
	DEFREG(UFSHCR,      0x480),
	DEFREG(UFSHCW,      0x488),
	DEFREG(BPMPR,       0x498),
	DEFREG(BPMPW,       0x4a0),
	DEFREG(BPMPDMAR,    0x4a8),
	DEFREG(BPMPDMAW,    0x4b0),
	DEFREG(AONR,        0x4b8),
	DEFREG(AONW,        0x4c0),
	DEFREG(AONDMAR,     0x4c8),
	DEFREG(AONDMAW,     0x4d0),
	DEFREG(SCER,        0x4d8),
	DEFREG(SCEW,        0x4e0),
	DEFREG(SCEDMAR,     0x4e8),
	DEFREG(SCEDMAW,     0x4f0),
	DEFREG(APEDMAR,     0x4f8),
	DEFREG(APEDMAW,     0x500),
	DEFREG(VICSRD1,     0x510),
	DEFREG(VIFALR,      0x5e0),
	DEFREG(VIFALW,      0x5e8),
	DEFREG(DLA0RDA,     0x5f0),
	DEFREG(DLA0FALRDB,  0x5f8),
	DEFREG(DLA0WRA,     0x600),
	DEFREG(DLA0FALWRB,  0x608),
	DEFREG(DLA1RDA,     0x610),
	DEFREG(DLA1FALRDB,  0x618),
	DEFREG(DLA1WRA,     0x620),
	DEFREG(DLA1FALWRB,  0x628),
	DEFREG(PVA0RDA,     0x630),
	DEFREG(PVA0RDB,     0x638),
	DEFREG(PVA0RDC,     0x640),
	DEFREG(PVA0WRA,     0x648),
	DEFREG(PVA0WRB,     0x650),
	DEFREG(PVA0WRC,     0x658),
	DEFREG(RCER,        0x690),
	DEFREG(RCEW,        0x698),
	DEFREG(RCEDMAR,     0x6a0),
	DEFREG(RCEDMAW,     0x6a8),
	DEFREG(PCIE0R,      0x6c0),
	DEFREG(PCIE0W,      0x6c8),
	DEFREG(PCIE1R,      0x6d0),
	DEFREG(PCIE1W,      0x6d8),
	DEFREG(PCIE2AR,     0x6e0),
	DEFREG(PCIE2AW,     0x6e8),
	DEFREG(PCIE3R,      0x6f0),
	DEFREG(PCIE3W,      0x6f8),
	DEFREG(PCIE4R,      0x700),
	DEFREG(PCIE4W,      0x708),
	DEFREG(PCIE5R,      0x710),
	DEFREG(PCIE5W,      0x718),
	DEFREG(ISPFALW,     0x720),
	DEFREG(DLA0RDA1,    0x748),
	DEFREG(DLA1RDA1,    0x750),
	DEFREG(PVA0RDA1,    0x758),
	DEFREG(PVA0RDB1,    0x760),
	DEFREG(PCIE5R1,     0x778),
	DEFREG(NVENCSRD1,   0x780),
	DEFREG(ISPRA1,      0x790),
	DEFREG(JESD204TXIF0RD, 0x810),
	DEFREG(JESD204TXIF1RD, 0x818),
	DEFREG(JESD204RXIF0WR, 0x820),
	DEFREG(JESD204RXIF1WR, 0x828),
	DEFREG(JESD204RXIF2WR, 0x830),
	DEFREG(JESD204RXIF3WR, 0x838),
	DEFREG(JESD204TXCTRLRD,0x840),
	DEFREG(JESD204TXCTRLWR,0x848),
	DEFREG(JESD204RXCTRLRD,0x850),
	DEFREG(JESD204RXCTRLWR,0x858),
	DEFREG(JESD204TXIF0RD1,0x860),
	DEFREG(JESD204TXIF1RD1,0x868),
	DEFREG(LDPCDECAR,      0x8c0),
	DEFREG(LDPCDECAR1,     0x8c8),
	DEFREG(LDPCDECAW,      0x8d0),
	DEFREG(LDPCDECBR,      0x8d8),
	DEFREG(LDPCDECBW,      0x8e0),
	DEFREG(LDPCENCAR,      0x8e8),
	DEFREG(LDPCENCAW,      0x8f8),
	DEFREG(LDPCENCBR,      0x900),
	DEFREG(LDPCENCBW,      0x908),
	DEFREG(PMA0AWR,        0x910),
	DEFREG(NVJPG1SRD,      0x918),
	DEFREG(NVJPG1SWR,      0x920),
};

static struct sid_to_oids sid_to_oids[] = {
	{
		.sid	= TEGRA_SID_AON,
		.noids	= 4,
		.oid	= {
			AONR,
			AONW,
			AONDMAR,
			AONDMAW,
		},
		.ord = NO_OVERRIDE,
		.name = "AON",
	},
	{
		.sid	= TEGRA_SID_APE,
		.noids	= 4,
		.oid	= {
			APER,
			APEW,
			APEDMAR,
			APEDMAW,
		},
		.ord = NO_OVERRIDE,
		.name = "APE",
	},
	{
		.sid	= TEGRA_SID_GPCDMA_0,
		.noids	= 4,
		.oid	= {
			AXIAPR,
			AXIAPW,
			AXISR,
			AXISW,

		},
		.ord = NO_OVERRIDE,
		.name = "GPCDMA",
	},
	{
		.sid	= TEGRA_SID_BPMP,
		.noids	= 4,
		.oid	= {
			BPMPR,
			BPMPW,
			BPMPDMAR,
			BPMPDMAW,
		},
		.ord = NO_OVERRIDE,
		.name = "BPMP",
	},
	{
		.sid	= TEGRA_SID_DCE,
		.noids	= 4,
		.oid	= {
			DCEDMAR,
			DCEDMAW,
			DCER,
			DCEW,
		},
		.ord = NO_OVERRIDE,
		.name = "DCE",
	},
	{
		.sid	= TEGRA_SID_NVDLA0,
		.noids	= 8,
		.oid	= {
			DLA0FALRDB,
			DLA0FALWRB,
			DLA0RDA,
			DLA0RDA1,
			DLA0RDB,
			DLA0RDB1,
			DLA0WRA,
			DLA0WRB,
		},
		.ord = SIM_OVERRIDE,
		.name = "NVDLA0",
	},
	{
		.sid	= TEGRA_SID_NVDLA1,
		.noids	= 8,
		.oid	= {
			DLA1FALRDB,
			DLA1FALWRB,
			DLA1RDA,
			DLA1RDA1,
			DLA1RDB,
			DLA1RDB1,
			DLA1WRA,
			DLA1WRB,
		},
		.ord = SIM_OVERRIDE,
		.name = "NVDLA1",
	},
	{
		.sid	= TEGRA_SID_EQOS,
		.noids	= 2,
		.oid	= {
			EQOSR,
			EQOSW,
		},
		.ord = OVERRIDE,
		.name = "EQOS",
	},
	{
		.sid	= TEGRA_SID_ETR,
		.noids	= 2,
		.oid	= {
			ETRR,
			ETRW,
		},
		.ord = OVERRIDE,
		.name = "ETR",
	},
	{
		.sid	= TEGRA_SID_HDA,
		.noids	= 2,
		.oid	= {
			HDAR,
			HDAW,
		},
		.ord = OVERRIDE,
		.name = "HDA",
	},
	{
		.sid	= TEGRA_SID_HC,
		.noids	= 1,
		.oid	= {
			HOST1XDMAR,
		},
		.ord = NO_OVERRIDE,
		.name = "HC",
	},
	{
		.sid	= TEGRA_SID_ISP,
		.noids	= 6,
		.oid	= {
			ISPFALR,
			ISPFALW,
			ISPRA,
			ISPRA1,
			ISPWA,
			ISPWB,
		},
		.ord = NO_OVERRIDE,
		.name = "ISP",
	},
	{
		.sid	= TEGRA_SID_JESD204,
		.noids	= 12,
		.oid	= {
			JESD204RXCTRLRD,
			JESD204RXCTRLWR,
			JESD204RXIF0WR,
			JESD204RXIF1WR,
			JESD204RXIF2WR,
			JESD204RXIF3WR,
			JESD204TXCTRLRD,
			JESD204TXCTRLWR,
			JESD204TXIF0RD,
			JESD204TXIF0RD1,
			JESD204TXIF1RD,
			JESD204TXIF1RD1,
		},
		.ord = NO_OVERRIDE,
		.name = "JESD204",
	},
	{
		.sid	= TEGRA_SID_LDPC,
		.noids	= 9,
		.oid	= {
			LDPCDECAR,
			LDPCDECAR1,
			LDPCDECAW,
			LDPCDECBR,
			LDPCDECBW,
			LDPCENCAR,
			LDPCENCAW,
			LDPCENCBR,
			LDPCENCBW,
		},
		.ord = NO_OVERRIDE,
		.name = "LDPC",
	},
	{
		.sid	= TEGRA_SID_MGBE,
		.noids	= 8,
		.oid	= {
			MGBEARD,
			MGBEAWR,
			MGBEBRD,
			MGBEBWR,
			MGBECRD,
			MGBECWR,
			MGBEDRD,
			MGBEDWR,
		},
		.ord = NO_OVERRIDE,
		.name = "MGBE",
	},
	{
		.sid	= TEGRA_SID_NVDEC,
		.noids	= 2,
		.oid	= {
			NVDECSRD,
			NVDECSWR,
		},
		.ord = NO_OVERRIDE,
		.name = "NVDEC",
	},
	{
		.sid	= TEGRA_SID_NVENC,
		.noids	= 3,
		.oid	= {
			NVENCSRD,
			NVENCSWR,
			NVENCSRD1,
		},
		.ord = NO_OVERRIDE,
		.name = "NVENC",
	},
	{
		.sid	= TEGRA_SID_NVJPG,
		.noids	= 2,
		.oid	= {
			NVJPGSRD,
			NVJPGSWR,
		},
		.ord = NO_OVERRIDE,
		.name = "NVJPG",
	},
	{
		.sid	= TEGRA_SID_NVJPG1,
		.noids	= 2,
		.oid	= {
			NVJPG1SRD,
			NVJPG1SWR,
		},
		.ord = NO_OVERRIDE,
		.name = "NVJPG",
	},
	{
		.sid	= TEGRA_SID_OFA,
		.noids	= 3,
		.oid	= {
			OFAR,
			OFAR1,
			OFAW,
		},
		.ord = NO_OVERRIDE,
		.name = "OFA",
	},
	{
		.sid	= TEGRA_SID_PCIE0,
		.noids	= 3,
		.oid	= {
			PCIE0R,
			PCIE0W,
		},
		.ord = SIM_OVERRIDE,
		.name = "PCIE0",
	},
	{
		.sid	= TEGRA_SID_PCIE1,
		.noids	= 2,
		.oid	= {
			PCIE1R,
			PCIE1W,
		},
		.ord = SIM_OVERRIDE,
		.name = "PCIE1",
	},
	{
		.sid	= TEGRA_SID_PCIE2,
		.noids	= 2,
		.oid	= {
			PCIE2AR,
			PCIE2AW,
		},
		.ord = SIM_OVERRIDE,
		.name = "PCIE2",
	},
	{
		.sid	= TEGRA_SID_PCIE3,
		.noids	= 2,
		.oid	= {
			PCIE3R,
			PCIE3W,
		},
		.ord = SIM_OVERRIDE,
		.name = "PCIE3",
	},
	{
		.sid	= TEGRA_SID_PCIE4,
		.noids	= 3,
		.oid	= {
			PCIE4R,
			PCIE4W,
		},
		.ord = SIM_OVERRIDE,
		.name = "PCIE4",
	},
	{
		.sid	= TEGRA_SID_PCIE5,
		.noids	= 3,
		.oid	= {
			PCIE5R,
			PCIE5W,
			PCIE5R1,
		},
		.ord = SIM_OVERRIDE,
		.name = "PCIE5",
	},
	{
		.sid	= TEGRA_SID_PCIE6,
		.noids	= 3,
		.oid	= {
			PCIE6AR,
			PCIE6AR1,
			PCIE6AW,
		},
		.ord = SIM_OVERRIDE,
		.name = "PCIE6",
	},
	{
		.sid	= TEGRA_SID_PCIE7,
		.noids	= 3,
		.oid	= {

			PCIE7AR,
			PCIE7AR1,
			PCIE7AW,
		},
		.ord = SIM_OVERRIDE,
		.name = "PCIE7",
	},
	{
		.sid	= TEGRA_SID_PCIE8,
		.noids	= 2,
		.oid	= {

			PCIE8AR,
			PCIE8AW,
		},
		.ord = SIM_OVERRIDE,
		.name = "PCIE8",
	},
	{
		.sid	= TEGRA_SID_PCIE9,
		.noids	= 2,
		.oid	= {

			PCIE9AR,
			PCIE9AW,
		},
		.ord = SIM_OVERRIDE,
		.name = "PCIE9",
	},
	{
		.sid	= TEGRA_SID_PCIE10,
		.noids	= 3,
		.oid	= {
			PCIE10AR,
			PCIE10AR1,
			PCIE10AW,
		},
		.ord = SIM_OVERRIDE,
		.name = "PCIE10",
	},
	{
		.sid	= TEGRA_SID_PMA,
		.noids	= 1,
		.oid	= {
			PMA0AWR,
		},
		.ord = SIM_OVERRIDE,
		.name = "PMA",
	},
	{
		.sid	= TEGRA_SID_PSC,
		.noids	= 2,
		.oid	= {
			PSCR,
			PSCW,
		},
		.ord = SIM_OVERRIDE,
		.name = "PSC",
	},
	{
		.sid	= TEGRA_SID_PVA0,
		.noids	= 8,
		.oid	= {
			PVA0RDA,
			PVA0RDA1,
			PVA0RDB,
			PVA0RDB1,
			PVA0RDC,
			PVA0WRA,
			PVA0WRB,
			PVA0WRC,
		},
		.ord = SIM_OVERRIDE,
		.name = "PVA0",
	},
	{
		.sid	= TEGRA_SID_RCE,
		.noids	= 4,
		.oid	= {
			RCER,
			RCEW,
			RCEDMAR,
			RCEDMAW,
		},
		.ord = NO_OVERRIDE,
		.name = "RCE",
	},
	{
		.sid	= TEGRA_SID_SCE,
		.noids	= 4,
		.oid	= {
			SCER,
			SCEW,
			SCEDMAR,
			SCEDMAW,
		},
		.ord = NO_OVERRIDE,
		.name = "SCE",
	},
	{
		.sid	= TEGRA_SID_SDMMC1A,
		.noids	= 2,
		.oid	= {
			SDMMCRA,
			SDMMCWA,
		},
		.ord = OVERRIDE,
		.name = "SDMMC1A",
	},
	{
		.sid	= TEGRA_SID_SDMMC4A,
		.noids	= 2,
		.oid	= {
			SDMMCRAB,
			SDMMCWAB,
		},
		.ord = OVERRIDE,
		.name = "SDMMC4A",
	},
	{
		.sid	= TEGRA_SID_SE,
		.noids	= 2,
		.oid	= {
			SESRD,
			SESWR,
		},
		.ord = NO_OVERRIDE,
		.name = "SE",
	},
	{
		.sid	= TEGRA_SID_SEU1,
		.noids	= 2,
		.oid	= {
			SEU1RD,
			SEU1WR,
		},
		.ord = SIM_OVERRIDE,
		.name = "SEU1",
	},
	{
		.sid	= TEGRA_SID_TSEC,
		.noids	= 2,
		.oid	= {
			TSECSRD,
			TSECSWR,
		},
		.ord = SIM_OVERRIDE,
		.name = "TSEC",
	},
	{
		.sid	= TEGRA_SID_UFSHC,
		.noids	= 2,
		.oid	= {
			UFSHCR,
			UFSHCW,
		},
		.ord = OVERRIDE,
		.name = "UFSHC",
	},
	{
		.sid	= TEGRA_SID_VIC,
		.noids	= 3,
		.oid = {
			VICSRD,
			VICSRD1,
			VICSWR,
		},
		.ord = NO_OVERRIDE,
		.name = "VIC",
	},
	{
		.sid	= TEGRA_SID_VI,
		.noids	= 1,
		.oid	= {
			VIW,
		},
		.ord = NO_OVERRIDE,
		.name = "VI",
	},
	{
		.sid	= TEGRA_SID_VIFALC,
		.noids	= 2,
		.oid	= {
			VIFALR,
			VIFALW,
		},
		.ord = NO_OVERRIDE,
		.name = "VIFALC",
	},
	{
		.sid	= TEGRA_SID_VI2,
		.noids	= 1,
		.oid	= {
			VI2W,
		},
		.ord = NO_OVERRIDE,
		.name = "VI2",
	},
	{
		.sid	= TEGRA_SID_VI2FALC,
		.noids	= 2,
		.oid	= {
			VI2FALR,
			VI2FALW,
		},
		.ord = NO_OVERRIDE,
		.name = "VI2FALC",
	},
	{
		.sid	= TEGRA_SID_XSPI0,
		.noids	= 2,
		.oid	= {
			XSPI0R,
			XSPI0W,
		},
		.ord = OVERRIDE,
		.name = "XSPI0",
	},
	{
		.sid	= TEGRA_SID_XSPI1,
		.noids	= 2,
		.oid	= {
			XSPI1R,
			XSPI1W,
		},
		.ord = OVERRIDE,
		.name = "XSPI1",
	},
	{
		.sid	= TEGRA_SID_XUSB_HOST,
		.noids	= 2,
		.oid	= {
			XUSB_HOSTR,
			XUSB_HOSTW,
		},
		.ord = OVERRIDE,
		.name = "XUSB_HOST",
	},
	{
		.sid	= TEGRA_SID_XUSB_DEV,
		.noids	= 2,
		.oid	= {
			XUSB_DEVR,
			XUSB_DEVW,
		},
		.ord = OVERRIDE,
		.name = "XUSB_DEV",
	},
};

static const struct tegra_mc_sid_soc_data tegra234_mc_soc_data = {
	.sid_override_reg = sid_override_reg,
	.nsid_override_reg = ARRAY_SIZE(sid_override_reg),
	.sid_to_oids = sid_to_oids,
	.nsid_to_oids = ARRAY_SIZE(sid_to_oids),
	.max_oids = MAX_OID,
};

static int tegra234_mc_sid_probe(struct platform_device *pdev)
{
	int err = 0;

	err = tegra_mc_sid_probe(pdev, &tegra234_mc_soc_data);
	if (err != 0)
		pr_err("tegra234 mc-sid probe failed\n");
	else
		pr_info("tegra234 mc-sid probe successful\n");
	return err;
}

static const struct of_device_id tegra234_mc_sid_of_match[] = {
	{ .compatible = "nvidia,tegra234-mc-sid", },
	{},
};
MODULE_DEVICE_TABLE(of, tegra234_mc_sid_of_match);

static struct platform_driver tegra234_mc_sid_driver = {
	.probe	= tegra234_mc_sid_probe,
	.remove = tegra_mc_sid_remove,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "tegra234-mc-sid",
		.of_match_table	= of_match_ptr(tegra234_mc_sid_of_match),
	},
};

static int __init tegra234_mc_sid_init(void)
{
	struct device_node *np;
	struct platform_device *pdev = NULL;

	np = of_find_compatible_node(NULL, NULL, "nvidia,tegra234-mc-sid");
	if (np) {
		pdev = of_platform_device_create(np, NULL,
						 platform_bus_type.dev_root);
		of_node_put(np);
	}

	if (IS_ERR_OR_NULL(pdev))
		return -ENODEV;

	return platform_driver_register(&tegra234_mc_sid_driver);
}
arch_initcall(tegra234_mc_sid_init);

MODULE_DESCRIPTION("MC StreamID configuration");
MODULE_AUTHOR("Pritesh Raithatha <praithatha@nvidia.com>");
MODULE_LICENSE("GPL v2");
