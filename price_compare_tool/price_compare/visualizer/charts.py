"""可视化: 基于 matplotlib 生成静态图表 (CLI 报告用).

非交互后端 (Agg), 输出 PNG 文件. Web 端使用 ECharts 动态渲染,
本模块仅服务 CLI 场景.

图表:
  1. price_by_platform.png  各平台均价/价格区间柱状图
  2. price_vs_sales.png     价格-销量散点图 (按平台着色)
  3. value_top.png          性价比 Top10 横向条形图
"""
from __future__ import annotations

import logging
import os
from typing import List

from ..models import Product

log = logging.getLogger(__name__)

# 平台 -> 中文名 + 配色
PLATFORM_META = {
    "jd":     ("京东", "#e1251b"),
    "taobao": ("淘宝", "#ff7300"),
    "pdd":    ("拼多多", "#e2231a"),
}


class Visualizer:
    """matplotlib 图表生成器."""

    def __init__(self, out_dir: str = "output"):
        import matplotlib
        matplotlib.use("Agg")  # 非交互, 适合服务端/CI
        import matplotlib.pyplot as plt
        self._plt = plt
        self.out_dir = out_dir
        os.makedirs(out_dir, exist_ok=True)
        # 中文字体兜底
        matplotlib.rcParams["font.sans-serif"] = [
            "WenQuanYi Zen Hei", "WenQuanYi Micro Hei",
            "Noto Sans CJK SC", "SimHei", "Arial Unicode MS", "DejaVu Sans",
        ]
        matplotlib.rcParams["axes.unicode_minus"] = False

    def render_all(self, products: List[Product]) -> List[str]:
        """生成全部图表, 返回文件路径列表."""
        paths = []
        if not products:
            log.warning("[viz] 无数据, 跳过绘图")
            return paths
        try:
            paths.append(self._price_by_platform(products))
        except Exception as e:  # noqa: BLE001
            log.warning("[viz] 均价图失败: %s", e)
        try:
            paths.append(self._price_vs_sales(products))
        except Exception as e:  # noqa: BLE001
            log.warning("[viz] 散点图失败: %s", e)
        try:
            paths.append(self._value_top(products))
        except Exception as e:  # noqa: BLE001
            log.warning("[viz] 性价比图失败: %s", e)
        return paths

    def _price_by_platform(self, products: List[Product]) -> str:
        plt = self._plt
        from statistics import mean, median
        plats = sorted({p.platform for p in products})
        avgs, medians, labels = [], [], []
        for p in plats:
            prices = [x.price for x in products if x.platform == p]
            avgs.append(mean(prices))
            medians.append(median(prices))
            labels.append(PLATFORM_META.get(p, (p, None))[0])

        import numpy as np
        x = np.arange(len(plats))
        w = 0.35
        fig, ax = plt.subplots(figsize=(8, 4.5), dpi=110)
        ax.bar(x - w/2, avgs, w, label="均价", color="#4a90e2")
        ax.bar(x + w/2, medians, w, label="中位价", color="#f5a623")
        ax.set_xticks(x); ax.set_xticklabels(labels)
        ax.set_ylabel("价格 (元)"); ax.set_title("各平台均价 / 中位价对比")
        ax.legend()
        for i, v in enumerate(avgs):
            ax.text(i - w/2, v, f"{v:.0f}", ha="center", va="bottom", fontsize=9)
        fig.tight_layout()
        path = os.path.join(self.out_dir, "price_by_platform.png")
        fig.savefig(path); plt.close(fig)
        return path

    def _price_vs_sales(self, products: List[Product]) -> str:
        plt = self._plt
        fig, ax = plt.subplots(figsize=(8, 4.5), dpi=110)
        for plat in sorted({p.platform for p in products}):
            pts = [p for p in products if p.platform == plat]
            name, color = PLATFORM_META.get(plat, (plat, "#888"))
            xs = [p.price for p in pts]
            ys = [p.sales for p in pts]
            ax.scatter(xs, ys, s=40, color=color, alpha=0.75, label=name)
        ax.set_xlabel("价格 (元)"); ax.set_ylabel("销量 (件)")
        ax.set_title("价格 - 销量 分布 (按平台着色)")
        ax.legend()
        fig.tight_layout()
        path = os.path.join(self.out_dir, "price_vs_sales.png")
        fig.savefig(path); plt.close(fig)
        return path

    def _value_top(self, products: List[Product]) -> str:
        plt = self._plt
        ranked = sorted(products,
                        key=lambda p: p.value_score or 0, reverse=True)[:10]
        ranked = list(reversed(ranked))  # 横向条形图自下而上
        names = [f"{p.title[:14]}…" if len(p.title) > 14 else p.title
                 for p in ranked]
        scores = [p.value_score or 0 for p in ranked]
        colors = []
        for p in ranked:
            colors.append({"性价比推荐": "#2ecc71", "适中": "#f5a623",
                           "偏高": "#e74c3c"}.get(p.value_tag, "#999"))

        fig, ax = plt.subplots(figsize=(8, 5), dpi=110)
        ax.barh(names, scores, color=colors)
        ax.set_xlabel("性价比评分"); ax.set_title("性价比 Top 10")
        ax.set_xlim(0, 1)
        for i, v in enumerate(scores):
            ax.text(v, i, f" {v:.2f}", va="center", fontsize=9)
        fig.tight_layout()
        path = os.path.join(self.out_dir, "value_top.png")
        fig.savefig(path); plt.close(fig)
        return path
