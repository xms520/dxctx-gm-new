"""淘宝采集器.

说明: 淘宝商品搜索 (s.taobao.com) 长期依赖登录 Cookie 与反爬 token,
未登录请求会被重定向至登录页或返回空壳 HTML, 无法稳定采集真实数据.

实现策略:
  - _search_real 真实尝试一次请求; 若识别到登录/反爬特征则返回空,
    由 BaseScraper 自动回退 MockScraper 生成演示数据;
  - 真实可用时 (有 cookie 注入) 解析 search 接口 JSON.
"""
from __future__ import annotations

import logging
import os
from typing import List, Optional

from ..models import Product
from .base import BaseScraper

log = logging.getLogger(__name__)

_SEARCH_URL = "https://s.taobao.com/search"
_UA = ("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
       "(KHTML, like Gecko) Chrome/124.0 Safari/537.36")


class TaobaoScraper(BaseScraper):
    platform = "taobao"

    #: 可选: 通过环境变量 TB_COOKIE 注入登录 Cookie 启用真实采集
    _cookie: Optional[str] = None

    def __init__(self, mock_provider=None):
        super().__init__(mock_provider)
        self._cookie = os.environ.get("TB_COOKIE")

    def _search_real(self, keyword: str, limit: int) -> List[Product]:
        if not self._cookie:
            # 无 Cookie 时淘宝搜索必然被登录墙拦截, 直接放弃真实采集
            log.info("[taobao] 无登录 Cookie, 跳过真实采集")
            return []

        import requests
        headers = {
            "User-Agent": _UA,
            "Cookie": self._cookie,
            "Referer": "https://www.taobao.com/",
        }
        try:
            resp = requests.get(_SEARCH_URL, params={"q": keyword},
                                headers=headers, timeout=12)
            text = resp.text
        except Exception as e:  # noqa: BLE001
            log.info("[taobao] 请求失败: %s", e)
            return []

        if "login" in text or "请登录" in text or len(text) < 2000:
            log.info("[taobao] 命中登录/反爬墙, 放弃真实采集")
            return []

        # 真实成功路径: 解析 g_page_config JSON (结构多变, 仅做尽力解析)
        items: List[Product] = []
        try:
            import json
            import re
            m = re.search(r"g_page_config\s*=\s*({.*?});\s*</script>", text, re.S)
            if not m:
                return []
            data = json.loads(m.group(1))
            auctions = (data.get("mods", {})
                        .get("itemlist", {})
                        .get("data", {})
                        .get("auctions", []))
            for a in auctions[:limit]:
                raw_price = a.get("view_price") or a.get("price")
                price = _to_float(str(raw_price))
                if price <= 0:
                    continue
                items.append(self._make(
                    keyword=keyword,
                    title=a.get("raw_title") or a.get("title") or "",
                    price=price,
                    sales=_parse_sales(str(a.get("view_sales", "0"))),
                    shop_name=a.get("nick") or a.get("shop_name") or "未知店铺",
                    shop_rating=None,
                    url=a.get("detail_url", "") or "",
                    image_url=a.get("pic_url", "") or "",
                ))
        except Exception as e:  # noqa: BLE001
            log.debug("[taobao] JSON 解析失败: %s", e)
        log.info("[taobao] 真实采集 %d 条 (关键词=%s)", len(items), keyword)
        return items


def _to_float(txt: str) -> float:
    import re
    m = re.search(r"\d+(?:\.\d+)?", txt)
    return float(m.group()) if m else 0.0


def _parse_sales(txt: str) -> int:
    import re
    m = re.match(r"([\d.]+)\s*万?", txt)
    if not m:
        return 0
    num = float(m.group(1))
    return int(num * 10000) if "万" in txt else int(num)
