"""拼多多采集器.

说明: 拼多多搜索接口 (mobile.yangkeduo.com) 需 app 签名 + token,
Web 端无公开未登录搜索页, 无法稳定采集真实数据.

实现策略: 与淘宝一致 —— 真实尝试 + 登录/反爬识别 + Mock 兜底.
"""
from __future__ import annotations

import logging
import os
from typing import List, Optional

from ..models import Product
from .base import BaseScraper

log = logging.getLogger(__name__)

_UA = ("Mozilla/5.0 (iPhone; CPU iPhone OS 16_0 like Mac OS X) "
       "AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148")


class PddScraper(BaseScraper):
    platform = "pdd"

    _cookie: Optional[str] = None

    def __init__(self, mock_provider=None):
        super().__init__(mock_provider)
        self._cookie = os.environ.get("PDD_COOKIE")

    def _search_real(self, keyword: str, limit: int) -> List[Product]:
        if not self._cookie:
            log.info("[pdd] 无登录 Cookie, 跳过真实采集")
            return []

        import requests
        # 拼多多无稳定公开搜索端点, 此处保留接入点供有 cookie 时扩展
        try:
            resp = requests.get("https://mobile.yangkeduo.com/search_result.html",
                                params={"search_key": keyword},
                                headers={"User-Agent": _UA, "Cookie": self._cookie},
                                timeout=12)
            text = resp.text
        except Exception as e:  # noqa: BLE001
            log.info("[pdd] 请求失败: %s", e)
            return []

        if "login" in text.lower() or len(text) < 2000:
            log.info("[pdd] 命中登录/反爬墙, 放弃真实采集")
            return []

        # 真实成功路径 (结构化 JSON 通常由 app 签名接口返回, 此处占位)
        items: List[Product] = []
        log.info("[pdd] 真实采集 %d 条 (关键词=%s)", len(items), keyword)
        return items
