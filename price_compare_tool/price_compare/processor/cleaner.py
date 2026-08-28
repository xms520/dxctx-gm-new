"""数据清洗/去重/排序.

职责:
  - normalize: 价格/销量归一化 (¥99.9 / 1万+ -> 数值);
  - dedup: 跨平台去重 (平台+标题+价格 业务键);
  - sort: 默认按价格升序, 可选销量/性价比.
"""
from __future__ import annotations

import logging
import re
from typing import List

from ..models import Product

log = logging.getLogger(__name__)


class Cleaner:
    """采集数据清洗器."""

    def run(self, products: List[Product]) -> List[Product]:
        if not products:
            return []
        cleaned = [self._normalize(p) for p in products]
        cleaned = [p for p in cleaned if p.title and p.price > 0]
        deduped = self._dedup(cleaned)
        log.info("[cleaner] 清洗: %d -> %d (去重%d条)",
                 len(products), len(deduped), len(products) - len(deduped))
        return deduped

    def _normalize(self, p: Product) -> Product:
        p.title = re.sub(r"\s+", " ", p.title).strip()
        p.price = round(float(p.price), 2)
        p.sales = int(p.sales or 0)
        if p.shop_rating is not None:
            p.shop_rating = round(float(p.shop_rating), 2)
        # 链接兜底
        if not p.url:
            p.url = ""
        return p

    def _dedup(self, products: List[Product]) -> List[Product]:
        seen = set()
        out: List[Product] = []
        for p in products:
            # 业务键: 平台 + 标题前40字符归一 + 价格
            norm = re.sub(r"\s+", "", p.title).lower()[:40]
            key = (p.platform, norm, round(p.price, 2))
            if key in seen:
                continue
            seen.add(key)
            out.append(p)
        return out


def sort_by_price(products: List[Product], ascending: bool = True) -> List[Product]:
    """按价格排序 (默认从低到高)."""
    return sorted(products, key=lambda p: p.price, reverse=not ascending)


def sort_by_value(products: List[Product]) -> List[Product]:
    """按性价比分 (高->低) 排序."""
    return sorted(products,
                  key=lambda p: p.value_score or 0, reverse=True)


def sort_by_sales(products: List[Product]) -> List[Product]:
    return sorted(products, key=lambda p: p.sales, reverse=True)
