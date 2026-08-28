"""性价比分析与推荐标注.

评分模型 (value_score, 0~1, 越高越值得买):
  value = 0.5 * (1 - 价格归一)      价格越低越好
        + 0.3 * 销量归一            销量越高越受欢迎
        + 0.2 * 店铺评分归一        评分越高越可靠

标签:
  - 性价比推荐: value_score 排名前 20%
  - 适中:       中间 55%
  - 偏高:       后 25%
"""
from __future__ import annotations

import logging
from typing import List, Optional, Tuple

from ..models import Product

log = logging.getLogger(__name__)

_W_PRICE, _W_SALES, _W_RATING = 0.5, 0.3, 0.2


class Analyzer:
    """性价比打分与推荐标注."""

    def run(self, products: List[Product]) -> List[Product]:
        if not products:
            return products
        # 各维度归一化基准
        prices = [p.price for p in products]
        sales = [p.sales for p in products]
        ratings = [r for r in (p.shop_rating for p in products) if r is not None]
        pmin, pmax = min(prices), max(prices)
        smin, smax = min(sales), max(sales)
        rmin = min(ratings) if ratings else 4.5
        rmax = max(ratings) if ratings else 5.0

        for p in products:
            price_norm = _norm(p.price, pmin, pmax)
            sales_norm = _norm(p.sales, smin, smax)
            rating_norm = _norm(p.shop_rating if p.shop_rating is not None
                                else rmin, rmin, rmax)
            score = (_W_PRICE * (1 - price_norm)
                     + _W_SALES * sales_norm
                     + _W_RATING * rating_norm)
            p.value_score = round(score, 4)

        self._tag(products)
        log.info("[analyzer] 性价比打分完成, 共 %d 条", len(products))
        return products

    def _tag(self, products: List[Product]) -> None:
        ordered = sorted(products, key=lambda x: x.value_score or 0,
                         reverse=True)
        n = len(ordered)
        rec_cut = max(1, int(n * 0.2))
        high_cut = max(rec_cut, int(n * 0.75))
        for i, p in enumerate(ordered):
            if i < rec_cut:
                p.value_tag = "性价比推荐"
            elif i < high_cut:
                p.value_tag = "适中"
            else:
                p.value_tag = "偏高"


def _norm(v: float, lo: float, hi: float) -> float:
    if hi == lo:
        return 0.5
    return max(0.0, min(1.0, (v - lo) / (hi - lo)))
