"""采集器基类.

设计说明 (架构核心):
  1. 各平台 (京东/淘宝/拼多多) 反爬策略差异极大:
     - 京东 search.jd.com 历史上对未登录 HTML 搜索相对宽松, 可直接解析;
     - 淘宝/拼多多搜索接口需登录 Cookie / 签名, 无凭据时无法稳定采集.
  2. 因此 BaseScraper 采用 "真实采集优先, Mock 兜底" 策略:
     - 先尝试真实 HTTP 采集;
     - 网络/反爬/鉴权失败时, 自动回退到 MockScraper 生成结构相同的演示数据,
       保证工具在任何环境下都可运行、可演示.
  3. 真实采集器与 Mock 产出完全相同的 Product 结构, 下游清洗/对比/分析无感知.
"""
from __future__ import annotations

import logging
from abc import ABC, abstractmethod
from typing import List, Optional

from ..models import Product

log = logging.getLogger(__name__)


class BaseScraper(ABC):
    """平台爬虫抽象基类."""

    #: 平台标识 jd / taobao / pdd
    platform: str = "base"
    #: 是否允许在真实采集失败时回退到 Mock 数据
    allow_mock_fallback: bool = True

    def __init__(self, mock_provider: Optional["BaseScraper"] = None):
        self._mock = mock_provider

    def search(self, keyword: str, limit: int = 20) -> List[Product]:
        """统一入口: 真实采集 -> 失败回退 Mock."""
        try:
            items = self._search_real(keyword, limit)
            if items:
                return items[:limit]
            log.warning("[%s] 真实采集返回空, 尝试 Mock 兜底", self.platform)
        except Exception as e:  # noqa: BLE001  采集器要稳健, 任何异常都不能崩溃
            log.warning("[%s] 真实采集失败 (%s), 尝试 Mock 兜底", self.platform, e)
        if self.allow_mock_fallback and self._mock is not None:
            return self._mock.search(keyword, limit)
        return []

    @abstractmethod
    def _search_real(self, keyword: str, limit: int) -> List[Product]:
        """平台真实采集逻辑 (子类实现)."""
        raise NotImplementedError

    def _make(self, **kw) -> Product:
        """快捷构造 Product, 自动填充 platform."""
        kw.setdefault("platform", self.platform)
        kw.setdefault("keyword", "")
        return Product(**kw)
