/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: James Liao <jamesjj.liao@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __DRV_CLK_MUX_H
#define __DRV_CLK_MUX_H

#include <linux/clk-provider.h>

struct mtk_mux_upd {
	int id;
	const char *name;
	const char * const *parent_names;

	u32 mux_ofs;
	u32 upd_ofs;

	s8 mux_shift;
	s8 mux_width;
	s8 gate_shift;
	s8 upd_shift;

	s8 num_parents;
};

#define MUX_UPD(_id, _name, _parents, _mux_ofs, _shift, _width, _gate,	\
			_upd_ofs, _upd) {				\
		.id = _id,						\
		.name = _name,						\
		.mux_ofs = _mux_ofs,					\
		.upd_ofs = _upd_ofs,					\
		.mux_shift = _shift,					\
		.mux_width = _width,					\
		.gate_shift = _gate,					\
		.upd_shift = _upd,					\
		.parent_names = _parents,				\
		.num_parents = ARRAY_SIZE(_parents),			\
	}

struct clk *mtk_clk_register_mux_upd(const struct mtk_mux_upd *tm,
		void __iomem *base, spinlock_t *lock);

void mtk_clk_register_mux_upds(const struct mtk_mux_upd *tms,
		int num, void __iomem *base, spinlock_t *lock,
		struct clk_onecell_data *clk_data);

#endif /* __DRV_CLK_MUX_H */
