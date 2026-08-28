"""采集->清洗->对比->分析 全流程编排.

Pipeline 是 CLI 与 Web 共用的核心入口, 输入关键词输出完整结果结构,
保证两端产物一致.
"""
from __future__ import annotations

import json
import logging
from dataclasses import asdict
from typing import List, Optional

from ..models import Product
from ..scrapers.manager import ScraperManager
from .cleaner import Cleaner, sort_by_price
from .comparator import Comparator
from .analyzer import Analyzer

log = logging.getLogger(__name__)


class Pipeline:
    """端到端处理流水线."""

    def __init__(self, platforms: Optional[List[str]] = None,
                 mock: bool = False, per_platform: int = 12):
        self.manager = ScraperManager(platforms=platforms, mock=mock,
                                      per_platform=per_platform)
        self.cleaner = Cleaner()
        self.comparator = Comparator()
        self.analyzer = Analyzer()

    def run(self, keyword: str) -> dict:
        """执行完整流水线, 返回结果结构 (供 CLI/Web 直接渲染)."""
        raw = self.manager.search(keyword)
        cleaned = self.cleaner.run(raw)
        ranked = self.analyzer.run(cleaned)           # 打分标注
        cmp = self.comparator.run(ranked)              # 对比统计 + 平台内排名
        # 默认展示: 价格升序 (性价比推荐优先浮顶可选)
        final_sorted = sort_by_price(ranked, ascending=True)

        log.info("[pipeline] 关键词=%s 原始%d 清洗%d 最终%d",
                  keyword, len(raw), len(cleaned), len(final_sorted))
        return {
            "keyword": keyword,
            "total": len(final_sorted),
            "raw_count": len(raw),
            "products": [p.to_dict() for p in final_sorted],
            "comparison": cmp,
        }


def run_pipeline(keyword: str, mock: bool = False,
                 platforms: Optional[List[str]] = None) -> dict:
    """便捷函数: 单次运行完整流水线."""
    return Pipeline(platforms=platforms, mock=mock).run(keyword)


def serialize(result: dict) -> str:
    """结果序列化为 JSON 字符串."""
    return json.dumps(result, ensure_ascii=False, indent=2)
