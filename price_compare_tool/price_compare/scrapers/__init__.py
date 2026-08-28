"""采集器模块: 各平台爬虫 + mock 兜底."""
from .base import BaseScraper
from .jd import JdScraper
from .taobao import TaobaoScraper
from .pdd import PddScraper
from .mock import MockScraper
from .manager import ScraperManager

__all__ = [
    "BaseScraper", "JdScraper", "TaobaoScraper", "PddScraper",
    "MockScraper", "ScraperManager",
]
