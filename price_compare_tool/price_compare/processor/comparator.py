"""横向对比器.

职责: 按平台 + 整体维度聚合统计, 计算平台内价格排名,
输出可直接展示的对比摘要 (供 CLI 表格与 Web 展示).
"""
from __future__ import annotations

import logging
import statistics
from typing import Dict, List

from ..models import Product

log = logging.getLogger(__name__)


class Comparator:
    """跨平台横向对比."""

    def run(self, products: List[Product]) -> dict:
        """返回对比结构: {per_platform, overall, ranked_products}."""
        if not products:
            return {"per_platform": {}, "overall": {}, "ranked_products": []}

        # 平台内价格排名 (1=最便宜)
        by_platform: Dict[str, List[Product]] = {}
        for p in products:
            by_platform.setdefault(p.platform, []).append(p)
        for plat, items in by_platform.items():
            items_sorted = sorted(items, key=lambda x: x.price)
            for idx, p in enumerate(items_sorted, start=1):
                p.rank_in_platform = idx

        per_platform = {
            plat: self._summary(items) for plat, items in by_platform.items()
        }
        overall = self._summary(products)

        # 整体价格升序 (默认展示顺序)
        ranked = sorted(products, key=lambda x: x.price)
        return {
            "per_platform": per_platform,
            "overall": overall,
            "ranked_products": [p.to_dict() for p in ranked],
        }

    def _summary(self, items: List[Product]) -> dict:
        prices = [p.price for p in items]
        return {
            "count": len(items),
            "min_price": round(min(prices), 2),
            "max_price": round(max(prices), 2),
            "avg_price": round(statistics.mean(prices), 2),
            "median_price": round(statistics.median(prices), 2),
            "total_sales": sum(p.sales for p in items),
            "avg_rating": round(
                statistics.mean([r for r in (p.shop_rating for p in items)
                                 if r is not None]), 2
            ) if any(p.shop_rating for p in items) else None,
        }
