"""京东采集器.

真实采集路径: search.jd.com 商品搜索 HTML 页面 (未登录可访问).
解析 li.gl-item 内的: 商品名 .p-name em, 价格 .p-price i, 店铺 .p-shop,
销量 .p-commit, 链接 .p-img a[href].

反爬/网络失败时由 BaseScraper.search 自动回退到 MockScraper.
"""
from __future__ import annotations

import logging
import re
from typing import List
from urllib.parse import quote, urljoin

from ..models import Product
from .base import BaseScraper

log = logging.getLogger(__name__)

_SEARCH_URL = "https://search.jd.com/Search"
_UA = ("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
       "(KHTML, like Gecko) Chrome/124.0 Safari/537.36")


class JdScraper(BaseScraper):
    platform = "jd"

    def _search_real(self, keyword: str, limit: int) -> List[Product]:
        import requests
        from bs4 import BeautifulSoup

        params = {"keyword": keyword, "enc": "utf-8", "page": "1"}
        headers = {
            "User-Agent": _UA,
            "Accept-Language": "zh-CN,zh;q=0.9",
            "Referer": "https://www.jd.com/",
        }
        resp = requests.get(_SEARCH_URL, params=params, headers=headers,
                            timeout=10)
        resp.encoding = "utf-8"
        if resp.status_code != 200 or not resp.text:
            log.info("[jd] HTTP %s, 无可用数据", resp.status_code)
            return []

        soup = BeautifulSoup(resp.text, "html.parser")
        items: List[Product] = []
        for li in soup.select("li.gl-item")[:limit]:
            try:
                title_el = li.select_one(".p-name em") or li.select_one(".p-name")
                price_el = li.select_one(".p-price i") or li.select_one(".p-price strong i")
                shop_el = li.select_one(".p-shop a") or li.select_one(".p-shop span")
                commit_el = li.select_one(".p-commit strong a")
                link_el = li.select_one(".p-img a")

                title = title_el.get_text(strip=True) if title_el else ""
                price_txt = price_el.get_text(strip=True) if price_el else "0"
                price = _to_float(price_txt)
                shop = shop_el.get_text(strip=True) if shop_el else "未知店铺"
                sales = _parse_sales(commit_el.get_text(strip=True)) if commit_el else 0
                href = link_el.get("href", "") if link_el else ""
                url = urljoin("https:", href) if href.startswith("//") else (href or "")
                if url and not url.startswith("http"):
                    url = urljoin("https://item.jd.com/", url)

                if not title or price <= 0:
                    continue
                items.append(self._make(
                    keyword=keyword, title=title, price=price, sales=sales,
                    shop_name=shop, shop_rating=None, url=url,
                ))
            except Exception as e:  # noqa: BLE001
                log.debug("[jd] 单条解析失败: %s", e)
                continue
        log.info("[jd] 真实采集 %d 条 (关键词=%s)", len(items), keyword)
        return items


def _to_float(txt: str) -> float:
    m = re.search(r"\d+(?:\.\d+)?", txt)
    return float(m.group()) if m else 0.0


def _parse_sales(txt: str) -> int:
    """解析 '1万+评价' -> 10000."""
    txt = txt.replace("评价", "").replace("+", "").strip()
    m = re.match(r"([\d.]+)\s*万?", txt)
    if not m:
        return 0
    num = float(m.group(1))
    return int(num * 10000) if "万" in txt else int(num)
