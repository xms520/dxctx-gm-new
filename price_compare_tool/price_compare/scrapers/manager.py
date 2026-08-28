"""采集器编排管理.

职责:
  - 为每个真实采集器注入对应平台的 MockScraper 作为兜底;
  - 并行/串行调度各平台采集, 汇总结果;
  - 提供 --mock 全局开关, 强制使用 Mock 数据 (稳定演示).
"""
from __future__ import annotations

import logging
from concurrent.futures import ThreadPoolExecutor, as_completed
from typing import Dict, List, Optional

from ..models import Product
from .base import BaseScraper
from .jd import JdScraper
from .taobao import TaobaoScraper
from .pdd import PddScraper
from .mock import MockScraper

log = logging.getLogger(__name__)

_PLATFORM_CLASSES = {
    "jd": JdScraper,
    "taobao": TaobaoScraper,
    "pdd": PddScraper,
}


class ScraperManager:
    """统一调度多平台采集."""

    def __init__(self, platforms: Optional[List[str]] = None,
                 mock: bool = False, per_platform: int = 12):
        self.mock_mode = mock
        self.per_platform = per_platform
        self.platforms = platforms or list(_PLATFORM_CLASSES.keys())
        unknown = set(self.platforms) - set(_PLATFORM_CLASSES)
        if unknown:
            raise ValueError(f"未知平台: {unknown}")
        # 为每个平台准备: 真实采集器 + Mock 兜底器
        self._scrapers: Dict[str, BaseScraper] = {}
        for p in self.platforms:
            mock_provider = MockScraper(p)
            real = _PLATFORM_CLASSES[p](mock_provider=mock_provider)
            self._scrapers[p] = real

    def search(self, keyword: str) -> List[Product]:
        """对关键词执行全平台采集."""
        if self.mock_mode:
            log.info("[manager] Mock 模式: 仅生成演示数据 (关键词=%s)", keyword)
            results: List[Product] = []
            for p, scraper in self._scrapers.items():
                results.extend(scraper._mock.search(keyword, self.per_platform))
            return results

        results: List[Product] = []
        with ThreadPoolExecutor(max_workers=len(self._scrapers)) as ex:
            future_to_platform = {
                ex.submit(scraper.search, keyword, self.per_platform): p
                for p, scraper in self._scrapers.items()
            }
            for fut in as_completed(future_to_platform):
                p = future_to_platform[fut]
                try:
                    items = fut.result()
                    log.info("[manager] %s 采集 %d 条", p, len(items))
                    results.extend(items)
                except Exception as e:  # noqa: BLE001
                    log.warning("[manager] %s 采集异常: %s", p, e)
        log.info("[manager] 汇总采集 %d 条 (关键词=%s)", len(results), keyword)
        return results
